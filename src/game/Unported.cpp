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
UNPORTED(InitGas)
UNPORTED(InitTank)
UNPORTED(InitTankShell)
UNPORTED(InitShooter)
UNPORTED(InitGuardElevat)
UNPORTED(InitRollingBarrel)
UNPORTED(InitBulldozer)
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
UNPORTED(InitLaserShot)
UNPORTED(InitGuardSilencer)
UNPORTED(InitBulletHor)
UNPORTED(InitBulletVert)
UNPORTED(InitMGunKidShot)
UNPORTED(InitShotToPlayer)
UNPORTED(InitTankShot)
UNPORTED(InitBoomerang)
UNPORTED(InitSnoringSymbol)

#include "Unported.h"

#define UNPORTED0(name) \
	void name() { if (gUnported == NULL) gUnported = #name; }

UNPORTED0(IntroSceneLogic)
UNPORTED(BridgeLogic)
UNPORTED(ChkSayHelpMe)
UNPORTED(GasLogic)
UNPORTED(TankLogic)
UNPORTED(ThankShellLogic)
UNPORTED(ShooterLogic)
UNPORTED(GuardElevator)
UNPORTED(RollingBarrelLogic)
UNPORTED(BulldozerLogic)
UNPORTED(JetpacTakeoff)
UNPORTED(JetpackSwitchLogic)
UNPORTED(JetpackLogic)
UNPORTED(GuardSwitchLogic)
UNPORTED(DogLogic)
UNPORTED(ArnoldLogic)
UNPORTED(DogBasementLogic)
UNPORTED(LorryShooterLogic)
UNPORTED(SpawnDogLogic)
UNPORTED(ScorpionLogic)
UNPORTED(BigBossLogic)
UNPORTED(ShotGunnerLogic)
UNPORTED(MachGunKidLogic)
UNPORTED(FireTrooperLoogic)
UNPORTED(FlamesLogic)
UNPORTED(SpawnTankShell)
UNPORTED(SpawnGuardElev)
UNPORTED(CowardDuckLogic)
UNPORTED(ShotGunnerShot)
UNPORTED(PowerSwitchLogic)
UNPORTED(CaptureGuardsLogic)
UNPORTED(DesertSecurityLogic)
UNPORTED(SentinelLogic)
UNPORTED(PrisonerLogic)
UNPORTED(LaserShotLogic)
UNPORTED(FakeMadnadLogic)
UNPORTED(GuardSilencerLogic)
UNPORTED(MGunKidShotLogic)
UNPORTED(TankShotLogic)
UNPORTED(BoomerangLogic)
UNPORTED(SnoringSymbolLogic)
UNPORTED(BigExplosionLogic)
UNPORTED(FlameOutScreen)
UNPORTED0(SetBulldozerKO)
UNPORTED0(ChkPasswords)
UNPORTED0(ChkCharTyped)
UNPORTED0(EndingSetup)
UNPORTED0(EndingLogic)
UNPORTED0(SetElevatorSpr)
UNPORTED0(MenuEquip)
UNPORTED0(ElevatorRoomLogic)
UNPORTED0(CaptureSceneLogic)
