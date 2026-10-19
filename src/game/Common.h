/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_COMMON_H
#define MG_COMMON_H


// Small helpers of the game logic (Banks0123.asm): room tables, doors,
// inventory, weapon data. Routines that switch banks leave banks 1, 2, 3
// selected, as the original.

#include <stdint.h>


// Memory as the Z80 sees it (ROM banks below #C000, variables above).
uint8_t		Mem(uint16_t address);
uint16_t	Mem16(uint16_t address);

void		SetBanks1_2_3();
void		SetBanks_4_5_6();
void		SetBanks_7_8_9();
void		SetBanks_A_B_C();
void		SetBanks_D_E_F();

uint16_t	HL_4xA(uint8_t a);
uint16_t	DEC_A_HL_4xA(uint8_t a);

uint8_t		GetNextRoomNum();
// Returns a pointer to the elevator's room data (+1) and sets its limits.
uint16_t	GetElevatorRoomDat();
void		GetElevatorPosY(uint8_t* playerY, uint8_t* elevatorY);
void		SetDefaultDoorLock();
void		ChkIsolatedRoom();
void		SetRadioArea();
// Returns the maximum number of shots; TempData points to the damages.
uint8_t		GetWeaponDamages(uint8_t weapon);
uint8_t		GetTempPHL_A(uint8_t a);
uint8_t		GET_HL_A_DEC(uint16_t hl, uint8_t a);
uint8_t		GetItemAmount(uint8_t item);
// Copies the impact area of a shape to TempImpactInfo (returned).
uint16_t	GetShapeInfo(uint8_t shape);
void		CalcCursorXYWeapon();
void		CalcCursorXYEquip();
// Inventory slot of an item or weapon: false (carry) if it is not there.
// 'address' and 'index' as the original leaves HL and C.
bool		GetItemInvAdd(uint8_t id, uint16_t* address, uint8_t* index);
bool		GetWeaponInvAdd(uint8_t id, uint16_t* address, uint8_t* index);
void		AddDoorsData();

// Tiles drawn with transparency (TIMP) and the room's tile map
uint16_t	CoordToBuffTile(uint8_t x, uint8_t y);
void		GetTileXY_B2(uint8_t tile, uint8_t* x, uint8_t* y);
void		DrawTileTIMP(uint8_t tile, uint8_t x, uint8_t y);
// Blocks of tiles: data = NY, NX, tiles (banks D-F).
void		DrawTilesBlock(uint16_t data, uint8_t x, uint8_t y);
void		DrawTileBlkTimp(uint16_t data, uint8_t x, uint8_t y);
// Tiles only (no size header), tile bank 1 or the current one.
void		DrawTileBlkTimp2(uint16_t tiles, uint8_t rows, uint8_t columns, uint8_t x, uint8_t y);
void		DrawTileBlkTimp3(uint16_t tiles, uint8_t rows, uint8_t columns, uint8_t x, uint8_t y);
void		DrawMetalGear(uint8_t x, uint8_t y);
void		EraseMetalGear(uint8_t x, uint8_t y);
void		SaveBackgTiles(uint16_t data, uint8_t x, uint8_t y);

void		SetAreaMusic();
void		SetAreaMusic2();

void		RemoveItem(uint16_t address);
void		SetActorSprColors(uint16_t actor);
void		SetActorSprColors2(uint16_t actor, uint8_t id);

#endif	// MG_COMMON_H
