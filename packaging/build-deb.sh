#!/bin/sh
# Build self-contained amd64 packages; Docker provides all Linux build tools.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(dirname "$SCRIPT_DIR")
UBUNTU=all
VERSION=4.1.2
OUTPUT="$REPO_ROOT/build-artifacts/deb"
usage() {
    echo "Usage: $0 [--ubuntu 24.04|26.04|all] [--version VERSION] [--output DIRECTORY]"
}
while [ "$#" -gt 0 ]; do
    case "$1" in
        --ubuntu|--version|--output)
            [ "$#" -ge 2 ] || { usage >&2; exit 2; }
            case "$1" in
                --ubuntu) UBUNTU=$2 ;; --version) VERSION=$2 ;; --output) OUTPUT=$2 ;;
            esac
            shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; exit 2 ;;
    esac
done
case "$UBUNTU" in 24.04|26.04) TARGETS=$UBUNTU ;; all) TARGETS="24.04 26.04" ;; *) usage >&2; exit 2 ;; esac
case "$VERSION" in ''|*[!0-9A-Za-z.+~]*|[!0-9]*) echo "Invalid package version: $VERSION" >&2; exit 2 ;; esac
command -v docker >/dev/null
command -v python3 >/dev/null
docker info >/dev/null
mkdir -p "$OUTPUT"
OUTPUT=$(CDPATH= cd -- "$OUTPUT" && pwd)
STAGING=$(mktemp -d "${TMPDIR:-/tmp}/t-delayer-deb.XXXXXX")
trap 'rm -rf "$STAGING"' EXIT HUP INT TERM
TAG_VERSION=$(printf '%s' "$VERSION" | tr '+~' '__')

for TARGET in $TARGETS; do
    DEB_VERSION="$VERSION-1ubuntu$TARGET"
    RUNTIME="t-delayer:runtime-$TAG_VERSION-ubuntu$TARGET-amd64"
    BASE="t-delayer:runtime-base-$TAG_VERSION-ubuntu$TARGET-amd64"
    PACKAGE="$STAGING/$TARGET"
    SHARE="$PACKAGE/usr/share/t-delayer"
    echo "Building Ubuntu $TARGET / amd64 images..."
    docker build --platform linux/amd64 --build-arg "UBUNTU_VERSION=$TARGET" \
        -f "$REPO_ROOT/docker-runtime/Dockerfile" -t "$BASE" "$REPO_ROOT"
    docker build --platform linux/amd64 --build-arg "RUNTIME_IMAGE=$BASE" \
        -f "$SCRIPT_DIR/Dockerfile" -t "$RUNTIME" "$REPO_ROOT"
    for IMAGE in "$RUNTIME"; do
        [ "$(docker image inspect --format '{{.Os}}/{{.Architecture}}' "$IMAGE")" = linux/amd64 ] || {
            echo "Image is not linux/amd64: $IMAGE" >&2; exit 1;
        }
    done
    mkdir -p "$PACKAGE/DEBIAN" "$PACKAGE/usr/sbin" "$PACKAGE/usr/lib/systemd/system" \
        "$SHARE" "$PACKAGE/usr/share/doc/t-delayer-docker"
    cp "$SCRIPT_DIR/debian/"* "$PACKAGE/DEBIAN/"
    sed "s/@VERSION@/$DEB_VERSION/g" "$SCRIPT_DIR/debian/control" > "$PACKAGE/DEBIAN/control"
    sed "s/@UBUNTU@/$TARGET/g" "$SCRIPT_DIR/debian/preinst" > "$PACKAGE/DEBIAN/preinst"
    cp "$SCRIPT_DIR/t-delayerctl.py" "$PACKAGE/usr/sbin/t-delayerctl"
    cp "$SCRIPT_DIR/t-delayer-docker.service" "$PACKAGE/usr/lib/systemd/system/"
    cp "$SCRIPT_DIR/INSTALL.md" "$PACKAGE/usr/share/doc/t-delayer-docker/INSTALL.md"
    docker image inspect "$RUNTIME" > "$STAGING/image-inspect.json"
    python3 - "$SCRIPT_DIR/t-delayerctl.py" "$STAGING/image-inspect.json" "$SHARE/images.json" "$TARGET" "$RUNTIME" <<'PY'
import importlib.util
import json
from pathlib import Path
import sys
spec = importlib.util.spec_from_file_location("ctl", sys.argv[1])
ctl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ctl)
image = json.loads(Path(sys.argv[2]).read_text())[0]
Path(sys.argv[3]).write_text(json.dumps({"ubuntu": sys.argv[4], "architecture": "amd64",
    "runtime": sys.argv[5], "runtime_id": image["Id"], "runtime_fingerprint": ctl.image_fingerprint(image)}) + "\n")
PY
    docker image save --output "$SHARE/images.tar" "$RUNTIME"
    chmod 755 "$PACKAGE/DEBIAN/"config "$PACKAGE/DEBIAN/"preinst \
        "$PACKAGE/DEBIAN/"postinst "$PACKAGE/DEBIAN/"prerm "$PACKAGE/DEBIAN/"postrm \
        "$PACKAGE/usr/sbin/t-delayerctl"
    chmod 644 "$PACKAGE/DEBIAN/control" "$PACKAGE/DEBIAN/templates"
    DEB_NAME="t-delayer-docker_${DEB_VERSION}_amd64.deb"
    docker run --rm --platform linux/amd64 \
        --mount "type=bind,src=$PACKAGE,dst=/package" \
        --mount "type=bind,src=$OUTPUT,dst=/output" ubuntu:24.04 \
        sh -eu -c '
            cd /package
            du -sk usr | awk "{print \"Installed-Size: \" \$1}" >> DEBIAN/control
            find usr -type f -print0 | sort -z | xargs -0 md5sum > DEBIAN/md5sums
            dpkg-deb --root-owner-group -Zgzip -z1 --build /package "/output/.$1.tmp"
            mv "/output/.$1.tmp" "/output/$1"
            cd /output
            sha256sum "$1" > "$1.sha256"
        ' sh "$DEB_NAME"
    echo "Created $OUTPUT/$DEB_NAME"
done
