/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/introscene.asm: Snake swims to the shore at the
// start of the game, takes the first call and climbs out of the water.
#include "IntroScene.h"

#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Player.h"
#include "Rom.h"
#include "Screens.h"
#include "Structures.h"
#include "Text.h"


using namespace var;


namespace {

// The original's "_" wrappers: the routine, then banks 1, 2 and the one
// fixed at #A000.
void
SetPlayerSpr_()
{
	AnimatePlayer();
	SetBanks_1_2_X();
}


void
SetSoundEntry__(uint8_t id)
{
	SetSoundEntryChk(id);
	SetBanks_1_2_X();
}


void
NextIntroScene()
{
	Ram(IntroSceneStatus)++;
	SetPlayerSpr_();
}


void
IntroChkExitScreen()
{
	ChkExitRoom();
	SetBanks_1_2_X();
	SetPlayerSpr_();
}


void
IntroSceneControls()
{
	ChkControlPlayer();
	SetBanks_1_2_X();
	ChkPlayerColl();
	SetBanks_1_2_X();
	IntroChkExitScreen();
}


// Snake moves as with these controls held.
void
IntroMove(uint8_t direction, uint8_t controls)
{
	Ram(ControlsTrigger) = 0;
	if (direction != 0)
		Ram(PlayerDirection) = direction;
	Ram(ControlsHold) = controls;
	IntroSceneControls();
}


bool
CountDown()
{
	return --Ram(IntroSceneCnt) == 0;
}


void
SetPlayerYdec(uint16_t value)
{
	SetRam16(PlayerYdec, value);
}

}	// namespace


void
IntroSceneLogic()
{
	switch (Ram(IntroSceneStatus)) {
		case 0:	// diving left, then up
			if (!CountDown()) {
				IntroMove(DIR_LEFT, 4);
				return;
			}
			Ram(PlayerAnimation) = 2;	// in the water
			Ram(PlayerDirection) = 1;
			Ram(IntroSceneCnt) = 0x30;
			NextIntroScene();
			return;
		case 1:	// looks at the entrance, dives again
			if (!CountDown()) {
				IntroMove(0, 0);
				return;
			}
			Ram(IntroSceneCnt) = 0x50;
			Ram(PlayerAnimation) = 4;	// under water
			Ram(PlayerDirection) = 3;
			NextIntroScene();
			return;
		case 2:	// dives left
			if (!CountDown()) {
				IntroMove(0, 4);
				return;
			}
			Ram(IntroSceneCnt) = 0x20;
			NextIntroScene();
			return;
		case 3:	// dives north, comes up facing right
			if (!CountDown()) {
				IntroMove(1, 1);
				return;
			}
			Ram(PlayerAnimation) = 2;
			Ram(PlayerDirection) = 4;
			Ram(IntroSceneCnt) = 0x40;
			NextIntroScene();
			return;
		case 4:	// waits; the radio rings
			if (!CountDown()) {
				if (Ram(IntroSceneCnt) != 0x20)
					return;
				Ram(RadioCallFlag) = 1;
				SetSoundEntry__(0x22);	// incoming call
				return;
			}
			Ram(RestoreSoundData) = 1;
			SetSoundEntry__(0x59);
			DrawRadio();
			SetBanks_1_2_X();
			Ram(RadioLedDelay) = 0x10;
			Ram(RadioLedCnt) = 0;
			Ram(EquipRadioStatus) = 0;
			NextIntroScene();
			return;
		case 5:
			RadioSignalUp();
			SetBanks_1_2_X();
			if (Ram(EquipRadioStatus) != 1)
				return;
			NextIntroScene();
			return;
		case 6:	// Operation Intrude N313
			NextIntroScene();
			SetSoundEntry__(0x5c);
			SetText(2);
			SetBanks_1_2_X();
			return;
		case 7:
			ExitRadio();
			SetBanks_1_2_X();
			Ram(IntroSceneCnt) = 0x28;
			SetRam16(PlayerMovSpeed, 0x200);
			NextIntroScene();
			return;
		case 8:	// swims right
			if (!CountDown()) {
				IntroMove(DIR_RIGHT, static_cast<uint8_t>(DIR_RIGHT << 1));
				return;
			}
			Ram(IntroSceneCnt) = 0x30;
			NextIntroScene();
			return;
		case 9:	// swims up to the ladder
			if (!CountDown()) {
				IntroMove(1, 1);
				return;
			}
			SetPlayerYdec(0x8800);
			SetRam16(PlayerMovSpeed, 0x0100);
			Ram(PlayerDirection) = 1;
			Ram(PlayerAnimation) = 5;	// climbing
			Ram(IntroSceneCnt) = 0x1c;
			NextIntroScene();
			return;
		case 10:	// climbs
			if (!CountDown()) {
				Ram(ControlsTrigger) = 0;
				Ram(PlayerDirection) = 1;
				Ram(ControlsHold) = 1;
				ControlPlayerV();
				SetBanks_1_2_X();
				IntroChkExitScreen();
				return;
			}
			SetPlayerYdec(0x6600);
			Ram(PlayerAnimation) = 0;
			Ram(PlayerDirection) = 1;
			Ram(StopPlayerFlag) = 1;
			Ram(IntroSceneCnt) = 0x0c;
			NextIntroScene();
			return;
		case 11:	// bounces at the top
		{
			if (CountDown()) {
				NextIntroScene();
				return;
			}
			Ram(ControlsHold) = 0;
			Ram(ControlsTrigger) = 0;
			Ram(PlayerDirection) = 1;
			uint8_t count = Ram(IntroSceneCnt);
			if ((count & 1) != 0) {
				// (!?) ADD_HL_A_ selects the banks before the table is
				// read
				SetBanks_1_2_X();
				uint8_t offset = Mem(static_cast<uint16_t>(SYM(BounceOffsets) + count));
				SetPlayerYdec(static_cast<uint16_t>(Ram16(PlayerYdec) + (offset << 8)));
			}
			SetPlayerSpr_();
			return;
		}
		case 12:
			Ram(PlayerControlMod) = 0;
			SetRam16(PlayerMovSpeed, 0x200);
			SetPlayerSpr_();
			return;
		default:
			if (gUnported == NULL)
				gUnported = "IntroSceneLogic: bad status";
			return;
	}
}
