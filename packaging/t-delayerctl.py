#!/usr/bin/python3
"""Host lifecycle helper for the single-container T-Delayer Debian package."""
import argparse
import fcntl
import hashlib
import ipaddress
import json
import os
from pathlib import Path
import re
import secrets
import socket
import subprocess
import tempfile
import time

CONFIG_DIR = Path("/etc/t-delayer")
STATE_DIR = Path("/var/lib/t-delayer")
SHARE_DIR = Path("/usr/share/t-delayer")
DROPIN_DIR = Path("/etc/systemd/system/t-delayer-docker.service.d")
CONTAINER = "t-delayer"
NETWORK = "t-delayer-bridge"
LABEL = "io.polylegio.t-delayer.package"
OWNER = "t-delayer-docker"


def docker(*args, optional=False):
    result = subprocess.run(["docker", *map(str, args)], capture_output=True, text=True)
    if result.returncode and not optional:
        raise RuntimeError(result.stderr.strip() or "Docker command failed")
    return result


def inspect(kind, name):
    result = docker(kind, "inspect", name, optional=True)
    if result.returncode:
        # Distinguish a missing object from an unavailable daemon.
        docker("info", "--format", "{{.OSType}}")
        if "No such" in result.stderr or "not found" in result.stderr:
            return None
        raise RuntimeError(result.stderr.strip())
    return json.loads(result.stdout)[0]


def check_owned(kind, name):
    obj = inspect(kind, name)
    if obj:
        labels = obj["Config"].get("Labels", {}) if kind == "container" else obj.get("Labels", {})
        if (labels or {}).get(LABEL) != OWNER:
            raise RuntimeError(f"Refusing to modify {kind} {name}: it is not owned by this package")
    return obj


def write_private(path, data):
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    descriptor, temporary = tempfile.mkstemp(dir=path.parent)
    try:
        with os.fdopen(descriptor, "w") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        Path(temporary).unlink(missing_ok=True)


def unicast(value, label):
    address = ipaddress.IPv4Address(value)
    if address.is_multicast or address.is_unspecified or str(address) == "255.255.255.255":
        raise ValueError(f"{label} must be a unicast IPv4 address")
    return address


def validate_config(config):
    mode = config["network_mode"]
    if mode not in ("host", "bridge", "existing"):
        raise ValueError("Network mode must be host, bridge, or existing")
    bind = ipaddress.IPv4Address(config["web_bind"])
    if bind.is_multicast or str(bind) == "255.255.255.255":
        raise ValueError("Web bind must be a local IPv4 address or 0.0.0.0")
    if type(config["web_port"]) is not int or not 1 <= config["web_port"] <= 65535:
        raise ValueError("Web port must be between 1 and 65535")
    if config["multicast_interface"]:
        unicast(config["multicast_interface"], "Multicast interface")
    path = config["video_path"]
    if not Path(path).is_absolute() or path == "/" or any(c in path for c in '\n\r\t,'):
        raise ValueError("Video directory must be an absolute path other than /, without commas or control characters")
    if mode == "existing":
        name = config["network_name"]
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,127}", name) or name in ("host", "none", "bridge", NETWORK):
            raise ValueError("Choose a pre-created user-defined Docker network")
        if config["container_ip"]:
            unicast(config["container_ip"], "Container IP")
    return config


def validate_docker(config):
    info = json.loads(docker("info", "--format", "{{json .}}").stdout)
    if info["OSType"] != "linux" or any("rootless" in item for item in info.get("SecurityOptions", [])):
        raise RuntimeError("A rootful Linux Docker Engine is required")
    check_owned("container", CONTAINER)
    if config["network_mode"] == "bridge":
        check_owned("network", NETWORK)
    if config["network_mode"] == "host" and config["multicast_interface"]:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
            probe.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF,
                             socket.inet_aton(config["multicast_interface"]))
    if config["network_mode"] == "existing":
        network = inspect("network", config["network_name"])
        if not network or network["Driver"] not in ("bridge", "macvlan", "ipvlan"):
            raise RuntimeError("Existing network must use the bridge, macvlan, or ipvlan driver")
        if network.get("Internal"):
            raise RuntimeError("Choose a network with external connectivity, not an internal-only network")
        if network["Driver"] == "ipvlan" and (network.get("Options") or {}).get("ipvlan_mode", "l2") != "l2":
            raise RuntimeError("Multicast requires an IPvlan L2 network")
        if network["Driver"] in ("macvlan", "ipvlan") and config["web_bind"] != "0.0.0.0":
            raise RuntimeError("Use web bind 0.0.0.0 with macvlan/ipvlan; access the UI at the container IP")
        if config["container_ip"]:
            address = ipaddress.IPv4Address(config["container_ip"])
            pools = network.get("IPAM", {}).get("Config") or []
            if not any(address in ipaddress.ip_network(p["Subnet"]) for p in pools
                       if p.get("Subnet") and ":" not in p["Subnet"]):
                raise RuntimeError("Static container IP must belong to the Docker network's IPv4 subnet")
            if any(config["container_ip"] == p.get("Gateway") for p in pools):
                raise RuntimeError("Static container IP cannot be the network gateway")
        return network["Driver"]
    return config["network_mode"]


def configuration():
    return validate_config(json.loads((CONFIG_DIR / "config.json").read_text()))


def configure():
    config = {
        "network_mode": os.environ.get("DELAYER_NETWORK_MODE", "host"),
        "network_name": os.environ.get("DELAYER_NETWORK_NAME", ""),
        "container_ip": os.environ.get("DELAYER_CONTAINER_IP", ""),
        "web_bind": os.environ.get("DELAYER_WEB_BIND", "127.0.0.1"),
        "web_port": int(os.environ.get("DELAYER_WEB_PORT", "8080")),
        "multicast_interface": os.environ.get("DELAYER_MULTICAST_INTERFACE", ""),
        "video_path": os.environ.get("DELAYER_VIDEO_PATH", "/var/lib/t-delayer/video"),
    }
    validate_config(config)
    validate_docker(config)
    CONFIG_DIR.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(CONFIG_DIR, 0o700)
    STATE_DIR.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(STATE_DIR, 0o700)
    if not (STATE_DIR / "credentials.json").exists():
        write_private(STATE_DIR / "credentials.json", json.dumps({
            "database_password": secrets.token_urlsafe(36), "web_password": secrets.token_urlsafe(24)
        }) + "\n")
    previous = CONFIG_DIR / "config.json"
    if previous.exists():
        config["web_cookie_secure"] = json.loads(previous.read_text()).get("web_cookie_secure", False)
    write_private(CONFIG_DIR / "config.json", json.dumps(config, indent=2) + "\n")
    # Ensure systemd waits for the recording filesystem before recreating containers.
    DROPIN_DIR.mkdir(parents=True, exist_ok=True)
    quoted_path = json.dumps(config["video_path"].replace("%", "%%"), ensure_ascii=False)
    write_private(DROPIN_DIR / "storage.conf", "[Unit]\nRequiresMountsFor=" + quoted_path + "\n")


def manifest():
    data = json.loads((SHARE_DIR / "images.json").read_text())
    if data["architecture"] != "amd64" or not re.fullmatch(r"sha256:[0-9a-f]{64}", data["runtime_id"]):
        raise RuntimeError("Invalid bundled image manifest")
    if not re.fullmatch(r"[0-9a-f]{64}", data.get("runtime_fingerprint", "")) or not data.get("runtime"):
        raise RuntimeError("Bundled image content fingerprint is missing or invalid")
    return data


def image_fingerprint(image):
    """Identify filesystem and execution config independently of Docker's store.

    Classic Docker uses a config digest as .Id; the containerd image store uses
    a manifest/index digest. Normalize inspect defaults that vary by API version.
    """
    config = image.get("Config") or {}
    health = config.get("Healthcheck") or {}
    identity = {
        "os": image["Os"], "architecture": image["Architecture"],
        "layers": image["RootFS"]["Layers"],
        "config": {
            **{key: config.get(key) or [] for key in ("Entrypoint", "Cmd", "Env", "Shell", "OnBuild")},
            **{key: config.get(key) or {} for key in ("Labels", "Volumes", "ExposedPorts")},
            **{key: config.get(key) or "" for key in ("User", "WorkingDir", "StopSignal")},
            "ArgsEscaped": bool(config.get("ArgsEscaped", False)),
            "Healthcheck": {"Test": health.get("Test") or [], **{
                key: health.get(key) or 0 for key in ("Interval", "Timeout", "StartPeriod", "StartInterval", "Retries")
            }},
        },
    }
    return hashlib.sha256(json.dumps(identity, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def bundled_image(optional=False):
    data = manifest()
    image = inspect("image", data["runtime"])
    valid = (image and image.get("Architecture") == "amd64" and image.get("Os") == "linux"
             and image_fingerprint(image) == data["runtime_fingerprint"])
    if not valid and not optional:
        raise RuntimeError("Bundled image is missing or its platform/content differs. Run sudo t-delayerctl load-images")
    return image if valid else None


def load_images():
    if bundled_image(optional=True) is None:
        print("Loading the bundled T-Delayer image...", flush=True)
        docker("image", "load", "--input", SHARE_DIR / "images.tar")
    bundled_image()


def remove_container():
    if check_owned("container", CONTAINER):
        docker("container", "stop", "--time", "80", CONTAINER)
        docker("container", "rm", CONTAINER)


def start():
    config = configuration()
    driver = validate_docker(config)
    # Pin the verified image by the receiving engine's immutable ID, not a tag
    # that might be changed between verification and container creation.
    image_id = bundled_image()["Id"]
    credentials = json.loads((STATE_DIR / "credentials.json").read_text())
    for value in credentials.values():
        if not value or any(c in value for c in "\n\r"):
            raise ValueError("Credentials must be non-empty and contain no newlines")
    STATE_DIR.mkdir(parents=True, exist_ok=True, mode=0o700)
    video = Path(config["video_path"])
    for directory in (video, STATE_DIR / "postgresql", STATE_DIR / "auth"):
        directory.mkdir(parents=True, exist_ok=True, mode=0o700)
    instances = CONFIG_DIR / "instances.json"
    if not instances.exists():
        write_private(instances, '{"instances":[]}\n')
    # Validate everything before stopping the running container.
    remove_container()
    network = config["network_mode"]
    if network == "bridge":
        if not check_owned("network", NETWORK):
            docker("network", "create", "--driver", "bridge", "--label", f"{LABEL}={OWNER}", NETWORK)
        network = NETWORK
    elif network == "existing":
        network = config["network_name"]
    args = ["container", "create", "--name", CONTAINER, "--label", f"{LABEL}={OWNER}",
            # Let systemd gate boot startup on recording mounts. Unlike
            # unless-stopped, on-failure does not auto-start on daemon startup.
            "--platform", "linux/amd64", "--pull", "never", "--restart", "on-failure",
            "--stop-timeout", "80", "--network", network,
            "--log-driver", "local", "--log-opt", "max-size=10m", "--log-opt", "max-file=3"]
    if config["network_mode"] == "existing" and config["container_ip"]:
        args.extend(["--ip", config["container_ip"]])
    if driver == "bridge":
        args.extend(["--publish", f'{config["web_bind"]}:{config["web_port"]}:8080'])
    for source, destination in ((video, "/home/video"), (STATE_DIR / "postgresql", "/var/lib/postgresql/data"),
                                (STATE_DIR / "auth", "/var/lib/delayer"), (instances, "/etc/delayer/instances.json")):
        option = ",readonly" if source == instances else ""
        args.extend(["--mount", f"type=bind,src={source},dst={destination}{option}"])
    env = {
        "PGPASSWORD": credentials["database_password"], "WEB_PASSWORD": credentials["web_password"],
        "WEB_USERNAME": "admin", "DELAYER_CONTROLLER_ID": "t-delayer-docker", "TZ": "UTC",
        "DELAYER_MULTICAST_INTERFACE": config["multicast_interface"],
        "WEB_PORT": str(config["web_port"]) if driver != "bridge" else "8080",
        "WEB_BIND": config["web_bind"] if driver == "host" else "0.0.0.0",
        "WEB_COOKIE_SECURE": "1" if config.get("web_cookie_secure", False) else "0",
    }
    # Avoid passwords in command arguments and installation logs.
    descriptor, env_path = tempfile.mkstemp(dir=CONFIG_DIR)
    try:
        with os.fdopen(descriptor, "w") as stream:
            stream.write("".join(f"{key}={value}\n" for key, value in env.items()))
        docker(*args, "--env-file", env_path, image_id)
    finally:
        Path(env_path).unlink(missing_ok=True)
    docker("container", "start", CONTAINER)
    deadline = time.monotonic() + 180
    while time.monotonic() < deadline:
        obj = inspect("container", CONTAINER)
        state = obj["State"]
        if state.get("Health", {}).get("Status") == "healthy":
            print(f"T-Delayer is ready. Web UI port: {config['web_port']} ({config['network_mode']} networking).")
            return
        # Initial PostgreSQL migrations can outlast the image health grace
        # period. A running container may recover; honor the full readiness budget.
        if state["Status"] not in ("running", "created"):
            break
        time.sleep(1)
    docker("container", "stop", "--time", "80", CONTAINER)
    raise RuntimeError("T-Delayer failed startup. Run sudo t-delayerctl logs, correct the configuration, "
                       "then sudo systemctl restart t-delayer-docker")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("configure", "validate", "load-images", "start", "stop",
                                             "remove-containers", "status", "logs", "credentials"))
    command = parser.parse_args().command
    if os.geteuid() != 0:
        raise SystemExit("Run this command as root (sudo)")
    # Serialize package hooks and manual lifecycle operations.
    with open("/run/lock/t-delayer-docker.lock", "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if command == "configure":
            configure()
        elif command == "validate":
            validate_docker(configuration())
            print("Configuration is valid.")
        elif command == "load-images":
            load_images()
        elif command == "start":
            start()
        elif command == "stop":
            if check_owned("container", CONTAINER):
                docker("container", "stop", "--time", "80", CONTAINER)
        elif command == "remove-containers":
            remove_container()
        elif command == "status":
            print(docker("ps", "--all", "--filter", f"label={LABEL}={OWNER}").stdout, end="")
        elif command == "logs":
            check_owned("container", CONTAINER)
            result = docker("logs", "--tail", "200", CONTAINER)
            print(result.stdout + result.stderr, end="")
        elif command == "credentials":
            credentials = json.loads((STATE_DIR / "credentials.json").read_text())
            print("Username: admin\nInitial password: " + credentials["web_password"])
            print("After a UI password change, use the new password; this initial password is no longer active.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError, RuntimeError) as error:
        raise SystemExit(str(error))
