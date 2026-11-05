# Ubuntu 26.04 Docker Build Environment

This directory contains the Docker setup for building T-Delayer on Ubuntu 26.04 for both `amd64`/`x86_64` and `arm64`/`aarch64`.

## Requirements

- Docker with BuildKit/buildx support.
- QEMU/binfmt support if the host CPU differs from the target architecture.

## Quick Start

Build the Docker images:

```sh
./docker-ubuntu26/scripts/build-images.sh all
```

Start build containers:

```sh
./docker-ubuntu26/scripts/start.sh all
```

Verify that the containers are running and that the toolchain is available:

```sh
./docker-ubuntu26/scripts/verify.sh all
```

Build project binaries and collect artifacts:

```sh
./docker-ubuntu26/scripts/build-project.sh all
```

Stop the containers:

```sh
./docker-ubuntu26/scripts/stop.sh all
```

## Output

Build artifacts are copied to:

```text
build-artifacts/ubuntu-26.04/amd64/bin/
build-artifacts/ubuntu-26.04/arm64/bin/
```

The current project builds one main `delayer` binary. That binary is used for the live recording/delayer process and for delayed playback/output according to the runtime database state. For convenience the artifact directory also contains `delayer-recorder` and `delayer-player` copies that point at the same current binary role.

The artifact bundle also includes useful test/support binaries when they build successfully:

- `read_chunk_file`
- `mc_receiver`
- `show_statistic`

Each architecture directory gets a `manifest.txt` with file type information for the produced binaries.
