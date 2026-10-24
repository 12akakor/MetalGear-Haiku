#!/bin/sh
# Copyright 2026, Akakor. Distributed under the terms of the MIT License.
#
# Makes the automatic HD room tiles from your ROM and puts them where the
# game looks for them. Checks first what is needed; your own drawings
# already in hd/tiles are kept.
#
#   tools/hd/make_hd.sh [rom] [hd folder]
#
# From the installed package (no source code needed):
#   /boot/system/apps/MetalGear/tools/make_hd.sh
#
# Without a ROM the one in the game's settings folder is used. Without a
# folder: on Haiku the game's settings folder
# (~/config/settings/MetalGear/hd), elsewhere ./hd (copy it to the Haiku
# machine afterwards).

set -u

die()
{
	echo "make_hd: $*" >&2
	exit 1
}

# Paths given are relative to where this is started; the work happens in
# the folder of the source code.
START=$(pwd)
absolute()
{
	case "$1" in
		/*) echo "$1" ;;
		*) echo "$START/$1" ;;
	esac
}
HERE=$(cd "$(dirname "$0")" && pwd) || die "cannot find my own folder"
# Installed package: the tools are built already and sit next to this
# script. Source code: they are built with make.
if [ -x "$HERE/gfx_export" ] && [ -x "$HERE/xbr_png" ] && [ -f "$HERE/hd_auto.py" ]; then
	BUILD=no
	GFX_EXPORT="$HERE/gfx_export"
	XBR_PNG="$HERE/xbr_png"
	HD_AUTO="$HERE/hd_auto.py"
	ROOT=$HOME
else
	BUILD=yes
	ROOT=$(cd "$HERE/../.." && pwd) || die "cannot find the source folder"
	GFX_EXPORT="$ROOT/gfx_export"
	XBR_PNG="$ROOT/xbr_png"
	HD_AUTO="$ROOT/tools/hd/hd_auto.py"
fi
cd "$ROOT" || die "cannot enter $ROOT"

HAIKU=no
[ "$(uname -s)" = "Haiku" ] && HAIKU=yes

# The game's settings folder (only on Haiku).
SETTINGS=""
if [ $HAIKU = yes ]; then
	if command -v finddir >/dev/null 2>&1; then
		SETTINGS="$(finddir B_USER_SETTINGS_DIRECTORY)/MetalGear"
	else
		SETTINGS="$HOME/config/settings/MetalGear"
	fi
fi

# The ROM.
ROM="${1:-}"
[ -n "$ROM" ] && ROM=$(absolute "$ROM")
if [ -z "$ROM" ] && [ -n "$SETTINGS" ]; then
	if [ -f "$SETTINGS/MetalGear.rom" ]; then
		ROM="$SETTINGS/MetalGear.rom"
	else
		for f in "$SETTINGS"/*.rom "$SETTINGS"/*.ROM; do
			[ -f "$f" ] && ROM="$f" && break
		done
	fi
fi
[ -n "$ROM" ] || die "no ROM given and none in the settings folder.
  usage: tools/hd/make_hd.sh [rom] [hd folder]"
[ -f "$ROM" ] || die "ROM not found: $ROM"

# Where the tiles go.
OUT="${2:-}"
[ -n "$OUT" ] && OUT=$(absolute "$OUT")
if [ -z "$OUT" ]; then
	if [ -n "$SETTINGS" ]; then
		OUT="$SETTINGS/hd"
	else
		OUT="$ROOT/hd"
	fi
fi
mkdir -p "$OUT/tiles" || die "cannot make $OUT/tiles"
OUT=$(cd "$OUT" && pwd)

# What is needed: a C++ compiler and make, python 3 with numpy, scipy and
# Pillow.
MISSING=""
if [ $BUILD = yes ]; then
	command -v make >/dev/null 2>&1 || MISSING="$MISSING make"
	command -v g++ >/dev/null 2>&1 || command -v c++ >/dev/null 2>&1 || MISSING="$MISSING g++"
fi

PYTHON=""
BEST=""
for p in python3.10 python3 python; do
	command -v $p >/dev/null 2>&1 || continue
	[ -z "$BEST" ] && BEST=$p
	if $p -c "import numpy, scipy.ndimage, PIL" >/dev/null 2>&1; then
		PYTHON=$p
		break
	fi
done
if [ -z "$PYTHON" ]; then
	if [ -z "$BEST" ]; then
		MISSING="$MISSING python3"
	else
		for m in numpy scipy PIL; do
			$BEST -c "import $m" >/dev/null 2>&1 || MISSING="$MISSING $m"
		done
	fi
fi
if [ -n "$MISSING" ]; then
	echo "make_hd: missing:$MISSING" >&2
	if [ $HAIKU = yes ]; then
		echo "  install with:" >&2
		echo "  pkgman install python3.10 numpy_python310 scipy_python310 pillow_python310" >&2
		case "$MISSING" in
			*make*|*g++*) echo "  pkgman install make gcc" >&2 ;;
		esac
	else
		echo "  install with:  python3 -m pip install numpy scipy pillow" >&2
	fi
	exit 1
fi
echo "make_hd: ROM $ROM"
echo "make_hd: python $PYTHON, tiles into $OUT/tiles"

# The tools, built from this source (the package has them built).
if [ $BUILD = yes ]; then
	make gfx_export xbr_png || die "building gfx_export and xbr_png failed"
fi

TMP=$(mktemp -d 2>/dev/null || echo "/tmp/make_hd.$$")
mkdir -p "$TMP" || die "cannot make a temporary folder"
trap 'rm -rf "$TMP"' EXIT INT TERM

"$GFX_EXPORT" "$ROM" "$TMP/export" || die "reading the rooms from the ROM failed"
[ -f "$TMP/export/tiles.csv" ] || die "no tiles from the ROM (is it a Metal Gear ROM?)"

BEFORE=$(ls "$OUT/tiles" | grep -c '\.png$')
$PYTHON "$HD_AUTO" "$TMP/export" "$OUT" "$XBR_PNG" \
	|| die "making the HD tiles failed"
AFTER=$(ls "$OUT/tiles" | grep -c '\.png$')
[ "$AFTER" -gt 0 ] || die "no HD tiles were written"

echo
echo "make_hd: $AFTER HD tiles in $OUT/tiles ($((AFTER - BEFORE)) new)"
if [ $HAIKU = yes ]; then
	echo "  Start the game again; F6 switches the HD room tiles on or off."
else
	echo "  Copy the folder $OUT to ~/config/settings/MetalGear/hd on Haiku."
fi
