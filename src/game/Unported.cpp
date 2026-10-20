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
UNPORTED(InitGuardElevat)
UNPORTED(InitRollingBarrel)
UNPORTED(InitBulldozer)
UNPORTED(InitGuardSwitch)
UNPORTED(InitLorryShooter)
UNPORTED(InitScorpion)
UNPORTED(InitBigBoss)
UNPORTED(InitShotGunner)
UNPORTED(InitMachGunKid)
UNPORTED(InitFireTrooper)
UNPORTED(InitCowardDuck)
UNPORTED(InitShotGunnerShot)
UNPORTED(InitPowerSwitch)
UNPORTED(InitCaptureScene)
UNPORTED(InitDesertSecurity)
UNPORTED(InitPrisoner)
UNPORTED(InitGuardSilencer)
UNPORTED(InitMGunKidShot)
UNPORTED(InitBoomerang)

#include "Unported.h"

#define UNPORTED0(name) \
	void name() { if (gUnported == NULL) gUnported = #name; }

UNPORTED(BridgeLogic)
UNPORTED(ChkSayHelpMe)
UNPORTED(GuardElevator)
UNPORTED(RollingBarrelLogic)
UNPORTED(BulldozerLogic)
UNPORTED(GuardSwitchLogic)
UNPORTED(LorryShooterLogic)
UNPORTED(ScorpionLogic)
UNPORTED(BigBossLogic)
UNPORTED(ShotGunnerLogic)
UNPORTED(MachGunKidLogic)
UNPORTED(FireTrooperLoogic)
UNPORTED(CowardDuckLogic)
UNPORTED(ShotGunnerShot)
UNPORTED(PowerSwitchLogic)
UNPORTED(CaptureGuardsLogic)
UNPORTED(DesertSecurityLogic)
UNPORTED(PrisonerLogic)
UNPORTED(FakeMadnadLogic)
UNPORTED(GuardSilencerLogic)
UNPORTED(MGunKidShotLogic)
UNPORTED(BoomerangLogic)
UNPORTED0(SetBulldozerKO)
UNPORTED0(ChkPasswords)
UNPORTED0(ChkCharTyped)
UNPORTED0(EndingSetup)
UNPORTED0(EndingLogic)
UNPORTED0(SetElevatorSpr)
UNPORTED0(MenuEquip)
UNPORTED0(ElevatorRoomLogic)
UNPORTED0(CaptureSceneLogic)
