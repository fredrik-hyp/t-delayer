"""Validate runtime settings and generate nginx config without shell interpolation."""
import os
from pathlib import Path
import re
import subprocess
import json
import ipaddress
import socket
from web_auth import AUTH_FILE, make_credentials

username = os.environ.get("WEB_USERNAME", "admin")
password = os.environ.get("WEB_PASSWORD", "")
if os.environ.get("WEB_PASSWORD_FILE"):
    password = Path(os.environ["WEB_PASSWORD_FILE"]).read_text().rstrip("\n")
if not re.fullmatch(r"[A-Za-z0-9_.-]{1,64}", username) or not password or len(password) > 1024 or "\n" in password:
    raise SystemExit("Set WEB_PASSWORD or WEB_PASSWORD_FILE, and a valid WEB_USERNAME")
port = int(os.environ.get("WEB_PORT", "8080"))
if not 1 <= port <= 65535:
    raise SystemExit("WEB_PORT must be between 1 and 65535")
multicast_interface = os.environ.get("DELAYER_MULTICAST_INTERFACE", "")
if multicast_interface:
    try:
        address = ipaddress.IPv4Address(multicast_interface)
        if address.is_multicast or address.is_unspecified or str(address) == "255.255.255.255":
            raise ValueError("a local unicast IPv4 address is required")
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
            probe.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, address.packed)
    except (ValueError, OSError) as error:
        raise SystemExit(f"Invalid DELAYER_MULTICAST_INTERFACE: {error}")
for directory in ("/run/delayer", "/run/delayerd", "/home/video"):
    Path(directory).mkdir(parents=True, exist_ok=True)
auth_file = Path(AUTH_FILE)
auth_file.touch(mode=0o600, exist_ok=True)
auth_file.chmod(0o600)
auth_file.write_text(json.dumps(make_credentials(username, password)))
unix_socket = os.environ.get("WEB_UNIX_SOCKET")
if unix_socket and unix_socket != "/run/delayer-web/nginx.sock":
    raise SystemExit("WEB_UNIX_SOCKET must be /run/delayer-web/nginx.sock")
if unix_socket:
    Path(unix_socket).parent.mkdir(parents=True, exist_ok=True)
listen = "unix:" + unix_socket if unix_socket else str(port)
template = Path("/opt/delayer/nginx.conf.template").read_text()
Path("/etc/nginx/nginx.conf").write_text(template.replace("${WEB_LISTEN}", listen))
subprocess.run(["nginx", "-t"], check=True)
# Do not propagate the UI password to the worker processes.
os.environ.pop("WEB_PASSWORD", None)
os.execv("/usr/bin/python3", ["python3", "/opt/delayer/control.py"])
