/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_SPRITES_H
#define MG_SPRITES_H


// Sprites of Snake, the shots and the actors (Banks0123.asm). Actors keep
// their sprites at +#20: count, then per sprite layer, Y, X, pattern,
// colour.

#include <stdint.h>


void		SetSnakeSprAtt();
void		SetSnakeSprCol();
void		SetSnakeSprPatt();
void		UpdateSnakeSpr();
void		SetShotSpr(uint16_t shot);
void		UpdateActorSprDat(uint16_t actor);
void		UpdateEnemySprRAM();
void		UpdateEnemySprRAM3(uint16_t actor);
void		HideEnemySprs();
void		SetSpritesFree(uint16_t sprite, uint8_t count);
void		SetActorSprCols(uint16_t actor, uint16_t table);
void		SetSpriteLayer(uint8_t layer, uint8_t index);
void		HideSprColor(uint16_t actor);
// Odd sprites colour a, even sprites colour c.
void		SetSprColor(uint16_t actor, uint8_t a, uint8_t c);
void		EraseSprAttRAM();
void		LoadSprMissilRocket(uint16_t shot, uint16_t table);
void		LoadSprTarget();
// VRAM to RAM; returns the RAM address after the data.
uint16_t	VRAMtoRAM(uint16_t vram, uint16_t ram, uint16_t count);
void		WriteVRAM(uint16_t vram, uint8_t value);

void		SwapSprBuffer();
void		UpdateSpritesShuf();
void		UpdateSprites();
void		SetBinoTargetSpr();
void		SetHUDSprMask();


#endif	// MG_SPRITES_H
