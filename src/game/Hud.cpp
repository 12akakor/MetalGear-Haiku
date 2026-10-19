/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/hud.asm and the HUD and menu parts of Banks0123.asm.
#include "Hud.h"

#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Rom.h"
#include "Screen.h"
#include "Structures.h"


using namespace var;


void
DrawLineHoriz(uint8_t x, uint8_t y, uint8_t length, uint8_t color)
{
	gScreen.Line(x, y, static_cast<uint8_t>(length - 1) + 1, false, color);
}


void
DrawLineVert(uint8_t x, uint8_t y, uint8_t length, uint8_t color)
{
	gScreen.Line(x, y, static_cast<uint8_t>(length - 1) + 1, true, color);
}


void
DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
	DrawLineVert(x, y, height, color);
	DrawLineHoriz(x, y, width, color);
	DrawLineHoriz(x, static_cast<uint8_t>(y - 1 + height), width, color);
	DrawLineVert(static_cast<uint8_t>(x - 1 + width), y, height, color);
}


// RenderNumber: a zero digit is not drawn until a non-zero one came (mask c).
static void
RenderNumber(uint8_t digit, uint8_t* c, uint8_t* x, uint8_t y)
{
	digit &= 0x0f;
	if (digit != 0)
		*c = 0xff;
	DrawChar(static_cast<uint8_t>((digit + '0') & *c), *c, *x, y);
	*x = static_cast<uint8_t>(*x + 8);
}


// Hundreds (hidden when 0), then tens and units (always shown).
void
Render3Numbers(uint16_t number, uint8_t x, uint8_t y)
{
	uint8_t c = 0;
	RenderNumber(Ram(number), &c, &x, y);
	number--;
	c = 0xff;
	RenderNumber(static_cast<uint8_t>(Ram(number) >> 4), &c, &x, y);
	RenderNumber(Ram(number), &c, &x, y);
}


// Red digits of the destruction countdown (two tiles high).
void
PrintDigit(uint8_t digit, uint8_t c, uint8_t* x, uint8_t y)
{
	uint16_t hl = static_cast<uint16_t>(SYM(RedDigitTiles) + (digit & 0x0f) * 2);
	DrawChar(Mem(hl), c, *x, y);
	DrawChar(Mem(static_cast<uint16_t>(hl + 1)), c, *x, static_cast<uint8_t>(y + 8));
	*x = static_cast<uint8_t>(*x + 8);
}


void
RenderHUD()
{
	DrawLife();
	DrawClass();
	DrawCallTimer();
	DrawWeaponHUD();
	DrawItemHUD();
}


// The CALL sign blinks during an incoming call (not in the menus).
void
DrawCallTimer()
{
	uint8_t c = 0;
	uint8_t mode = Ram(GameMode);
	if (Ram(RadioCallFlag) == 1 && mode != 2 && mode != 3) {
		uint8_t tick = Ram(TickCounter);
		if ((tick & 0x08) == 0) {
			if ((tick & 7) == 0)
				SetSoundEntryChk(0x22);
			c = 0xff;
		}
	}
	if (Ram(DestructionTimerOn) != 0) {
		DrawDestrucTimer();
		return;
	}
	uint16_t text = SYM(txtCALL);
	PrintText(static_cast<uint16_t>(text + 2), Mem(text), Mem(static_cast<uint16_t>(text + 1)),
		c);
}


void
DrawDestrucTimer()
{
	if (Ram(EndingStatus) >= 4)
		return;
	uint8_t x = 0x70;
	const uint8_t y = 0xc1;
	PrintDigit(static_cast<uint8_t>(Ram(DestructTimerH) >> 4), 1, &x, y);
	PrintDigit(Ram(DestructTimerH), 1, &x, y);
	PrintDigit(static_cast<uint8_t>(Ram(DestructTimer) >> 4), 1, &x, y);
	PrintDigit(Ram(DestructTimer), 1, &x, y);
}


void
DecrementLife_4()
{
	DecrementLife_B(4);
}


void
DecrementLife_B(uint8_t amount)
{
	if (Ram(Life) == 0) {
		SetDead();
		return;
	}
	int count = amount == 0 ? 256 : amount;
	for (int i = 0; i < count; i++) {
		if (--Ram(Life) == 0) {
			SetDead();
			return;
		}
	}
	DrawLife();
}


void
SetDead()
{
	DrawLife();
	Ram(PlayerControlMod) = CONTROL_DEAD;
	Ram(PlayerAnimation) = 6;
	Ram(GameMode) = GAME_MODE_DEAD;
	Ram(DeadTimer) = 0x80;
	Ram(DamageDelayTimer) = 0;
	for (int i = 0; i < 0x18; i++)
		Ram(static_cast<uint16_t>(SprShootsAtt + i)) = 0xe0;	// remove all bullets
	SetSoundEntryChk(0x44);	// Just another dead soldier
}


void
DrawLife()
{
	PrintTextXY(SYM(txtLife));
	DrawRect(0x31, 0xc1, 0x32, 0x08, 0x0e);
	uint8_t life = Ram(Life);
	if (life >= 2)
		FillRect(0x88, 0x32, 0xc2, 0, life, 6);
	if (life == 0x30 || life == 0x2f)
		return;
	FillRect(0, static_cast<uint8_t>(life + 0x32), 0xc2, 0, static_cast<uint8_t>(0x30 - life),
		6);
}


static void
DrawStars(uint8_t count, uint8_t c, uint8_t* x, uint8_t y)
{
	int n = count == 0 ? 256 : count;
	for (int i = 0; i < n; i++) {
		DrawChar(c, c, *x, y);
		*x = static_cast<uint8_t>(*x + 8);
	}
}


void
DrawClass()
{
	PrintTextXY(SYM(txtClass));
	uint8_t x = 0x34;
	const uint8_t y = 0xc9;
	DrawStars(static_cast<uint8_t>(Ram(Class) + 1), 0x3b, &x, y);
	if (Ram(Class) == 3)
		return;
	DrawStars(static_cast<uint8_t>(3 - Ram(Class)), 0, &x, y);
}


// Weapon pictures: X odd = 16x16 (drawn 8 pixels further right), else 32x16.
static void
DrawWeaponIcon(uint16_t gfx, uint8_t x, uint8_t y)
{
	uint8_t sx = Mem(gfx);
	uint8_t sy = Mem(static_cast<uint16_t>(gfx + 1));
	uint8_t nx = 0x20;
	uint8_t ny = 0x10;
	if ((sx & 1) != 0) {
		nx = 0x10;
		x = static_cast<uint8_t>(x + 8);
	}
	VDP_Copy_Byte(sx, sy, 1, x, y, nx, ny);
}


void
DrawWeaponHUD()
{
	SetBanks_4_5_6();
	DrawRect(0x9f, 0xc1, 0x3a, 0x12, 0x0e);
	EraseWeaponBox();
	uint8_t weapon = Ram(SelectedWeapon);
	if (weapon == 0) {
		SetBanks1_2_3();
		return;
	}
	DrawWeaponIcon(static_cast<uint16_t>(SYM(WeaponGfxXY)
		+ static_cast<uint8_t>((weapon - 1) * 2)), 0xa0, 0xc2);
	SetBanks1_2_3();
	uint16_t hl;
	uint8_t index;
	GetWeaponInvAdd(Ram(SelectedWeapon), &hl, &index);
	RenderAmmoHUD(static_cast<uint16_t>(hl + 2));
}


void
RenderAmmoHUD(uint16_t ammo)
{
	Render3Numbers(ammo, 0xc0, 0xc8);
}


void
EraseWeaponBox()
{
	FillRect(0, 0xa0, 0xc2, 0, 0x38, 0x10);
}


// Item pictures: 16x16 at (X & ~1, Y) of page 1.
static void
DrawItemIcon(uint8_t item, uint8_t x, uint8_t y)
{
	uint16_t xy = GetPointerDE2A(SYM(ItemGfxXY), static_cast<uint8_t>(item - 1));
	VDP_Copy_Byte(static_cast<uint8_t>(xy & 0xfe), static_cast<uint8_t>(xy >> 8), 1, x, y,
		0x10, 0x10);
}


void
DrawItemHUD()
{
	SetBanks_4_5_6();
	DrawRect(0xde, 0xc1, 0x1b, 0x12, 0x0e);
	EraseItemBox();
	uint8_t item = Ram(SelectedItem);
	if (item == 0) {
		SetBanks1_2_3();
		return;
	}
	DrawItemIcon(item, 0xe0, 0xc2);
	SetBanks1_2_3();
	item = Ram(SelectedItem);
	if (item < SELECTED_CARD1 || item >= SELECTED_RATION)
		return;
	// the card number
	uint16_t hl;
	uint8_t index;
	GetItemInvAdd(item, &hl, &index);
	DrawChar(Ram(static_cast<uint16_t>(hl + 1)), 0xff, 0xf0, 0xc8);
}


void
EraseItemBox()
{
	FillRect(0, 0xe0, 0xc2, 0, 0x18, 0x10);
}


// The amount next to an item on the equipment screen.
void
DrawItemAmountInv(uint16_t item, uint8_t index)
{
	SetBanks_4_5_6();
	uint16_t xy = GetPointerDE2A(SYM(ItemEquipXY), static_cast<uint8_t>(index + 1));
	SetBanks1_2_3();
	uint8_t x = static_cast<uint8_t>((xy >> 8) + 48);
	uint8_t y = static_cast<uint8_t>((xy & 0xff) + 4);
	FillRect(0, x, y, 0, 0x18, 0x08);
	Render3Numbers(static_cast<uint16_t>(item + 2), x, y);
}


// SUB 1 / DAA
static uint8_t
BcdDecrement(uint8_t value, bool* borrow)
{
	*borrow = value == 0;
	if (value == 0)
		return 0x99;
	if ((value & 0x0f) == 0)
		return static_cast<uint8_t>(value - 7);
	return static_cast<uint8_t>(value - 1);
}


void
DecItemUnits(uint16_t units, uint8_t kind)
{
	bool borrow;
	Ram(units) = BcdDecrement(Ram(units), &borrow);
	uint16_t hl = static_cast<uint16_t>(units + 1);
	if (borrow)
		Ram(hl)--;	// hundreds
	if (kind == 0) {
		RenderAmmoHUD(hl);
		return;
	}
	bool left = (Ram(hl) | Ram(static_cast<uint16_t>(hl - 1))) != 0;
	if (kind == 1) {
		// weapon used up (plastic bombs, mines...)
		if (left) {
			RenderAmmoHUD(hl);
			return;
		}
		RemoveItem(static_cast<uint16_t>(hl - 2));
		Ram(SelectedWeapon) = 0;
		DrawWeaponHUD();
		return;
	}
	uint16_t item;
	uint8_t index;
	GetItemInvAdd(Ram(static_cast<uint16_t>(hl - 2)), &item, &index);
	if (left) {
		DrawItemAmountInv(item, index);
		return;
	}
	RemoveItem(item);
	SetBanks_4_5_6();
	uint16_t xy = GetPointerDE2A(SYM(ItemEquipXY), static_cast<uint8_t>(index + 1));
	FillRect(0, static_cast<uint8_t>((xy >> 8) + 8), static_cast<uint8_t>((xy & 0xff) - 4), 0,
		0x48, 0x10);	// the icon and the name
	Ram(SelectedItem) = 0;
	DrawItemHUD();
}


// Weapons: two columns of four from (#18, #28), the suppressor at (#60,
// #A8); the ammunition after the name.
void
DrawWeaponMenu()
{
	SetBanks_4_5_6();
	PrintTextXY(SYM(txtWeaponSelect));
	if (Ram(EquipRemoved) != 0) {
		SetBanks1_2_3();
		return;
	}
	const uint16_t kY = TempData2 + 1;	// next weapon's Y, X
	const uint16_t kX = TempData2 + 2;
	SetRam16(kY, 0x1828);
	Ram(static_cast<uint16_t>(TempData3 + 1)) = 0x18;
	uint16_t weapon = Weapons;
	for (int b = 8; b > 0; b--) {
		uint8_t id = Ram(weapon);
		if (id != 0) {
			Ram(TempData) = id;
			SetRam16(static_cast<uint16_t>(TempData + 1),
				Ram16(static_cast<uint16_t>(weapon + 1)));
			DrawWeaponIcon(static_cast<uint16_t>(SYM(WeaponGfxXY)
				+ static_cast<uint8_t>((id - 1) * 2)), Ram(kX), Ram(kY));
			uint16_t name = GetPointerDE2A(SYM(idxWeaponName), static_cast<uint8_t>(id - 1));
			PrintText(name, static_cast<uint8_t>(Ram(kX) + 32), Ram(kY), 0xff);
			if (Ram(TempData) != SUPRESSOR) {
				uint16_t xy = static_cast<uint16_t>(Ram16(kY) + 0x5008);
				SetBanks1_2_3();
				Render3Numbers(TempData2, static_cast<uint8_t>(xy >> 8),
					static_cast<uint8_t>(xy));
				SetBanks_4_5_6();
			}
		}
		Ram(kY) = static_cast<uint8_t>(Ram(kY) + Ram(static_cast<uint16_t>(TempData3 + 1)));
		weapon = static_cast<uint16_t>(weapon + 4);
		if (b == 5)
			SetRam16(kY, 0x8828);	// right column
		else if (b == 2)
			SetRam16(kY, 0x60a8);	// suppressor
	}
	SetBanks1_2_3();
}


// Items: three columns, #18, #68 and #B8, from Y #28; the amount after
// the ration (#16).
void
DrawEquipMenu()
{
	SetBanks_4_5_6();
	PrintTextXY(SYM(txtEquipmentSel));
	if (Ram(EquipRemoved) != 0) {
		SetBanks1_2_3();
		return;
	}
	const uint16_t kY = TempData2 + 1;
	const uint16_t kX = TempData2 + 2;
	SetRam16(kY, 0x1828);
	uint16_t item = Equipment;
	for (int b = 25; b > 0; b--) {
		uint8_t id = Ram(item);
		if (id != 0) {
			Ram(TempData) = id;
			SetRam16(static_cast<uint16_t>(TempData + 1),
				Ram16(static_cast<uint16_t>(item + 1)));
			DrawItemIcon(id, Ram(kX), Ram(kY));
			uint16_t name = GetPointerDE2A(SYM(idxItemName), static_cast<uint8_t>(id - 1));
			PrintText(name, static_cast<uint8_t>(Ram(kX) + 16), Ram(kY), 0xff);
			if (Ram(TempData) == 0x16) {
				uint16_t xy = static_cast<uint16_t>(Ram16(kY) + 0x2808);
				SetBanks1_2_3();
				Render3Numbers(TempData2, static_cast<uint8_t>(xy >> 8),
					static_cast<uint8_t>(xy));
				SetBanks_4_5_6();
			}
		}
		Ram(kY) = static_cast<uint8_t>(Ram(kY) + 0x10);
		item = static_cast<uint16_t>(item + 4);
		if (b == 17)
			SetRam16(kY, 0x6828);
		else if (b == 8)
			SetRam16(kY, 0xb828);
	}
	SetBanks1_2_3();
}
