/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_PLAYER_H
#define MG_PLAYER_H


// Snake: control modes (walk, punch, elevator, parachute, air flow,
// ladders, intro), water, rank, sprite animation; tile collisions
// (Banks0123.asm, logic/collisions.asm, logic/damagegas.asm).

#include <stdint.h>


void		InitPlayerVars();
void		PlayerControlLogic();
void		GetPlayerDir();
void		GetDirUpDown();
void		GetDirLeftRight();
void		ControlPlayerV();
void		ControlPlayerH();
void		ChkControlPlayer();
void		chkPunch();
void		ResetPlayerSpd();
void		ChkPlayerColl();
void		PunchLogic2();
void		ChkCtrlElevator();
void		ChkWater();
void		ChkRoofAirFlow();
void		ChkStartClimb();
void		ChkExitLadders();
void		ChkExitRoom();
void		ChkLimitXElevator();
void		ChkLadderLimits();
void		ExitRoom(uint8_t direction);
uint16_t	MovePlayerX();
uint16_t	MovePlayerY();
void		DowngradeRank();
void		IncRescued();
void		IncClassLv();
void		AnimatePlayer();
void		SetSprIdle();
void		SetGrenaTargetSpr();

// Collisions with tiles: true when one of the two points of the side
// (shape, direction) is on a solid tile.
bool		ChkTileCollision(uint16_t ix, uint8_t shape, uint8_t direction);
bool		ChkTileCollision_(uint8_t x, uint8_t y, uint8_t shape, uint8_t direction);
// The tile under Snake and the one to its right (high, low byte).
uint16_t	GetTilePlayer();
uint16_t	GetTileInXY(uint8_t x, uint8_t y);

void		ChkGasRooms();
void		DecrementLife_C(uint8_t delay);
void		DecrementLife_2();


#endif	// MG_PLAYER_H
