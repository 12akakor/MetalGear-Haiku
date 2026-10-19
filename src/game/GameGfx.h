/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_GAME_GFX_H
#define MG_GAME_GFX_H


// Loading the game's graphics: doors, sprites of the room and of the
// weapons, items, radio, pitfalls; palettes (Banks0123.asm).

#include <stdint.h>


// Packed graphics: n < #80 = n times the next byte, #80 = new VRAM
// address (next 2 bytes), #80 + n = n bytes, 0 = end.
void		UnpackGfx1(uint16_t vram, uint16_t data);
void		UnpackGfx(uint16_t data);

void		LoadGfxDoors();
// 'rows' rows of 'columns' tiles, each row 8 lines (#400 bytes) lower.
uint16_t	LoadTilesGfxBlk(uint16_t source, uint16_t vram, uint8_t columns, uint8_t rows);
void		LoadRoomSpr();
void		LoadSprProjectile();
void		LoadGameGfx();
void		PrintTransceiv();

void		SetupPitfall(uint16_t source, uint16_t vram, uint8_t count);
void		DrawPitfall();
void		DrawPitfall_();
void		DrawPitfallTiles(uint16_t data, uint8_t x, uint8_t y);
void		DrawPitfallTile(uint8_t tile, uint8_t x, uint8_t y);
void		GetPitfallXY(uint8_t tile, uint8_t* x, uint8_t* y);

void		SetMenuWeaponPal();
void		SetSnakePal();
void		SetRadioPal();
void		SetSprPal();

void		UpdateRadio();


#endif	// MG_GAME_GFX_H
