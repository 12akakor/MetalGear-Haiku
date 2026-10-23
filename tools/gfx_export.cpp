/*
 * Exports the graphics of every room for drawing HD versions.
 *
 * Development tool: it runs the original code on the reference machine
 * (the room loading and drawing routines), records every 8x8 tile the
 * game copies to the screen and writes:
 *   rooms/room_NNN.png        the room as the game draws it (256x192)
 *   rooms/room_NNN.keys       the tile key of every 8x8 cell (32x24)
 *   rooms_x4/room_NNN.png     the room enlarged 4x, to paint on
 *   tiles/<key>.png           every distinct tile, original size
 *   sheets/tileset_N.png      all tiles of a tileset, 8x enlarged, on a grid
 *   tiles.csv                 key, tileset, rooms that use the tile
 * The key is a hash of the tile's pixels (colour numbers) plus the number
 * of the room palette, "<hash>_p<palette>". The game computes the same key
 * when it draws a tile and uses hd/tiles/<key>.png when it exists.
 *
 *   gfx_export <rom> <outdir>
 */
#include "../src/msx/Machine.h"
#include "../src/game/GameData.h"
#include "../src/game/RomSymbols.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb/stb_image_write.h"


namespace {

const uint16_t kRoom = 0xc130;
#define kLoadRoomTiles RomSymbol(sym::LoadRoomTiles)
#define kSetRoomPal RomSymbol(sym::SetRoomPal)
#define kRenderRoom RomSymbol(sym::RenderRoom)
#define kSetRoomPal2 RomSymbol(sym::SetRoomPal2)
const uint16_t kReturn = 0xe000;

Machine sMachine;
uint64_t sTime = 0;

void
call(uint16_t address)
{
	Z80Regs& r = sMachine.Cpu().r;
	sMachine.Write(0x6000, 1);
	sMachine.Write(0x8000, 2);
	sMachine.Write(0xa000, 3);
	uint8_t saved = sMachine.Ram()[kReturn - 0xc000];
	sMachine.Ram()[kReturn - 0xc000] = 0x76;
	uint16_t sp = 0xf000;
	sp -= 2;
	sMachine.Write(sp, kReturn & 0xff);
	sMachine.Write((uint16_t)(sp + 1), kReturn >> 8);
	r.sp = sp;
	r.pc = address;
	r.iff1 = r.iff2 = false;
	long guard = 0;
	while (r.pc != kReturn && ++guard < 50000000) {
		if (sMachine.onStep)
			sMachine.onStep();
		sTime += (uint64_t)sMachine.StepInstruction();
		sMachine.GetVdp().SetTime(sTime);
	}
	sMachine.Ram()[kReturn - 0xc000] = saved;
	if (guard >= 50000000)
		fprintf(stderr, "routine %04X did not return\n", address);
}

void
palette_rgb(uint8_t rgb[16][3])
{
	const uint16_t* p = sMachine.GetVdp().Palette();
	for (int i = 0; i < 16; i++) {
		int r = (p[i] >> 4) & 7, b = p[i] & 7, g = (p[i] >> 8) & 7;
		rgb[i][0] = (uint8_t)(r * 255 / 7);
		rgb[i][1] = (uint8_t)(g * 255 / 7);
		rgb[i][2] = (uint8_t)(b * 255 / 7);
	}
}

int
pixel(int x, int y)
{
	const uint8_t* vram = sMachine.GetVdp().Vram();
	uint8_t b = vram[(((uint32_t)y << 7) | ((uint32_t)x >> 1)) & 0x1ffff];
	return (x & 1) ? (b & 0x0f) : (b >> 4);
}

std::string
key_of(const uint8_t* rgb, int count)
{
	uint64_t h = 1469598103934665603ULL;
	for (int i = 0; i < count; i++) {
		h ^= rgb[i];
		h *= 1099511628211ULL;
	}
	char buf[20];
	snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)h);
	return buf;
}

struct Tile {
	std::vector<uint8_t> rgb;	/* 8x8x3 */
	int tileset;
	int palette;
	int position;				/* tile number in VRAM page 1 (0-511) */
	std::set<int> rooms;
};

}	// namespace


int
main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: gfx_export <rom> <outdir>\n");
		return 2;
	}
	std::vector<uint8_t> rom(0x20000);
	FILE* fp = fopen(argv[1], "rb");
	if (fp == NULL || fread(rom.data(), 1, rom.size(), fp) != rom.size())
		return 1;
	fclose(fp);
	std::string out = argv[2];
	mkdir(out.c_str(), 0755);
	mkdir((out + "/rooms").c_str(), 0755);
	mkdir((out + "/tiles").c_str(), 0755);
	mkdir((out + "/sheets").c_str(), 0755);
	mkdir((out + "/rooms_x4").c_str(), 0755);

	std::string error;
	if (!SetGameRom(rom, &error)) {
		fprintf(stderr, "%s\n", error.c_str());
		return 1;
	}
	sMachine.LoadRom(rom);
	sMachine.Reset();
	/* Start a game: SPACE on the title screen. */
	for (long f = 0; f < 2000; f++) {
		if (f == 400 || f == 700 || f == 1000 || f == 1300)
			sMachine.SetKey(8, 0, true);
		if (f == 405 || f == 705 || f == 1005 || f == 1305)
			sMachine.SetKey(8, 0, false);
		sMachine.RunFrame();
	}
	if (sMachine.Ram()[0] != 5) {
		fprintf(stderr, "game did not start (status %d)\n", sMachine.Ram()[0]);
		return 1;
	}

	std::map<std::string, Tile> tiles;
	std::vector<std::string> order;
	int room = 0;
	int tileset = 0;
	int paletteId = 0;
	uint8_t pal[16][3];
	std::vector<std::string> roomKeys(32 * 24);
	sMachine.onStep = [&]() {
		if (sMachine.Cpu().r.pc == kSetRoomPal2)		/* SetRoomPal2: A = palette */
			paletteId = sMachine.Cpu().r.a;
	};
	sMachine.GetVdp().onCommand = [&](const Vdp::Command& c) {
		if (c.cmd != 0x0d || c.nx != 8 || c.ny != 8 || (c.dy >> 8) != 0)
			return;
		std::vector<uint8_t> rgb(8 * 8 * 3);
		uint8_t indices[64];
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				int i = pixel(c.sx + x, c.sy + y);
				indices[y * 8 + x] = (uint8_t)i;
				memcpy(&rgb[(y * 8 + x) * 3], pal[i], 3);
			}
		}
		char suffix[16];
		snprintf(suffix, sizeof(suffix), "_p%02d", paletteId);
		std::string key = key_of(indices, 64) + suffix;
		int position = ((c.sy & 255) / 8) * 32 + c.sx / 8;
		if (c.dx % 8 == 0 && c.dy % 8 == 0 && c.dy < 192)
			roomKeys[(c.dy / 8) * 32 + c.dx / 8] = key;
		auto it = tiles.find(key);
		if (it == tiles.end()) {
			tiles[key] = { rgb, tileset, paletteId, position, { room } };
			order.push_back(key);
		} else
			it->second.rooms.insert(room);
	};

	for (room = 0; room < 251; room++) {
		sMachine.Ram()[kRoom - 0xc000] = (uint8_t)room;
		/* Flashlight selected: dark rooms get their normal palette. */
		sMachine.Ram()[0xc135 - 0xc000] = 3;
		call(kLoadRoomTiles);
		call(kSetRoomPal);
		palette_rgb(pal);
		/* tileset ID of the room: RoomGfxSetIds nibble, as LoadRoomTiles
		   stores it in CurrentTileSet */
		tileset = sMachine.Ram()[0xc157 - 0xc000];
		std::fill(roomKeys.begin(), roomKeys.end(), std::string("-"));
		call(kRenderRoom);
		palette_rgb(pal);
		std::vector<uint8_t> image(256 * 192 * 3);
		for (int y = 0; y < 192; y++) {
			for (int x = 0; x < 256; x++)
				memcpy(&image[(y * 256 + x) * 3], pal[pixel(x, y)], 3);
		}
		char name[64];
		snprintf(name, sizeof(name), "/rooms/room_%03d.png", room);
		stbi_write_png((out + name).c_str(), 256, 192, 3, image.data(), 256 * 3);
		/* 4x enlarged template to paint on */
		std::vector<uint8_t> big(1024 * 768 * 3);
		for (int y = 0; y < 768; y++)
			for (int x = 0; x < 1024; x++)
				memcpy(&big[(y * 1024 + x) * 3], &image[((y / 4) * 256 + x / 4) * 3], 3);
		snprintf(name, sizeof(name), "/rooms_x4/room_%03d.png", room);
		stbi_write_png((out + name).c_str(), 1024, 768, 3, big.data(), 1024 * 3);
		snprintf(name, sizeof(name), "/rooms/room_%03d.keys", room);
		FILE* kf = fopen((out + name).c_str(), "w");
		for (int y = 0; y < 24; y++) {
			for (int x = 0; x < 32; x++)
				fprintf(kf, x ? " %s" : "%s", roomKeys[y * 32 + x].c_str());
			fprintf(kf, "\n");
		}
		fclose(kf);
	}

	/* tiles, csv, sheets */
	FILE* csv = fopen((out + "/tiles.csv").c_str(), "w");
	fprintf(csv, "key,tileset,palette,position,rooms\n");
	std::map<std::pair<int, int>, std::vector<std::string> > perSet;
	for (auto& key : order) {
		Tile& t = tiles[key];
		stbi_write_png((out + "/tiles/" + key + ".png").c_str(), 8, 8, 3, t.rgb.data(), 24);
		fprintf(csv, "%s,%d,%d,%d,", key.c_str(), t.tileset, t.palette, t.position);
		bool first = true;
		for (int r : t.rooms) {
			fprintf(csv, first ? "%d" : " %d", r);
			first = false;
		}
		fprintf(csv, "\n");
		perSet[std::make_pair(t.tileset, t.palette)].push_back(key);
	}
	fclose(csv);
	const int scale = 8, cell = 8 * scale + 2, columns = 16;
	for (auto& s : perSet) {
		std::sort(s.second.begin(), s.second.end(),
			[&](const std::string& a, const std::string& b) {
				return tiles[a].position < tiles[b].position; });
		int rows = ((int)s.second.size() + columns - 1) / columns;
		int w = columns * cell, h = rows * cell;
		std::vector<uint8_t> sheet((size_t)w * h * 3, 40);
		for (size_t i = 0; i < s.second.size(); i++) {
			Tile& t = tiles[s.second[i]];
			int ox = (int)(i % columns) * cell + 1, oy = (int)(i / columns) * cell + 1;
			for (int y = 0; y < 8 * scale; y++)
				for (int x = 0; x < 8 * scale; x++)
					memcpy(&sheet[((size_t)(oy + y) * w + ox + x) * 3],
						&t.rgb[((y / scale) * 8 + x / scale) * 3], 3);
		}
		char name[64];
		snprintf(name, sizeof(name), "/sheets/tileset_%d_palette_%02d.png", s.first.first,
			s.first.second);
		stbi_write_png((out + name).c_str(), w, h, 3, sheet.data(), w * 3);
	}
	printf("%zu rooms, %zu distinct tiles, %zu tilesets\n", (size_t)251, order.size(), perSet.size());
	return 0;
}
