#!/usr/bin/env python3
"""Isolated real-Docker test for the package's single-container deployment.

Build the matching package/image first. No production containers or data are used.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import secrets
import socket
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("ctl", ROOT / "packaging/t-delayerctl.py")
ctl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ctl)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--image", required=True)
parser.add_argument("--smoke", action="store_true", help="Check startup and recreation without the full multicast suite")
args = parser.parse_args()
suffix = secrets.token_hex(5)
ctl.CONTAINER = "delayer-package-test-" + suffix
ctl.NETWORK = "delayer-package-test-" + suffix


def run(*command, **kwargs):
    return subprocess.run(command, check=True, **kwargs)


with tempfile.TemporaryDirectory(prefix="delayer-package-test-") as temporary:
    root = Path(temporary).resolve()
    ctl.CONFIG_DIR = root / "etc"
    ctl.STATE_DIR = root / "state"
    ctl.SHARE_DIR = root / "share"
    ctl.DROPIN_DIR = root / "systemd"
    ctl.SHARE_DIR.mkdir()
    image = json.loads(ctl.docker("image", "inspect", args.image).stdout)[0]
    (ctl.SHARE_DIR / "images.json").write_text(json.dumps({"architecture": "amd64", "runtime_id": image["Id"],
        "runtime": args.image, "runtime_fingerprint": ctl.image_fingerprint(image)}))
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        web_port = probe.getsockname()[1]
    os.environ.update(DELAYER_NETWORK_MODE="bridge", DELAYER_WEB_PORT=str(web_port),
                      DELAYER_VIDEO_PATH=str(root / "video"), DELAYER_MULTICAST_INTERFACE="127.0.0.1")
    try:
        ctl.configure()
        fixture = json.loads((ROOT / "docker-runtime/instances.example.json").read_text())
        fixture["instances"][1]["autostart"] = True
        (ctl.CONFIG_DIR / "instances.json").write_text(json.dumps(fixture))
        ctl.start()
        run("docker", "cp", str(ROOT / "docker-runtime/tests"), ctl.CONTAINER + ":/tests")

        def execute(*command, **kwargs):
            return run("docker", "exec", ctl.CONTAINER, *command, **kwargs)

        # Verify all services run together and DB has no TCP listener.
        execute("psql", "-U", "delayer", "-d", "delayerdb", "-Atc", "SHOW listen_addresses")
        execute("python3", "-c", "import socket; s=socket.socket(); "
                "assert s.connect_ex(('127.0.0.1',5432)) != 0; print('PASS PostgreSQL uses Unix socket only')")
        if not args.smoke:
            execute("python3", "-m", "unittest", "discover", "-s", "/tests", "-p", "test_*.py", "-q")
            execute("python3", "/tests/integration.py")
            execute("psql", "-U", "delayer", "-d", "delayerdb", "-v", "ON_ERROR_STOP=1", "-c",
                    "INSERT INTO streaminterfaces (name) VALUES ('test-unused')")
            execute("python3", "/tests/configuration_transfer.py")
            execute("python3", "/tests/password_change.py", "change")
        saved = () if args.smoke else ("expected-configuration.json", "previous-session-cookie", "changed-password-cookie")
        for name in saved:
            run("docker", "cp", ctl.CONTAINER + ":/tmp/" + name, str(root / name))
        old_credentials = (ctl.STATE_DIR / "credentials.json").read_bytes()
        # Reconfigure to a pre-created bridge, exercising container replacement.
        os.environ.update(DELAYER_NETWORK_MODE="existing", DELAYER_NETWORK_NAME=ctl.NETWORK)
        # Existing package-owned network is intentionally a reserved name; use a second test network.
        external = ctl.NETWORK + "-external"
        ctl.docker("network", "create", external)
        os.environ["DELAYER_NETWORK_NAME"] = external
        try:
            ctl.configure()
            ctl.start()
            run("docker", "cp", str(ROOT / "docker-runtime/tests"), ctl.CONTAINER + ":/tests")
            for name in saved:
                run("docker", "cp", str(root / name), ctl.CONTAINER + ":/tmp/" + name)
            if not args.smoke:
                run("docker", "exec", "-e", "TEST_LOGIN_PASSWORD=rotated-test-password", ctl.CONTAINER,
                    "python3", "/tests/verify_persistence.py")
                execute("python3", "/tests/password_change.py", "verify")
            assert old_credentials == (ctl.STATE_DIR / "credentials.json").read_bytes()
            if not args.smoke:
                assert list((root / "video").glob("st1_chunk_*.bin"))
            print("PASS single-container recreation and bridge/existing network selection", flush=True)
            if not args.smoke:
                print("PASS database, video, configuration and changed password survive container recreation", flush=True)
        except BaseException:
            result = ctl.docker("logs", "--tail", "100", ctl.CONTAINER, optional=True)
            print(result.stdout + result.stderr, flush=True)
            raise
        finally:
            ctl.remove_container()
            ctl.docker("network", "rm", external)
    except BaseException:
        result = ctl.docker("logs", "--tail", "100", ctl.CONTAINER, optional=True)
        print(result.stdout + result.stderr)
        raise
    finally:
        ctl.remove_container()
        if ctl.check_owned("network", ctl.NETWORK):
            ctl.docker("network", "rm", ctl.NETWORK)
