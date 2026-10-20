/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/guardalert.asm: guards in an alert chase
// Snake, walk around obstacles, shoot, keep away from him (red alert),
// and turn back at doors and lorries.
#include "Guards.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Player.h"
#include "Random.h"
#include "Rom.h"
#include "Structures.h"
#include "Weapons.h"


using namespace var;


namespace {

inline uint8_t&
A(uint16_t ix, int field)
{
	return Ram(static_cast<uint16_t>(ix + field));
}


void GuardChasePlayer2(uint16_t ix, uint8_t direction);


void
SetWalkSpeedTable(uint16_t ix, uint16_t table, uint8_t direction)
{
	uint16_t speeds = GetPointerDE2A(table, direction);
	A(ix, ALERT_GUARD::SpeedY) = static_cast<uint8_t>(speeds);
	A(ix, ALERT_GUARD::SpeedX) = static_cast<uint8_t>(speeds >> 8);
}


void
AnimateGuard(uint16_t ix)
{
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
}


// The walk ends after a random time (red alert guards: sooner).
void
SetGuardRndCounter(uint16_t ix)
{
	uint8_t base = A(ix, ALERT_GUARD::ID) == ID_GUARD_REDALERT ? 0x0a : 0x14;
	A(ix, ALERT_GUARD::Counter) = static_cast<uint8_t>(((RefreshR() ^ A(ix, ALERT_GUARD::ANIM_CNT))
		& 0x0f) + base);
}


void
GuardSetNewDirection(uint16_t ix, uint8_t direction)
{
	A(ix, ALERT_GUARD::Direction) = direction;
	AnimateGuard2(ix, direction);
	SetWalkSpeedFast(ix);
	SetGuardRndCounter(ix);
}


void
ContinueThisWay(uint16_t ix, uint8_t direction)
{
	A(ix, ALERT_GUARD::Status) = 0;	// chase the player
	GuardSetNewDirection(ix, direction);
}


void
AddEnemyShot(uint16_t ix)
{
	AddEnemy(ID_GUARD_BULLET, static_cast<uint8_t>(A(ix, ACTOR::Y) - 0x10), A(ix, ACTOR::X));
}


void
GuardShot(uint16_t ix, uint8_t wait)
{
	A(ix, ALERT_GUARD::Counter) = wait;
	A(ix, ALERT_GUARD::Status) = 1;
	A(ix, ALERT_GUARD::Moving) = 0;
	AddEnemyShot(ix);
}


// Red alert guards keep their distance.
bool
ChkNearPlayer(uint16_t ix)
{
	if (A(ix, ACTOR::ID) != ID_GUARD_REDALERT)
		return false;
	return GetDistancePlayer(ix) < 0x30;
}


void
SetGuardWalkAway(uint16_t ix)
{
	uint8_t other;
	uint8_t main = GetDirToPlayer(ix, &other);
	uint8_t away = GetOppositeDir(other);
	uint8_t awayOther = GetOppositeDir(main);
	if (!ChkTileCollision(ix, 0, away)) {
		GuardChasePlayer2(ix, away);
		return;
	}
	A(ix, ALERT_GUARD::Status) = 3;
	A(ix, ALERT_GUARD::WalkAwayDir) = away;
	GuardSetNewDirection(ix, awayOther);
}


void
GuardChasePlayer(uint16_t ix)
{
	uint8_t other;
	uint8_t direction = GetDirToPlayer(ix, &other);
	if (!ChkTileCollision(ix, 0, direction)) {
		GuardChasePlayer2(ix, direction);
		return;
	}
	A(ix, ALERT_GUARD::Status) = 2;	// walk around the obstacle
	A(ix, ALERT_GUARD::WalkAwayDir) = direction;
	GuardSetNewDirection(ix, other);
}


// Walks on, or now and then stops to shoot.
void
GuardChasePlayer2(uint16_t ix, uint8_t direction)
{
	A(ix, ALERT_GUARD::Direction) = direction;
	AnimateGuard2(ix, direction);
	SetWalkSpeedFast(ix);
	uint8_t wait = static_cast<uint8_t>(((RefreshR() ^ A(ix, ALERT_GUARD::ANIM_CNT)) & 7) + 0x0f);
	if ((wait & 3) == 0) {
		SetGuardRndCounter(ix);
		return;
	}
	GuardShot(ix, wait);
}


void
GuardWalk(uint16_t ix)
{
	AnimateGuard(ix);
	if (!ChkTileCollision(ix, 0, A(ix, ALERT_GUARD::Direction))) {
		if (--A(ix, ALERT_GUARD::Counter) != 0)
			return;
	}
	if (ChkNearPlayer(ix))
		SetGuardWalkAway(ix);
	else
		GuardChasePlayer(ix);
}


void
GuardWaitShot(uint16_t ix)
{
	uint8_t other;
	GuardLookDirection2(ix, GetDirToPlayer(ix, &other));
	if (--A(ix, ALERT_GUARD::Counter) != 0)
		return;
	SetGuardRndCounter(ix);
	A(ix, ALERT_GUARD::Status) = 0;
	A(ix, ALERT_GUARD::Moving) = 1;
	if (A(ix, ALERT_GUARD::ID) == ID_GUARD_REDALERT)
		AddEnemyShot(ix);
}


void
GuardAvoidObstacle(uint16_t ix)
{
	AnimateGuard(ix);
	uint8_t away = A(ix, ALERT_GUARD::WalkAwayDir);
	if (!ChkTileCollision(ix, 0, away)) {
		ContinueThisWay(ix, away);
		return;
	}
	if (!ChkTileCollision(ix, 0, A(ix, ALERT_GUARD::Direction)))
		return;
	uint8_t direction = GetOppositeDir(A(ix, ALERT_GUARD::WalkAwayDir));
	if (!ChkTileCollision(ix, 0, direction)) {
		ContinueThisWay(ix, direction);
		return;
	}
	ContinueThisWay(ix, GetOppositeDir(A(ix, ALERT_GUARD::Direction)));
}


void
GuardWalkAwayShot(uint16_t ix)
{
	AnimateGuard(ix);
	if (ChkTileCollision(ix, 0, A(ix, ALERT_GUARD::Direction))) {
		GuardChasePlayer(ix);
		return;
	}
	if (--A(ix, ALERT_GUARD::Counter) != 0)
		return;
	if (ChkNearPlayer(ix)) {
		SetGuardWalkAway(ix);
		return;
	}
	GuardShot(ix, 0x14);
}


void
GuardWaitChkAlert(uint16_t ix)
{
	if (--A(ix, ALERT_GUARD::WalkAwayDir) != 0)
		return;
	A(ix, ALERT_GUARD::Status) = 0;
	if (Ram(AlertMode) != 0)
		return;
	SetAlertMode(ix);
}


void
MoveAwayExit(uint16_t ix)
{
	A(ix, ACTOR::Status) = 0;
	A(ix, ACTOR::Wait) = 0x0a;
	uint8_t direction = GetOppositeDir(A(ix, ACTOR::Direction));
	A(ix, ACTOR::Direction) = direction;
	AnimateGuard2(ix, direction);
	SetWalkSpeedFast(ix);
}


// Guards wade in water (other sprites); at a door or lorry they turn back.
void
ChkGuardWater(uint16_t ix)
{
	uint16_t tiles = GetTileInXY(static_cast<uint8_t>(A(ix, ACTOR::X) - 4), A(ix, ACTOR::Y));
	uint8_t h = static_cast<uint8_t>(tiles >> 8);
	uint8_t l = static_cast<uint8_t>(tiles);
	if (l == 1 || h == 1) {
		MoveAwayExit(ix);
		return;
	}
	if (Ram(Room) < 70)
		return;
	A(ix, ACTOR::BASE_SPR_ID) = 0;
	if (l != 0x6d) {
		if (static_cast<uint8_t>(l - 0x6f) >= 8)
			return;
		if (static_cast<uint8_t>(h - 0x6f) >= 8)
			return;
	}
	A(ix, ACTOR::BASE_SPR_ID) = 0x49;	// in the water
}


// Rooms where the guards of an alert keep coming.
void
SetRespawnTime()
{
	uint8_t room = Ram(Room);
	uint16_t timer = 0;
	if (room != 216) {
		if (room != 187 && room != 154 && static_cast<uint8_t>(room - 88) >= 5)
			return;
		timer = static_cast<uint16_t>(0x0a00 | ((RefreshR() & 0x0f) + 0x10));
	}
	SetRam16(AlertRespawnTimer, timer);
}

}	// namespace


void
InitGuardAlert(uint16_t ix)
{
	A(ix, ALERT_GUARD::Status) = 4;	// waits, then the alert
	if (Ram(AlertMode) == 0) {
		uint8_t room = Ram(Room);
		uint16_t flag = 0;
		if (room == 127)
			flag = Guard1ExitedLorry;
		else if (room == 131)
			flag = Guard2ExitedLorry;
		else if (room == 132)
			flag = Guard3ExitedLorry;
		if (flag != 0 && Ram(flag) != 0) {
			DismissActor0(ix);	// this guard is out of the lorry
			return;
		}
	}
	A(ix, ALERT_GUARD::COLLISION_CFG) = 3;
	uint8_t other;
	uint8_t direction = GetDirToPlayer(ix, &other);
	A(ix, ALERT_GUARD::Direction) = direction;
	AnimateGuard2(ix, direction);
	SetWalkSpeedFast(ix);
	A(ix, ALERT_GUARD::speedYdec) = 0;
	A(ix, ALERT_GUARD::SpeedXdec) = 0;
	A(ix, ALERT_GUARD::BaseSpriteId) = 0;
	A(ix, ALERT_GUARD::Counter) = 0x14;
	A(ix, ALERT_GUARD::Moving) = 1;
	A(ix, ALERT_GUARD::WalkAwayDir) = 1;
	SetRespawnTime();
}


void
GuardAlertLogic(uint16_t ix)
{
	ChkGuardWater(ix);
	switch (A(ix, ALERT_GUARD::Status)) {
		case 0:
			GuardWalk(ix);
			return;
		case 1:
			GuardWaitShot(ix);
			return;
		case 2:
			GuardAvoidObstacle(ix);
			return;
		case 3:
			GuardWalkAwayShot(ix);
			return;
		case 4:
			GuardWaitChkAlert(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "GuardAlertLogic: bad status";
			return;
	}
}


void
SetWalkSpeed(uint16_t ix)
{
	// (!?) DirectionSpeeds starts with the JR before it: up to right are
	// the bytes after it, overlapping DirectionSpeeds2
	SetWalkSpeedTable(ix, SYM(DirectionSpeeds), A(ix, ALERT_GUARD::Direction));
}


void
SetWalkSpeedFast(uint16_t ix)
{
	SetWalkSpeedFast2(ix, A(ix, ALERT_GUARD::Direction));
}


void
SetWalkSpeedFast2(uint16_t ix, uint8_t direction)
{
	SetWalkSpeedTable(ix, SYM(DirectionSpeeds2), direction);
}


void
AnimateGuard2(uint16_t ix, uint8_t direction)
{
	uint8_t sprite = static_cast<uint8_t>((direction - 1) * 2);
	if ((A(ix, ACTOR::ANIM_CNT) & 4) != 0)
		sprite++;
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(sprite + A(ix, ACTOR::BASE_SPR_ID));
}


void
GuardLookDirection(uint16_t ix)
{
	GuardLookDirection2(ix, A(ix, ACTOR::Direction));
}


void
GuardLookDirection2(uint16_t ix, uint8_t direction)
{
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(direction + 7 + A(ix, ACTOR::BASE_SPR_ID));
}


uint8_t
GetDistancePlayer(uint16_t ix)
{
	uint8_t dx = static_cast<uint8_t>(Ram(PlayerX) - A(ix, ACTOR::X));
	if (Ram(PlayerX) < A(ix, ACTOR::X))
		dx = static_cast<uint8_t>(-dx);
	uint8_t dy = static_cast<uint8_t>(Ram(PlayerY) - A(ix, ACTOR::Y));
	if (Ram(PlayerY) < A(ix, ACTOR::Y))
		dy = static_cast<uint8_t>(-dy);
	return dy < dx ? dx : dy;
}


uint8_t
GetRandom3(uint16_t ix)
{
	return static_cast<uint8_t>((RefreshR() ^ A(ix, ACTOR::ANIM_CNT)) & 3);
}
