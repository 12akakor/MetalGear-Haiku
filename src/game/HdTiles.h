/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_HD_TILES_H
#define MG_HD_TILES_H


// HD tiles: drawn (or generated, tools/hd/hd_auto.py) versions of the 8x8
// room tiles, hd/tiles/<key>.png. The key is the one of tools/gfx_export:
// a hash of the tile's 64 colour numbers plus the room palette,
// "<hash>_p<palette>". Every frame the background is looked up cell by
// cell; a cell whose pixels match a tile is shown in HD, anything else
// (sprites, changed cells, palette effects) keeps the normal picture.

#include <stdint.h>
#include <string>


class Screen;

// Side of an HD tile in pixels (4x).
enum { kHdTileSize = 32 };

// Reads <dir>/tiles/*.png; returns the number of tiles (0: none).
int		LoadHdTiles(const std::string& dir);
int		HdTileCount();

// Called by SetRoomPal2: the room palette now in use.
void	HdSetRoomPalette(uint8_t palette, const Screen& screen);

// For the frame just rendered: per pixel the HD tile (32x32, 0xffRRGGBB)
// that covers it, or NULL, and its place in the 8x8 cell (x | y << 3).
// 'sprites' marks the pixels drawn by sprites (from RenderScreenIndexed).
// Returns false when nothing is shown in HD.
bool	HdBuildOverlay(const Screen& screen, const uint8_t* sprites,
			const uint32_t** tiles, uint8_t* local);


#endif	// MG_HD_TILES_H
