/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_HUD_H
#define MG_HUD_H


// The status bar (life, class, call sign, weapon and item boxes), the
// weapon and equipment screens and the drawing helpers they use
// (logic/hud.asm and Banks0123.asm).

#include <stdint.h>


// Lines and rectangles (VDP LINE command): 'length' pixels; 0 = 256.
void		DrawLineHoriz(uint8_t x, uint8_t y, uint8_t length, uint8_t color);
void		DrawLineVert(uint8_t x, uint8_t y, uint8_t length, uint8_t color);
void		DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);

// Numbers in BCD: hundreds at 'number', tens and units at number - 1.
void		Render3Numbers(uint16_t number, uint8_t x, uint8_t y);
void		PrintDigit(uint8_t digit, uint8_t c, uint8_t* x, uint8_t y);

void		RenderHUD();
void		DrawLife();
void		DrawClass();
void		DrawCallTimer();
void		DrawDestrucTimer();
void		DrawWeaponHUD();
void		EraseWeaponBox();
void		RenderAmmoHUD(uint16_t ammo);
void		DrawItemHUD();
void		EraseItemBox();
void		DrawItemAmountInv(uint16_t item, uint8_t index);
void		DecrementLife_4();
void		DecrementLife_B(uint8_t amount);
void		SetDead();

// Uses one unit of a weapon (kind 0), consumable weapon (1) or item (2).
void		DecItemUnits(uint16_t units, uint8_t kind);

void		DrawWeaponMenu();
void		DrawEquipMenu();


#endif	// MG_HUD_H
