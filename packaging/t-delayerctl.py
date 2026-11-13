#!/usr/bin/python3
"""Host lifecycle helper for default and named T-Delayer installations."""
import argparse
from contextlib import contextmanager
import errno
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
INSTALLATION_LABEL = "io.polylegio.t-delayer.installation"
CONFIG_ROOT = CONFIG_DIR
STATE_ROOT = STATE_DIR
SYSTEMD_ROOT = Path("/etc/systemd/system")
LOCK_PATH = Path("/run/lock/t-delayer-docker.lock")
INSTALLATION = None
SERVICE = "t-delayer-docker.service"
CONTROLLER_ID = "t-delayer-docker"


def installation_name(name):
    if name in (None, "default"):
        return None
    if not re.fullmatch(r"[a-z0-9][a-z0-9_-]{0,39}", name):
        raise ValueError("Installation name must be 1-40 lowercase letters, digits, underscores or dashes")
    return name


def installation_paths(name):
    name = installation_name(name)
    service = f"t-delayer@{name}.service" if name else "t-delayer-docker.service"
    return dict(INSTALLATION=name, CONFIG_DIR=CONFIG_ROOT / "installations" / name if name else CONFIG_ROOT,
                STATE_DIR=STATE_ROOT / "installations" / name if name else STATE_ROOT,
                DROPIN_DIR=SYSTEMD_ROOT / (service + ".d"), SERVICE=service,
                CONTAINER=f"t-delayer-{name}" if name else "t-delayer",
                NETWORK=f"t-delayer-{name}-bridge" if name else "t-delayer-bridge",
                CONTROLLER_ID=f"t-delayer-docker-{name}" if name else "t-delayer-docker")


@contextmanager
def using_installation(name):
    values = installation_paths(name)
    previous = {key: globals()[key] for key in values}
    globals().update(values)
    try:
        yield
    finally:
        globals().update(previous)


@contextmanager
def registry_lock():
    with LOCK_PATH.open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield


def installations():
    result = []
    if (CONFIG_ROOT / "config.json").is_file():
        result.append(None)
    root = CONFIG_ROOT / "installations"
    if root.is_dir():
        for directory in sorted(root.iterdir()):
            if directory.is_dir() and (directory / "config.json").is_file():
                name = installation_name(directory.name)
                if name is None or directory.is_symlink():
                    raise ValueError(f"Invalid installation directory: {directory}")
                result.append(name)
    return result


def systemctl(*args):
    result = subprocess.run(["systemctl", *args], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or "systemctl failed")
    return result


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
        identity = (labels or {}).get(INSTALLATION_LABEL)
        if identity != (INSTALLATION or "default") and not (INSTALLATION is None and identity is None):
            raise RuntimeError(f"Refusing to modify {kind} {name}: it belongs to another installation")
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


def configure(overrides=None):
    previous = CONFIG_DIR / "config.json"
    saved = json.loads(previous.read_text()) if previous.exists() else {}
    config = dict(network_mode="host", network_name="", container_ip="", web_bind="127.0.0.1",
                  web_port=8080, multicast_interface="", video_path=str(STATE_DIR / "video"))
    if INSTALLATION is not None:
        config.update(saved)
    environment = dict(network_mode="DELAYER_NETWORK_MODE", network_name="DELAYER_NETWORK_NAME",
                       container_ip="DELAYER_CONTAINER_IP", web_bind="DELAYER_WEB_BIND",
                       web_port="DELAYER_WEB_PORT", multicast_interface="DELAYER_MULTICAST_INTERFACE",
                       video_path="DELAYER_VIDEO_PATH")
    for key, variable in environment.items():
        if variable in os.environ:
            config[key] = int(os.environ[variable]) if key == "web_port" else os.environ[variable]
    config.update({key: value for key, value in (overrides or {}).items() if value is not None})
    validate_config(config)
    driver = validate_docker(config)
    validate_isolation(config, driver)
    validate_listener(config, driver)
    CONFIG_DIR.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(CONFIG_DIR, 0o700)
    STATE_DIR.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(STATE_DIR, 0o700)
    if not (STATE_DIR / "credentials.json").exists():
        write_private(STATE_DIR / "credentials.json", json.dumps({
            "database_password": secrets.token_urlsafe(36), "web_password": secrets.token_urlsafe(24)
        }) + "\n")
    config["web_cookie_secure"] = saved.get("web_cookie_secure", False)
    write_private(CONFIG_DIR / "config.json", json.dumps(config, indent=2) + "\n")
    if not (CONFIG_DIR / "instances.json").exists():
        write_private(CONFIG_DIR / "instances.json", '{"instances":[]}\n')
    # Each service waits for its own database/auth and recording mounts.
    DROPIN_DIR.mkdir(parents=True, exist_ok=True)
    quoted_path = json.dumps(config["video_path"].replace("%", "%%"), ensure_ascii=False)
    write_private(DROPIN_DIR / "storage.conf", "[Unit]\nRequiresMountsFor=" + quoted_path + "\n")


def overlaps(first, second):
    first, second = Path(first).resolve(), Path(second).resolve()
    return first == second or first in second.parents or second in first.parents


def network_driver(config):
    if config["network_mode"] != "existing":
        return config["network_mode"]
    network = inspect("network", config["network_name"])
    if not network:
        raise RuntimeError(f"Network {config['network_name']} no longer exists")
    return network["Driver"]


def bind_overlap(first, second):
    return first == second or "0.0.0.0" in (first, second)


def validate_isolation(config, driver=None):
    driver = driver or network_driver(config)
    for protected in (STATE_DIR / "postgresql", STATE_DIR / "auth", CONFIG_DIR):
        if overlaps(config["video_path"], protected):
            raise ValueError("Recording directory must be separate from database, authentication and configuration directories")
    for name in installations():
        if name == INSTALLATION:
            continue
        paths = installation_paths(name)
        peer = validate_config(json.loads((paths["CONFIG_DIR"] / "config.json").read_text()))
        for protected in (paths["STATE_DIR"] / "postgresql", paths["STATE_DIR"] / "auth",
                          paths["CONFIG_DIR"], Path(peer["video_path"])):
            if overlaps(config["video_path"], protected):
                raise ValueError(f"Recording directory overlaps installation {name or 'default'}")
        # Also reject a peer recording into this installation's private directories.
        if any(overlaps(peer["video_path"], path) for path in
               (STATE_DIR / "postgresql", STATE_DIR / "auth", CONFIG_DIR)):
            raise ValueError(f"Private storage overlaps recording directory of {name or 'default'}")
        peer_driver = network_driver(peer)
        if (driver in ("host", "bridge") and peer_driver in ("host", "bridge")
                and config["web_port"] == peer["web_port"] and bind_overlap(config["web_bind"], peer["web_bind"])):
            raise ValueError(f"Web listener conflicts with installation {name or 'default'}")
        if (config["network_mode"] == peer["network_mode"] == "existing"
                and config["network_name"] == peer["network_name"] and config["container_ip"]
                and config["container_ip"] == peer["container_ip"]):
            raise ValueError(f"Container IP conflicts with installation {name or 'default'}")


def available_port(bind):
    for port in range(8080, 65536):
        candidate = dict(network_mode="host", network_name="", container_ip="", web_bind=bind,
                         web_port=port, multicast_interface="", video_path=str(STATE_DIR / "video"))
        try:
            validate_isolation(candidate, "host")
        except ValueError as error:
            if "Web listener conflicts" in str(error):
                continue
            raise
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
                probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                probe.bind((bind, port))
            return port
        except OSError as error:
            if error.errno == errno.EADDRINUSE:
                continue
            raise ValueError(f"Web bind address is unavailable: {bind}") from error
    raise RuntimeError("No available web port; specify --web-port")


def validate_listener(config, driver):
    if driver not in ("host", "bridge"):
        return
    owned = check_owned("container", CONTAINER)
    listeners = []
    if owned and owned.get("State", {}).get("Status") == "running":
        host = owned.get("HostConfig") or {}
        if host.get("NetworkMode") == "host":
            env = dict(item.split("=", 1) for item in owned["Config"].get("Env", []) if "=" in item)
            listeners.append((env.get("WEB_BIND") or "0.0.0.0", int(env.get("WEB_PORT", "8080"))))
        else:
            for bindings in (host.get("PortBindings") or {}).values():
                listeners.extend((item.get("HostIp") or "0.0.0.0", int(item["HostPort"])) for item in bindings or [])
    # The existing container releases its listener when it is recreated.
    if any(port == config["web_port"] and bind_overlap(bind, config["web_bind"]) for bind, port in listeners):
        return
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
            # Match nginx: closed HTTP connections in TIME_WAIT must not block
            # a restart, while another active listener still fails bind().
            probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            probe.bind((config["web_bind"], config["web_port"]))
    except OSError as error:
        raise ValueError(f"Web listener {config['web_bind']}:{config['web_port']} is unavailable") from error


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
    validate_isolation(config, driver)
    validate_listener(config, driver)
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
            docker("network", "create", "--driver", "bridge", "--label", f"{LABEL}={OWNER}",
                   "--label", f"{INSTALLATION_LABEL}={INSTALLATION or 'default'}", NETWORK)
        network = NETWORK
    elif network == "existing":
        network = config["network_name"]
    args = ["container", "create", "--name", CONTAINER, "--label", f"{LABEL}={OWNER}",
            "--label", f"{INSTALLATION_LABEL}={INSTALLATION or 'default'}",
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
        "WEB_USERNAME": "admin", "DELAYER_CONTROLLER_ID": CONTROLLER_ID, "TZ": "UTC",
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
                       f"then sudo systemctl restart {SERVICE}")


def create(overrides, no_start=False):
    if INSTALLATION is None:
        raise ValueError("create requires a named installation; default is reserved for the existing installation")
    with registry_lock():
        if (CONFIG_DIR / "config.json").exists() or (STATE_DIR / "credentials.json").exists():
            raise ValueError("Installation already exists; use configure to retain its data and credentials")
        if inspect("container", CONTAINER):
            raise RuntimeError(f"Container {CONTAINER} already exists")
        load_images()
        overrides = dict(overrides)
        if overrides.get("web_port") is None and "DELAYER_WEB_PORT" not in os.environ:
            overrides["web_port"] = available_port(overrides.get("web_bind") or os.environ.get("DELAYER_WEB_BIND", "127.0.0.1"))
        configure(overrides)
    # Never hold the registry lock while systemd invokes this helper again.
    systemctl("daemon-reload")
    systemctl("enable", SERVICE)
    if not no_start:
        systemctl("start", SERVICE)
    print(f"Created {INSTALLATION}: {CONTAINER}. Initial login: sudo t-delayerctl credentials {INSTALLATION}")


def list_installations():
    print("NAME\tCONTAINER\tSTATE\tWEB")
    for name in installations():
        with using_installation(name):
            config = configuration()
            obj = check_owned("container", CONTAINER)
            state = obj["State"]["Status"] if obj else "not created"
            print(f"{name or 'default'}\t{CONTAINER}\t{state}\t{config['web_bind']}:{config['web_port']}")


def remove_installations():
    # Used by package removal. Persistent data, passwords and recordings stay.
    names = set(installations()) | {None}
    result = docker("ps", "--all", "--filter", f"label={LABEL}={OWNER}", "--format", "{{.Names}}")
    for container in result.stdout.splitlines():
        obj = inspect("container", container)
        name = (obj.get("Config", {}).get("Labels") or {}).get(INSTALLATION_LABEL) if obj else None
        if name and name != "default" and container == "t-delayer-" + installation_name(name):
            names.add(name)
    for name in sorted(names, key=lambda item: item or ""):
        with using_installation(name):
            if Path("/run/systemd/system").is_dir():
                systemctl("stop", SERVICE)
                systemctl("disable", SERVICE)
            with registry_lock():
                remove_container()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("create", "list", "configure", "validate", "load-images", "start", "stop",
                                             "restart", "remove-containers", "remove-installations", "status", "logs", "credentials"))
    parser.add_argument("name", nargs="?", help="Installation name; omitted/default keeps the existing installation")
    parser.add_argument("--direct", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--no-start", action="store_true", help="Create and enable for boot without starting now")
    for option in ("network-mode", "network-name", "container-ip", "web-bind", "web-port", "multicast-interface", "video-path"):
        parser.add_argument("--" + option, type=int if option == "web-port" else str)
    args = parser.parse_args()
    if os.geteuid() != 0:
        raise SystemExit("Run this command as root (sudo)")
    name = installation_name(args.name)
    if args.direct and name is None:
        parser.error("--direct is reserved for named systemd instances")
    overrides = {key: getattr(args, key) for key in ("network_mode", "network_name", "container_ip", "web_bind",
                                                   "web_port", "multicast_interface", "video_path")}
    if args.command not in ("configure", "create") and any(value is not None for value in overrides.values()):
        parser.error("Configuration options require create or configure")
    if args.no_start and args.command != "create":
        parser.error("--no-start requires create")
    if args.command in ("list", "load-images", "remove-installations") and args.name is not None:
        parser.error("This command applies to the whole package and takes no installation name")
    if args.command == "list":
        list_installations()
        return
    if args.command == "remove-installations":
        remove_installations()
        return
    with using_installation(name):
        if args.command == "create":
            create(overrides, args.no_start)
            return
        if (name is not None and args.command in ("start", "stop", "restart") and not args.direct) or args.command == "restart":
            if args.command != "stop":
                configuration()  # Fail before calling systemd for an unknown installation.
            systemctl(args.command, SERVICE)
            return
        with registry_lock():
            if args.command == "configure":
                configure(overrides)
            elif args.command == "validate":
                config = configuration()
                validate_isolation(config, validate_docker(config))
                print("Configuration is valid.")
            elif args.command == "load-images":
                load_images()
            elif args.command == "start":
                start()
            elif args.command == "stop":
                if check_owned("container", CONTAINER):
                    docker("container", "stop", "--time", "80", CONTAINER)
            elif args.command == "remove-containers":
                remove_container()
            elif args.command == "status":
                print(docker("ps", "--all", "--filter", f"label={LABEL}={OWNER}", "--filter", f"name=^/{CONTAINER}$").stdout, end="")
            elif args.command == "logs":
                check_owned("container", CONTAINER)
                result = docker("logs", "--tail", "200", CONTAINER)
                print(result.stdout + result.stderr, end="")
            elif args.command == "credentials":
                credentials = json.loads((STATE_DIR / "credentials.json").read_text())
                print("Username: admin\nInitial password: " + credentials["web_password"])
                print("After a UI password change, use the new password; this initial password is no longer active.")
        if args.command == "configure":
            systemctl("daemon-reload")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError, RuntimeError) as error:
        raise SystemExit(str(error))
