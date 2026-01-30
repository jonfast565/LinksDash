#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
SRC_SVG="$ROOT_DIR/assets/linksdash_icon.svg"
OUT_DIR="$ROOT_DIR/assets/icons"
MAC_DIR="$OUT_DIR/macos"
WIN_DIR="$OUT_DIR/windows"
LINUX_DIR="$OUT_DIR/linux"
PYTHON_BIN=${PYTHON_BIN:-python3}

if [[ ! -f "$SRC_SVG" ]]; then
  echo "SVG not found: $SRC_SVG" >&2
  exit 1
fi

TMP_DIR=$(mktemp -d)
cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT

BASE_PNG="$TMP_DIR/icon_1024.png"

render_svg() {
  if command -v rsvg-convert >/dev/null 2>&1; then
    rsvg-convert -w 1024 -h 1024 -o "$BASE_PNG" "$SRC_SVG"
    return 0
  fi

  if command -v inkscape >/dev/null 2>&1; then
    inkscape "$SRC_SVG" --export-type=png --export-filename="$BASE_PNG" -w 1024 -h 1024 >/dev/null 2>&1
    return 0
  fi

  if command -v magick >/dev/null 2>&1; then
    magick -background none "$SRC_SVG" -resize 1024x1024 "$BASE_PNG"
    return 0
  fi

  if command -v convert >/dev/null 2>&1; then
    convert -background none "$SRC_SVG" -resize 1024x1024 "$BASE_PNG"
    return 0
  fi

  if command -v qlmanage >/dev/null 2>&1; then
    qlmanage -t -s 1024 -o "$TMP_DIR" "$SRC_SVG" >/dev/null 2>&1
    local thumb
    thumb=$(ls "$TMP_DIR"/*.png | head -n1 || true)
    if [[ -z "$thumb" ]]; then
      echo "qlmanage failed to produce PNG" >&2
      exit 1
    fi
    mv "$thumb" "$BASE_PNG"
    return 0
  fi

  echo "No SVG renderer found. Install rsvg-convert, inkscape, or ImageMagick." >&2
  exit 1
}

resize_png() {
  local size=$1
  local out=$2
  if command -v rsvg-convert >/dev/null 2>&1; then
    rsvg-convert -w "$size" -h "$size" -o "$out" "$SRC_SVG"
    return 0
  fi
  if command -v inkscape >/dev/null 2>&1; then
    inkscape "$SRC_SVG" --export-type=png --export-filename="$out" -w "$size" -h "$size" >/dev/null 2>&1
    return 0
  fi
  if command -v sips >/dev/null 2>&1; then
    sips -Z "$size" "$BASE_PNG" --out "$out" >/dev/null 2>&1
  else
    if command -v magick >/dev/null 2>&1; then
      magick "$BASE_PNG" -resize "${size}x${size}" "$out"
    else
      echo "No PNG resizer found (sips or ImageMagick)." >&2
      exit 1
    fi
  fi
}

render_svg

mkdir -p "$MAC_DIR" "$WIN_DIR" "$LINUX_DIR"

# macOS .icns
ICONSET_DIR="$TMP_DIR/LinksDash.iconset"
mkdir -p "$ICONSET_DIR"

resize_png 16 "$ICONSET_DIR/icon_16x16.png"
resize_png 32 "$ICONSET_DIR/icon_16x16@2x.png"
resize_png 32 "$ICONSET_DIR/icon_32x32.png"
resize_png 64 "$ICONSET_DIR/icon_32x32@2x.png"
resize_png 128 "$ICONSET_DIR/icon_128x128.png"
resize_png 256 "$ICONSET_DIR/icon_128x128@2x.png"
resize_png 256 "$ICONSET_DIR/icon_256x256.png"
resize_png 512 "$ICONSET_DIR/icon_256x256@2x.png"
resize_png 512 "$ICONSET_DIR/icon_512x512.png"
resize_png 1024 "$ICONSET_DIR/icon_512x512@2x.png"

if command -v iconutil >/dev/null 2>&1; then
  iconutil -c icns "$ICONSET_DIR" -o "$MAC_DIR/LinksDash.icns"
else
  echo "iconutil not found; cannot create .icns" >&2
  exit 1
fi

# Windows .ico
WIN_PNG_DIR="$TMP_DIR/win"
mkdir -p "$WIN_PNG_DIR"
resize_png 16 "$WIN_PNG_DIR/icon_16.png"
resize_png 24 "$WIN_PNG_DIR/icon_24.png"
resize_png 32 "$WIN_PNG_DIR/icon_32.png"
resize_png 48 "$WIN_PNG_DIR/icon_48.png"
resize_png 64 "$WIN_PNG_DIR/icon_64.png"
resize_png 128 "$WIN_PNG_DIR/icon_128.png"
resize_png 256 "$WIN_PNG_DIR/icon_256.png"

$PYTHON_BIN "$ROOT_DIR/scripts/png2ico.py" \
  "$WIN_DIR/LinksDash.ico" \
  "$WIN_PNG_DIR/icon_16.png" \
  "$WIN_PNG_DIR/icon_24.png" \
  "$WIN_PNG_DIR/icon_32.png" \
  "$WIN_PNG_DIR/icon_48.png" \
  "$WIN_PNG_DIR/icon_64.png" \
  "$WIN_PNG_DIR/icon_128.png" \
  "$WIN_PNG_DIR/icon_256.png"

# Linux convenience PNG
resize_png 256 "$LINUX_DIR/linksdash_icon_256.png"

echo "Generated icons in $OUT_DIR"
