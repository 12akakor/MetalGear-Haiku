#!/bin/sh
# Copyright 2026, Akakor. Distributed under the terms of the MIT License.
#
# Makes the automatic HD room tiles from your ROM and puts them where the
# game looks for them (the settings folder). Needs python3 with numpy,
# scipy and Pillow. Your own drawings already in hd/tiles are kept.
#
#   tools/hd/make_hd.sh <rom> [hd folder]
set -e
ROM=$1
if [ -z "$ROM" ]; then
	echo "usage: tools/hd/make_hd.sh <rom> [hd folder]"
	exit 2
fi
if [ -n "$2" ]; then
	OUT=$2
elif [ -d "$HOME/config/settings" ]; then
	OUT=$HOME/config/settings/MetalGear/hd	# Haiku
else
	OUT=hd
fi
make gfx_export xbr_png
TMP=$(mktemp -d)
./gfx_export "$ROM" "$TMP/export"
python3 tools/hd/hd_auto.py "$TMP/export" "$OUT" ./xbr_png
rm -rf "$TMP"
echo "HD tiles in $OUT (F6 in the game switches them on or off)"
