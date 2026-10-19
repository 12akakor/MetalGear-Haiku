/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/gamedemo.asm.
#include "Demo.h"

#include "Common.h"
#include "Game.h"
#include "GameGfx.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Input.h"
#include "Rom.h"
#include "Screens.h"
#include "Structures.h"
#include "Text.h"


using namespace var;


// Modes 1 and 3 are recorded games (rooms 5 and 31, the second with the
// hand gun), 0 and 2 the radio tutorial.
void
SetupDemoPlay()
{
	ClearGameVars();
	SetBanks_1_2_X();
	uint8_t id = static_cast<uint8_t>((Ram(DemoPlayId) + 1) & 3);
	Ram(DemoPlayId) = id;
	uint16_t data = GetPointerDE2A(SYM(idxDemoPlayCtrl), id);
	SetBanks_1_2_X();
	Ram(DemolHoldTime) = Mem(data);
	data++;
	StoreControls(ControlsHold, Mem(data));
	SetBanks_1_2_X();
	SetRam16(DemoDataPointer, data);

	if (id == 1 || id == 3) {
		if (id == 1) {
			Ram(Room) = 5;
		} else {
			Ram(Room) = 31;
			Ram(SelectedWeapon) = HAND_GUN;
			Ram(WeaponInUse) = HAND_GUN;
			Ram(Weapons) = HAND_GUN;
		}
		InitGame();
		SetBanks_1_2_X();
		if (Ram(MusicInDemoMode) == 0) {
			Ram(AreaMusic) = 0x2c;	// Theme of Tara
			SetSoundEntry(0x2c);
			SetBanks_1_2_X();
		}
		bool first = id < 2;
		Ram(PlayerY) = first ? 0x70 : 0x28;
		Ram(PlayerX) = first ? 0x10 : 0x70;
	} else {
		SetMenuWeaponPal();
		SetBanks_1_2_X();
		LoadGameGfx();
		SetBanks_1_2_X();
		Ram(RadioFreq) = FREQ_BIGBOSS;
		Ram(Life) = 0x10;
		DrawRadio();
		SetBanks_1_2_X();
		Ram(RadioLedDelay) = 0x10;
		Ram(TutorialStatus) = 0;
		Ram(RadioLedCnt) = 0;
		Ram(EquipRadioStatus) = 0;
	}
	Ram(PlayingFlag) = 1;
}


void
GameDemoLogic()
{
	if (Ram(DemoPlayId) & 1) {
		GameLogic();
		SetBanks_1_2_X();
		return;
	}
	switch (Ram(TutorialStatus)) {
		case 1:
			TextBoxLogic();
			SetBanks_1_2_X();
			if (Ram(GameMode) == GAME_MODE_RADIO)
				Ram(TutorialStatus)++;
			return;
		case 2:
			return;
		default:
			// the radio's signal comes up, then Big Boss explains
			RadioSignalUp();
			SetBanks_1_2_X();
			if (Ram(EquipRadioStatus) != 1)
				return;
			Ram(TutorialStatus)++;
			SetTextUnskippable(36);
			SetBanks_1_2_X();
			return;
	}
}


// Replays the recorded controls: (frames, controls) pairs, #FF ends.
void
DemoControler()
{
	if (--Ram(DemolHoldTime) == 0) {
		uint16_t data = static_cast<uint16_t>(Ram16(DemoDataPointer) + 1);
		uint8_t time = Mem(data);
		if (time == 0xff) {
			Ram(PlayingFlag) = 0;
			return;
		}
		Ram(DemolHoldTime) = time;
		SetRam16(DemoDataPointer, static_cast<uint16_t>(data + 1));
	}
	StoreControls(ControlsHold, Mem(Ram16(DemoDataPointer)));
	SetBanks_1_2_X();
}
