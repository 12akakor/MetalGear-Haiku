/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "HdTiles.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>

#include "Options.h"
#include "Screen.h"
#include "ScreenRender.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STB_IMAGE_STATIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "../../third_party/stb/stb_image.h"
#pragma GCC diagnostic pop


namespace {

struct HdTile {
	uint8_t		palette;
	std::vector<uint32_t> pixels;	// 32x32, 0xffRRGGBB
};

// By pixel hash; a hash can occur with several room palettes.
std::unordered_map<uint64_t, std::vector<HdTile>> sTiles;
int sCount = 0;

int sPalette = -1;
uint8_t sPaletteRB[16];
uint8_t sPaletteG[16];


uint64_t
CellHash(const uint8_t* indices)
{
	uint64_t h = 1469598103934665603ULL;
	for (int i = 0; i < 64; i++) {
		h ^= indices[i];
		h *= 1099511628211ULL;
	}
	return h;
}


// "<16 hex digits>_p<palette>.png"
bool
ParseName(const char* name, uint64_t* hash, int* palette)
{
	size_t length = strlen(name);
	if (length < 21 || strcmp(name + length - 4, ".png") != 0 || name[16] != '_'
		|| name[17] != 'p')
		return false;
	char hex[17];
	memcpy(hex, name, 16);
	hex[16] = '\0';
	char* end;
	*hash = strtoull(hex, &end, 16);
	if (*end != '\0')
		return false;
	*palette = static_cast<int>(strtol(name + 18, &end, 10));
	return strcmp(end, ".png") == 0 && *palette >= 0 && *palette < 256;
}


// Any size that is a multiple of 8 becomes 32x32: averaged when larger,
// repeated when smaller.
std::vector<uint32_t>
ToTileSize(const unsigned char* rgb, int size)
{
	std::vector<uint32_t> out(kHdTileSize * kHdTileSize);
	for (int y = 0; y < kHdTileSize; y++) {
		for (int x = 0; x < kHdTileSize; x++) {
			int x0 = x * size / kHdTileSize, x1 = (x + 1) * size / kHdTileSize;
			int y0 = y * size / kHdTileSize, y1 = (y + 1) * size / kHdTileSize;
			if (x1 <= x0)
				x1 = x0 + 1;
			if (y1 <= y0)
				y1 = y0 + 1;
			unsigned r = 0, g = 0, b = 0, n = 0;
			for (int sy = y0; sy < y1; sy++) {
				for (int sx = x0; sx < x1; sx++) {
					const unsigned char* p = rgb + (sy * size + sx) * 3;
					r += p[0];
					g += p[1];
					b += p[2];
					n++;
				}
			}
			out[y * kHdTileSize + x] = 0xff000000u | ((r / n) << 16) | ((g / n) << 8) | (b / n);
		}
	}
	return out;
}

}	// namespace


int
LoadHdTiles(const std::string& dir)
{
	sTiles.clear();
	sCount = 0;
	std::string tiles = dir + "/tiles";
	DIR* d = opendir(tiles.c_str());
	if (d == NULL)
		return 0;
	for (;;) {
		struct dirent* e = readdir(d);
		if (e == NULL)
			break;
		uint64_t hash;
		int palette;
		if (!ParseName(e->d_name, &hash, &palette))
			continue;
		int width, height, channels;
		std::string path = tiles + "/" + e->d_name;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);
		if (data == NULL)
			continue;
		if (width == height && width >= 8 && width % 8 == 0) {
			sTiles[hash].push_back({ static_cast<uint8_t>(palette),
				ToTileSize(data, width) });
			sCount++;
		}
		stbi_image_free(data);
	}
	closedir(d);
	return sCount;
}


int
HdTileCount()
{
	return sCount;
}


void
HdSetRoomPalette(uint8_t palette, const Screen& screen)
{
	sPalette = palette;
	for (int i = 0; i < 16; i++) {
		sPaletteRB[i] = screen.PaletteRB(i);
		sPaletteG[i] = screen.PaletteG(i);
	}
}


bool
HdBuildOverlay(const Screen& screen, const uint8_t* sprites, const uint32_t** tiles,
	uint8_t* local)
{
	if (!gOptions.hdTiles || sCount == 0 || sPalette < 0)
		return false;
	// The HD tiles are painted for the room palette: while it is changed
	// (fading, flashing, the dark without flashlight) the normal picture
	// is shown.
	for (int i = 0; i < 16; i++) {
		if (screen.PaletteRB(i) != sPaletteRB[i] || screen.PaletteG(i) != sPaletteG[i])
			return false;
	}
	if ((screen.Register(1) & 0x40) == 0)
		return false;
	int lines = ((screen.Register(9) & 0x80) != 0) ? 212 : 192;
	if (lines > 192)
		lines = 192;	// the room; the HUD below has no HD tiles
	uint32_t page = static_cast<uint32_t>((screen.Register(2) & 0x60) << 10);

	// The cells of the page shown (32 x 32), looked up when first needed.
	const uint32_t* cell[32 * 32];
	bool looked[32 * 32];
	memset(looked, 0, sizeof(looked));
	bool any = false;
	memset(tiles, 0, sizeof(const uint32_t*) * kScreenWidth * kScreenHeight);
	for (int line = 0; line < lines; line++) {
		int vline = (line + screen.Register(23)) & 0xff;
		int cy = vline >> 3;
		for (int cx = 0; cx < 32; cx++) {
			int c = cy * 32 + cx;
			if (!looked[c]) {
				looked[c] = true;
				cell[c] = NULL;
				uint8_t indices[64];
				for (int y = 0; y < 8; y++) {
					const uint8_t* row = screen.fVram + page + ((cy * 8 + y) & 0xff) * 128
						+ cx * 4;
					for (int x = 0; x < 4; x++) {
						indices[y * 8 + x * 2] = static_cast<uint8_t>(row[x] >> 4);
						indices[y * 8 + x * 2 + 1] = static_cast<uint8_t>(row[x] & 0x0f);
					}
				}
				auto it = sTiles.find(CellHash(indices));
				if (it != sTiles.end()) {
					for (const HdTile& t : it->second) {
						if (t.palette == sPalette)
							cell[c] = t.pixels.data();
					}
				}
			}
			if (cell[c] == NULL)
				continue;
			for (int x = cx * 8; x < cx * 8 + 8; x++) {
				int p = line * kScreenWidth + x;
				if (sprites != NULL && sprites[p] != 0)
					continue;
				tiles[p] = cell[c];
				local[p] = static_cast<uint8_t>((x & 7) | ((vline & 7) << 3));
				any = true;
			}
		}
	}
	return any;
}
