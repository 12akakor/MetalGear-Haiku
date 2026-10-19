/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Routines that are not rewritten yet: they flag gUnported, so the game
// goes back to the title and the tests show what is missing. Each one moves
// to its module when it is rewritten.
#include "Actors.h"
#include "GameMain.h"

#include <stddef.h>


#define UNPORTED(name) \
	void name(uint16_t) { if (gUnported == NULL) gUnported = #name; }

UNPORTED(InitBridge)
UNPORTED(InitEllenVoice)
UNPORTED(InitCamera)
UNPORTED(InitMines)
UNPORTED(InitGas)
UNPORTED(InitTank)
UNPORTED(InitGuardAlert)
UNPORTED(InitTankShell)
UNPORTED(InitShooter)
UNPORTED(InitGuardElevat)
UNPORTED(InitRollingBarrel)
UNPORTED(InitPitfall)
UNPORTED(InitMetalGear_)
UNPORTED(InitBulldozer)
UNPORTED(InitGuardLorry)
UNPORTED(InitJetpackTakeoff)
UNPORTED(InitJetpackSwitch)
UNPORTED(InitJetpack)
UNPORTED(InitTankShellBoss)
UNPORTED(InitGuardSwitch)
UNPORTED(InitDog)
UNPORTED(InitArnold)
UNPORTED(InitDogBasement)
UNPORTED(InitLorryShooter)
UNPORTED(InitSpawnDog)
UNPORTED(InitScorpion)
UNPORTED(InitBigBoss)
UNPORTED(InitShotGunner)
UNPORTED(InitMachGunKid)
UNPORTED(InitFireTrooper)
UNPORTED(InitFlame)
UNPORTED(InitHindD)
UNPORTED(InitSpawnTankShell)
UNPORTED(InitSpawnGuardElev)
UNPORTED(InitCowardDuck)
UNPORTED(InitShotGunnerShot)
UNPORTED(InitPowerSwitch)
UNPORTED(InitCaptureScene)
UNPORTED(InitDesertSecurity)
UNPORTED(InitGuardShot)
UNPORTED(InitSentinel)
UNPORTED(InitPrisoner)
UNPORTED(InitCameraLaser)
UNPORTED(InitLaserShot)
UNPORTED(InitGuardSilencer)
UNPORTED(InitBulletHor)
UNPORTED(InitBulletVert)
UNPORTED(InitMGunKidShot)
UNPORTED(InitShotToPlayer)
UNPORTED(InitTankShot)
UNPORTED(InitBoomerang)
UNPORTED(InitSnoringSymbol)
UNPORTED(DismissActor)
UNPORTED(AnimateGuard2)

#include "Unported.h"

#define UNPORTED0(name) \
	void name() { if (gUnported == NULL) gUnported = #name; }

UNPORTED0(DrawOpenPitfalls)
UNPORTED0(ExitRadio)
UNPORTED0(IntroSceneLogic)
UNPORTED0(ChkSaveGameStatus)
UNPORTED0(SetMaxAmmoVals)
UNPORTED0(LimitAmmo)
