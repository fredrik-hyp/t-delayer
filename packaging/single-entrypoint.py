"""Supervise PostgreSQL and the existing delayer/UI controller in one container."""
import os
from pathlib import Path
import pwd
import signal
import subprocess
import tempfile
import time


def main():
    # Never expose the database over TCP, including in Docker host network mode.
    os.environ.update(PGHOST="/run/postgresql", PGPORT="5432", PGDATABASE="delayerdb",
                      PGUSER="delayer", POSTGRES_DB="delayerdb", POSTGRES_USER="delayer")
    password = os.environ.get("PGPASSWORD", "")
    if not password or "\n" in password:
        raise RuntimeError("Set a non-empty PGPASSWORD without newlines")
    postgres_user = pwd.getpwnam("postgres")
    for directory in ("/run/postgresql", "/var/lib/postgresql/data"):
        Path(directory).mkdir(parents=True, exist_ok=True)
        os.chown(directory, postgres_user.pw_uid, postgres_user.pw_gid)
        os.chmod(directory, 0o700)
    bindirs = sorted(Path("/usr/lib/postgresql").glob("*/bin"))
    if len(bindirs) != 1:
        raise RuntimeError("Expected exactly one installed PostgreSQL major version")
    bindir = bindirs[0]
    cluster = Path("/var/lib/postgresql/data/cluster")
    marker = cluster / ".delayer-schema-ready"
    fresh = not cluster.exists()
    if not fresh and not marker.exists():
        raise RuntimeError("Database initialization was interrupted. Restore a backup, or remove "
                           "the incomplete cluster directory if it contains no required data. See INSTALL.md.")
    if not fresh and (cluster / "PG_VERSION").read_text().strip() != bindir.parent.name:
        raise RuntimeError("PostgreSQL major version changed. Use pg_dump/restore before upgrading Ubuntu. See INSTALL.md.")

    def pg(command, *args, **kwargs):
        return subprocess.run(["runuser", "-u", "postgres", "--", str(bindir / command), *args],
                              check=True, **kwargs)

    if fresh:
        descriptor, password_path = tempfile.mkstemp(dir="/run/postgresql")
        try:
            with os.fdopen(descriptor, "w") as stream:
                stream.write(password + "\n")
            os.chown(password_path, postgres_user.pw_uid, postgres_user.pw_gid)
            pg("initdb", "-D", str(cluster), "--username=delayer", "--encoding=UTF8", "--locale=C.UTF-8",
               "--auth-local=scram-sha-256", "--auth-host=scram-sha-256", "--pwfile=" + password_path)
        finally:
            Path(password_path).unlink()
    stopping = False

    def stop(_signum, _frame):
        nonlocal stopping
        stopping = True

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    database = subprocess.Popen(["runuser", "-u", "postgres", "--", str(bindir / "postgres"),
                                 "-D", str(cluster), "-k", "/run/postgresql",
                                 "-c", "listen_addresses=", "-c", "unix_socket_permissions=0700"])
    app = None
    try:
        for _ in range(120):
            if stopping or database.poll() is not None:
                raise RuntimeError("PostgreSQL stopped during startup")
            ready = subprocess.run([str(bindir / "pg_isready"), "-q"], check=False)
            if ready.returncode == 0:
                break
            time.sleep(0.5)
        else:
            raise RuntimeError("PostgreSQL did not become ready")
        if fresh:
            subprocess.run([str(bindir / "createdb"), "delayerdb"], check=True)
            subprocess.run(["sh", "/opt/delayer/init-db.sh"], check=True)
            marker.write_text("schema=31\n")
        # The controller never receives a request to initialize an unfinished DB.
        app = subprocess.Popen(["python3", "/opt/delayer/entrypoint.py"])
        while not stopping:
            if database.poll() is not None:
                raise RuntimeError("PostgreSQL exited unexpectedly")
            if app.poll() is not None:
                raise RuntimeError(f"Delayer controller exited with status {app.returncode}")
            time.sleep(0.5)
    finally:
        # Stop senders/writers before shutting down their database.
        if app is not None and app.poll() is None:
            app.terminate()
            try:
                app.wait(timeout=40)
            except subprocess.TimeoutExpired:
                app.kill()
                app.wait()
        if database.poll() is None:
            pg("pg_ctl", "-D", str(cluster), "-m", "fast", "-w", "-t", "30", "stop")
        database.wait(timeout=10)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        raise SystemExit(str(error))
