#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/env.sh"

TARGET="${1:-all}"

for ARCH in $(for_each_arch "$TARGET"); do
	PLATFORM=$(platform_for_arch "$ARCH")
	IMAGE=$(image_for_arch "$ARCH")

	printf 'Building %s for %s...\n' "$IMAGE" "$PLATFORM"
	docker buildx build \
		--load \
		--platform "$PLATFORM" \
		-t "$IMAGE" \
		-f "$DOCKER_ENV_DIR/Dockerfile" \
		"$DOCKER_ENV_DIR"
done
