/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_SHOTS_H
#define MG_SHOTS_H


// Shots, shells, lasers, gas, flames and other small actors
// (logic/actors/guardshot.asm, shottoplayer.asm, bullethv.asm,
// snoringsymbol.asm, bigexplosion.asm, tankshot.asm, tankshell.asm,
// shellspawner.asm, gas.asm, lasershot.asm, flame.asm).

#include <stdint.h>


void		InitGuardShot(uint16_t ix);
void		InitShotToPlayer(uint16_t ix);
void		InitBulletHor(uint16_t ix);
void		InitBulletVert(uint16_t ix);

void		InitSnoringSymbol(uint16_t ix);
void		SnoringSymbolLogic(uint16_t ix);
void		BigExplosionLogic(uint16_t ix);

void		InitTankShellBoss(uint16_t ix);
void		InitTankShot(uint16_t ix);
void		TankShotLogic(uint16_t ix);
void		InitTankShell(uint16_t ix);
void		ThankShellLogic(uint16_t ix);
void		InitSpawnTankShell(uint16_t ix);
void		SpawnTankShell(uint16_t ix);

void		InitGas(uint16_t ix);
void		GasLogic(uint16_t ix);

void		InitLaserShot(uint16_t ix);
void		LaserShotLogic(uint16_t ix);

void		InitFlame(uint16_t ix);
void		FlamesLogic(uint16_t ix);
void		FlameOutScreen(uint16_t ix);


#endif	// MG_SHOTS_H
