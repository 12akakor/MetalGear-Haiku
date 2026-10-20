/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_BOSSES_H
#define MG_BOSSES_H


// Bosses and other big enemies (logic/actors/tank.asm, arnold.asm,
// scorpion.asm, machinegunkid.asm, shotgunner.asm, ...).

#include <stdint.h>


void		InitTank(uint16_t ix);
void		TankLogic(uint16_t ix);
void		InitArnold(uint16_t ix);
void		ArnoldLogic(uint16_t ix);
void		InitScorpion(uint16_t ix);
void		ScorpionLogic(uint16_t ix);
void		InitMachGunKid(uint16_t ix);
void		MachGunKidLogic(uint16_t ix);
void		InitMGunKidShot(uint16_t ix);
void		MGunKidShotLogic(uint16_t ix);
void		InitShotGunner(uint16_t ix);
void		ShotGunnerLogic(uint16_t ix);
void		InitShotGunnerShot(uint16_t ix);
void		ShotGunnerShot(uint16_t ix);
void		InitFireTrooper(uint16_t ix);
void		FireTrooperLoogic(uint16_t ix);
void		InitBigBoss(uint16_t ix);
void		BigBossLogic(uint16_t ix);
void		InitCowardDuck(uint16_t ix);
void		CowardDuckLogic(uint16_t ix);
void		InitBoomerang(uint16_t ix);
void		BoomerangLogic(uint16_t ix);


#endif	// MG_BOSSES_H
