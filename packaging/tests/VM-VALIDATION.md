# Ubuntu installation and systemd reboot validation

## Version 4.1.2: attended installation and recording-disk ordering

Completed on **12 November 2026**. Both unchanged **4.1.2 amd64** packages passed
in fresh full Ubuntu VMs. Docker and systemd inside the guests were real services,
with no Docker/systemd test doubles.

| Check | Ubuntu 24.04.5 LTS | Ubuntu 26.04.1 LTS |
| --- | --- | --- |
| Fresh attended APT installation with actual Docker dependencies | PASS | PASS |
| All five English debconf prompts appear exactly once, including web-bind | PASS | PASS |
| Saved network, web-bind, web-port and recording-directory answers | PASS | PASS |
| Service enabled; container healthy; on-failure restart policy | PASS | PASS |
| `systemd-analyze verify` exits successfully | PASS | PASS |
| Full guest reboot, confirmed by a changed kernel boot ID | PASS | PASS |
| Docker active while the recording mount and application start remain blocked | PASS | PASS |
| Container stays stopped; no new files beneath the unmounted recording directory | PASS | PASS |
| Automatic healthy startup after releasing the recording mount | PASS | PASS |
| Database row, recording sentinel, credentials and UI authentication survive reboot | PASS | PASS |

### Environment and attended method

Each fresh QEMU amd64 guest used UEFI, its own Linux kernel, systemd as PID 1,
two virtual CPUs, 3 GiB RAM, and a separate ext4 recording block disk. The disk
was mounted at `/srv/delayer-video` by UUID through `/etc/fstab` with `nofail`.
The guests ran under software emulation on an ARM host. Docker hosted only the
QEMU runner; no host Docker socket or privileged runner was used.

Ubuntu 24.04 used kernel `6.8.0-147-generic`, systemd 255, and Docker 29.1.3
(`29.1.3-0ubuntu3~24.04.2`). Ubuntu 26.04 used kernel `7.0.0-38-generic`,
systemd 259, and Docker 29.1.3 (`29.1.3-0ubuntu4.1`). Both Docker engines used
an overlayfs/containerd image store. Official Ubuntu cloud-image SHA-256
checksums were verified before boot; the exact image hashes are in `result.json`.

The harness ran APT in a real guest pseudo-terminal with
`DEBIAN_FRONTEND=readline` and priority `high`, without application preseeding.
It entered `host`, web-bind `127.0.0.1`, port `18080`, a blank multicast interface,
and recording directory `/srv/delayer-video`. It checked that each prompt appeared
exactly once and that the generated configuration contained those answers.
APT's `-y` handled its dependency confirmation; debconf remained attended.
`install.log` contains the actual terminal transcript, and `attended-install.json`
records the prompt assertions. Both installations initialized new databases.

A test-only systemd prerequisite held the real recording mount across reboot.
The harness verified an active Docker daemon, stopped application container,
queued application start job and unchanged underlying directory, then repeated
the negative checks after 15 seconds. Releasing the gate let the already queued
boot job continue. No new application service start command was issued.
The container became healthy, the database row and recording sentinel were read
back, and credential/authentication file checksums were verified.

`systemd-analyze verify` returned zero for both guests. On Ubuntu 26.04 it also
printed warnings about removed `CPUAccounting=` settings in Ubuntu's unrelated
`xfs_scrub_all.service` and `system-xfs_scrub.slice`; these were not application
unit errors.

### Measured ordering

Monotonic microseconds since guest boot; service time is the start of its
`ExecStart` helper, before container creation and readiness checks.

| Release | Recording mount active | Service `ExecStart` | Service starts after mount |
| --- | ---: | ---: | ---: |
| 24.04 | 136965505 | 136999208 | **33.703 ms** |
| 26.04 | 181742841 | 181790712 | **47.871 ms** |

Both units included `RequiresMountsFor=/var/lib/t-delayer /srv/delayer-video`
and an ordering dependency on `srv-delayer\x2dvideo.mount`.

### Exact artifacts and evidence

The checksums below match both the source artifacts and `/tmp/delayer.deb`
inside each guest. No package or application-runtime change was needed.

| Package | SHA-256 |
| --- | --- |
| `t-delayer-docker_4.1.2-1ubuntu24.04_amd64.deb` | `929e864621c43cd4becba60b828bb328a2052eb53fc957a554edcfed8eae5fa3` |
| `t-delayer-docker_4.1.2-1ubuntu26.04_amd64.deb` | `f604927571f9bb8090b23a8885e5c8539ac331a255f17976b0903389b80f8770` |

Raw evidence is retained under `build-artifacts/vm-tests/2026-11-12-4.1.2-attended/`,
with a separate directory for each release:

- `result.json`: all assertions, exact package/cloud-image hashes and distinct boot IDs.
- `attended-install.json` and `install.log`: actual attended installation and saved-answer checks.
- `docker-and-package.txt`: guest package checksum, Docker version and image driver.
- `blocked-jobs.txt`: queued application and mount jobs while the gate is held.
- `after-release.txt`: mount/start timestamps, healthy status and persistence checks.
- `boot-journal.txt`, `guest.txt`, `commands.log`, `serial.log`: guest and boot evidence.
- The parent `runner.log` records completion of both VM tests.

See [the test instructions](README.md#real-ubuntu-vm-installation-and-reboot-tests)
and [the harness](vm/run.py); use `--version 4.1.2 --attended` with fresh guest
disks to repeat this run. The guests were shut down after verification. Disposable
VM disks, downloaded cloud images and the test SSH key were removed after evidence
collection. Physical multicast hardware, throughput, abrupt power loss and physical
disk failure are outside this validation.

## Historical validation: version 4.1.1

Completed on **11 November 2026**. Both final **4.1.1 amd64** packages passed.

| Check | Ubuntu 24.04.5 LTS | Ubuntu 26.04.1 LTS |
| --- | --- | --- |
| Real package installation/configuration and Docker dependencies | PASS | PASS |
| Service enabled; container healthy | PASS | PASS |
| `systemd-analyze verify` for the service | PASS | PASS |
| Full guest reboot, confirmed by a changed kernel boot ID | PASS | PASS |
| Docker active while recording mount and application start remain blocked | PASS | PASS |
| Container stays stopped; underlying directory receives no new files | PASS | PASS |
| Automatic startup after the mount gate is released | PASS | PASS |
| Database row and recording-file sentinel survive reboot | PASS | PASS |
| Database credentials and UI authentication file checksums survive reboot | PASS | PASS |

### Environment and method

The tests used full QEMU amd64 VMs with UEFI firmware, their own Linux kernels,
systemd as PID 1, and actual Ubuntu `docker.io` packages. Each guest had two
virtual CPUs, 3 GiB RAM, and a separate ext4 recording block disk mounted at
`/srv/delayer-video` through a UUID entry in `/etc/fstab`. The mount used
`nofail`, so the application's explicit mount dependency was exercised even
when the rest of the host could continue booting.

The guests ran under software emulation on an ARM host. Docker hosted the QEMU
processes; the guest's Docker and systemd were genuine services. The guest
kernels were `6.8.0-147-generic` and `7.0.0-38-generic`, with systemd 255 and 259,
respectively. Both used Docker 29.1.3 with its containerd image store.

Initial installation used APT and installed Docker, containerd and runc from
Ubuntu repositories. After correcting the issues described below, the final
4.1.1 candidates were unpacked and configured using `dpkg --unpack` followed by
`apt-get --fix-broken install` in the same disposable guests. Ubuntu 24.04's
application database and authentication directory were reset before the final
installation to verify cold PostgreSQL initialization.

A test-only systemd prerequisite deliberately held the recording mount across
reboot. The harness checked the stopped container, queued application start
job, active Docker daemon, and unchanged underlying directory, then repeated
the negative checks after 15 seconds. Releasing the gate allowed the existing
boot job to continue. No new application service start command was issued.

### Measured ordering

These are monotonic microseconds since the respective guest boot. The service
timestamps refer to the start of its `ExecStart` helper, before container
creation and readiness checks.

| Release | Recording mount active | Service `ExecStart` | Service starts after mount |
| --- | ---: | ---: | ---: |
| 24.04 | 287002971 | 287050630 | **47.659 ms** |
| 26.04 | 283889119 | 283989305 | **100.186 ms** |

Both generated services reported
`RequiresMountsFor=/var/lib/t-delayer /srv/delayer-video` and an ordering
dependency on `srv-delayer\x2dvideo.mount`. The queued application start waited
for that mount, while the container remained stopped. After release, both
containers became healthy and all persistence assertions passed.

### Issues found and corrected

1. **Docker image-store compatibility.** Version 4.1.0 tried to locate the loaded
   image using the build host's image ID. The receiving containerd image store
   assigned a manifest digest instead of the classic store's config digest.
   Version 4.1.1 verifies the filesystem layers and execution configuration,
   then starts the verified image using the receiving engine's immutable ID.
   This store-dependent ID behavior is also described in
   [Moby issue 51934](https://github.com/moby/moby/issues/51934).
2. **Premature startup failure during database initialization.** A temporary
   unhealthy probe stopped the container before the configured readiness
   deadline. Startup now allows a running container to recover within its full
   180-second budget. The final Ubuntu 24.04 cold initialization passed, and a
   regression test checks recovery from an initial unhealthy probe.

The eight packaging regression tests passed. Package-hook tests also passed
for both releases, including reconfiguration, removal and data retention.
At that time, **4.1.1** superseded the earlier 4.1.0 artifacts. Version 4.1.2 is validated above.

### Exact final artifacts

| Package | SHA-256 |
| --- | --- |
| `t-delayer-docker_4.1.1-1ubuntu24.04_amd64.deb` | `10f1995f89e5e2592b02bf76ad1184f3384e493e674c818465a0596477f0e502` |
| `t-delayer-docker_4.1.1-1ubuntu26.04_amd64.deb` | `27476bef6aef853ab334d2f980859ba4338d98af26625329a4aa603cff89ec7a` |

The final result JSONs record these same package checksums and distinct
before/after boot IDs. Raw evidence is retained under
`build-artifacts/vm-tests/2026-11-11-final/`, separately for each release:

- `result.json`: assertions, package/cloud-image hashes, and boot IDs.
- `install.log`: actual package installation and unit validation.
- `blocked-jobs.txt`: queued mount/application jobs during the held mount.
- `after-release.txt`: timestamps, healthy status and persistence checks.
- `boot-journal.txt`: Docker, disk-gate, mount and application boot ordering.
- `guest.txt`, `commands.log`, `serial.log`: guest details and diagnostic evidence.

See [the test instructions](README.md#real-ubuntu-vm-installation-and-reboot-tests)
and [the harness](vm/run.py) to repeat the tests. The test runner shut down the
guests after verification. Hardware multicast, throughput, abrupt power loss,
and physical disk failure remain outside this validation.
