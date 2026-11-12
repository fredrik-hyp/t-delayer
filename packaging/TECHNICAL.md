# T-Delayer technical manual

Package generation for Ubuntu 24.04 LTS and 26.04 LTS on amd64.

## Build packages

From the repository root on a machine with Python 3 and a working Docker CLI/daemon:

```sh
./packaging/build-deb.sh --ubuntu all --version 4.1.2
# Or build one release:
./packaging/build-deb.sh --ubuntu 24.04 --version 4.1.2
./packaging/build-deb.sh --ubuntu 26.04 --version 4.1.2
```

The build machine needs network access to Ubuntu registries/APT repositories,
several GB of free space, and support for `linux/amd64` containers. On ARM build
machines, Docker must provide amd64 emulation (Docker Desktop normally does).
All compilation and `dpkg-deb` operations run in Linux containers. The C debug
and foreground release builds and `read_chunk_file` build run as part of the
image build. The final image includes the release binary and English web UI.

Output defaults to `build-artifacts/deb/`; change it with `--output DIRECTORY`:

```text
t-delayer-docker_4.1.2-1ubuntu24.04_amd64.deb
t-delayer-docker_4.1.2-1ubuntu26.04_amd64.deb
```

Each package contains a complete single-container image matching its Ubuntu
release, the image content manifest, installer questions, service, helper, and the
installation manual. Installation loads that bundled image; it does not compile source code
or pull an application image from a registry. APT still needs repositories for
missing host dependencies unless you have provisioned them beforehand.

## Installer questions and web-bind

The English debconf template `t-delayer-docker/web-bind` is a string with default
`127.0.0.1`, labelled **Web UI bind address (web-bind):**. The package config
script asks it in its own step after the Docker network questions. Its priority
is `critical`, so an attended frontend displays it even when the frontend is set
to show only critical questions. Other network and runtime questions use `high`.

Debconf's normal seen flags are retained. APT can run the config script during
preconfiguration and again during package configuration; resetting these flags
would repeat the same question. Previously configured installations retain their
answers on upgrade. To show all questions again, use:

```sh
sudo dpkg-reconfigure -f readline -p low t-delayer-docker
```

The noninteractive frontend continues to use preseeded or default values without
prompting. The post-install script reads the debconf answer into `web_bind` in
`/etc/t-delayer/config.json`. Reconfiguration validates the result and recreates
the container while preserving persistent data and credentials.

In host mode the address controls nginx's listener. In bridge mode it controls
Docker's published host address; nginx listens on port 8080 inside the container.
Macvlan/ipvlan require `0.0.0.0` and expose the chosen port at the container IP.
The bind address does not select the multicast interface.

## Installer verification

Run the unit tests and disposable package tests described in
`packaging/tests/README.md`. The attended test drives a real terminal with the
readline frontend, real dpkg/debconf and the package maintainer scripts. It checks
fresh installation, loopback/all-interface/custom web-bind values, critical
priority, host/bridge listener generation, repeated config execution, unattended
preseeding, and data retention. Docker and systemd are test doubles in this test.

Both unchanged 4.1.2 amd64 packages passed fresh attended installation in full
Ubuntu 24.04 and 26.04 VMs on 12 November 2026, using real Docker and systemd.
All five installer prompts appeared exactly once and their saved answers were
verified. Both guests then passed a full reboot with the recording mount held:
Docker ran while the container remained stopped and the application start job
waited. Releasing the mount triggered automatic healthy startup, with database,
recording, credential and authentication data preserved. The complete evidence
and earlier 4.1.1 results are recorded in `packaging/tests/VM-VALIDATION.md`.
