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

UNPORTED(InitEllenVoice)
UNPORTED(InitScorpion)
UNPORTED(InitBigBoss)
UNPORTED(InitShotGunner)
UNPORTED(InitMachGunKid)
UNPORTED(InitFireTrooper)
UNPORTED(InitCowardDuck)
UNPORTED(InitShotGunnerShot)
UNPORTED(InitCaptureScene)
UNPORTED(InitPrisoner)
UNPORTED(InitGuardSilencer)
UNPORTED(InitMGunKidShot)
UNPORTED(InitBoomerang)

#include "Unported.h"

#define UNPORTED0(name) \
	void name() { if (gUnported == NULL) gUnported = #name; }

UNPORTED(ChkSayHelpMe)
UNPORTED(ScorpionLogic)
UNPORTED(BigBossLogic)
UNPORTED(ShotGunnerLogic)
UNPORTED(MachGunKidLogic)
UNPORTED(FireTrooperLoogic)
UNPORTED(CowardDuckLogic)
UNPORTED(ShotGunnerShot)
UNPORTED(CaptureGuardsLogic)
UNPORTED(PrisonerLogic)
UNPORTED(FakeMadnadLogic)
UNPORTED(GuardSilencerLogic)
UNPORTED(MGunKidShotLogic)
UNPORTED(BoomerangLogic)
UNPORTED0(ChkPasswords)
UNPORTED0(ChkCharTyped)
UNPORTED0(EndingSetup)
UNPORTED0(EndingLogic)
UNPORTED0(MenuEquip)
UNPORTED0(CaptureSceneLogic)
