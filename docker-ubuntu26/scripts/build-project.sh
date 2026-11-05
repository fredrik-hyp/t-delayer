#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/env.sh"

TARGET="${1:-all}"

mkdir -p "$ARTIFACT_ROOT"

for ARCH in $(for_each_arch "$TARGET"); do
	CONTAINER=$(container_for_arch "$ARCH")
	OUT_DIR="$ARTIFACT_ROOT/$ARCH"
	BIN_DIR="$OUT_DIR/bin"
	MANIFEST="$OUT_DIR/manifest.txt"

	if ! docker container inspect "$CONTAINER" >/dev/null 2>&1 || [ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null || printf false)" != true ]; then
		printf 'Container %s is not running; starting it first.\n' "$CONTAINER"
		"$SCRIPT_DIR/start.sh" "$ARCH"
	fi

	rm -rf "$OUT_DIR"
	mkdir -p "$BIN_DIR"

	{
		printf 'T-Delayer Ubuntu %s artifacts\n' "$UBUNTU_VERSION"
		printf 'target_arch=%s\n' "$ARCH"
		printf 'built_at=%s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
		printf '\n'
	} > "$MANIFEST"

	printf 'Building project in %s...\n' "$CONTAINER"
	docker exec -e TARGET_ARCH="$ARCH" "$CONTAINER" sh -lc '
		set -eu
		BUILD_ROOT=/tmp/t-delayer-build
		OUT_DIR=/workspace/build-artifacts/ubuntu-26.04/$TARGET_ARCH
		BIN_DIR=$OUT_DIR/bin

		rm -rf "$BUILD_ROOT"
		mkdir -p "$BUILD_ROOT"
		cp -R /workspace/pov-common /workspace/pov-database /workspace/pov-delayer2 "$BUILD_ROOT"/

		cd "$BUILD_ROOT/pov-delayer2"
		rm -f \
			delayer \
			recorder \
			player \
			tools/read_chunk_file/read_chunk_file \
			tools/show_statistic/*.o \
			tools/show_statistic/show_statistic \
			tools/mc_analyser/*.o \
			tools/mc_analyser/mc_receiver

		./make.sh -d -T
		cd tools/mc_analyser
		make clean >/dev/null 2>&1 || true
		make

		install -m 555 "$BUILD_ROOT/pov-delayer2/delayer" "$BIN_DIR/delayer"
		install -m 555 "$BUILD_ROOT/pov-delayer2/recorder" "$BIN_DIR/recorder"
		install -m 555 "$BUILD_ROOT/pov-delayer2/player" "$BIN_DIR/player"
		install -m 555 "$BUILD_ROOT/pov-delayer2/recorder" "$BIN_DIR/delayer-recorder"
		install -m 555 "$BUILD_ROOT/pov-delayer2/player" "$BIN_DIR/delayer-player"
		install -m 555 "$BUILD_ROOT/pov-delayer2/tools/read_chunk_file/read_chunk_file" "$BIN_DIR/read_chunk_file"
		install -m 555 "$BUILD_ROOT/pov-delayer2/tools/mc_analyser/mc_receiver" "$BIN_DIR/mc_receiver"
		install -m 555 "$BUILD_ROOT/pov-delayer2/tools/show_statistic/show_statistic" "$BIN_DIR/show_statistic"
	'

file "$BIN_DIR/delayer" >> "$MANIFEST"
if [ -x "$BIN_DIR/delayer" ]; then
	file "$BIN_DIR/recorder" >> "$MANIFEST"
	file "$BIN_DIR/player" >> "$MANIFEST"
	file "$BIN_DIR/delayer-recorder" >> "$MANIFEST"
	file "$BIN_DIR/delayer-player" >> "$MANIFEST"
fi
	file "$BIN_DIR/read_chunk_file" >> "$MANIFEST"
	file "$BIN_DIR/mc_receiver" >> "$MANIFEST"
	file "$BIN_DIR/show_statistic" >> "$MANIFEST"

	printf '\nArtifacts written to %s\n' "$OUT_DIR" >> "$MANIFEST"
	printf 'Artifacts for %s written to %s\n' "$ARCH" "$OUT_DIR"
done
