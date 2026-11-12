"""Regression checks for installer validation, ownership, and persistent secrets."""
import importlib.util
import copy
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("ctl", ROOT / "packaging/t-delayerctl.py")
ctl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ctl)


def settings(**overrides):
    return dict(network_mode="host", network_name="", container_ip="", web_bind="127.0.0.1",
                web_port=8080, multicast_interface="", video_path="/srv/video", **overrides)


class PackagingTests(unittest.TestCase):
    def test_invalid_settings(self):
        for key, value in (("network_mode", "none"), ("web_port", 0), ("web_port", 65536),
                           ("web_port", True), ("web_bind", "239.1.1.1"), ("web_bind", "bad"),
                           ("multicast_interface", "239.1.1.1"), ("multicast_interface", "eth0"),
                           ("video_path", "relative"), ("video_path", "/"), ("video_path", "/srv/a,b")):
            config = settings()
            config[key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                ctl.validate_config(config)

    def test_existing_network_validation(self):
        config = settings()
        config.update(network_mode="existing", network_name="lan", container_ip="192.0.2.10")
        ctl.validate_config(config)
        network = {"Driver": "macvlan", "IPAM": {"Config": [{"Subnet": "192.0.2.0/24", "Gateway": "192.0.2.1"}]}}
        info = type("Result", (), {"stdout": '{"OSType":"linux"}'})()
        with patch.object(ctl, "docker", return_value=info), patch.object(ctl, "check_owned"), \
                patch.object(ctl, "inspect", return_value=network):
            with self.assertRaisesRegex(RuntimeError, "0.0.0.0"):
                ctl.validate_docker(config)
            config["web_bind"] = "0.0.0.0"
            self.assertEqual(ctl.validate_docker(config), "macvlan")
            config["container_ip"] = "198.51.100.1"
            with self.assertRaisesRegex(RuntimeError, "subnet"):
                ctl.validate_docker(config)
            config["container_ip"] = "192.0.2.1"
            with self.assertRaisesRegex(RuntimeError, "gateway"):
                ctl.validate_docker(config)
            network.update(Driver="ipvlan", Options={"ipvlan_mode": "l3"})
            with self.assertRaisesRegex(RuntimeError, "L2"):
                ctl.validate_docker(config)

    def test_unowned_container_never_removed(self):
        with patch.object(ctl, "inspect", return_value={"Config": {"Labels": {}}}), \
                patch.object(ctl, "docker") as docker:
            with self.assertRaisesRegex(RuntimeError, "not owned"):
                ctl.remove_container()
            docker.assert_not_called()

    def test_configuration_keeps_secrets_and_cookie_option(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with patch.object(ctl, "CONFIG_DIR", root / "etc"), patch.object(ctl, "STATE_DIR", root / "state"), \
                    patch.object(ctl, "DROPIN_DIR", root / "systemd"), patch.object(ctl, "validate_docker"), \
                    patch.dict(os.environ, {}, clear=True):
                ctl.configure()
                credentials = (ctl.STATE_DIR / "credentials.json").read_bytes()
                self.assertEqual((ctl.STATE_DIR / "credentials.json").stat().st_mode & 0o777, 0o600)
                config = ctl.configuration()
                config["web_cookie_secure"] = True
                (ctl.CONFIG_DIR / "config.json").write_text(json.dumps(config))
                os.environ.update(DELAYER_NETWORK_MODE="bridge", DELAYER_VIDEO_PATH='/srv/video with spaces')
                ctl.configure()
                self.assertEqual(credentials, (ctl.STATE_DIR / "credentials.json").read_bytes())
                self.assertTrue(ctl.configuration()["web_cookie_secure"])
                self.assertIn('RequiresMountsFor="/srv/video with spaces"',
                              (ctl.DROPIN_DIR / "storage.conf").read_text())
                before = (ctl.CONFIG_DIR / "config.json").read_bytes()
                os.environ["DELAYER_WEB_PORT"] = "70000"
                with self.assertRaises(ValueError):
                    ctl.configure()
                self.assertEqual(before, (ctl.CONFIG_DIR / "config.json").read_bytes())

    def test_image_manifest_pins_amd64_digest(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(ctl, "SHARE_DIR", Path(directory)):
            path = Path(directory) / "images.json"
            path.write_text(json.dumps({"architecture": "arm64", "runtime_id": "sha256:" + "a" * 64}))
            with self.assertRaises(RuntimeError):
                ctl.manifest()
            path.write_text(json.dumps({"architecture": "amd64", "runtime_id": "latest"}))
            with self.assertRaises(RuntimeError):
                ctl.manifest()

    def test_image_identity_survives_containerd_digest_change(self):
        image = {"Id": "sha256:" + "b" * 64, "Os": "linux", "Architecture": "amd64",
                 "RootFS": {"Layers": ["sha256:" + "c" * 64]},
                 "Config": {"Entrypoint": ["/usr/bin/tini", "--", "python3"],
                            "Env": ["PGHOST=/run/postgresql"], "Healthcheck": {"Test": ["CMD", "true"]}}}
        data = {"runtime": "t-delayer:test", "runtime_id": "sha256:" + "a" * 64,
                "runtime_fingerprint": ctl.image_fingerprint(image)}
        with patch.object(ctl, "manifest", return_value=data), patch.object(ctl, "inspect", return_value=image), \
                patch.object(ctl, "docker") as docker:
            ctl.load_images()
            docker.assert_not_called()  # The correct image must not be repeatedly imported.
            self.assertEqual(ctl.bundled_image()["Id"], image["Id"])
            image["RootFS"]["Layers"] = ["sha256:" + "d" * 64]
            with self.assertRaisesRegex(RuntimeError, "content differs"):
                ctl.bundled_image()

    def test_execution_config_mutation_is_rejected(self):
        image = {"Os": "linux", "Architecture": "amd64", "RootFS": {"Layers": ["layer"]},
                 "Config": {"Entrypoint": ["/trusted"], "Healthcheck": {"Test": ["CMD", "true"]}}}
        original = ctl.image_fingerprint(image)
        equivalent = copy.deepcopy(image)
        equivalent["Config"].update(Cmd=None, Labels=None)
        equivalent["Config"]["Healthcheck"]["StartInterval"] = 0
        self.assertEqual(original, ctl.image_fingerprint(equivalent))
        equivalent["Config"]["Entrypoint"] = ["/unexpected"]
        self.assertNotEqual(original, ctl.image_fingerprint(equivalent))

    def test_initial_unhealthy_probe_can_recover_within_startup_budget(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            config = settings()
            config["video_path"] = str(root / "video")
            config_dir, state_dir = root / "etc", root / "state"
            config_dir.mkdir()
            state_dir.mkdir()
            (state_dir / "credentials.json").write_text(json.dumps({"database_password": "database-test", "web_password": "web-test"}))
            probes = [{"State": {"Status": "running", "Health": {"Status": value}}}
                      for value in ("unhealthy", "healthy")]
            with patch.object(ctl, "CONFIG_DIR", config_dir), patch.object(ctl, "STATE_DIR", state_dir), \
                    patch.object(ctl, "configuration", return_value=config), \
                    patch.object(ctl, "validate_docker", return_value="host"), \
                    patch.object(ctl, "bundled_image", return_value={"Id": "sha256:" + "a" * 64}), \
                    patch.object(ctl, "check_owned", return_value=None), \
                    patch.object(ctl, "inspect", side_effect=probes), patch.object(ctl.time, "sleep"), \
                    patch.object(ctl, "docker") as docker:
                ctl.start()
                self.assertFalse(any(call.args[:2] == ("container", "stop") for call in docker.call_args_list))


if __name__ == "__main__":
    unittest.main()
