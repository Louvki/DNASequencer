#!/bin/sh
set -eu

PROJECT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
RESOURCES_DIR="$PROJECT_DIR/Resources"
ICONSET_DIR="$PROJECT_DIR/Builds/MacOSX/AppIcon.iconset"
OUTPUT_ICNS="$PROJECT_DIR/Builds/MacOSX/Icon.icns"
SRC512="$RESOURCES_DIR/AppIcon512.png"
SRC1024="$RESOURCES_DIR/AppIcon1024.png"

if [ ! -f "$SRC512" ] || [ ! -f "$SRC1024" ]; then
    echo "Missing AppIcon512.png or AppIcon1024.png in Resources" >&2
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT_ICNS")"
rm -rf "$ICONSET_DIR"
mkdir -p "$ICONSET_DIR"

sips -z 16 16 "$SRC512" --out "$ICONSET_DIR/icon_16x16.png" >/dev/null
sips -z 32 32 "$SRC512" --out "$ICONSET_DIR/icon_16x16@2x.png" >/dev/null
sips -z 32 32 "$SRC512" --out "$ICONSET_DIR/icon_32x32.png" >/dev/null
sips -z 64 64 "$SRC512" --out "$ICONSET_DIR/icon_32x32@2x.png" >/dev/null
sips -z 128 128 "$SRC512" --out "$ICONSET_DIR/icon_128x128.png" >/dev/null
sips -z 256 256 "$SRC512" --out "$ICONSET_DIR/icon_128x128@2x.png" >/dev/null
sips -z 256 256 "$SRC512" --out "$ICONSET_DIR/icon_256x256.png" >/dev/null
sips -z 512 512 "$SRC512" --out "$ICONSET_DIR/icon_256x256@2x.png" >/dev/null
cp "$SRC512" "$ICONSET_DIR/icon_512x512.png"
cp "$SRC1024" "$ICONSET_DIR/icon_512x512@2x.png"

iconutil -c icns "$ICONSET_DIR" -o "$OUTPUT_ICNS"
rm -rf "$ICONSET_DIR"
