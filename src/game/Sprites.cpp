/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from Banks0123.asm (SetSnakeSprAtt .. UpdateEnemySprRAM,
// SetSpritesFree, SetActorSprCols, SetSprColor ...).
#include "Sprites.h"

#include "Common.h"
#include "GameGfx.h"
#include "GfxBasic.h"
#include "Options.h"
#include "Rom.h"
#include "Screen.h"
#include "Structures.h"


using namespace var;


static inline uint16_t
IncLow(uint16_t address, int n = 1)
{
	return static_cast<uint16_t>((address & 0xff00) | ((address + n) & 0xff));
}


static inline int
Djnz(uint8_t count)
{
	return count == 0 ? 256 : count;
}


static uint16_t
SnakeSprAttr()
{
	uint8_t set = Mem(static_cast<uint16_t>(SYM(SnakeSprAttIds) + Ram(SnakeSprId)));
	return GetPointerDE2A(SYM(idxSnakeSprAttr), set);
}


// Positions and patterns; layer 16 on in the elevators.
void
SetSnakeSprAtt()
{
	SetBanks_A_B_C();
	uint16_t source = SnakeSprAttr();
	uint8_t room = Ram(Room);
	uint16_t dest = SprAttRAM;
	if (room >= 240 && room != 251)
		dest = SnakeAttrLow;
	int count = Djnz(Mem(source++));
	for (int i = 0; i < count; i++) {
		uint8_t dy = Mem(source);
		Ram(dest) = dy == 0x80 ? 0xe0 : static_cast<uint8_t>(Ram(PlayerY) + dy);
		source++;
		dest = IncLow(dest);
		Ram(dest) = static_cast<uint8_t>(Ram(PlayerX) + Mem(source));
		source++;
		dest = IncLow(dest);
		Ram(dest++) = Mem(source++);	// pattern (LDI)
		source++;
		dest = IncLow(dest);
	}
	SetBanks1_2_3();
}


// Colours (16 lines per sprite); red every other frame after damage.
void
SetSnakeSprCol()
{
	SetBanks_A_B_C();
	uint16_t source = SnakeSprAttr();
	uint16_t dest = Ram(Room) >= 240 ? 0xe900 : 0xe800;
	if (Ram(DamageDelayTimer) != 0 && (Ram(TickCounter) & 1) == 0)
		source = SYM(SnakeAttrDamage);
	int count = Djnz(Mem(source));
	source = static_cast<uint16_t>(source + 4);
	for (int i = 0; i < count; i++) {
		uint8_t color = Mem(source);
		for (int line = 0; line < 16; line++)
			Ram(dest++) = color;
		source = static_cast<uint16_t>(source + 4);
	}
	SetBanks1_2_3();
}


// Packed patterns: n < #80 = n times the next byte, #80 + n = n bytes,
// 0 or #80 = end.
void
SetSnakeSprPatt()
{
	uint32_t vram = 0xf800;
	SetBanks_A_B_C();
	uint16_t source = GetPointerDE2A(SYM(idxSnakeSpr), Ram(SnakeSprId));
	for (;;) {
		uint8_t a = Mem(source);
		uint8_t n = a & 0x7f;
		if (n == 0)
			break;
		source++;
		if (n == a) {
			uint8_t value = Mem(source++);
			for (int i = 0; i < n; i++)
				gScreen.Write(vram++, value);
		} else {
			for (int i = 0; i < n; i++)
				gScreen.Write(vram++, Mem(source++));
		}
	}
	SetBanks1_2_3();
}


void
UpdateSnakeSpr()
{
	SetSnakeSprAtt();
	SetSnakeSprCol();
	SetSnakeSprPatt();
}


// Shot sprites from their attribute list (#80 = hidden).
void
SetShotSpr(uint16_t shot)
{
	SetBanks_A_B_C();
	uint16_t source = GetPointerDE2A(SYM(idxShotSprAtt),
		Ram(static_cast<uint16_t>(shot + PLAYER_SHOT::SpriteID)));
	uint16_t hl = static_cast<uint16_t>(shot + ACTOR::Spr1Y);
	int count = Djnz(Ram(static_cast<uint16_t>(shot + PLAYER_SHOT::NumSprites)));
	for (int i = 0; i < count; i++) {
		uint8_t dy = Mem(source);
		if (dy == 0x80) {
			Ram(hl) = 0xe1;
			hl = static_cast<uint16_t>(hl + 3);
		} else {
			Ram(hl++) = static_cast<uint8_t>(dy + Ram(static_cast<uint16_t>(shot + PLAYER_SHOT::Y)));
			source++;
			Ram(hl++) = static_cast<uint8_t>(Mem(source)
				+ Ram(static_cast<uint16_t>(shot + PLAYER_SHOT::X)));
			source++;
			Ram(hl++) = Mem(source++);
			Ram(hl) = Mem(source);
		}
		hl = static_cast<uint16_t>(hl + 2);
		source++;
	}
	SetBanks1_2_3();
}


// Sprite positions and patterns of an actor from its sprite list: Y
// offset, X offset, pattern; Y offsets #91-#A5 select a common offset list
// (idxSprOffsets) for the rest of the sprites, followed by patterns only.
void
UpdateActorSprDat(uint16_t ix)
{
	if (Ram(static_cast<uint16_t>(ix + ACTOR::NumSprites)) == 0)
		return;
	SetBanks_4_5_6();
	if (Ram(static_cast<uint16_t>(ix + ACTOR::ID)) >= 0xf0) {
		SetBanks1_2_3();
		return;
	}
	uint8_t y = Ram(static_cast<uint16_t>(ix + ACTOR::Y));
	uint8_t x = Ram(static_cast<uint16_t>(ix + ACTOR::X));
	uint16_t de = GetPointerDE2A(SYM(idxSprites), Ram(static_cast<uint16_t>(ix + ACTOR::SpriteId)));
	uint16_t hl = static_cast<uint16_t>(ix | 0x20);
	int count = Djnz(Ram(hl));
	hl = IncLow(hl, 2);
	for (int i = 0; i < count; i++) {
		uint8_t dy = Mem(de++);
		uint8_t common = static_cast<uint8_t>(dy - 0x91);
		if (dy >= 0x91 && common < 0x15) {
			uint16_t offsets = GetPointerDE2A(SYM(idxSprOffsets), common);
			for (; i < count; i++) {
				Ram(hl) = static_cast<uint8_t>(Mem(offsets++) + Ram(static_cast<uint16_t>(ix
					+ ACTOR::Y)));
				hl = IncLow(hl);
				Ram(hl) = static_cast<uint8_t>(Mem(offsets++) + Ram(static_cast<uint16_t>(ix
					+ ACTOR::X)));
				hl = IncLow(hl);
				Ram(hl) = Mem(de++);
				hl = IncLow(hl, 3);
			}
			break;
		}
		Ram(hl) = static_cast<uint8_t>(dy + y);
		hl = IncLow(hl);
		Ram(hl) = static_cast<uint8_t>(Mem(de++) + x);
		hl = IncLow(hl);
		Ram(hl) = Mem(de++);
		hl = IncLow(hl, 3);
	}
	SetBanks1_2_3();
}


void
UpdateEnemySprRAM()
{
	uint16_t hl = EnemyList;
	for (int i = 0; i < 16; i++) {
		if (Ram(hl) != 0)
			UpdateEnemySprRAM3(hl);
		hl = static_cast<uint16_t>(hl + 0x80);
	}
}


// Copies an actor's sprites to the sprite attribute and colour tables.
void
UpdateEnemySprRAM3(uint16_t hl)
{
	hl = static_cast<uint16_t>(hl | 0x20);
	uint8_t count = Ram(hl);
	if (count == 0)
		return;
	hl = IncLow(hl);
	for (int i = 0; i < count; i++) {
		uint8_t layer = Ram(hl);
		hl = IncLow(hl);
		uint16_t de = IncLow(EnemySprAttRAM, layer * 4);
		for (int k = 0; k < 3; k++)
			Ram(de++) = Ram(hl++);
		if (Ram(hl) == 0) {
			uint16_t y = static_cast<uint16_t>((de & 0xff00) | ((de - 3) & 0xff));
			Ram(y) = 0xe1;	// colour 0: hidden
		}
		uint16_t colors = static_cast<uint16_t>((0xe8a0 / 2 + static_cast<uint8_t>(layer * 8)) * 2);
		uint8_t color = Ram(hl);
		for (int line = 0; line < 16; line++) {
			Ram(colors) = color;
			colors = IncLow(colors);
		}
		hl = IncLow(hl);
	}
}


void
HideEnemySprs()
{
	for (int i = 0; i < 0x58; i++)
		Ram(static_cast<uint16_t>(EnemySprAttRAM + i)) = 0xe0;
}


void
SetSpritesFree(uint16_t hl, uint8_t count)
{
	int n = Djnz(count);
	for (int i = 0; i < n; i++) {
		uint16_t de = static_cast<uint16_t>(EnemySprAttRAM + static_cast<uint8_t>(Ram(hl) * 4));
		Ram(de) = 0xe0;
		hl = static_cast<uint16_t>(hl + 5);
	}
}


// Colours from a table indexed by the actor ID (the actor is at TempData3).
void
SetActorSprCols(uint16_t ix, uint16_t table)
{
	if (Ram(static_cast<uint16_t>(ix + ACTOR::NumSprites)) == 0)
		return;
	uint16_t de = GetPointerDE2A(table, Ram(static_cast<uint16_t>(ix + ACTOR::ID)));
	uint16_t hl = static_cast<uint16_t>(Ram16(TempData3) | 0x20);
	int count = Djnz(Ram(hl));
	hl = IncLow(hl, 5);
	for (int i = 0; i < count; i++) {
		Ram(hl) = Mem(de++);
		hl = IncLow(hl, 5);
	}
}


void
SetSpriteLayer(uint8_t layer, uint8_t index)
{
	uint16_t hl = static_cast<uint16_t>(Ram16(TempData3)
		+ static_cast<uint8_t>(index * 5 + ACTOR::Spr1Layer));
	Ram(hl) = layer;
}


void
HideSprColor(uint16_t ix)
{
	SetSprColor(ix, 0, 0);
}


void
SetSprColor(uint16_t ix, uint8_t a, uint8_t c)
{
	uint16_t hl = static_cast<uint16_t>(ix | 0x20);
	uint8_t n = Ram(hl);
	if (n == 0)
		return;
	uint8_t pairs = static_cast<uint8_t>((n >> 1) | (n << 7));	// RRC
	hl = IncLow(hl, 5);
	int count = Djnz(pairs);
	for (int i = 0; i < count; i++) {
		Ram(hl) = a;
		hl = static_cast<uint16_t>(hl + 5);
		Ram(hl) = c;
		hl = static_cast<uint16_t>(hl + 5);
	}
}


void
EraseSprAttRAM()
{
	for (int i = 0; i < 0x80; i++)
		Ram(static_cast<uint16_t>(SprAttRAM + i)) = 0xe0;
}


void
LoadSprMissilRocket(uint16_t shot, uint16_t table)
{
	SetBanks_A_B_C();
	uint8_t direction = Ram(static_cast<uint16_t>(shot + PLAYER_SHOT::Direction));
	UnpackGfx1(0xf880, GetPointerDE2A(table, static_cast<uint8_t>(direction - 1)));
	SetBanks1_2_3();
}


void
LoadSprTarget()
{
	SetBanks_4_5_6();
	UnpackGfx(SYM(SprTarget));
	SetBanks1_2_3();
}


uint16_t
VRAMtoRAM(uint16_t vram, uint16_t ram, uint16_t count)
{
	uint32_t address = vram;
	int n = count == 0 ? 0x10000 : count;
	for (int i = 0; i < n; i++) {
		Ram(ram++) = gScreen.Read(address);
		address = (address + 1) & 0x1ffff;
	}
	return ram;
}


void
WriteVRAM(uint16_t vram, uint8_t value)
{
	gScreen.Write(vram, value);
}


// logic/updatesprites.asm: two sprite tables in VRAM, switched every tick
// (attributes #F600/#F200, colours #F400/#F000).
void
SwapSprBuffer()
{
	bool second = (Ram16(SprColAddress) & 0x0400) != 0;
	SetRam16(SprAttAddress, second ? 0xf200 : 0xf600);
	SetRam16(SprColAddress, second ? 0xf000 : 0xf400);
	WRTVDP(5, second ? 0xef : 0xe7);
}


// Sprites in a rotating order, so that with more than 8 on a line each
// one disappears only now and then.
void
UpdateSpritesShuf()
{
	uint8_t offset = static_cast<uint8_t>((Ram(SprShuffleOffset) + 0x68) & 0x78);
	Ram(SprShuffleOffset) = offset;
	if (gOptions.noFlicker)
		offset = 0;	// always the same order (the variable still rotates)
	uint32_t vram = Ram16(SprColAddress);
	uint8_t a = static_cast<uint8_t>(offset * 2);
	for (int i = 0; i < 16; i++) {
		uint16_t hl = static_cast<uint16_t>((0x7400 | a) * 2);
		for (int k = 0; k < 32; k++)
			gScreen.Write(vram++, Ram(static_cast<uint16_t>(hl + k)));
		a = static_cast<uint8_t>(a + 0x90);
	}
	a = offset;
	for (int i = 0; i < 16; i++) {
		uint16_t hl = static_cast<uint16_t>(0xea00 | a);
		for (int k = 0; k < 8; k++)
			gScreen.Write(vram++, Ram(static_cast<uint16_t>(hl + k)));
		a = static_cast<uint8_t>((a + 0x48) & 0x78);
	}
}


void
UpdateSprites()
{
	RAMtoVRAM(SpritesColors, Ram16(SprColAddress), 0x280);
}


// Binoculars: the target sprites only.
void
SetBinoTargetSpr()
{
	RAMtoVRAM(BinocularSprCol, static_cast<uint16_t>(Ram16(SprColAddress) + 0x40), 0x1c0);
	RAMtoVRAM(SprShootsAtt, static_cast<uint16_t>(Ram16(SprAttAddress) + 0x10), 0x70);
}


// logic/hudspritemask.asm: in the elevators 16 sprites over the HUD hide
// the elevator's sprites there.
void
SetHUDSprMask()
{
	uint16_t de = SprAttRAM;
	for (int i = 0; i < 8; i++) {
		for (int k = 0; k < 8; k++)
			Ram(de++) = Mem(static_cast<uint16_t>(SYM(MaskSprAttrib) + k));
	}
	for (int i = 0; i < 256; i++)
		Ram(static_cast<uint16_t>(SpritesColors + i)) = 0x0f;
}
