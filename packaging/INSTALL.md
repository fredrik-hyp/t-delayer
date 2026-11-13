# T-Delayer Docker package: installation and administration

This package runs **PostgreSQL, the delayer processes, nginx, and the web UI
controller in one Docker container per installation**. The default container is
named `t-delayer`; additional named installations run alongside it. A host systemd
service manages each container. No Docker Compose installation is required. PostgreSQL
accepts connections only through a Unix socket inside the container; it does not
listen on a host or container TCP port.

## Supported hosts and requirements

- Ubuntu Server **24.04 LTS or 26.04 LTS**, **amd64 (x86-64)**, with systemd.
  Install the package built for your exact Ubuntu release. Ubuntu releases named
  24.06 and 26.06 do not exist.
- A rootful Linux Docker Engine and its CLI, from Ubuntu (`docker.io`) or
  Docker's official repository (`docker-ce`). The package declares this dependency;
  APT installs `docker.io` if neither is present. Do not mix these distributions.
  Rootless Docker, Podman, and Docker Desktop are not deployment targets.
- Root/sudo access, Python 3, and debconf (installed as package dependencies).
- A practical starting allocation is 2 CPU cores and 4 GiB RAM. This is a sizing
  recommendation, not a measured capacity guarantee. Allow more for additional
  streams/outputs and verify performance at the intended traffic rate.
- Free space for the `.deb`, its unpacked image archive, Docker's loaded image,
  PostgreSQL, and recordings. Reserve at least 5 GiB for installation overhead
  in addition to the OS, database growth, and video. Docker normally stores its
  files under `/var/lib/docker`; the bundled archive is under `/usr/share/t-delayer`.
- Recording storage with sustained write/read throughput for all streams.
  Approximate payload storage is **bitrate in Mbit/s × 0.45 GB per hour** per input,
  plus fixed-record/header overhead. At 20 Mbit/s, allow more than 9 GB per hour.
  Use a dedicated mounted disk for production; mount it before installing.
- For physical multicast: a suitable NIC, multicast routing, switch/IGMP
  configuration, UDP firewall rules, and sufficient network capacity. Host
  networking is recommended. Bridge port publishing does not forward physical
  multicast group traffic automatically. Choose a local unicast IPv4 address
  when selecting the multicast NIC. Keep the host clock synchronized.
- A browser to use the UI. Remote access should use an HTTPS reverse proxy.
  The default UI bind is `127.0.0.1`; open the chosen web port only when required.

Install host prerequisites from Ubuntu, for example:

```sh
sudo apt update
sudo apt install docker.io python3 debconf
sudo systemctl enable --now docker
sudo docker info
```

Do not run a legacy systemd delayer process or another container for the same
streams/output destinations during migration. This package does not import an
existing installation's database or recordings automatically.

## Interactive installation

Each package contains a complete single-container image matching its Ubuntu
release, the image content manifest, installer questions, service, helper, and this
manual. Installation loads that bundled image; it does not compile source code
or pull an application image from a registry. APT still needs repositories for
missing host dependencies unless you have provisioned them beforehand.

Copy the matching `.deb` to the host and run:

```sh
sudo apt install ./t-delayer-docker_4.2.0-1ubuntu24.04_amd64.deb
# On Ubuntu 26.04, use the ubuntu26.04 package instead.
```

The installer asks in English. **Web UI bind address (web-bind):** is a separate
step after the Docker network questions:

| Setting | Meaning |
| --- | --- |
| Docker network type | `host` (default), `bridge`, or `existing` |
| Existing network name | Only for `existing`; create the network first |
| Static container IPv4 | Optional for an existing network; otherwise Docker allocates it |
| Web-bind address | Default `127.0.0.1`; host address for host/bridge; `0.0.0.0` for macvlan/ipvlan |
| Web UI TCP port | Default `8080` |
| Multicast interface IPv4 | Optional; blank uses routing; host NIC IP in host mode, container IP in macvlan/ipvlan |
| Recording directory | Default `/var/lib/t-delayer/video`, mounted as `/home/video` |

The installer generates independent random database and initial UI passwords,
loads the image, creates persistent storage, enables `t-delayer-docker.service`,
and starts the container. First startup initializes the database through schema
version 31. Wait for installation to finish, then inspect:

```sh
sudo systemctl status t-delayer-docker
sudo t-delayerctl status
sudo t-delayerctl credentials
```

For the default settings, open `http://127.0.0.1:8080` on the host, or use an SSH
tunnel from your workstation:

```sh
ssh -L 8080:127.0.0.1:8080 user@delayer-host
```

Open `http://127.0.0.1:8080` locally and sign in as `admin` with the initial
password. Use **Change password** in the UI after signing in. The helper shows
only the initial password; after a UI password change, the saved UI password
hash takes precedence. Container recreation and package upgrades preserve it.

## Network choices

The attended **Web UI bind address (web-bind):** question accepts:

| Bind address | UI access |
| --- | --- |
| `127.0.0.1` (default) | Host only; use an SSH tunnel or local HTTPS proxy for remote access |
| `0.0.0.0` | All IPv4 interfaces; restrict access with your firewall and use HTTPS remotely |
| A local IPv4 address | One host interface in host/bridge mode; the address must belong to this host |

For an existing macvlan/ipvlan network, enter `0.0.0.0` and connect to the
container's IP address. The bind address is separate from the multicast interface.

**Host:** the container shares the host's network namespace. Multicast can use
the host's NICs and routing. nginx binds directly to the chosen web bind/port;
there is no Docker port mapping. PostgreSQL remains on its private Unix socket.
For a host with multiple NICs, enter the multicast NIC's local IPv4 address.

**Bridge:** the package creates the user-defined `t-delayer-bridge` network.
The UI is published at the chosen host bind/port; nginx uses port 8080 inside
the container. This mode suits UI tests and multicast traffic between containers
joined to that same bridge. It does not configure multicast forwarding from the
physical LAN. The host NIC's IPv4 address is not local to a bridge container;
leave the multicast interface blank unless you specifically configure its IP.

**Existing:** attach to a user-defined bridge, macvlan, or ipvlan network created
by the administrator. The installer checks the driver and optional static IP.
It never changes or removes this network. Bridge networks publish the UI as
above. Macvlan/ipvlan use the container's address and the chosen web port
directly, without a published host port; set the web bind to `0.0.0.0`.
Use an IPvlan L2 network for LAN multicast, not an L3 network.

For example, adapt these **example addresses** and interface to your LAN before
creating a macvlan network:

```sh
sudo docker network create --driver macvlan \
  --subnet 192.168.42.0/24 --gateway 192.168.42.1 \
  --ip-range 192.168.42.192/27 -o parent=enp3s0 delayer-lan
```

Select `existing`, network `delayer-lan`, a reserved unused container address
such as `192.168.42.200`, web bind `0.0.0.0`, port `8080`, and optionally multicast
interface `192.168.42.200`. Reserve the address/range outside the LAN DHCP pool.
Access `http://192.168.42.200:8080` from another LAN machine. Macvlan normally
prevents direct host-to-container communication; configure a host macvlan
interface if you need it. NIC/switch support for multiple MAC addresses is
required. Physical macvlan/ipvlan multicast must be verified on the actual host.

## Named installations on the same server

The existing installation remains `default`. All commands without a name retain
its container `t-delayer`, service `t-delayer-docker.service`, configuration,
credentials and data paths. `default` is also accepted explicitly; no migration
or renaming is required.

Create and start two independent installations with separate web ports:

```sh
sudo t-delayerctl create production --web-port 9010
sudo t-delayerctl create test --web-port 9020
sudo t-delayerctl list
sudo t-delayerctl credentials production
sudo t-delayerctl credentials test
```

Names use 1–40 lowercase letters, digits, underscores or dashes, starting with a
letter or digit. `default` is reserved. Each installation starts with its own
empty stream configuration; add streams in its UI or import JSON. Passwords,
PostgreSQL, recordings and login sessions are independent. Stream names may be
reused between installations because their databases and recording directories
are separate.

`create` validates settings, writes private configuration and credentials, and
enables/starts `t-delayer@NAME.service`. Use `--no-start` to enable it for boot
without starting immediately. If `--web-port` is omitted, the helper selects a
free host port from 8080, skipping listeners reserved by other configured
installations, including stopped ones. Defaults are host networking and web bind
`127.0.0.1`; select `--web-bind 0.0.0.0` or a local address for remote access.

Creation and reconfiguration accept `--network-mode`, `--network-name`,
`--container-ip`, `--web-bind`, `--web-port`, `--multicast-interface`, and
`--video-path`. The same multicast/network requirements described above apply.
For example, place a named recording tree on a dedicated mounted disk:

```sh
sudo t-delayerctl create archive --web-port 9030 --video-path /srv/archive-video
sudo t-delayerctl configure test --web-port 9021
sudo t-delayerctl restart test
```

Named reconfiguration preserves settings not specified and keeps credentials.
It takes effect on restart. Configuration changes never move existing recordings.
A restart interrupts that installation's recording/playback and reapplies stream
autostart settings; temporary playback positions are not restored. Other
installations continue running.

```sh
sudo t-delayerctl status production
sudo t-delayerctl logs production
sudo t-delayerctl stop test
sudo t-delayerctl start test
sudo systemctl disable t-delayer@test.service   # Disable this installation at boot
sudo journalctl -u t-delayer@production.service -n 100
```

| Resource | Default | Named `production` |
| --- | --- | --- |
| Container | `t-delayer` | `t-delayer-production` |
| Host configuration | `/etc/t-delayer/` | `/etc/t-delayer/installations/production/` |
| Database, auth, credentials | `/var/lib/t-delayer/` | `/var/lib/t-delayer/installations/production/` |
| Default recording path | `/var/lib/t-delayer/video` | `/var/lib/t-delayer/installations/production/video` |
| Host service | `t-delayer-docker.service` | `t-delayer@production.service` |
| Managed bridge | `t-delayer-bridge` | `t-delayer-production-bridge` |

The helper rejects overlapping recording/private-data paths, conflicting host
web listeners, and duplicate static IPs on the same Docker network before
replacing a container. The database uses a private Unix socket in every
container, so additional database TCP ports are unnecessary. Choose distinct
multicast output host:port destinations across installations; output validation
inside a single UI does not inspect other independent databases.

All installations share the package's verified runtime image. A package upgrade
recreates the default installation as before. Running named installations keep
their current containers until individually restarted; their next start uses the
updated image. The package does not pin separate image versions per installation.

Package removal stops/disables all owned installation services and removes their
containers. Purge also removes generated `config.json` files and storage drop-ins.
Both retain databases, credentials, auth, bootstraps, recording files and images.
To reconnect retained named data after purge, use `configure NAME` with its
previous network/recording settings, then `systemctl enable --now t-delayer@NAME`;
`create` refuses to overwrite retained credentials.

## Change configuration after installation

To change network mode, network name, static IP, UI listener, multicast interface,
or recording directory, run:

```sh
sudo dpkg-reconfigure -f readline -p low t-delayer-docker
```

This repeats the English questions, validates the configuration, and restarts
the single container. Recording and playback are interrupted during the restart.
Existing database settings, recordings, UI accounts, and passwords are retained.
Already selected autostart instances start again; temporary playback positions
and manual output states are not preserved by the application.

At **Web UI bind address (web-bind):**, enter `127.0.0.1`, `0.0.0.0`, or the
chosen local IPv4 address. Installation and upgrades may skip questions already
answered in debconf; the command above shows them again. A noninteractive frontend
does not ask questions. For a fresh attended installation, you can explicitly use
`sudo env DEBIAN_FRONTEND=readline apt install ./PACKAGE.deb`.

To change inputs, outputs, retention/buffer time, or per-instance autostart,
use **Configure** on a stopped instance in the UI. Use **New instance** to add
streams and **Start instance** when ready. These settings are stored in
PostgreSQL. **Export JSON / Import JSON** transfers instance configuration,
not database contents, passwords, or recordings. All playback times use UTC.

`/etc/t-delayer/config.json` is generated from debconf. Prefer `dpkg-reconfigure`
for settings represented by the installer; direct edits of those settings are
overwritten on reconfiguration/upgrade. For an HTTPS reverse proxy, the optional
`web_cookie_secure` boolean can be set to `true` in this JSON file and is
preserved by reconfiguration. Forward the original Host header and use HTTPS
in the browser. Then apply the change:

```sh
sudo t-delayerctl validate
sudo systemctl restart t-delayer-docker
```

Changing the recording directory does **not** move files. Stop the service,
copy the complete recording tree to the new mounted disk, retaining filenames
and permissions, then run `dpkg-reconfigure` to select that directory. The
database continues to use `/home/video` inside the container. For persistent
disks, configure `/etc/fstab`; systemd is configured to wait for the selected
filesystem. Do not remove or rename a mount while recording is active.

## Unattended installation

Preseed debconf and use the noninteractive frontend. For example:

```sh
cat <<'EOF' | sudo debconf-set-selections
t-delayer-docker t-delayer-docker/network-mode select host
t-delayer-docker t-delayer-docker/web-bind string 127.0.0.1
t-delayer-docker t-delayer-docker/web-port string 8080
t-delayer-docker t-delayer-docker/multicast-interface string 192.168.42.99
t-delayer-docker t-delayer-docker/video-path string /srv/delayer-video
EOF
sudo env DEBIAN_FRONTEND=noninteractive apt install ./t-delayer-docker_4.2.0-1ubuntu24.04_amd64.deb
```

For `existing`, also preseed `network-name` and optionally `container-ip`.
Use addresses local to your deployment. Defaults are used for omitted fields.
An invalid configuration fails installation rather than silently changing modes.
Correct it with `sudo dpkg-reconfigure t-delayer-docker` once configured, or
preseed corrected values and run `sudo dpkg --configure t-delayer-docker` if
the package is still unconfigured.

## Operations, backup, upgrades, and removal

```sh
sudo systemctl stop t-delayer-docker
sudo systemctl start t-delayer-docker
sudo systemctl restart t-delayer-docker
sudo t-delayerctl logs
sudo journalctl -u t-delayer-docker -n 100
sudo docker logs --follow t-delayer
```

Docker's `on-failure` policy restarts the container after an unexpected failure.
Systemd controls boot startup and waits for the recording filesystem; the
container is not independently started by Docker when the daemon starts.
After a Docker daemon failure/recovery, check the service and restart
`t-delayer-docker` if necessary. On startup,
the wrapper starts PostgreSQL first and initializes/validates the database before
starting the controller. Shutdown stops the controller and delayer processes
before stopping PostgreSQL. A failed individual stream is reported by the UI;
it is not automatically restarted by the controller. Docker health and UI status
should both be monitored. `active (exited)` is normal for the oneshot host unit;
use `t-delayerctl status` for current container health. Docker logs rotate locally.

Persistent host locations for the default installation:

| Host path | Contents |
| --- | --- |
| `/etc/t-delayer/config.json` | Installer-generated runtime settings |
| `/etc/t-delayer/instances.json` | Initial empty bootstrap; UI settings live in PostgreSQL |
| `/var/lib/t-delayer/credentials.json` | Database password and initial UI password; root only |
| `/var/lib/t-delayer/postgresql/cluster` | PostgreSQL data directory |
| `/var/lib/t-delayer/auth` | UI password hash |
| Chosen recording directory | Chunk files; default `/var/lib/t-delayer/video` |

For a consistent filesystem backup of one installation, stop its service and
back up its configuration, state directory and recording directory if elsewhere,
then start the service. A backup of all of `/etc/t-delayer` and `/var/lib/t-delayer`
includes named installations; stop all listed installation services first. Preserve ownership and permissions, including PostgreSQL's numeric
UID. Restore using the same Ubuntu/package release. Keep credentials with the
database: generating a new bootstrap password does not change an existing DB.

For a logical database backup while the container is running:

```sh
sudo sh -c 'umask 077; docker exec t-delayer pg_dump --clean --if-exists -U delayer -d delayerdb > /root/delayerdb.sql'
```

Back up before installing an updated `.deb` with `apt install ./NEW_PACKAGE.deb`.
An upgrade recreates the container using the bundled image while retaining data
and debconf settings. This version initializes new databases to schema 31;
future schema changes require explicit migration planning.

**An Ubuntu 24.04 to 26.04 host upgrade also changes the bundled PostgreSQL major
version (16 to 18). Existing PostgreSQL data directories cannot be opened by the
other major version.** Before changing release, export with `pg_dump`, take the
full backup, and stop the service. Move the old `postgresql/cluster` directory
aside on the host (retain it for recovery), then install the new release's
package to initialize a fresh cluster. Stop all UI instances before restoring:

```sh
sudo docker exec -i t-delayer psql -U delayer -d delayerdb \
  --single-transaction -v ON_ERROR_STOP=1 < /root/delayerdb.sql
sudo systemctl restart t-delayer-docker
```

Retain the original credentials, auth directory, recording tree, and controller
ID during migration. The controller reconnects after the container restart.
Perform this procedure in a maintenance window and verify restored instances
and playback before discarding the old cluster.

Remove with `sudo apt remove t-delayer-docker`. Removal stops and removes the
owned containers, including named installations. `sudo apt purge t-delayer-docker` additionally clears debconf
answers and generated host configuration. **Both preserve database files,
credentials, auth data, recordings, and loaded Docker images.** The owned bridge
network is retained and can be reused on reinstall. External networks are never
removed. Delete retained data manually only after making any required backups.

## Troubleshooting

- Installation failure: inspect `sudo t-delayerctl logs` and the service journal.
  Check free disk space, unused web port, selected bind address, Docker status,
  and whether an external network already exists. Correct debconf answers and
  run `sudo dpkg --configure t-delayer-docker` for an unfinished installation.
- An incorrect host multicast interface is rejected before container recreation;
  a non-local container interface causes startup to fail. Clear it or select an
  IPv4 address belonging to the selected network namespace.
- UI unavailable: check bind/port and firewall/proxy settings. The default bind
  accepts only local connections. Macvlan access normally requires another LAN
  machine, not the host itself.
- No multicast: verify host routing, NIC address, IGMP membership and switches.
  Use host networking for physical LAN traffic. Port publishing is not a
  multicast router.
- Interrupted first database initialization: the wrapper refuses to reuse an
  incomplete cluster without its `.delayer-schema-ready` marker. Restore a
  known-good backup. Only if this was a new empty installation with no required
  data, stop the service and move the incomplete `postgresql/cluster` directory
  aside, then restart to initialize a new cluster. Do not add the marker by hand.
- Wrong PostgreSQL major version: follow the logical migration procedure above;
  do not modify `PG_VERSION` or delete an existing cluster to bypass the check.
- Missing loaded image: run `sudo t-delayerctl load-images`, then restart the
  service. The bundled image archive is retained for this purpose.

Background: [Ubuntu releases](https://ubuntu.com/about/release-cycle),
[Docker Engine on Ubuntu](https://docs.docker.com/engine/install/ubuntu/),
[host networking](https://docs.docker.com/engine/network/drivers/host/),
[bridge networking](https://docs.docker.com/engine/network/drivers/bridge/), and
[macvlan networking](https://docs.docker.com/engine/network/drivers/macvlan/).
