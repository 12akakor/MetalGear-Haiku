/*
 * Loading and drawing rooms, rewritten from Banks0123.asm.
 */
#include "room_gfx.h"

#include "gfx_basic.h"
#include "rom.h"


namespace {

enum {
	SELECTED_FLASHLIGHT = 3,
	SELECTED_GOGGLES = 4
};

/* LoadTileset: IX = tileset block (5 bytes). */
void
LoadTileset(uint16_t ix)
{
	if ((rom(ix) & 0x40) != 0) {
		/* LoadTilesFlip: the graphics of the previous block, mirrored */
		uint8_t count = ram(var::TempData);
		uint16_t dest = TileToVramAdd(rom((uint16_t)(ix + 1)));
		uint16_t source = ram16((uint16_t)(var::TempData + 1));
		Load3bppTileFlip(source, &dest, count);
		return;
	}
	uint8_t count = rom((uint16_t)(ix + 1));
	ram(var::TempData) = count;
	uint16_t dest = TileToVramAdd(rom((uint16_t)(ix + 2)));
	uint16_t source = rom16((uint16_t)(ix + 3));
	set_ram16((uint16_t)(var::TempData + 1), source);
	Load3bppTiles(source, &dest, count);
}


void
LoadColliTiles()
{
	set_banks(1);
	uint8_t tileset = ram(var::CurrentTileSet);
	set_banks(7);
	uint16_t data = rom16((uint16_t)(SYM(IdxColisTiles) + tileset * 2));
	uint16_t out = var::CollisionTiles;
	for (int i = 0; i < 32; i++) {
		uint8_t a = rom(data++);
		for (int bit = 0; bit < 8; bit++) {
			a = (uint8_t)((a << 1) | (a >> 7));
			ram(out) = (uint8_t)(a & 1);
			out = (uint16_t)((out & 0xff00) | (uint8_t)(out + 1));
		}
	}
	set_banks(1);
}

}	// namespace


void
LoadPowSwitTiles()
{
	set_banks(13);
	SetTilesetColors();
	uint16_t dest = 0x9048;
	Load3bppTiles(SYM(gfxPowSwitch), &dest, 4);
	set_banks(1);
}


void
LoadGfxCrates()
{
	set_banks(7);
	SetTilesetColors();
	uint16_t dest = 0x9400;
	Load3bppTiles(SYM(GfxCrates), &dest, 8);
	dest = 0x9840;
	Load3bppTileFlip(SYM(GfxCrates), &dest, 8);
	set_banks(1);
}


void
LoadRoomTiles()
{
	SetTilesetColors();
	set_banks(7);
	uint8_t tileset = GetNibbleRoom(SYM(RoomGfxSetIds));
	if (tileset == ram(var::CurrentTileSet)) {
		set_banks(1);
		return;
	}
	ram(var::CurrentTileSet) = tileset;

	LoadPowSwitTiles();
	set_banks(7);
	if (tileset != 6) {
		LoadGfxCrates();		/* not for the Metal Gear tileset */
		set_banks(7);
	}

	uint16_t ix = GetPointerDE2A(SYM(idxTileSets), tileset);
	for (int block = 0; block < 3; block++) {
		if ((rom(ix) & 0x80) != 0)
			break;
		LoadTileset(ix);
		ix = (uint16_t)(ix + 5);
	}
	LoadColliTiles();
}


/* SetRoomPal: palette of the room; dark rooms are black without the
   flashlight, the goggles give the infrared palette. */
void
SetRoomPal()
{
	set_banks(4);
	uint8_t room = ram(var::Room);
	uint8_t item = ram(var::SelectedItem);
	bool checkFlashlight = room == 220 || room == 221
		|| (room != 251 && room < 126 && room >= 123);
	if (room != 251 && checkFlashlight && item != SELECTED_FLASHLIGHT) {
		SetRoomPal2(0x0b);		/* black */
		return;
	}
	if (room != 251 && item == SELECTED_GOGGLES) {
		SetRoomPal2(0x0a);		/* infrared */
		return;
	}
	SetRoomPal2(GetNibbleHL_A2(SYM(IdsRoomPal), room));
}


void
SetRoomPal2(uint8_t palette)
{
	set_banks(4);
	SetPalette(GetPointerDE2A(SYM(idxRoomPalettes), palette));
	set_banks(1);
}


/* UnpackMetatiles: a row of 8 metatiles (4x4 tiles each) into the room
   tile buffer (32 tiles wide). */
void
UnpackMetatiles(uint16_t buffer, uint16_t map)
{
	ram(var::MetatilesNX) = 8;
	set_ram16(var::TempData, buffer);
	set_ram16(var::TempData3, map);
	uint8_t set = (uint8_t)(GetNibbleRoom(SYM(MetaTileSetIDs)) - 1);
	uint16_t metatiles = GetPointerDE2A(SYM(idxMetatileSet), set);
	set_ram16(var::TempData2, metatiles);
	do {
		uint16_t m = ram16(var::TempData3);
		uint8_t id = rom(m);
		set_ram16(var::TempData3, (uint16_t)(m + 1));
		uint16_t hl = (uint16_t)((uint8_t)(id - 1) * 4);
		hl = (uint16_t)(hl * 4 + ram16(var::TempData2));
		uint16_t de = ram16(var::TempData);
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++)
				ram(de++) = rom(hl++);
			de = (uint16_t)(de + 0x1c);
		}
		set_ram16(var::TempData, (uint16_t)(de - 0x7c));
	} while (--ram(var::MetatilesNX) != 0);
}


/* RenderRoom: 6 rows of metatiles into the buffer, then 32x24 tiles. */
void
RenderRoom()
{
	set_banks(13);
	ClearPage0();
	set_ram16(var::TempData, var::RoomTileBuffer);
	uint16_t map = GetPointerDE2A(SYM(idxRooms), ram(var::Room));
	for (int row = 0; row < 6; row++) {
		uint16_t de = ram16(var::TempData);
		UnpackMetatiles(de, map);
		set_ram16(var::TempData, (uint16_t)(de + 0x80));
		map = (uint16_t)(map + 8);
	}
	set_banks(1);
	uint16_t hl = var::RoomTileBuffer;
	uint8_t x = 0, y = 0;
	for (int i = 0; i < 0x300; i++) {
		DrawTile(ram(hl++), x, y);
		CalcNextCharPos(&x, &y);
	}
}
