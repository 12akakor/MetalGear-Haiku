/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Loading and drawing rooms, rewritten from Banks0123.asm.
#include "RoomGfx.h"

#include "GfxBasic.h"
#include "Rom.h"


namespace {

enum {
	SELECTED_FLASHLIGHT = 3,
	SELECTED_GOGGLES = 4
};

// LoadTileset: IX = tileset block (5 bytes).
void
LoadTileset(uint16_t ix)
{
	if ((Rom(ix) & 0x40) != 0) {
		// LoadTilesFlip: the graphics of the previous block, mirrored
		uint8_t count = Ram(var::TempData);
		uint16_t dest = TileToVramAdd(Rom(static_cast<uint16_t>(ix + 1)));
		uint16_t source = Ram16(static_cast<uint16_t>(var::TempData + 1));
		Load3bppTileFlip(source, &dest, count);
		return;
	}
	uint8_t count = Rom(static_cast<uint16_t>(ix + 1));
	Ram(var::TempData) = count;
	uint16_t dest = TileToVramAdd(Rom(static_cast<uint16_t>(ix + 2)));
	uint16_t source = Rom16(static_cast<uint16_t>(ix + 3));
	SetRam16(static_cast<uint16_t>(var::TempData + 1), source);
	Load3bppTiles(source, &dest, count);
}


void
LoadColliTiles()
{
	SetBanks(1);
	uint8_t tileset = Ram(var::CurrentTileSet);
	SetBanks(7);
	uint16_t data = Rom16(static_cast<uint16_t>(SYM(IdxColisTiles) + tileset * 2));
	uint16_t out = var::CollisionTiles;
	for (int i = 0; i < 32; i++) {
		uint8_t a = Rom(data++);
		for (int bit = 0; bit < 8; bit++) {
			a = static_cast<uint8_t>((a << 1) | (a >> 7));
			Ram(out) = static_cast<uint8_t>(a & 1);
			out = static_cast<uint16_t>((out & 0xff00) | static_cast<uint8_t>(out + 1));
		}
	}
	SetBanks(1);
}

}	// namespace


void
LoadPowSwitTiles()
{
	SetBanks(13);
	SetTilesetColors();
	uint16_t dest = 0x9048;
	Load3bppTiles(SYM(gfxPowSwitch), &dest, 4);
	SetBanks(1);
}


void
LoadGfxCrates()
{
	SetBanks(7);
	SetTilesetColors();
	uint16_t dest = 0x9400;
	Load3bppTiles(SYM(GfxCrates), &dest, 8);
	dest = 0x9840;
	Load3bppTileFlip(SYM(GfxCrates), &dest, 8);
	SetBanks(1);
}


void
LoadRoomTiles()
{
	SetTilesetColors();
	SetBanks(7);
	uint8_t tileset = GetNibbleRoom(SYM(RoomGfxSetIds));
	if (tileset == Ram(var::CurrentTileSet)) {
		SetBanks(1);
		return;
	}
	Ram(var::CurrentTileSet) = tileset;

	LoadPowSwitTiles();
	SetBanks(7);
	if (tileset != 6) {
		LoadGfxCrates();	// not for the Metal Gear tileset
		SetBanks(7);
	}

	uint16_t ix = GetPointerDE2A(SYM(idxTileSets), tileset);
	for (int block = 0; block < 3; block++) {
		if ((Rom(ix) & 0x80) != 0)
			break;
		LoadTileset(ix);
		ix = static_cast<uint16_t>(ix + 5);
	}
	LoadColliTiles();
}


// SetRoomPal: palette of the room; dark rooms are black without the
// flashlight, the goggles give the infrared palette.
void
SetRoomPal()
{
	SetBanks(4);
	uint8_t room = Ram(var::Room);
	uint8_t item = Ram(var::SelectedItem);
	bool checkFlashlight = room == 220 || room == 221
		|| (room != 251 && room < 126 && room >= 123);
	if (room != 251 && checkFlashlight && item != SELECTED_FLASHLIGHT) {
		SetRoomPal2(0x0b);	// black
		return;
	}
	if (room != 251 && item == SELECTED_GOGGLES) {
		SetRoomPal2(0x0a);	// infrared
		return;
	}
	SetRoomPal2(GetNibbleHL_A2(SYM(IdsRoomPal), room));
}


void
SetRoomPal2(uint8_t palette)
{
	SetBanks(4);
	SetPalette(GetPointerDE2A(SYM(idxRoomPalettes), palette));
	SetBanks(1);
}


// UnpackMetatiles: a row of 8 metatiles (4x4 tiles each) into the room
// tile buffer (32 tiles wide).
void
UnpackMetatiles(uint16_t buffer, uint16_t map)
{
	Ram(var::MetatilesNX) = 8;
	SetRam16(var::TempData, buffer);
	SetRam16(var::TempData3, map);
	uint8_t set = static_cast<uint8_t>(GetNibbleRoom(SYM(MetaTileSetIDs)) - 1);
	uint16_t metatiles = GetPointerDE2A(SYM(idxMetatileSet), set);
	SetRam16(var::TempData2, metatiles);
	do {
		uint16_t m = Ram16(var::TempData3);
		uint8_t id = Rom(m);
		SetRam16(var::TempData3, static_cast<uint16_t>(m + 1));
		uint16_t hl = static_cast<uint16_t>(static_cast<uint8_t>(id - 1) * 4);
		hl = static_cast<uint16_t>(hl * 4 + Ram16(var::TempData2));
		uint16_t de = Ram16(var::TempData);
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++)
				Ram(de++) = Rom(hl++);
			de = static_cast<uint16_t>(de + 0x1c);
		}
		SetRam16(var::TempData, static_cast<uint16_t>(de - 0x7c));
	} while (--Ram(var::MetatilesNX) != 0);
}


// RenderRoom: 6 rows of metatiles into the buffer, then 32x24 tiles.
void
RenderRoom()
{
	SetBanks(13);
	ClearPage0();
	SetRam16(var::TempData, var::RoomTileBuffer);
	uint16_t map = GetPointerDE2A(SYM(idxRooms), Ram(var::Room));
	for (int row = 0; row < 6; row++) {
		uint16_t de = Ram16(var::TempData);
		UnpackMetatiles(de, map);
		SetRam16(var::TempData, static_cast<uint16_t>(de + 0x80));
		map = static_cast<uint16_t>(map + 8);
	}
	SetBanks(1);
	uint16_t hl = var::RoomTileBuffer;
	uint8_t x = 0, y = 0;
	for (int i = 0; i < 0x300; i++) {
		DrawTile(Ram(hl++), x, y);
		CalcNextCharPos(&x, &y);
	}
}
