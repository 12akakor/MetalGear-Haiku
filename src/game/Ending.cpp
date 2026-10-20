/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/ending.asm. The original's "_" wrappers select
// banks 1, 2 and the fixed one after each routine; that is kept.
#include "Ending.h"

#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "Player.h"
#include "Rom.h"
#include "RoomGfx.h"
#include "Screens.h"
#include "Sprites.h"
#include "Structures.h"
#include "Text.h"


using namespace var;


namespace {

// The Z80's DAA after an addition (sub false) or a subtraction.
uint8_t
Daa(uint8_t a, bool carry, bool half, bool sub, bool* carryOut)
{
	uint8_t diff = 0;
	bool newCarry = carry;
	if (carry || a > 0x99) {
		diff = 0x60;
		newCarry = true;
	}
	if (half || (a & 0x0f) > 9)
		diff |= 0x06;
	*carryOut = newCarry;
	return static_cast<uint8_t>(sub ? a - diff : a + diff);
}


void
Banks()
{
	SetBanks_1_2_X();
}


void
SetSoundEntry__(uint8_t id)
{
	SetSoundEntryChk(id);
	Banks();
}


void
NextEndingStatus()
{
	Ram(EndingStatus)++;
}


void
EndingSetText(uint8_t text)
{
	NextEndingStatus();
	SetTextUnskippable(text);
	Banks();
}


// The text window: true when it is over (the original's Z flag).
bool
EndingTextBox()
{
	TextBoxLogic();
	Banks();
	return Ram(GameMode) == GAME_MODE_RADIO;
}


void
EndingSnakeRun3()
{
	SetSnakeSprAtt();
	Banks();
	SetSnakeSprPatt();
	Banks();
}


// Snake runs away to the left.
void
EndingSnakeRun()
{
	Ram(SnakeSprId) = (Ram(TickCounter) & 8) != 0 ? 5 : 4;
	SetRam16(PlayerXdec, static_cast<uint16_t>(Ram16(PlayerXdec) - 8));
	EndingSnakeRun3();
}


// Explosion frames, then the light flickers between two palettes.
void
EndingExplosion()
{
	uint8_t count = Ram(EndingCnt);
	if (count == 0xe8) {
		SetSoundEntry__(0x56);
		return;
	}
	if (count == 0xe0) {
		DrawTileBlkTimp(SYM(ExploxionTiles1), 0x90, 0x60);
		Banks();
		return;
	}
	if (count == 0xd0) {
		DrawTileBlkTimp(SYM(ExploxionTiles2), 0x80, 0x50);
		Banks();
		return;
	}
	if (count == 0xc0) {
		DrawTileBlkTimp(SYM(ExploxionTiles3), 0x80, 0x40);
		Banks();
		return;
	}
	if (count > 0xc0 || (count & 7) != 0)
		return;
	SetRoomPal2((count & 0x10) != 0 ? 0x0f : 0x0d);
	Banks();
}


// The destruction timer counts down while Snake runs.
void
EndingEscaping()
{
	EndingSnakeRun();
	uint8_t low = Ram(DestructTimer);
	if ((Ram(DestructTimerH) | low) == 0) {
		Ram(EndingCnt) = 0;
		FillRect(0, 0x70, 0xc0, 0, 0x20, 0x10);	// erases the timer
		Banks();
		NextEndingStatus();
		return;
	}
	bool borrow;
	Ram(DestructTimer) = Daa(static_cast<uint8_t>(low - 1), low == 0, (low & 0x0f) == 0, true,
		&borrow);
	if (borrow)
		Ram(DestructTimerH)--;
	DrawDestrucTimer();
	Banks();
}

}	// namespace


void
EndingSetup()
{
	DisableScreen();
	Banks();
	ClearScreen();
	Banks();
	Ram(Room) = 251;	// for the tiles of the ending
	LoadRoomTiles();
	Banks();
	SetRoomPal();
	Banks();
	Ram(EndingStatus) = 0;
	SetRam16(DestructTimer, 0x200);
	Ram(GameMode) = GAME_MODE_RADIO;
	Ram(PlayerX) = 0x80;
	Ram(PlayerY) = 0x80;
	Ram(RadioFreq) = FREQ_BIGBOSS_BUILDING2;
	EnableScreen();
	Banks();
}


void
EndingLogic()
{
	switch (Ram(EndingStatus)) {
		case 1:	// the explosion; Snake turns round
			EndingSnakeRun();
			if (--Ram(EndingCnt) != 0) {
				EndingExplosion();
				return;
			}
			Ram(EndingCnt) = 0x80;
			Ram(SnakeSprId) = 0;
			EndingSnakeRun3();
			FillRect(0, 0x80, 0x40, 0, 0x40, 0x28);	// (A is 0 after SetSnakeSprPatt)
			Banks();
			NextEndingStatus();
			return;
		case 2:	// stares back
			if (--Ram(EndingCnt) != 0)
				return;
			Ram(EndingCnt) = 0;
			NextEndingStatus();
			return;
		case 3:	// the second explosion
			if (--Ram(EndingCnt) != 0) {
				EndingExplosion();
				return;
			}
			NextEndingStatus();
			return;
		case 4:	// the radio, once the explosion is quiet
			if (Ram(static_cast<uint16_t>(SoundWorkAreaB + 2)) != 0)
				return;
			ClearSprAttr();
			Banks();
			EraseSprAttRAM();
			Banks();
			DrawRadio();
			Banks();
			Ram(EndingCnt) = 0x40;
			NextEndingStatus();
			return;
		case 5:	// "This is Solid Snake..."
			if (--Ram(EndingCnt) != 0)
				return;
			PrintSend();
			Banks();
			EndingSetText(0x9b);
			return;
		case 6:
			if (!EndingTextBox())
				return;
			PrintRecv();
			Banks();
			SetSoundEntry__(0x50);
			NextEndingStatus();
			return;
		case 7:	// tunes to the news
		{
			if ((Ram(TickCounter) & 3) != 0)
				return;
			uint8_t freq = Ram(RadioFreq);
			bool carry;
			Ram(RadioFreq) = Daa(static_cast<uint8_t>(freq + 1), freq == 0xff,
				(freq & 0x0f) == 0x0f, false, &carry);
			DrawRadioFreq();
			Banks();
			if (Ram(RadioFreq) != FREQ_NEWS)
				return;
			Ram(RadioLedDelay) = 0x10;
			Ram(RadioLedCnt) = 0;
			Ram(EquipRadioStatus) = 0;
			NextEndingStatus();
			return;
		}
		case 8:
			RadioSignalUp();
			Banks();
			if (Ram(EquipRadioStatus) != 1)
				return;
			SetSoundEntry__(0x5c);
			EndingSetText(31);	// This is radio KNK
			return;
		case 9:
			if (!EndingTextBox())
				return;
			SetSoundEntry__(0x41);	// ending music
			EndingSetText(45);	// staff
			return;
		case 10:
			if (!EndingTextBox())
				return;
			Ram(MusicToSet) = 0x5c;
			CopyPalToRAM();
			Banks();
			NextEndingStatus();
			return;
		case 11:
		{
			bool done = FadeOutLogic();
			Banks();
			if (!done)
				return;
			ClearScreen();
			Banks();
			Ram(EndingCnt) = 0x70;
			NextEndingStatus();
			return;
		}
		case 12:
			if (--Ram(EndingCnt) != 0)
				return;
			SetSoundEntry__(0x5c);
			SetPaletteColor(0x0e, 0x77, 0x07);	// white
			Banks();
			EndingSetText(15);	// Big Boss: I'll never die
			return;
		case 13:
			if (!EndingTextBox())
				return;
			EnableScreen();
			Banks();
			NextEndingStatus();
			return;
		default:	// 0 (and after 13): escaping, the countdown
			EndingEscaping();
			return;
	}
}
