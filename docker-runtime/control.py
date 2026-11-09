"""Container-local process owner and HTTP API. nginx is the only TCP listener.

Commands use the existing delayercoms / SIGUSR1 protocol. Never spawn a process
from a request-supplied command or PID, and never run multiple API workers.
"""
from contextlib import contextmanager
from datetime import datetime, timezone
import grp
import hashlib
from http.server import BaseHTTPRequestHandler
import ipaddress
import json
import logging
import os
from pathlib import Path
import re
import signal
import socket
import socketserver
import subprocess
import sys
import threading
import time

import psycopg2
from psycopg2.extras import RealDictCursor
from web_auth import AuthError, Sessions

LOG = logging.getLogger("delayer-control")
SOCKET = "/run/delayer/control.sock"
STATES = {"Live", "Play", "Pause"}


class APIError(Exception):
    def __init__(self, status, message):
        self.status, self.message = status, message


@contextmanager
def database():
    conn = psycopg2.connect(connect_timeout=5, options="-c timezone=UTC -c statement_timeout=5000")
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                yield cur
    finally:
        conn.close()


def integer(value, low, high, label):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f"{label} must be an integer from {low} to {high}")
    return value


def endpoint(value, label):
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object")
    address = ipaddress.IPv4Address(value.get("host", ""))
    if not address.is_multicast:
        raise ValueError(f"{label}.host must be an IPv4 multicast address")
    integer(value.get("port"), 1, 65535, f"{label}.port")


def load_config(path):
    config = json.loads(Path(path).read_text())
    if not isinstance(config, dict):
        raise ValueError("configuration must be an object")
    return validate_instances(config.get("instances"))


def validate_instances(instances):
    if not isinstance(instances, list):
        raise ValueError("instances must be a list")
    names, destinations = set(), set()
    for item in instances:
        if not isinstance(item, dict):
            raise ValueError("each instance must be an object")
        name = item.get("name", "")
        if not isinstance(name, str) or not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]{0,39}", name) or name in names:
            raise ValueError("instance names must be unique, safe names of at most 40 characters")
        names.add(name)
        if type(item.get("autostart", True)) is not bool:
            raise ValueError("autostart must be boolean")
        # Name-only entries attach existing DB configurations without modifying them.
        if "source" not in item and "outputs" not in item:
            continue
        endpoint(item.get("source"), name + ".source")
        integer(item.get("msbuffer", 3600000), 10000, 2147483647, "msbuffer")
        outputs = item.get("outputs")
        if not isinstance(outputs, list) or not 1 <= len(outputs) <= 8:
            raise ValueError("each instance needs 1 to 8 outputs")
        for output in outputs:
            endpoint(output, name + ".output")
            integer(output.get("ttl", 7), 1, 255, "ttl")
            if type(output.get("rawudp", False)) is not bool:
                raise ValueError("rawudp must be boolean")
            if not isinstance(output.get("name", "Output"), str) or not 1 <= len(output.get("name", "Output")) <= 100:
                raise ValueError("output name must contain 1 to 100 characters")
            dest = (output["host"], output["port"])
            if dest in destinations:
                raise ValueError("outputs must have distinct multicast host:port destinations")
            destinations.add(dest)
    return instances


def validate_configuration(body):
    if not isinstance(body, dict) or set(body) - {"name", "source", "outputs", "msbuffer", "autostart"}:
        raise ValueError("Unknown configuration fields")
    if "source" not in body or "outputs" not in body:
        raise ValueError("An input and at least one output are required")
    return validate_instances([dict(body, autostart=body.get("autostart", False))])[0]


def provision(items):
    """Create missing configurations only; never rewrite an existing stream."""
    with database() as cur:
        for item in sorted(items, key=lambda entry: entry["name"]):
            name = item["name"]
            cur.execute("SELECT pg_advisory_xact_lock(hashtextextended(%s, 0))", ("provision:" + name,))
            cur.execute("SELECT id FROM streaminterfaces WHERE name=%s", (name,))
            if cur.fetchone():
                continue
            if "source" not in item:
                raise ValueError(f"{name} is absent from DB; provide source and outputs")
            insert_configuration(cur, item)


def insert_configuration(cur, item):
    name = item["name"]
    cur.execute("INSERT INTO streaminterfaces (interfacetype,name,usertext) VALUES ('streamtransporter',%s,'Docker') RETURNING id", (name,))
    interface_id = cur.fetchone()["id"]
    cur.execute("""INSERT INTO streaminterfaceconfigs
        (streaminterface_id,typeofconfig,autostart,msbuffer,resendmode,mode)
        VALUES (%s,'stored',false,%s,'normal','delayer') RETURNING id""",
                (interface_id, item.get("msbuffer", 3600000)))
    config_id = cur.fetchone()["id"]
    source = item["source"]
    cur.execute("""INSERT INTO streaminterfacesources
        (streaminterfaceconfig_id,name,sourcetype,host,port,priority,talkbackhost,talkbackport)
        VALUES (%s,%s,'udp',%s,%s,0,'',0)""", (config_id, name + "_input", source["host"], source["port"]))
    primary_id = None
    for index, output in enumerate(item["outputs"]):
        values = (config_id if index == 0 else primary_id, output.get("name", f"Output {index}"),
                  output["host"], output["port"], output.get("ttl", 7), output.get("rawudp", False), index)
        if index == 0:
            cur.execute("""INSERT INTO streaminterfacesinks
                (streaminterfaceconfig_id,name,sinktype,host,port,ttl,rawudp,priority,talkbackhost,talkbackport)
                VALUES (%s,%s,'udp',%s,%s,%s,%s,%s,'',0) RETURNING id""", values)
            primary_id = cur.fetchone()["id"]
        else:
            cur.execute("""INSERT INTO streaminterfacebackupsinks
                (streaminterfacesink_id,name,sinktype,host,port,ttl,rawudp,priority,talkbackhost,talkbackport)
                VALUES (%s,%s,'udp',%s,%s,%s,%s,%s,'',0)""", values)


def read_instance(cur, name):
    cur.execute("""SELECT i.id,i.name,c.id AS config_id,c.msbuffer,c.mode
        FROM streaminterfaces i JOIN streaminterfaceconfigs c ON c.streaminterface_id=i.id
        WHERE i.name=%s""", (name,))
    rows = cur.fetchall()
    if len(rows) != 1 or rows[0]["mode"] != "delayer":
        raise ValueError(f"{name} must have exactly one configuration, in delayer mode")
    result = dict(rows[0])
    cur.execute("SELECT host,port,name FROM streaminterfacesources WHERE streaminterfaceconfig_id=%s", (result["config_id"],))
    sources = cur.fetchall()
    if len(sources) != 1:
        raise ValueError(f"{name} must have exactly one input")
    result["source"] = dict(sources[0])
    cur.execute("SELECT id,name,host,port,ttl,rawudp FROM streaminterfacesinks WHERE streaminterfaceconfig_id=%s", (result["config_id"],))
    primary = cur.fetchall()
    if len(primary) != 1:
        raise ValueError(f"{name} must have exactly one primary output")
    cur.execute("SELECT id,name,host,port,ttl,rawudp FROM streaminterfacebackupsinks WHERE streaminterfacesink_id=%s ORDER BY priority,id", (primary[0]["id"],))
    outputs = primary + cur.fetchall()
    if len(outputs) > 8:
        raise ValueError(f"{name} exceeds 8 outputs")
    result["outputs"] = [dict(output, pathnumber=index) for index, output in enumerate(outputs)]
    return result


def update_configuration(cur, runtime, item):
    """Update a stopped instance atomically while retaining existing sink IDs."""
    cur.execute("UPDATE streaminterfaceconfigs SET msbuffer=%s WHERE id=%s",
                (item.get("msbuffer", 3600000), runtime["config_id"]))
    cur.execute("UPDATE streaminterfacesources SET host=%s,port=%s WHERE streaminterfaceconfig_id=%s",
                (item["source"]["host"], item["source"]["port"], runtime["config_id"]))
    primary_id = runtime["outputs"][0]["id"]
    for index, output in enumerate(item["outputs"]):
        values = (output.get("name", f"Output {index}"), output["host"], output["port"],
                  output.get("ttl", 7), output.get("rawudp", False), index)
        if index == 0:
            cur.execute("""UPDATE streaminterfacesinks SET name=%s,host=%s,port=%s,ttl=%s,rawudp=%s,priority=%s
                WHERE id=%s""", values + (primary_id,))
        elif index < len(runtime["outputs"]):
            cur.execute("""UPDATE streaminterfacebackupsinks SET name=%s,host=%s,port=%s,ttl=%s,rawudp=%s,priority=%s
                WHERE id=%s""", values + (runtime["outputs"][index]["id"],))
        else:
            cur.execute("""INSERT INTO streaminterfacebackupsinks
                (name,host,port,ttl,rawudp,priority,streaminterfacesink_id,sinktype,talkbackhost,talkbackport)
                VALUES (%s,%s,%s,%s,%s,%s,%s,'udp','',0)""", values + (primary_id,))
    for removed in runtime["outputs"][len(item["outputs"]):]:
        cur.execute("DELETE FROM streaminterfacebackupsinks WHERE id=%s", (removed["id"],))
    # Output positions and old commands belong to the previous configuration.
    cur.execute("DELETE FROM delayerstats WHERE streaminterface_id=%s", (runtime["id"],))
    cur.execute("DELETE FROM streaminterfacelivestatistics WHERE streaminterface_id=%s", (runtime["id"],))
    cur.execute("DELETE FROM delayercoms WHERE streaminterface_id=%s", (runtime["id"],))


def managed_configs(seeds, controller_id):
    """Bootstrap once, then load persisted UI membership and autostart settings."""
    provision(seeds)
    with database() as cur:
        for item in seeds:
            runtime = read_instance(cur, item["name"])
            cur.execute("""INSERT INTO delayerwebinstances (streaminterface_id,controller_id,autostart)
                VALUES (%s,%s,%s) ON CONFLICT (streaminterface_id) DO NOTHING""",
                        (runtime["id"], controller_id, item.get("autostart", True)))
            cur.execute("SELECT controller_id FROM delayerwebinstances WHERE streaminterface_id=%s", (runtime["id"],))
            if cur.fetchone()["controller_id"] != controller_id:
                raise ValueError(f"{item['name']} is already owned by another controller")
        cur.execute("""SELECT i.name,w.autostart FROM delayerwebinstances w
            JOIN streaminterfaces i ON i.id=w.streaminterface_id WHERE w.controller_id=%s ORDER BY i.id""", (controller_id,))
        return [dict(row) for row in cur.fetchall()]


class Instance:
    def __init__(self, config):
        self.config = config
        self.name = config["name"]
        self.lock = threading.RLock()
        self.process = None
        self.last_exit_code = None
        self.started_at = None
        self.stopped = True
        self.runtime = None

    def start(self):
        with self.lock:
            if self.process and self.process.poll() is None:
                return
            with database() as cur:
                runtime = read_instance(cur, self.name)
                # Fresh readiness and command data belong to this launch only.
                cur.execute("DELETE FROM delayerstats WHERE streaminterface_id=%s", (runtime["id"],))
                cur.execute("DELETE FROM streaminterfacelivestatistics WHERE streaminterface_id=%s", (runtime["id"],))
                cur.execute("DELETE FROM delayercoms WHERE streaminterface_id=%s", (runtime["id"],))
                cur.execute("SELECT video_path FROM systeminfo LIMIT 1")
                storage = Path(cur.fetchone()["video_path"])
                if not storage.is_dir() or not os.access(storage, os.W_OK):
                    raise APIError(409, f"Recording directory is unavailable: {storage}")
            # The PID namespace and /run are private to this container. The only
            # process we signal is the Popen object we own, never a file's PID.
            Path(f"/run/delayerd/udp_delayer_{self.name}.pid").unlink(missing_ok=True)
            self.runtime = runtime
            self.last_exit_code = None
            self.stopped = False
            self.started_at = datetime.now(timezone.utc)
            self.process = subprocess.Popen(["/usr/local/bin/delayer", self.name, "nodaemon"])
            LOG.info("Started %s pid=%s", self.name, self.process.pid)

    def stop(self):
        with self.lock:
            self.stopped = True
            if self.process and self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=25)
                except subprocess.TimeoutExpired:
                    LOG.warning("%s did not stop within 25s; killing", self.name)
                    self.process.kill()
                    self.process.wait()
            if self.process:
                self.last_exit_code = self.process.poll()

    def snapshot(self, cur):
        with self.lock:
            runtime = self.runtime or read_instance(cur, self.name)
            code = self.process.poll() if self.process else None
            running = self.process is not None and code is None
            if self.process and code is not None:
                self.last_exit_code = code
            cur.execute("SELECT pathnumber,state,startplayat,startpos,lastpos,currpos,playto FROM delayerstats WHERE streaminterface_id=%s", (runtime["id"],))
            stats = {row["pathnumber"]: dict(row) for row in cur.fetchall()} if running else {}
            cur.execute("""SELECT pathnumber,bitratein,bitrateout,bufferlevel,udpdrops,timestamp,
                EXTRACT(EPOCH FROM (now()-timestamp)) AS age_seconds
                FROM streaminterfacelivestatistics WHERE streaminterface_id=%s""", (runtime["id"],))
            live = {row["pathnumber"]: dict(row) for row in cur.fetchall()} if running else {}
            outputs = []
            for output in runtime["outputs"]:
                path = output["pathnumber"]
                metric = live.get(path, {})
                fresh = running and metric.get("age_seconds") is not None and float(metric["age_seconds"]) < 10
                stat = stats.get(path, {})
                ready = fresh and stat.get("state") in STATES
                outputs.append(dict(output, status=stat if running else {}, metrics=metric,
                                    ready=ready, stale=running and not fresh))
            ready = running and all(output["ready"] for output in outputs)
            cur.execute("SELECT id,pathnumber,action,timestamp FROM delayercoms WHERE streaminterface_id=%s ORDER BY id", (runtime["id"],))
            pending = [dict(row) for row in cur.fetchall()]
            cur.execute("SELECT timestamp,level,text FROM streaminterfacelogs WHERE streaminterface_id=%s ORDER BY timestamp DESC,id DESC LIMIT 8", (runtime["id"],))
            logs = [dict(row) for row in cur.fetchall()]
            return dict(name=self.name, id=runtime["id"], source=runtime["source"], msbuffer=runtime["msbuffer"],
                        autostart=self.config.get("autostart", False),
                        process_state="running" if ready else "starting" if running else "stopped" if self.stopped else "exited",
                        pid=self.process.pid if running else None, last_exit_code=self.last_exit_code,
                        started_at=self.started_at, outputs=outputs, pending=pending, logs=logs)

    def command(self, path, body):
        with self.lock:
            with database() as cur:
                snapshot = self.snapshot(cur)
                if not self.process or self.process.poll() is not None:
                    raise APIError(409, "The instance is stopped")
                outputs = {item["pathnumber"]: item for item in snapshot["outputs"]}
                if path not in outputs:
                    raise APIError(404, "Output does not exist")
                if not outputs[path]["ready"]:
                    raise APIError(409, "Wait until the instance has fresh status")
                if not outputs[path]["status"].get("startpos") or not outputs[path]["status"].get("currpos"):
                    raise APIError(409, "No recorded data in the buffer yet")
                command = validate_command(body, outputs[path]["status"])
                cur.execute("""INSERT INTO delayercoms
                    (streaminterface_id,pathnumber,action,jumptopos,startplayat,playto)
                    VALUES (%s,%s,%s,%s,%s,%s) RETURNING id""",
                            (snapshot["id"], path, command["action"], command["jumptopos"], command["startplayat"], command["playto"]))
                command_id = cur.fetchone()["id"]
            # The row must commit before the worker is signalled.
            try:
                self.process.send_signal(signal.SIGUSR1)
            except ProcessLookupError:
                with database() as cur:
                    cur.execute("DELETE FROM delayercoms WHERE id=%s", (command_id,))
                raise APIError(409, "The instance exited before the command could be delivered")
            LOG.info("Command id=%s instance=%s path=%s action=%s", command_id, self.name, path, command["action"])
            return {"accepted": True, "command_id": command_id, "instance": self.name, "pathnumber": path}


def parse_time(value, field):
    if value is None or value == "":
        return None
    if not isinstance(value, str) or len(value) > 64:
        raise APIError(400, f"{field}: use ISO 8601 with a time zone")
    try:
        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
        if parsed.tzinfo is None or parsed.year < 1970 or parsed.year > 2200:
            raise ValueError()
        # DB uses timestamp without time zone, consistently UTC.
        return parsed.astimezone(timezone.utc).replace(tzinfo=None)
    except ValueError:
        raise APIError(400, f"{field}: use ISO 8601 with a time zone")


def validate_command(body, stat):
    action = body.get("action")
    if action not in ("jump", "play", "pause", "playto"):
        raise APIError(400, "Choose jump, play, pause or playto")
    if set(body) - {"action", "jumptopos", "startplayat", "playto"}:
        raise APIError(400, "Unknown command field")
    result = {key: parse_time(body.get(key), key) for key in ("jumptopos", "startplayat", "playto")}
    result["action"] = action
    if action == "jump":
        pos = result["jumptopos"]
        if pos is None:
            raise APIError(400, "Jump requires a recorded time")
        if not stat.get("startpos") or not stat.get("lastpos") or not stat["startpos"] <= pos <= stat["lastpos"]:
            raise APIError(409, "The time is outside the available buffer")
        if result["startplayat"] and result["startplayat"] <= datetime.now(timezone.utc).replace(tzinfo=None):
            raise APIError(400, "The scheduled start must be in the future")
    elif result["jumptopos"] or result["startplayat"]:
        raise APIError(400, "Recorded time and scheduled start can only be used with jump")
    if action == "pause" and result["playto"]:
        raise APIError(400, "Pause cannot have a stop time")
    if result["playto"]:
        earliest = result["startplayat"] or datetime.now(timezone.utc).replace(tzinfo=None)
        if result["playto"] <= earliest:
            raise APIError(400, "The stop time must be after the playback start in wall-clock time")
    # playto with null explicitly clears an existing stop position.
    return result


def json_value(value):
    if isinstance(value, datetime):
        return value.replace(tzinfo=timezone.utc).isoformat().replace("+00:00", "Z")
    from decimal import Decimal
    if isinstance(value, Decimal):
        return float(value)
    raise TypeError(type(value).__name__)


class APIHandler(BaseHTTPRequestHandler):
    server_version = "DelayerControl"

    def log_message(self, fmt, *args):
        LOG.info(fmt, *args)

    def respond(self, status, data, cookie=None):
        payload = json.dumps(data, default=json_value).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(payload)))
        if cookie is not None:
            self.send_header("Set-Cookie", cookie)
        if status == 429:
            self.send_header("Retry-After", "60")
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        self.handle_request(False)

    def do_POST(self):
        self.handle_request(True)

    def request_json(self):
        if self.headers.get("X-Delayer-Control") != "1" or self.headers.get_content_type() != "application/json":
            raise APIError(403, "JSON and X-Delayer-Control: 1 are required")
        origin = self.headers.get("Origin")
        host = self.headers.get("Host", "")
        if origin and origin not in ("http://" + host, "https://" + host):
            raise APIError(403, "The request must come from the web interface origin")
        length = int(self.headers.get("Content-Length", "0"))
        if not 0 < length <= 16384:
            raise APIError(400, "Invalid request size")
        body = json.loads(self.rfile.read(length))
        if not isinstance(body, dict):
            raise APIError(400, "The request must be a JSON object")
        return body

    def handle_request(self, post):
        try:
            manager = self.server.manager
            if manager.shutdown.is_set():
                raise APIError(503, "The control service is shutting down")
            if not post and self.path == "/healthz":
                with database() as cur:
                    cur.execute("SELECT 1")
                healthy = manager.healthy()
                self.respond(200 if healthy else 503, {"ok": healthy})
                return
            cookie = self.headers.get("Cookie", "")
            authenticated = manager.auth.authenticated(cookie)
            if not post and self.path == "/api/auth/session":
                self.respond(200, {"authenticated": authenticated,
                                   "username": manager.auth.username if authenticated else None})
                return
            if post and self.path == "/api/auth/login":
                body = self.request_json()
                if set(body) - {"username", "password"}:
                    raise APIError(400, "Provide a username and password")
                token = manager.auth.login(body.get("username"), body.get("password"),
                                           self.headers.get("X-Real-IP", "local"))
                manager.auth.revoke(cookie)
                self.respond(200, {"authenticated": True, "username": manager.auth.username}, manager.auth.cookie(token))
                return
            if post and self.path == "/api/auth/logout":
                self.request_json()
                manager.auth.revoke(cookie)
                self.respond(200, {"authenticated": False}, manager.auth.cookie())
                return
            if not authenticated:
                raise APIError(401, "Sign in to open the control room")
            if not post and self.path == "/api/status":
                with database() as cur:
                    instances = [item.snapshot(cur) for item in manager.list_instances()]
                self.respond(200, {"sampled_at": datetime.now(timezone.utc), "instances": instances})
                return
            match = re.fullmatch(r"/api/instances/([A-Za-z0-9_.-]+)/(start|stop|configuration|outputs/(\d+)/commands)", self.path)
            creating = self.path == "/api/instances"
            if not post or (not creating and (not match or match[1] not in manager.instances)):
                raise APIError(404, "Resource does not exist")
            body = self.request_json()
            if creating:
                self.respond(201, manager.create_instance(body))
                return
            instance = manager.instances[match[1]]
            if match[2] == "start":
                instance.start()
                self.respond(202, {"accepted": True})
            elif match[2] == "stop":
                instance.stop()
                self.respond(200, {"stopped": True})
            elif match[2] == "configuration":
                self.respond(200, manager.configure_instance(instance.name, body))
            else:
                self.respond(202, instance.command(int(match[3]), body))
        except (BrokenPipeError, ConnectionResetError):
            return
        except (APIError, AuthError) as error:
            self.respond(error.status, {"error": error.message})
        except (ValueError, json.JSONDecodeError) as error:
            self.respond(400, {"error": str(error)})
        except psycopg2.Error:
            LOG.exception("Database request failed")
            self.respond(503, {"error": "The database is unavailable"})
        except Exception:
            LOG.exception("Request failed")
            self.respond(500, {"error": "Internal error; see the container log"})


class HTTPServer(socketserver.ThreadingMixIn, socketserver.UnixStreamServer):
    daemon_threads = True


class Manager:
    def __init__(self, configs):
        self.seed_configs = configs
        self.instances = {item["name"]: Instance(item) for item in configs}
        self.lock = threading.RLock()
        self.owner_lock = threading.RLock()
        fallback_id = hashlib.sha256("\0".join(sorted(self.instances)).encode()).hexdigest()[:16]
        self.controller_id = os.environ.get("DELAYER_CONTROLLER_ID", "streams-" + fallback_id)
        if not re.fullmatch(r"[A-Za-z0-9_.-]{1,100}", self.controller_id):
            raise ValueError("Invalid DELAYER_CONTROLLER_ID")
        self.auth = Sessions.load(self.controller_id)
        self.owner = None
        self.nginx = None
        self.http = None
        self.shutdown = threading.Event()

    def list_instances(self):
        with self.lock:
            return list(self.instances.values())

    def check_destinations(self, item):
        occupied = {(output["host"], output["port"])
                    for instance in self.instances.values() if instance.name != item["name"]
                    for output in instance.runtime["outputs"]}
        if any((output["host"], output["port"]) in occupied for output in item["outputs"]):
            raise APIError(409, "Another instance already uses this output address and port")

    def create_instance(self, body):
        item = validate_configuration(body)
        name = item["name"]
        with self.lock:
            if name in self.instances:
                raise APIError(409, "The instance name already exists")
            self.check_destinations(item)
            with self.owner_lock, self.owner.cursor() as cur:
                cur.execute("SELECT pg_try_advisory_lock(hashtextextended(%s,0))", ("delayer:" + name,))
                if not cur.fetchone()[0]:
                    raise APIError(409, "The instance is already owned by another container")
            try:
                with database() as cur:
                    cur.execute("SELECT id FROM streaminterfaces WHERE name=%s", (name,))
                    if cur.fetchone():
                        raise APIError(409, "The instance name already exists in the database")
                    insert_configuration(cur, item)
                    runtime = read_instance(cur, name)
                    cur.execute("""INSERT INTO delayerwebinstances (streaminterface_id,controller_id,autostart)
                        VALUES (%s,%s,%s)""", (runtime["id"], self.controller_id, item["autostart"]))
                instance = Instance(item)
                instance.runtime = runtime
                self.instances[name] = instance
            except Exception:
                with self.owner_lock, self.owner.cursor() as cur:
                    cur.execute("SELECT pg_advisory_unlock(hashtextextended(%s,0))", ("delayer:" + name,))
                raise
            LOG.info("Created instance %s with %s outputs", name, len(item["outputs"]))
            return {"saved": True, "name": name}

    def configure_instance(self, name, body):
        item = validate_configuration(body)
        if item["name"] != name:
            raise APIError(400, "The instance name cannot be changed")
        with self.lock:
            instance = self.instances[name]
            with instance.lock:
                if instance.process and instance.process.poll() is None:
                    raise APIError(409, "Stop the instance before changing its input or outputs")
                self.check_destinations(item)
                with database() as cur:
                    runtime = read_instance(cur, name)
                    update_configuration(cur, runtime, item)
                    cur.execute("""UPDATE delayerwebinstances SET autostart=%s
                        WHERE streaminterface_id=%s AND controller_id=%s""",
                                (item["autostart"], runtime["id"], self.controller_id))
                    refreshed = read_instance(cur, name)
                instance.config = item
                instance.runtime = refreshed
                LOG.info("Updated configuration for %s", name)
                return {"saved": True, "name": name}

    def claim(self):
        self.owner = psycopg2.connect(connect_timeout=5, options="-c statement_timeout=5000")
        self.owner.autocommit = True
        with self.owner.cursor() as cur:
            # Session locks exclude two containers controlling the same DB stream.
            for instance in sorted(self.instances.values(), key=lambda item: item.name):
                cur.execute("SELECT pg_try_advisory_lock(hashtextextended(%s,0))", ("delayer:" + instance.name,))
                if not cur.fetchone()[0]:
                    raise ValueError(f"{instance.name} is already owned by another container")

    def healthy(self):
        if self.shutdown.is_set() or not self.nginx or self.nginx.poll() is not None:
            return False
        return all(item.stopped or (item.process is not None and item.process.poll() is None)
                   for item in self.list_instances())

    def run(self):
        try:
            configs = managed_configs(self.seed_configs, self.controller_id)
            self.instances = {item["name"]: Instance(item) for item in configs}
            self.claim()
            destinations = set()
            with database() as cur:
                for instance in self.instances.values():
                    instance.runtime = read_instance(cur, instance.name)
                    for output in instance.runtime["outputs"]:
                        dest = (output["host"], output["port"])
                        if dest in destinations:
                            raise ValueError("Managed instances share an output host:port")
                        destinations.add(dest)
            Path(SOCKET).unlink(missing_ok=True)
            self.http = HTTPServer(SOCKET, APIHandler)
            self.http.manager = self
            os.chown(SOCKET, 0, grp.getgrnam("www-data").gr_gid)
            os.chmod(SOCKET, 0o660)
            threading.Thread(target=self.http.serve_forever, daemon=True).start()
            web_socket = os.environ.get("WEB_UNIX_SOCKET")
            if web_socket:
                # Remove a previous launch's socket only after ownership is held.
                Path(web_socket).unlink(missing_ok=True)
            self.nginx = subprocess.Popen(["nginx", "-g", "daemon off;"])
            if web_socket:
                for _ in range(200):
                    if Path(web_socket).exists():
                        os.chown(web_socket, 0, grp.getgrnam("www-data").gr_gid)
                        os.chmod(web_socket, 0o660)
                        break
                    time.sleep(.01)
                else:
                    raise RuntimeError("nginx did not create its forwarding socket")
            for instance in self.list_instances():
                if instance.config.get("autostart", True):
                    instance.start()
            while not self.shutdown.wait(1):
                if self.nginx.poll() is not None:
                    raise RuntimeError("nginx exited")
                # Loss of the lock connection means we must relinquish all workers.
                with self.owner_lock, self.owner.cursor() as cur:
                    cur.execute("SELECT 1")
        finally:
            self.shutdown.set()
            if self.http:
                self.http.shutdown()
                self.http.server_close()
            # Stop workers concurrently so container grace time is independent of count.
            threads = [threading.Thread(target=item.stop) for item in self.list_instances()]
            for thread in threads:
                thread.start()
            for thread in threads:
                thread.join()
            if self.nginx and self.nginx.poll() is None:
                self.nginx.terminate()
                try:
                    self.nginx.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    self.nginx.kill()
                    self.nginx.wait()
            if self.owner:
                self.owner.close()


def healthcheck():
    import http.client
    connection = http.client.HTTPConnection("localhost", timeout=6)
    connection.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    connection.sock.settimeout(6)
    connection.sock.connect(SOCKET)
    connection.request("GET", "/healthz")
    response = connection.getresponse()
    healthy = response.status == 200
    response.read()
    connection.close()
    sys.exit(0 if healthy else 1)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    if sys.argv[1:] == ["--healthcheck"]:
        healthcheck()
    manager = Manager(load_config(os.environ.get("DELAYER_CONFIG", "/etc/delayer/instances.json")))
    signal.signal(signal.SIGTERM, lambda *_: manager.shutdown.set())
    signal.signal(signal.SIGINT, lambda *_: manager.shutdown.set())
    manager.run()
