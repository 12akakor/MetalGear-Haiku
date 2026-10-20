/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/prisoner.asm and fakemadnar.asm.
#include "Prisoners.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "Player.h"
#include "Random.h"
#include "Rom.h"
#include "Structures.h"
#include "Text.h"


using namespace var;


namespace {

inline uint8_t&
A(uint16_t ix, int field)
{
	return Ram(static_cast<uint16_t>(ix + field));
}


void
SetAsRescued(uint16_t ix)
{
	Ram(static_cast<uint16_t>(A(ix, PRISONER::P_RESCUED_L) | (A(ix, PRISONER::P_RESCUED_H) << 8)))
		= 1;
}


void
PrisonerNextStat(uint16_t ix)
{
	A(ix, ACTOR::Status)++;
}


void
RescueAndShowTxt(uint16_t ix, uint8_t text)
{
	IncRescued();
	SetText(text);
	PrisonerNextStat(ix);
}


// Rescued: the prisoner's text; the rescues count for Snake's rank.
void
PrisonerRescued(uint16_t ix)
{
	if (A(ix, ACTOR::ID) == ID_MADNAR) {
		// (!?) he can be "rescued" again and again until Ellen is
		if (Ram(static_cast<uint16_t>(RescuedArray + 0x14)) == 0) {
			RescueAndShowTxt(ix, 124);	// save Ellen first
			return;
		}
		SetAsRescued(ix);
		RescueAndShowTxt(ix, 125);
		return;
	}
	uint8_t room = Ram(Room);
	if (room == 167) {
		// Ellen
		SetAsRescued(ix);
		PrisonerNextStat(ix);
		IncRescued();
		SetText(0x81);
		return;
	}
	if (room == 193) {
		// the prisoners of Coward Duck's room: Jennifer's brother
		if (A(ix, ACTOR::Y) != 0x54) {
			RescueAndShowTxt(ix, 131);
			return;
		}
		SetAsRescued(ix);
		RescueAndShowTxt(ix, 140);
		return;
	}
	uint16_t hl = SYM(PrisonerTexts);
	for (int i = 0; i < 19; i++, hl = static_cast<uint16_t>(hl + 2)) {
		if (Mem(hl) != room)
			continue;
		SetAsRescued(ix);
		SetText(Mem(static_cast<uint16_t>(hl + 1)));
		IncRescued();
		PrisonerNextStat(ix);
		return;
	}
}

}	// namespace


void
InitPrisoner(uint16_t ix)
{
	A(ix, PRISONER::SpriteId) = 0x3e;
	A(ix, PRISONER::IDX_SAME_ID) = CountEnemyType(A(ix, PRISONER::ID));
	A(ix, PRISONER::ANIM_CNT) = RefreshR();
	A(ix, PRISONER::FALL_TIME) = 0x20;
	A(ix, PRISONER::TIMER) = 2;
	// CPIR: the index counts down from the end of the list (0 if the room
	// is not there)
	uint8_t room = Ram(Room);
	uint8_t c = 23;
	uint16_t rooms = SYM(RoomsPrisoner);
	for (int i = 0; i < 23; i++) {
		c--;
		if (Mem(static_cast<uint16_t>(rooms + i)) == room)
			break;
	}
	uint16_t rescued = static_cast<uint16_t>(RescuedArray + c);
	if (Ram(rescued) != 0) {
		DismissActor0(ix);
		return;
	}
	A(ix, PRISONER::P_RESCUED_L) = static_cast<uint8_t>(rescued);
	A(ix, PRISONER::P_RESCUED_H) = static_cast<uint8_t>(rescued >> 8);
}


void
PrisonerLogic(uint16_t ix)
{
	switch (A(ix, ACTOR2::Status)) {
		case 0:	// waits for Snake
			if (A(ix, PRISONER::ID) == ID_ELLEN)
				ChkSayHelpMe(ix);
			Anim2FramesActor(ix, 0x0f);
			if (Ram(Room) == 193 && Ram(Card8Taken) == 0)
				return;	// not before Coward Duck is defeated
			if ((A(ix, ACTOR2::TOUCH_INFO) & 0x80) == 0)
				return;
			A(ix, ACTOR2::TIMER) = 2;
			A(ix, ACTOR2::SpriteId) = 0x40;	// free
			PrisonerNextStat(ix);
			return;
		case 1:
			if (--A(ix, PRISONER::TIMER) != 0)
				return;
			PrisonerNextStat(ix);
			return;
		case 2:
			PrisonerRescued(ix);
			return;
		case 3:
			return;
		default:
			if (gUnported == NULL)
				gUnported = "PrisonerLogic: bad status";
			return;
	}
}


// Ellen's "Help me!" from the next room.
void
InitEllenVoice(uint16_t ix)
{
	if (Ram(static_cast<uint16_t>(RescuedArray + 0x14)) != 0) {
		DismissActor0(ix);
		return;
	}
	A(ix, PRISONER::COLLISION_CFG) = 0;
	A(ix, PRISONER::TIMER) = 0x20;
}


void
ChkSayHelpMe(uint16_t ix)
{
	if (--A(ix, ACTOR2::TIMER) != 0)
		return;
	A(ix, ACTOR2::TIMER) = 0xc0;
	SetTextUnskippable(128);	// Help me!
}


// The fake Dr. Madnar: a trap.
void
FakeMadnadLogic(uint16_t ix)
{
	switch (A(ix, ACTOR2::Status)) {
		case 0:
			Anim2FramesActor(ix, 0x0f);
			if ((A(ix, ACTOR2::TOUCH_INFO) & 0x80) == 0)
				return;
			A(ix, ACTOR2::SpriteId) = 0x40;
			NextActorStatus(ix);
			return;
		case 1:
			SetTextUnskippable(109);	// You are caught in a trap
			NextActorStatus(ix);
			return;
		case 2:
			NextActorStatus(ix);
			A(ix, ACTOR2::MOVING)++;
			SetActorSpeedY(ix, 0x100);	// falls
			Ram(static_cast<uint16_t>(RescuedArray + 0x16)) = 1;
			A(ix, ACTOR2::Timer) = 0x10;
			AddEnemy(ID_PITFALL, 0x60, 0x80);
			return;
		case 3:
			if (--A(ix, ACTOR2::Timer) != 0)
				return;
			DismissActor0(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "FakeMadnadLogic: bad status";
			return;
	}
}
