/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/lorry.asm and madnarbigbossevent.asm.
#include "Events.h"

#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Rom.h"
#include "Structures.h"
#include "Text.h"


using namespace var;


void
ChkLorryMov()
{
	uint8_t room = Ram(Room);
	for (int i = 0; i < 6; i++) {
		if (Mem(static_cast<uint16_t>(SYM(MovingLorries) + i)) == room) {
			Ram(EventCnt) = 0x90;
			Ram(GameMode) = GAME_MODE_LORRY;
			return;
		}
	}
}


// "I goofed. The lorry started to move", the engine and a shaking screen.
void
LorryMoving()
{
	uint8_t count = --Ram(EventCnt);
	if (count == 0) {
		SetSoundEntryChk(0x28);	// engine off
		SetBanks_1_2_X();
		Ram(GameMode) = 0;
		WRTVDP(23, 0);
		return;
	}
	if (count == 0x8e && Ram(LorryMovTextF) == 0) {
		Ram(LorryMovTextF) = 1;
		SetTextUnskippable(91);
		SetBanks_1_2_X();
		return;
	}
	if (count == 0x8d) {
		SetSoundEntryChk(0x1f);
		SetBanks_1_2_X();
	}
	if (count >= 0x80)
		return;
	SetBanks_1_2_X();
	uint8_t offset = Mem(static_cast<uint16_t>(SYM(VertScrollOffset) + ((count & 0x0e) >> 1)));
	WRTVDP(23, offset);
}


// Room 133: Madnar has been moved (once); room 111: Big Boss will order
// to switch the MSX off.
void
ChkMadnarMsx()
{
	uint8_t room = Ram(Room);
	if (room == 133) {
		if (Ram(MadnarMoved) != 0)
			return;
		Ram(EventCnt) = 2;
		Ram(GameMode) = 0x0c;
		return;
	}
	if (room == 111)
		Ram(SwitchOffMSXF) = 1;
}


void
chkMadnarLate()
{
	uint8_t count = --Ram(EventCnt);
	if (count == 0) {
		SetTextUnskippable(89);	// It's too late
		SetBanks_1_2_X();
		return;
	}
	if (count == 1)
		return;
	Ram(GameMode) = 0;
	Ram(MadnarMoved) = 1;
}
