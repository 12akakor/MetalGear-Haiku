/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from Banks0123.asm (LoadGfxDoors .. LoadSprProjectile,
// UnpackGfx, the pitfall tiles, UpdateRadio).
#include "GameGfx.h"

#include "Common.h"
#include "GfxBasic.h"
#include "RoomGfx.h"
#include "Rom.h"
#include "Screen.h"
#include "Structures.h"


using namespace var;


void
UnpackGfx1(uint16_t vram, uint16_t data)
{
	uint32_t address = vram;
	for (;;) {
		uint8_t a = Mem(data);
		if (a == 0)
			return;
		data++;
		if ((a & 0x80) == 0) {
			uint8_t value = Mem(data++);
			for (int i = 0; i < a; i++) {
				gScreen.Write(address, value);
				address = (address + 1) & 0x1ffff;
			}
			continue;
		}
		a &= 0x7f;
		if (a == 0) {
			address = Mem16(data);
			data = static_cast<uint16_t>(data + 2);
			continue;
		}
		for (int i = 0; i < a; i++) {
			gScreen.Write(address, Mem(data++));
			address = (address + 1) & 0x1ffff;
		}
	}
}


void
UnpackGfx(uint16_t data)
{
	UnpackGfx1(Mem16(data), static_cast<uint16_t>(data + 2));
}


uint16_t
LoadTilesGfxBlk(uint16_t source, uint16_t vram, uint8_t columns, uint8_t rows)
{
	do {
		uint16_t dest = vram;
		source = LoadTilesGfx(source, &dest, columns);
		vram = static_cast<uint16_t>(vram + 0x400);
	} while (--rows != 0);
	return source;
}


void
LoadGfxDoors()
{
	SetBanks_7_8_9();
	LoadTilesGfxBlk(SYM(GfxDoorFront), 0xd060, 4, 4);
	LoadTilesGfxBlk(SYM(GfxDoorElevator), 0xe060, 4, 4);
	LoadTilesGfxBlk(SYM(GfxDoorLeft), 0xd070, 1, 4);
	LoadTilesGfxBlk(SYM(GfxDoorRight), 0xd074, 1, 4);
	LoadTilesGfxBlk(SYM(GfxDoorDown), 0xe070, 4, 1);
	SetBanks1_2_3();
}


// Sprite set of the room: entries pattern, data pointer (3 bytes).
void
LoadRoomSpr()
{
	SetBanks_A_B_C();
	uint8_t set = Mem(static_cast<uint16_t>(SYM(SpritesetRooms) + Ram(Room)));
	uint16_t de = GetPointerDE2A(SYM(idxSprSet), set);
	for (;;) {
		uint8_t pattern = Mem(de);
		if (pattern == 0xff)
			break;
		uint16_t vram = static_cast<uint16_t>(pattern * 8 + 0xf800);
		UnpackGfx1(vram, Mem16(static_cast<uint16_t>(de + 1)));
		de = static_cast<uint16_t>(de + 3);
	}
	SetBanks1_2_3();
}


// The shot of the weapon and its explosion.
void
LoadSprProjectile()
{
	SetBanks_A_B_C();
	uint8_t weapon = Ram(SelectedWeapon);
	if (weapon == 0) {
		SetBanks1_2_3();
		return;
	}
	if (weapon != ROCKET_LAUNCHER && weapon != MISSILE)
		UnpackGfx1(0xf880, GetPointerDE2A(SYM(idxSprWeapon), static_cast<uint8_t>(weapon - 1)));
	if (weapon >= GRENADE_LAUNCHER) {
		if (weapon == GRENADE_LAUNCHER || weapon == LAND_MINE)
			UnpackGfx1(0xf900, SYM(SprExplosionS));
		else
			UnpackGfx1(0xf8c0, SYM(SprExplosionB));
	}
	SetBanks1_2_3();
}


// Items, alert sign, radio, cameras, Snake's portrait.
void
LoadGameGfx()
{
	LoadGfxCrates();
	SetBanks_4_5_6();
	SetColorsIndexes(SYM(ColorsItems));
	uint16_t dest = 0xb000;
	Load3bppTiles(SYM(GfxItems), &dest, 0x84);
	dest = 0xc400;
	Load3bppTiles(SYM(gfxAlertIcon), &dest, 4);
	SetColorsIndexes(SYM(ColorsTileset));
	dest = 0xc804;
	Load3bppTiles(SYM(gfxRadio), &dest, 0x1f);
	dest = 0xcc00;
	Load3bppTileFlip(SYM(gfxRadio2), &dest, 7);
	SetColorsIndexes2(SYM(ColorsCameras), 4);
	dest = 0xc010;
	Load2bppTile(SYM(gfxCameraV), &dest, 8);
	dest = 0xc410;
	Load2bppTile(SYM(gfxCameraH), &dest, 8);
	SetBanks_7_8_9();
	SetColorsIndexes(SYM(ColSnakePic));
	dest = 0xc040;
	Load3bppTiles(SYM(gfxSnakePortrait), &dest, 0x10);
	dest = 0xc440;
	Load3bppTiles(SYM(gfxSnakePortrait2), &dest, 3);
	SetBanks1_2_3();
}


void
PrintTransceiv()
{
	SetBanks_4_5_6();
	PrintTextXY(SYM(txtTransceiv));
	SetBanks1_2_3();
}


// Pitfall tiles are kept in page 1 from (#38, #98).
void
GetPitfallXY(uint8_t tile, uint8_t* x, uint8_t* y)
{
	*x = static_cast<uint8_t>((tile & 0x1f) * 8 + 0x38);
	*y = static_cast<uint8_t>(((tile & 0xe0) >> 2) + 0x98);
}


void
DrawPitfallTile(uint8_t tile, uint8_t x, uint8_t y)
{
	uint8_t sx, sy;
	GetPitfallXY(tile, &sx, &sy);
	VDP_Copy_Byte(sx, sy, 5, x, y, 8, 8);
}


// Tiles, #FE = next row, #FF = end.
void
DrawPitfallTiles(uint16_t data, uint8_t x, uint8_t y)
{
	uint8_t dx = x;
	uint8_t dy = y;
	for (;;) {
		uint8_t tile = Mem(data++);
		if (tile == 0xff)
			return;
		if (tile == 0xfe) {
			y = static_cast<uint8_t>(y + 8);
			dx = x;
			dy = y;
			continue;
		}
		DrawPitfallTile(tile, dx, dy);
		CalcNextCharPos(&dx, &dy);
	}
}


void
DrawPitfall()
{
	DrawPitfallTiles(SYM(PitfallTileMap), 0x40, 0xa0);
	SetBanks1_2_3();
}


void
DrawPitfall_()
{
	SetBanks_7_8_9();
	DrawPitfall();
}


void
SetupPitfall(uint16_t source, uint16_t vram, uint8_t count)
{
	SetBanks_7_8_9();
	Load2bppTile(source, &vram, count);
	DrawPitfall();
}


void
SetMenuWeaponPal()
{
	SetBanks_4_5_6();
	SetPalette(SYM(PalMenuWeapon));
	SetBanks1_2_3();
}


void
SetSnakePal()
{
	SetPalette(SYM(SnakePal));
}


void
SetRadioPal()
{
	SetBanks_4_5_6();
	SetPalette(SYM(RadioPalette));
	SetBanks1_2_3();
}


void
SetSprPal()
{
	SetBanks_A_B_C();
	uint8_t set = Mem(static_cast<uint16_t>(SYM(SpritesetRooms) + Ram(Room)));
	SetBanks_4_5_6();
	SetPalette(GetPointerDE2A(SYM(idxSprSetPals), set));
	SetBanks1_2_3();
}


// Who can be called in this room: 2 bytes per person (bits 7-4 person,
// bits 3-2 reply mode, bit 0 last; text id), into RadioPersonsDat (#20
// bytes each: frequency, -, reply mode, text id).
void
UpdateRadio()
{
	SetBanks_D_E_F();
	uint16_t hl = GetPointerDE2A(SYM(idxRoomRadio), Ram(Room));
	uint8_t count = 0;
	if (Mem(hl) != 0) {
		uint16_t de = RadioPersonsDat;
		for (;;) {
			uint8_t person = static_cast<uint8_t>((Mem(hl) >> 4) & 0x0f);
			uint8_t freq = Mem(static_cast<uint16_t>(SYM(RadioFreqs)
				+ static_cast<uint8_t>(person - 1)));
			Ram(de) = freq;
			uint8_t mode = static_cast<uint8_t>((Mem(hl) >> 2) & 3);
			Ram(static_cast<uint16_t>(de + 2)) = mode;
			if ((mode & 2) != 0)
				Ram(RadioFreq) = freq;	// tuned in already
			uint8_t flags = Mem(hl);
			hl++;
			Ram(static_cast<uint16_t>(de + 3)) = Mem(hl);
			count++;
			if ((flags & 1) != 0)
				break;
			hl++;
			de = static_cast<uint16_t>(de + 0x20);
		}
	}
	Ram(NumRadioPersons) = count;
	SetBanks1_2_3();
}
