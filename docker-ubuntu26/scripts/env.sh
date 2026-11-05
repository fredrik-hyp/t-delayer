#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DOCKER_ENV_DIR=$(dirname "$SCRIPT_DIR")
REPO_ROOT=$(dirname "$DOCKER_ENV_DIR")

UBUNTU_VERSION=26.04
IMAGE_PREFIX=t-delayer-build
CONTAINER_PREFIX=t-delayer-ubuntu26
ARTIFACT_ROOT="$REPO_ROOT/build-artifacts/ubuntu-26.04"

normalize_arch() {
	case "${1:-}" in
		amd64|x86_64)
			printf '%s\n' amd64
			;;
		arm64|aarch64)
			printf '%s\n' arm64
			;;
		all|'')
			printf '%s\n' all
			;;
		*)
			printf 'Unsupported architecture: %s\n' "$1" >&2
			printf 'Use one of: amd64, x86_64, arm64, aarch64, all\n' >&2
			exit 2
			;;
	esac
}

platform_for_arch() {
	case "$1" in
		amd64)
			printf '%s\n' linux/amd64
			;;
		arm64)
			printf '%s\n' linux/arm64
			;;
		*)
			printf 'Unsupported normalized architecture: %s\n' "$1" >&2
			exit 2
			;;
	esac
}

image_for_arch() {
	printf '%s:ubuntu%s-%s\n' "$IMAGE_PREFIX" "$UBUNTU_VERSION" "$1"
}

container_for_arch() {
	printf '%s-%s\n' "$CONTAINER_PREFIX" "$1"
}

for_each_arch() {
	_arch=$(normalize_arch "${1:-all}")
	if [ "$_arch" = all ]; then
		printf '%s\n' amd64
		printf '%s\n' arm64
	else
		printf '%s\n' "$_arch"
	fi
}
