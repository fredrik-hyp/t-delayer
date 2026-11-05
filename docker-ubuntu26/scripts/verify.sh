#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/env.sh"

TARGET="${1:-all}"

for ARCH in $(for_each_arch "$TARGET"); do
	CONTAINER=$(container_for_arch "$ARCH")

	printf 'Verifying %s...\n' "$CONTAINER"
	if ! docker container inspect "$CONTAINER" >/dev/null 2>&1; then
		printf 'Container %s does not exist. Run scripts/start.sh %s first.\n' "$CONTAINER" "$ARCH" >&2
		exit 1
	fi

	if [ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER")" != true ]; then
		printf 'Container %s exists but is not running. Run scripts/start.sh %s first.\n' "$CONTAINER" "$ARCH" >&2
		exit 1
	fi

	docker exec "$CONTAINER" sh -lc '
		set -eu
		printf "container_arch="
		uname -m
		printf "ubuntu="
		. /etc/os-release
		printf "%s\n" "$VERSION_ID"
		cc --version | sed -n "1p"
		make --version | sed -n "1p"
		pg_config --version
		test -d /workspace/pov-delayer2
	'
done
