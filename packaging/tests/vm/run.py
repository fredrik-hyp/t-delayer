#!/usr/bin/env python3
"""Boot real amd64 Ubuntu guests and verify package/systemd/disk ordering.

Runs in the isolated QEMU runner container, with no privileged host access.
Only the guests' disposable recording block devices are formatted.
"""
import argparse
import base64
from datetime import datetime, timezone
import functools
import hashlib
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import shutil
import subprocess
import threading
import time
import traceback

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--ubuntu", choices=("24.04", "26.04", "all"), default="all")
parser.add_argument("--version", default="4.1.2")
parser.add_argument("--attended", action="store_true", help="Install with actual readline prompts driven through a guest PTY")
parser.add_argument("--resume", action="store_true", help="Reuse existing disposable guest disks after a test interruption")
parser.add_argument("--fresh-app", action="store_true", help="Reset application data only in the disposable guest before installation")
args = parser.parse_args()
WORK = Path("/work")
RESULTS = Path("/results")
WORK.mkdir(parents=True, exist_ok=True)
RESULTS.mkdir(parents=True, exist_ok=True)
if shutil.disk_usage(WORK).free < 8 * 1024**3:
    raise SystemExit("Provide at least 8 GiB free on /work for disposable VM disks and cloud images")


def event(message):
    print(datetime.now(timezone.utc).isoformat(timespec="seconds"), message, flush=True)


def command(*argv, **kwargs):
    return subprocess.run(argv, check=True, **kwargs)


class QuietHTTP(SimpleHTTPRequestHandler):
    def log_message(self, *_):
        pass


server = ThreadingHTTPServer(("0.0.0.0", 8000), functools.partial(QuietHTTP, directory="/packages"))
threading.Thread(target=server.serve_forever, daemon=True).start()
key = WORK / "id_ed25519"
if not key.exists():
    command("ssh-keygen", "-q", "-t", "ed25519", "-N", "", "-f", str(key))
public_key = key.with_suffix(".pub").read_text().strip()


def run_guest(release):
    codename = {"24.04": "noble", "26.04": "resolute"}[release]
    root = WORK / release
    output = RESULTS / release
    root.mkdir(exist_ok=True)
    output.mkdir(exist_ok=True)
    overlay = root / "system.qcow2"
    video = root / "recording.qcow2"
    resume = args.resume and overlay.exists() and video.exists()
    result = {"ubuntu": release, "architecture": "amd64", "version": args.version,
              "installation_frontend": "readline" if args.attended else "noninteractive",
              "checks": {}, "status": "running"}

    def save():
        (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")

    def passed(name, evidence=None):
        result["checks"][name] = evidence if evidence is not None else True
        event(f"Ubuntu {release}: PASS {name}")
        save()

    filename = f"{codename}-server-cloudimg-amd64.img"
    base_url = f"https://cloud-images.ubuntu.com/{codename}/current/"
    image = root / filename
    event(f"Ubuntu {release}: verifying official cloud image and checksum cache")
    if not image.exists() or not (root / "SHA256SUMS").exists():
        checksum_temp = root / "SHA256SUMS.download"
        command("curl", "--fail", "--location", "--retry", "3", "--connect-timeout", "20",
                "--output", str(checksum_temp), base_url + "SHA256SUMS")
        checksum_temp.replace(root / "SHA256SUMS")
    checksums = (root / "SHA256SUMS").read_text().splitlines()
    expected = next(line.split()[0] for line in checksums if line.split()[-1].lstrip("*") == filename)
    if not image.exists():
        temporary_image = image.with_suffix(".download")
        command("curl", "--fail", "--location", "--retry", "3", "--connect-timeout", "20",
                "--speed-limit", "1024", "--speed-time", "60", "--output", str(temporary_image), base_url + filename)
        temporary_image.replace(image)
    with image.open("rb") as stream:
        actual = hashlib.file_digest(stream, "sha256").hexdigest() if hasattr(hashlib, "file_digest") else ""
    if actual != expected:
        raise RuntimeError("Official cloud image checksum mismatch")
    result["cloud_image"] = {"url": base_url + filename, "sha256": actual}
    user_data = "#cloud-config\n" + json.dumps({
        "users": [{"name": "ubuntu", "groups": ["sudo"], "shell": "/bin/bash",
                   "sudo": "ALL=(ALL) NOPASSWD:ALL", "lock_passwd": True,
                   "ssh_authorized_keys": [public_key]}],
        "ssh_pwauth": False, "disable_root": True,
        "write_files": [{"path": "/etc/ssh/sshd_config.d/50-test.conf",
                         "content": "PasswordAuthentication no\n"}],
        "runcmd": [["touch", "/var/lib/delayer-vm-cloud-ready"]]
    }) + "\n"
    if not resume:
        (root / "user-data").write_text(user_data)
        (root / "meta-data").write_text(f"instance-id: delayer-{release}-{time.time_ns()}\nlocal-hostname: delayer-{codename}-test\n")
        command("genisoimage", "-quiet", "-output", str(root / "seed.iso"), "-volid", "cidata", "-joliet", "-rock",
                str(root / "user-data"), str(root / "meta-data"))
    # These are new VM overlays, never disks from the host or another VM.
    if not resume:
        for path in (overlay, video):
            path.unlink(missing_ok=True)
        command("qemu-img", "create", "-f", "qcow2", "-F", "qcow2", "-b", str(image), str(overlay), "24G")
        command("qemu-img", "create", "-f", "qcow2", str(video), "2G")
        shutil.copyfile("/usr/share/OVMF/OVMF_VARS_4M.fd", root / "OVMF_VARS.fd")
    port = 2224 if release == "24.04" else 2226
    serial = (output / "serial.log").open("w")
    qemu = subprocess.Popen([
        "qemu-system-x86_64", "-machine", "q35", "-accel", "tcg,thread=multi", "-cpu", "max",
        "-smp", "2", "-m", "3072", "-nographic", "-monitor", "none",
        "-drive", "if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd",
        "-drive", f"if=pflash,format=raw,file={root / 'OVMF_VARS.fd'}",
        "-drive", f"file={overlay},if=virtio,format=qcow2,cache=writeback",
        "-drive", f"file={video},if=none,id=video,format=qcow2,cache=writeback",
        "-device", "virtio-blk-pci,drive=video,serial=delayer-video-test",
        "-drive", f"file={root / 'seed.iso'},media=cdrom,readonly=on",
        "-netdev", f"user,id=net,hostfwd=tcp:127.0.0.1:{port}-:22",
        "-device", "virtio-net-pci,netdev=net", "-rtc", "base=utc"
    ], stdout=serial, stderr=subprocess.STDOUT)
    ssh_base = ["ssh", "-i", str(key), "-p", str(port), "-o", "BatchMode=yes", "-o", "ConnectTimeout=15",
                "-o", "StrictHostKeyChecking=accept-new", "-o", f"UserKnownHostsFile={root / 'known_hosts'}", "ubuntu@127.0.0.1"]
    (root / "known_hosts").unlink(missing_ok=True)

    def ssh(script, *, timeout=120, check=True):
        remote = "sudo -n bash -s"
        completed = subprocess.run([*ssh_base, remote], input="set -euo pipefail\n" + script,
                                   text=True, capture_output=True, timeout=timeout)
        with (output / "commands.log").open("a") as log:
            log.write("\n$ " + script + "\n" + completed.stdout + completed.stderr)
        if check and completed.returncode:
            raise RuntimeError(f"Guest command failed ({completed.returncode}): {script[:160]}\n" + completed.stdout + completed.stderr)
        return completed

    def wait_ssh(previous_boot=None):
        deadline = time.monotonic() + 1200
        last_progress = 0
        while time.monotonic() < deadline:
            if qemu.poll() is not None:
                raise RuntimeError("QEMU exited; see serial.log")
            try:
                probe = ssh("test -e /var/lib/delayer-vm-cloud-ready; cat /proc/sys/kernel/random/boot_id", timeout=25, check=False)
                boot = probe.stdout.strip()
                if probe.returncode == 0 and boot and boot != previous_boot:
                    return boot
            except subprocess.TimeoutExpired:
                pass
            if time.monotonic() - last_progress > 45:
                event(f"Ubuntu {release}: waiting for guest boot/SSH" + (" after reboot" if previous_boot else ""))
                last_progress = time.monotonic()
            time.sleep(3)
        raise RuntimeError("Guest did not reach SSH readiness; see serial.log")

    try:
        event(f"Ubuntu {release}: booting amd64 VM with real systemd and a recording block disk")
        boot_id = wait_ssh()
        facts = ssh("test \"$(cat /proc/1/comm)\" = systemd; cat /etc/os-release; uname -mr; "
                    "ps -p 1 -o comm=; systemctl --version | sed -n '1p'; systemctl is-system-running || true").stdout
        (output / "guest.txt").write_text(facts)
        assert f'VERSION_ID="{release}"' in facts and "x86_64" in facts and "systemd" in facts
        passed("real Ubuntu amd64 guest with systemd PID 1", {"boot_id": boot_id})
        package = f"t-delayer-docker_{args.version}-1ubuntu{release}_amd64.deb"
        assert (Path("/packages") / package).is_file(), package
        with (Path("/packages") / package).open("rb") as stream:
            result["package_sha256"] = hashlib.file_digest(stream, "sha256").hexdigest()
        event(f"Ubuntu {release}: formatting only the disposable VM recording disk, then installing package with APT")
        reset_app = """
systemctl stop t-delayer-docker.service 2>/dev/null || true
docker container stop --timeout 80 t-delayer 2>/dev/null || true
docker container rm t-delayer 2>/dev/null || true
rm -rf /var/lib/t-delayer/postgresql /var/lib/t-delayer/auth
""" if args.fresh_app else ""
        if args.attended:
            helper = base64.b64encode(Path(__file__).with_name('attended_install.py').read_bytes()).decode()
            ssh(f"printf '%s' '{helper}' | base64 -d > /root/delayer-attended-install.py")
        if args.attended:
            install_command = """
# No application preseeding: all application answers must come from the PTY.
! dpkg-query -W -f='${Status}' t-delayer-docker 2>/dev/null | grep -q 'install ok installed'
python3 /root/delayer-attended-install.py
"""
        else:
            install_command = """
printf '%s\\n' \
 't-delayer-docker t-delayer-docker/network-mode select host' \
 't-delayer-docker t-delayer-docker/web-bind string 127.0.0.1' \
 't-delayer-docker t-delayer-docker/video-path string /srv/delayer-video' | debconf-set-selections
export DEBIAN_FRONTEND=noninteractive
if dpkg-query -W t-delayer-docker >/dev/null 2>&1; then
 # Replacing a half-configured same-version local package through --reinstall
 # can fail in APT before unpacking. Unpack the candidate, then let APT finish.
 dpkg --unpack /tmp/delayer.deb
 apt-get --fix-broken install -y
else
 apt-get install -y /tmp/delayer.deb
fi
"""
        install = f"""
mountunit=$(systemd-escape --path --suffix=mount /srv/delayer-video)
rm -f "/etc/systemd/system/$mountunit.d/test-gate.conf"
systemctl stop delayer-test-disk-gate.service 2>/dev/null || true
rm -f /etc/systemd/system/delayer-test-disk-gate.service
systemctl daemon-reload
{reset_app}
video_device=/dev/disk/by-id/virtio-delayer-video-test
test -b "$video_device"
test "$(lsblk -ndo SERIAL "$video_device")" = delayer-video-test
filesystem=$(blkid -s TYPE -o value "$video_device" || true)
if test -z "$filesystem"; then mkfs.ext4 -q -L delayer-video "$video_device"; else test "$filesystem" = ext4; fi
mkdir -p /srv/delayer-video
if ! mountpoint -q /srv/delayer-video; then touch /srv/delayer-video/underlay-sentinel; fi
uuid=$(blkid -s UUID -o value "$video_device")
if ! grep -Fq "UUID=$uuid " /etc/fstab; then
 printf 'UUID=%s /srv/delayer-video ext4 defaults,nofail 0 2\\n' "$uuid" >> /etc/fstab
fi
systemctl daemon-reload
if ! mountpoint -q /srv/delayer-video; then mount /srv/delayer-video; fi
findmnt --mountpoint /srv/delayer-video
curl -fSL http://10.0.2.2:8000/{package} -o /tmp/delayer.deb
if ! dpkg-query -W -f='${{Status}}' docker.io 2>/dev/null | grep -q 'install ok installed'; then apt-get update; fi
{install_command}
systemctl is-enabled t-delayer-docker.service
systemctl is-active t-delayer-docker.service docker.service
test "$(docker inspect -f '{{{{.State.Health.Status}}}}' t-delayer)" = healthy
test "$(docker inspect -f '{{{{.HostConfig.RestartPolicy.Name}}}}' t-delayer)" = on-failure
systemd-analyze verify /usr/lib/systemd/system/t-delayer-docker.service
"""
        # APT/image decompression under software emulation may take several minutes.
        pending = subprocess.Popen([*ssh_base, "sudo -n bash -s"], stdin=subprocess.PIPE,
                                   stdout=(output / "install.log").open("w"), stderr=subprocess.STDOUT, text=True)
        pending.stdin.write("set -euxo pipefail\n" + install)
        pending.stdin.close()
        deadline = time.monotonic() + 2400
        while pending.poll() is None:
            if time.monotonic() > deadline:
                pending.terminate()
                raise RuntimeError("APT installation timed out; see install.log")
            event(f"Ubuntu {release}: APT installation/image initialization is running (install.log)")
            time.sleep(30)
        if pending.returncode:
            raise RuntimeError("APT installation failed; see install.log")
        passed("APT installation with actual Docker and systemd", {"package": package})
        if args.attended:
            attended = json.loads(ssh("cat /root/delayer-attended-result.json").stdout)
            (output / 'attended-install.json').write_text(json.dumps(attended, indent=2) + '\n')
            passed("attended APT: real terminal prompts and saved web-bind answer", attended)
        passed("service enabled and container healthy")
        ssh("""
docker exec t-delayer psql -v ON_ERROR_STOP=1 -U delayer -d delayerdb -c \
 "CREATE TABLE IF NOT EXISTS vm_boot_sentinel(value text); TRUNCATE vm_boot_sentinel; INSERT INTO vm_boot_sentinel VALUES ('preserved')"
printf 'recording disk data\\n' > /srv/delayer-video/recording-sentinel
sha256sum /var/lib/t-delayer/credentials.json /var/lib/t-delayer/auth/auth.json > /root/delayer-before.sha256
cat > /etc/systemd/system/delayer-test-disk-gate.service <<'UNIT'
[Unit]
Description=Test gate: hold the real recording disk mount until explicitly released
DefaultDependencies=no
[Service]
Type=oneshot
ExecStart=/bin/sh -c 'until test -e /run/delayer-test-disk-release; do sleep 1; done'
TimeoutStartSec=900
UNIT
mountunit=$(systemd-escape --path --suffix=mount /srv/delayer-video)
mkdir -p "/etc/systemd/system/$mountunit.d"
printf '[Unit]\\nRequires=delayer-test-disk-gate.service\\nAfter=delayer-test-disk-gate.service\\n' > "/etc/systemd/system/$mountunit.d/test-gate.conf"
systemctl daemon-reload
systemctl cat t-delayer-docker.service
""")
        event(f"Ubuntu {release}: rebooting; the recording disk mount will be deliberately held")
        ssh("systemctl reboot", check=False)
        new_boot = wait_ssh(boot_id)
        result["reboot_boot_id"] = new_boot
        blocked = ssh("""
for attempt in $(seq 1 120); do
 if systemctl is-active --quiet docker.service && test "$(systemctl show -p ActiveState --value delayer-test-disk-gate.service)" = activating; then break; fi
 sleep 1
done
systemctl is-active --quiet docker.service
test "$(systemctl show -p ActiveState --value delayer-test-disk-gate.service)" = activating
! mountpoint -q /srv/delayer-video
test "$(docker inspect -f '{{.State.Running}}' t-delayer)" = false
test "$(systemctl show -p ActiveState --value t-delayer-docker.service)" != active
test "$(find /srv/delayer-video -mindepth 1 -maxdepth 1 -printf '%f\\n')" = underlay-sentinel
systemctl list-jobs --no-pager
systemctl list-jobs --no-legend | awk '$2 == "t-delayer-docker.service" && $3 == "start" {found=1} END {exit !found}'
sleep 15
! mountpoint -q /srv/delayer-video
test "$(docker inspect -f '{{.State.Running}}' t-delayer)" = false
test "$(find /srv/delayer-video -mindepth 1 -maxdepth 1 -printf '%f\\n')" = underlay-sentinel
""", timeout=240).stdout
        (output / "blocked-jobs.txt").write_text(blocked)
        passed("reboot: Docker active while recording disk and container remain blocked")
        passed("no recording data written to unmounted underlying directory")
        event(f"Ubuntu {release}: releasing disk mount; waiting for automatic container startup")
        after = ssh("""
touch /run/delayer-test-disk-release
for attempt in $(seq 1 360); do
 if systemctl is-active --quiet t-delayer-docker.service; then break; fi
 sleep 1
done
systemctl is-active t-delayer-docker.service
mountunit=$(systemd-escape --path --suffix=mount /srv/delayer-video)
mount_time=$(systemctl show -p ActiveEnterTimestampMonotonic --value "$mountunit")
app_time=$(systemctl show -p ExecMainStartTimestampMonotonic --value t-delayer-docker.service)
test "$mount_time" -gt 0
test "$app_time" -ge "$mount_time"
printf 'mount_active_usec=%s\\nservice_exec_start_usec=%s\\n' "$mount_time" "$app_time"
test "$(docker inspect -f '{{.State.Health.Status}}' t-delayer)" = healthy
test "$(cat /srv/delayer-video/recording-sentinel)" = 'recording disk data'
test ! -e /srv/delayer-video/underlay-sentinel
test "$(docker exec t-delayer psql -At -U delayer -d delayerdb -c 'SELECT value FROM vm_boot_sentinel')" = preserved
sha256sum --check /root/delayer-before.sha256
findmnt --mountpoint /srv/delayer-video
systemctl show t-delayer-docker.service -p RequiresMountsFor -p After -p ActiveState -p SubState
docker inspect -f '{{.State.StartedAt}} {{.State.Health.Status}}' t-delayer
""", timeout=420).stdout
        (output / "after-release.txt").write_text(after)
        passed("container startup follows recording mount activation", after)
        passed("database, recording disk and authentication persist after reboot")
        journal = ssh("mountunit=$(systemd-escape --path --suffix=mount /srv/delayer-video); "
                      "journalctl -b --no-pager -o short-monotonic -u docker.service -u t-delayer-docker.service "
                      "-u delayer-test-disk-gate.service -u \"$mountunit\"").stdout
        (output / "boot-journal.txt").write_text(journal)
        result["status"] = "passed"
        save()
        event(f"Ubuntu {release}: ALL VM CHECKS PASSED")
    except BaseException as error:
        result["status"] = "failed"
        result["error"] = str(error)
        save()
        try:
            diagnostics = ssh("systemctl --failed --no-pager; systemctl status t-delayer-docker --no-pager || true; "
                              "journalctl -b --no-pager -n 150; docker logs --tail 100 t-delayer || true", check=False).stdout
            (output / "failure-diagnostics.txt").write_text(diagnostics)
        except BaseException:
            pass
        raise
    finally:
        try:
            ssh("systemctl poweroff", check=False, timeout=20)
            qemu.wait(timeout=45)
        except BaseException:
            qemu.terminate()
            try:
                qemu.wait(timeout=10)
            except subprocess.TimeoutExpired:
                qemu.kill()
                qemu.wait()
        serial.close()


failed = False
for release in (["24.04", "26.04"] if args.ubuntu == "all" else [args.ubuntu]):
    try:
        run_guest(release)
    except BaseException:
        traceback.print_exc()
        failed = True
server.shutdown()
raise SystemExit(1 if failed else 0)
