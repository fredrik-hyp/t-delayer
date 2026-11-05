#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/env.sh"

TARGET="${1:-all}"

for ARCH in $(for_each_arch "$TARGET"); do
	CONTAINER=$(container_for_arch "$ARCH")

	if docker container inspect "$CONTAINER" >/dev/null 2>&1; then
		if [ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER")" = true ]; then
			printf 'Stopping %s...\n' "$CONTAINER"
			docker stop "$CONTAINER" >/dev/null
		else
			printf 'Container %s is already stopped.\n' "$CONTAINER"
		fi
	else
		printf 'Container %s does not exist.\n' "$CONTAINER"
	fi
done
