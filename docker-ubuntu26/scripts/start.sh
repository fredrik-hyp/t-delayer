#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/env.sh"

TARGET="${1:-all}"

mkdir -p "$ARTIFACT_ROOT"

for ARCH in $(for_each_arch "$TARGET"); do
	PLATFORM=$(platform_for_arch "$ARCH")
	IMAGE=$(image_for_arch "$ARCH")
	CONTAINER=$(container_for_arch "$ARCH")

	if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
		printf 'Image %s is missing; building it first.\n' "$IMAGE"
		"$SCRIPT_DIR/build-images.sh" "$ARCH"
	fi

	if docker container inspect "$CONTAINER" >/dev/null 2>&1; then
		if [ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER")" = true ]; then
			printf 'Container %s is already running.\n' "$CONTAINER"
		else
			printf 'Starting existing container %s...\n' "$CONTAINER"
			docker start "$CONTAINER" >/dev/null
		fi
	else
		printf 'Creating and starting %s for %s...\n' "$CONTAINER" "$PLATFORM"
		docker run -d \
			--name "$CONTAINER" \
			--platform "$PLATFORM" \
			-v "$REPO_ROOT:/workspace" \
			-w /workspace \
			"$IMAGE" >/dev/null
	fi
done
