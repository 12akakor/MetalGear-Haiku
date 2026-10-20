/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/capturescene.asm.
#include "CaptureScene.h"

#include "Actors.h"
#include "Game.h"
#include "GameGfx.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Guards.h"
#include "Rom.h"
#include "Screens.h"
#include "Sprites.h"
#include "Structures.h"
#include "Text.h"


using namespace var;


namespace {

inline uint8_t&
A(uint16_t ix, int field)
{
	return Ram(static_cast<uint16_t>(ix + field));
}


// The second guard comes from the right, above or below Snake.
void
AddCaptureGuardB(uint16_t ix)
{
	A(ix, ACTOR::Status) = 6;	// this one does nothing more
	SetTextUnskippable(6);	// Don't move!
	uint8_t y = Ram(PlayerY) < 0x98 ? 0xb0 : 0x88;
	uint16_t guard = AddEnemy(ID_CAPTURE_GUARD, y, 0xf0);
	A(guard, ACTOR::Status) = 2;
	A(guard, ACTOR::Moving) = 1;
	A(guard, ACTOR::Direction) = DIR_LEFT;
	A(guard, ACTOR::SpriteId) = 0x0a;
	SetWalkSpeedFast(guard);
}


// Snake loses his equipment and wakes up in the cell of room 165.
void
PutInPrison()
{
	if (--Ram(CaptureTimer) != 0)
		return;
	Ram(EquipRemoved) = 1;
	Ram(SelectedWeapon) = 0;
	Ram(SelectedItem) = 0;
	Ram(AlertMode) = 0;
	Ram(AlertRespawnTimer) = 0;
	Ram(Room) = 165;
	Ram(PreviousRoom) = 8;
	Ram(PlayerY) = 0x50;
	Ram(PlayerX) = 0x80;
	EraseSprAttRAM();
	DisableScreen();
	SetSnakePal();
	Ram(GameMode) = 0;
	InitGame4();
}

}	// namespace


void
CaptureSceneLogic()
{
	switch (Ram(CaptureStatus)) {
		case 0:	// the first guard: "Don't move!"
			Ram(CaptureStatus)++;
			AddEnemy(ID_CAPTURE_GUARD, Ram(PlayerY), 0xf0);
			return;
		case 1:	// the guards move and talk
			return;
		case 2:
			Ram(CaptureStatus)++;
			Ram(CaptureTimer) = 0x3c;
			Ram(MusicToSet) = 0x5c;	// music off
			return;
		case 3:
			if (--Ram(CaptureTimer) != 0)
				return;
			Ram(CaptureStatus)++;
			CopyPalToRAM();
			return;
		case 4:
			if (!FadeOutLogic())
				return;
			Ram(CaptureStatus)++;
			Ram(CaptureTimer) = 0x10;
			return;
		case 5:
			PutInPrison();
			return;
		default:
			if (gUnported == NULL)
				gUnported = "CaptureSceneLogic: bad status";
			return;
	}
}


void
InitCaptureScene(uint16_t ix)
{
	A(ix, ACTOR::SpriteId) = 0x0a;	// left
	A(ix, ACTOR::COLLISION_CFG) = 0;
	A(ix, ACTOR::Wait) = 2;
}


void
CaptureGuardsLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:
			if (--A(ix, ACTOR::Wait) != 0)
				return;
			A(ix, ACTOR::Status)++;
			return;
		case 1:
			AddCaptureGuardB(ix);
			return;
		case 2:	// guard B walks left...
			AnimateGuard2(ix, A(ix, ACTOR::Direction));
			if (A(ix, ACTOR::X) != 0xb8)
				return;
			A(ix, ACTOR::Status)++;
			A(ix, ACTOR::Direction) = A(ix, ACTOR::Y) == 0x88 ? DIR_DOWN : DIR_UP;
			SetWalkSpeedFast(ix);
			return;
		case 3:	// ...then up or down to Snake
			AnimateGuard2(ix, A(ix, ACTOR::Direction));
			if ((Ram(PlayerY) & 0xfe) != A(ix, ACTOR::Y))
				return;
			A(ix, ACTOR::Status)++;
			A(ix, ACTOR::Moving) = 0;
			A(ix, ACTOR::SpriteId) = 0x0b;
			A(ix, ACTOR::Wait) = 2;
			return;
		case 4:
			if (--A(ix, ACTOR::Wait) != 0)
				return;
			A(ix, ACTOR::Status)++;
			A(ix, ACTOR::Wait) = 0x1e;
			SetTextUnskippable(7);	// You are captured
			return;
		case 5:
			if (--A(ix, ACTOR::Wait) != 0)
				return;
			A(ix, ACTOR::Status)++;
			Ram(CaptureStatus)++;
			return;
		case 6:
			return;
		default:
			if (gUnported == NULL)
				gUnported = "CaptureGuardsLogic: bad status";
			return;
	}
}
