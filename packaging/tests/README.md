# Package verification

The [Ubuntu VM validation report](VM-VALIDATION.md) records successful fresh
attended installation and real systemd reboot tests for both 4.1.2 packages on
Ubuntu 24.04 and 26.04, including the delayed recording mount. The previous
4.1.1 validation and fixes are retained as historical evidence in that report.

Version 4.1.2 passed the attended package tests below on both Ubuntu releases
on 2026-11-12. Results and terminal transcripts are retained under
`build-artifacts/debian-qa-4.1.2/{24.04,26.04}/evidence/`. The separate full VM
validation passed on the same date; its evidence is under
`build-artifacts/vm-tests/2026-11-12-4.1.2-attended/{24.04,26.04}/`.

From the repository root:

```sh
python3 -m unittest discover -s packaging/tests -p 'test_*.py' -v
./packaging/build-deb.sh --ubuntu all --version 4.1.2
python3 packaging/tests/runtime.py --image t-delayer:runtime-4.1.2-ubuntu24.04-amd64
python3 packaging/tests/runtime.py --image t-delayer:runtime-4.1.2-ubuntu26.04-amd64
```

The runtime test uses unique containers/networks and temporary bind directories.
It runs the existing C/UDP/UI integration suite inside the single container,
checks PostgreSQL's Unix-only listener, reconfigures from an owned bridge to an
existing bridge, recreates the container, and verifies configuration, recordings,
and changed-password persistence. All test resources are removed. `--smoke`
limits the run to service startup and recreation.

Exercise the actual Debian maintainer scripts in a disposable container, once
per release (replace `24.04` with `26.04` for the second run):

```sh
mkdir -p build-artifacts/debian-qa-4.1.2/24.04/share
mkdir -p build-artifacts/debian-qa-4.1.2/24.04/evidence
docker run --rm --platform linux/amd64 --entrypoint sh \
  -e DELAYER_PACKAGE_TEST=isolated \
  -e DELAYER_ATTENDED_TRANSCRIPT=/results/attended-installer.log \
  --mount "type=bind,src=$PWD/build-artifacts/debian-qa-4.1.2/24.04/share,dst=/usr/share/t-delayer" \
  --mount "type=bind,src=$PWD/build-artifacts/debian-qa-4.1.2/24.04/evidence,dst=/results" \
  --mount "type=bind,src=$PWD/packaging/tests,dst=/tests,readonly" \
  --mount "type=bind,src=$PWD/build-artifacts/deb,dst=/packages,readonly" \
  t-delayer:runtime-4.1.2-ubuntu24.04-amd64 \
  /tests/debian.sh /packages/t-delayer-docker_4.1.2-1ubuntu24.04_amd64.deb
```

This test uses real `dpkg` and debconf with Docker/systemd test doubles: it
forces dependency checks because those host services are intentionally absent
inside the disposable runtime image. It drives a real PTY/readline frontend for
fresh attended installation and reconfiguration, including loopback, all-interface
and custom IPv4 web-bind answers, critical priority, and a repeated config call
that must not prompt twice. It also checks unattended preseeding, host/bridge
listener generation, credentials, uninstall, purge, and data retention. The
transcript is saved under the evidence directory. The share bind mount keeps the
unpacked bundled image archive on the host filesystem. This is not a test of the
host's Docker daemon or systemd integration. Use the full VM harness below for
those integration checks.

## Real Ubuntu VM installation and reboot tests

The QEMU harness boots full amd64 Ubuntu cloud images with their own kernels,
systemd as PID 1, a real Docker Engine, and a separate virtual recording block
disk. Docker only hosts the QEMU process; Docker/systemd inside the guest are
not mocked. No privileged runner or host Docker socket mount is needed.

Allow at least 8 GiB free for cloud images and guest disks, and at least
4 GiB available RAM for the runner. A guest uses 3 GiB RAM and two virtual CPUs;
the releases run sequentially. On ARM hosts, QEMU software emulation is slower
than a native amd64 machine. Downloads use official Ubuntu cloud images and
are checked against the published SHA-256 checksums.

```sh
docker build -t t-delayer:vm-test-runner packaging/tests/vm
docker volume create t-delayer-vm-test-work
mkdir -p build-artifacts/vm-tests/manual-run
docker run --rm --name t-delayer-vm-test \
  --mount type=volume,src=t-delayer-vm-test-work,dst=/work \
  --mount "type=bind,src=$PWD/build-artifacts/deb,dst=/packages,readonly" \
  --mount "type=bind,src=$PWD/build-artifacts/vm-tests/manual-run,dst=/results" \
  t-delayer:vm-test-runner --ubuntu all --version 4.1.2 --attended
```

Use `--ubuntu 24.04` or `--ubuntu 26.04` for one release. The guest installs the
matching existing `.deb` with APT, including real dependencies. The harness:

1. With `--attended`, drives the actual readline frontend through a guest PTY,
   without application preseeding. It checks all five prompts appear exactly
   once, including web-bind, and verifies the saved answers. APT's `-y` answers
   its dependency confirmation; debconf remains attended. Without this flag the
   harness uses the original noninteractive preseed flow.
2. Verifies the service is enabled and the container becomes healthy.
3. Writes database, recording, and authentication persistence sentinels.
4. Reboots the guest, deliberately blocking the actual recording disk's mount
   behind a test-only systemd gate.
5. Verifies Docker is running while the application start job waits, the
   container is stopped, and the underlying unmounted directory is unchanged.
6. Releases the disk gate and verifies automatic startup without issuing a new
   service start command. It compares monotonic mount and application-start
   timestamps and verifies persisted data.

Results include `result.json`, `attended-install.json`, `install.log`, `boot-journal.txt`, serial-console
logs, blocked jobs, and post-mount timing evidence per release. Guest disks and
the test SSH key remain only in the named work volume. Remove that volume after
collecting evidence if it is no longer needed:

```sh
docker volume rm t-delayer-vm-test-work
```

If Docker's internal disk is short of space, replace the `/work` volume with a
bind mount of a dedicated empty directory on a filesystem with free space.
Check both the host filesystem and Docker's internal disk before starting.

Physical multicast in host mode or macvlan/ipvlan still needs verification on
the deployment NIC, switch, and routing setup. A VM storage/boot test does not
establish physical multicast behavior.
