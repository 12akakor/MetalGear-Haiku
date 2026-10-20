/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/guard.asm, chkdiscover.asm, guardlorry.asm
// and guardelevator.asm (SaveAlertIconBacknd, Draw16x16).
#include "Guards.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Random.h"
#include "Rom.h"
#include "Sprites.h"
#include "Structures.h"
#include "Text.h"
#include "Weapons.h"


using namespace var;


namespace {

inline uint8_t&
A(uint16_t ix, int field)
{
	return Ram(static_cast<uint16_t>(ix + field));
}


inline uint16_t
Add16(uint16_t address, int n)
{
	return static_cast<uint16_t>(address + n);
}


// --- What guards and cameras see -------------------------------------------

// Is the player in the view range: across a vertical view...
bool
ChkViewVertical()
{
	uint8_t enemyX = Ram(Add16(TempData, 2));
	uint8_t a = static_cast<uint8_t>(Ram(PlayerX) - 8);
	if (a >= enemyX)
		return false;
	uint8_t limit = static_cast<uint8_t>(a + 0x10);
	return enemyX < limit;
}


// ...or across a horizontal one.
bool
ChkViewHorizontal(uint16_t ix)
{
	uint8_t half = 6;
	uint8_t width = 0x0c;
	if (A(ix, ACTOR::ID) == ID_CAMERA) {
		half = 4;
		width = 8;
	}
	uint8_t enemyY = Ram(Add16(TempData, 1));
	uint8_t a = static_cast<uint8_t>(Ram(PlayerY) - half);
	if (a >= enemyY)
		return false;
	uint8_t limit = static_cast<uint8_t>(a + width);
	return enemyY < limit;
}


uint8_t
TileDistance(uint8_t distance)
{
	distance = static_cast<uint8_t>((distance & 0xf8) >> 3);
	return distance == 0 ? 1 : distance;
}


bool
IsHandrail(uint8_t tile)
{
	return (tile >= 0x68 && tile <= 0x6c) || tile == 0x6e
		|| (tile >= 0x8b && tile <= 0x8e);
}


// Looks 'count' tiles from the actor towards the player; nothing in the
// way: the player is seen.
void
ChkViewObstacles(uint16_t ix, uint8_t count, int8_t offset)
{
	uint16_t hl = CoordToBuffTile(Ram(Add16(TempData, 2)), Ram(Add16(TempData, 1)));
	int n = count == 0 ? 256 : count;
	for (int i = 0; i < n; i++) {
		SetRam16(Add16(TempData2, 1), hl);
		uint8_t tile = Ram(hl);
		if (Ram(CurrentTileSet) != 0 || !IsHandrail(tile)) {
			if (Ram(Add16(CollisionTiles, tile)) != 0)
				return;	// an obstacle in the view
		}
		hl = static_cast<uint16_t>(Ram16(Add16(TempData2, 1)) + offset);
	}

	if (A(ix, ACTOR::ID) != ID_CAMERA) {
		GuardSetAlarm(ix);
		return;
	}
	A(ix, ACTOR::SEE_PLAYER_F) = 1;
	Ram(AlertSignNotOnScreen) = 1;	// no alert sign over a camera
	Ram(AlertRespawnTimer) = 0x28;
	SetAlert(ix);
}


// --- Guard patrol ----------------------------------------------------------

}	// namespace


// The guard reached his destination point: true when the original drops
// its caller (the guard stops at the point).
bool
ChkReachPoint(uint16_t ix)
{
	uint8_t x = A(ix, ACTOR::X);
	uint8_t y = A(ix, ACTOR::Y);
	uint8_t destX = A(ix, ACTOR::DestinationX);
	uint8_t destY = A(ix, ACTOR::DestinationY);
	switch (A(ix, ACTOR::Direction)) {
		case 1:	// down
			if (destY >= y)
				return false;
			break;
		case 2:	// left
			if (destX < x)
				return false;
			break;
		case 3:	// right
			if (destX >= x)
				return false;
			break;
		default:	// up
			if (destY < y)
				return false;
			break;
	}
	return UpdateActorPath(ix);
}


namespace {

// A sleepy guard falls asleep after a while. True when he does (the
// original drops the patrol routine and its caller).
bool
ChkSleepyGuard(uint16_t ix)
{
	if (Ram(SleepyGuardFlag) == 0)
		return false;
	if (--A(ix, ACTOR::AwakeTime) != 0)
		return false;
	A(ix, ACTOR::Moving) = 0;
	A(ix, ACTOR::GuardStatusOld) = A(ix, ACTOR::GuardStatus);
	A(ix, ACTOR::GuardStatus) = 3;	// sleeping
	A(ix, ACTOR::SpriteId) = 2;
	A(ix, ACTOR::SleepingTime) = 0;	// 256
	SetTextUnskippable(33);	// I'm sleepy
	AddEnemy(0x40, static_cast<uint8_t>(A(ix, ACTOR::Y) - 0x23), A(ix, ACTOR::X));	// Zzz
	return true;
}


bool
GuardPatrolLogic2(uint16_t ix)
{
	if (ChkSleepyGuard(ix))
		return true;
	Anim2FramesActor(ix, 7);
	return ChkReachPoint(ix);
}


void
GuardPatrolLogic(uint16_t ix)
{
	if (GuardPatrolLogic2(ix))
		return;
	ChangeGuardSprDir(ix);
}


// Waits, then turns 90 degrees (and maybe 180 more).
void
GuardPatrolTurn(uint16_t ix)
{
	if (--A(ix, ACTOR::Wait) != 0)
		return;
	uint8_t direction = A(ix, ACTOR::Direction);
	A(ix, ACTOR::PreviousDirection) = direction;
	direction = static_cast<uint8_t>((direction ^ 2) | ((RefreshR() >> 2) & 1));
	A(ix, ACTOR::Direction) = direction;
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(direction + 8);	// standing
	A(ix, ACTOR::Wait) = 0x10;
	A(ix, ACTOR::GuardStatus)++;
}


// Waits, then walks on in the direction he had.
void
GuardPatrolWait(uint16_t ix)
{
	if (--A(ix, ACTOR::Wait) != 0)
		return;
	A(ix, ACTOR::Moving)++;
	A(ix, ACTOR::Direction) = A(ix, ACTOR::PreviousDirection);
	A(ix, ACTOR::GuardStatus) = 0;
}


void
GuardSleeping(uint16_t ix)
{
	if (ListenShotsChkTouch(ix)) {
		GuardSetAlarm(ix);
		return;
	}
	if (--A(ix, ACTOR::SleepingTime) != 0)
		return;
	A(ix, ACTOR::AwakeTime) = 0xc0;
	Ram(Add16(ix, 0x8d)) = 0;	// the life of the Zzz sign (the next actor)
	A(ix, ACTOR::GuardStatus)++;
}


void
GuardWakeUp(uint16_t ix)
{
	A(ix, ACTOR::Moving)++;
	A(ix, ACTOR::GuardStatus) = A(ix, ACTOR::GuardStatusOld);
	SetTextUnskippable(34);	// Overslept
}


// --- Guard leaving a lorry --------------------------------------------------

// The flag that tells whether this guard is out of his lorry (rooms 5 and
// 7), or 0.
uint16_t
ExitedLorryFlag(uint16_t ix)
{
	uint8_t room = Ram(Room);
	if (room == 5)
		return Guard1ExitedLorry;
	if (room != 7)
		return 0;
	uint8_t index = A(ix, ACTOR::IDX_SAME_ID);
	if (index == 1)
		return Add16(Guard1ExitedLorry, 1);
	if (index == 2)
		return Add16(Guard1ExitedLorry, 2);
	return 0;
}


void
InitGuardLorry2(uint16_t ix)
{
	uint16_t flag = ExitedLorryFlag(ix);
	if (flag != 0)
		Ram(flag) = 0;	// in the lorry
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::idxPathPoint) = 0;
	uint8_t timer = 0x64;
	if ((Ram(ControlConfig) & 0x40) != 0)
		timer = static_cast<uint8_t>(RefreshR() | 0x80);
	A(ix, ACTOR::LORRY_TIMER) = timer;
	A(ix, ACTOR::SpriteId) = 2;	// down
	ResetActorSpeed(ix);
}

}	// namespace


// --- What guards and cameras see -------------------------------------------

void
ChkActSeePlayer(uint16_t ix)
{
	ChkSeePlayer(ix, A(ix, ACTOR::Direction), A(ix, ACTOR::X), A(ix, ACTOR::Y));
}


void
ChkSeePlayer(uint16_t ix, uint8_t direction, uint8_t x, uint8_t y)
{
	Ram(TempData) = direction;
	Ram(Add16(TempData, 1)) = y;
	Ram(Add16(TempData, 2)) = x;
	if (Ram(AlertMode) != 0)
		return;
	uint8_t sprite = Ram(SnakeSprId);
	if (sprite == 37 || sprite == 38)
		return;	// deep water
	if (Ram(PlayerAnimation) == 7) {
		// in the cardboard box: seen only when it moves
		if ((Ram(PlayerSpeedY) | Ram(Add16(PlayerSpeedY, 1))) == 0
			&& (Ram(PlayerSpeedX) | Ram(Add16(PlayerSpeedX, 1))) == 0)
			return;
	}
	if ((A(ix, ACTOR::TOUCH_INFO) & 0x80) != 0) {
		GuardSetAlarm(ix);
		return;
	}

	uint8_t enemyX = Ram(Add16(TempData, 2));
	uint8_t enemyY = Ram(Add16(TempData, 1));
	switch (Ram(TempData)) {
		case 0:	// up
			if (!ChkViewVertical() || enemyY < Ram(PlayerY))
				return;
			ChkViewObstacles(ix, TileDistance(static_cast<uint8_t>(enemyY - Ram(PlayerY))),
				-0x20);
			return;
		case 1:	// down
			if (!ChkViewVertical() || enemyY >= Ram(PlayerY))
				return;
			ChkViewObstacles(ix, TileDistance(static_cast<uint8_t>(Ram(PlayerY) - enemyY)),
				0x20);
			return;
		case 2:	// left
			if (!ChkViewHorizontal(ix) || enemyX < Ram(PlayerX))
				return;
			ChkViewObstacles(ix, TileDistance(static_cast<uint8_t>(enemyX - Ram(PlayerX))),
				-1);
			return;
		case 3:	// right
			if (!ChkViewHorizontal(ix) || enemyX >= Ram(PlayerX))
				return;
			ChkViewObstacles(ix, TileDistance(static_cast<uint8_t>(Ram(PlayerX) - enemyX)),
				1);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "ChkSeePlayer: bad direction";
			return;
	}
}


// A guard raises the alarm: the alert sign appears next to him. Rooms
// with a red alert make the guards come back.
void
GuardSetAlarm(uint16_t ix)
{
	if (Ram(AlertMode) != 0)
		return;
	uint8_t room = Ram(Room);
	uint8_t red = 0;
	if (room < 128) {
		uint8_t bits = Mem(Add16(SYM(RedAlertRooms), room >> 3));
		red = static_cast<uint8_t>((bits >> (7 - (room & 7))) & 1);
	}
	Ram(RedAlertFlag) = red;

	// (!?) the original adds 8 where it meant the offset to the left or
	// right of the guard
	uint8_t x = static_cast<uint8_t>((A(ix, ACTOR::X) + 8) & 0xf8);
	uint8_t y = static_cast<uint8_t>((A(ix, ACTOR::Y) - 24) & 0xf8);
	if (y >= 0xbf)
		y = 0;
	Ram(XY_AlertIcon) = y;
	Ram(Add16(XY_AlertIcon, 1)) = x;
	SaveAlertIconBacknd(x, y);

	uint8_t iconX = 0;
	uint8_t respawn = 0;
	if (Ram(RedAlertFlag) != 0) {
		iconX = 0x10;	// red alert sign
		respawn = 0x1e;
	}
	Ram(AlertRespawnTimer) = respawn;
	Draw16x16(iconX, 0x80, x, y);
	SetAlert(ix);
	if (A(ix, ACTOR::ID) == ID_CAMERA)
		return;
	TransformAlertGuard(ix);
}


void
SetAlert(uint16_t ix)
{
	SetAlertMode(ix);
	SetAlertRoom();
}


void
SetAlertRoom()
{
	Ram(RoomAlertTrigged) = Ram(Room);
	Ram(AlertIconTimer) = 0x10;
}


bool
ListenShotsChkTouch(uint16_t ix)
{
	if (A(ix, ACTOR::ID) == ID_CAMERA)
		return false;
	if ((A(ix, ACTOR::TOUCH_INFO) & 0x80) != 0)
		return true;
	if (Ram(SelectedWeapon) < GRENADE_LAUNCHER && Ram(InvSupressor) != 0)
		return false;	// handgun or SMG with the suppressor
	// (!?) after a shot that is not exploding, A no longer holds 0, so
	// the next shots are compared with that value
	uint8_t a = 0;
	uint16_t hl = PlayerShotsList;
	for (int i = 0; i < 6; i++, hl = Add16(hl, 0x40)) {
		if (Ram(hl) == a)
			continue;
		a = static_cast<uint8_t>(Ram(Add16(hl, 1)) - 1);
		if (a == 0)
			return true;
	}
	return false;
}


void
SaveAlertIconBacknd(uint8_t sx, uint8_t sy)
{
	VDP_Copy_Byte(sx, sy, 4, 0x60, 0x80, 0x10, 0x10);
}


void
Draw16x16(uint8_t sx, uint8_t sy, uint8_t dx, uint8_t dy)
{
	VDP_Copy_Dot(sx, sy, 0x48, dx, dy, 0x10, 0x10);
}


// --- Guard patrol ----------------------------------------------------------

// Follows the path, stops, looks around; sleepy guards fall asleep.
void
GuardLogic(uint16_t ix)
{
	if (A(ix, ACTOR::REMOVE_GUARD) != 0) {
		DismissActor0(ix);
		return;
	}
	if (A(ix, ACTOR::GuardStatus) != 3)
		ChkActSeePlayer(ix);
	if (Ram(AlertMode) != 0) {
		TransformAlertGuard(ix);
		return;
	}
	switch (A(ix, ACTOR::GuardStatus)) {
		case 0:
			GuardPatrolLogic(ix);
			return;
		case 1:
			GuardPatrolTurn(ix);
			return;
		case 2:
			GuardPatrolWait(ix);
			return;
		case 3:
			GuardSleeping(ix);
			return;
		case 4:
			GuardWakeUp(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "GuardLogic: bad status";
			return;
	}
}


// --- Guard leaving a lorry (rooms 5 and 7) ----------------------------------

void
InitGuardLorry(uint16_t ix)
{
	uint8_t count = static_cast<uint8_t>(CountEnemyType(ID_GUARD_SLOW)
		+ CountEnemyType(ID_GUARD_EXIT_LORRY));
	A(ix, ACTOR::NUM_GUARDS) = count;
	A(ix, ACTOR::IDX_SAME_ID) = count;
	A(ix, ACTOR::COLLISION_CFG) = 0;
	InitGuardPath2(ix, count);
	if (Ram(Room) == 7 && A(ix, ACTOR::IDX_SAME_ID) != 1)
		A(ix, ACTOR::IdxGuardSpeed) = 8;	// the second guard walks faster
	InitGuardLorry2(ix);
}


void
GuardLorryLogic(uint16_t ix)
{
	uint8_t status = A(ix, ACTOR::Status);
	if (status == 0) {
		// waits in the lorry, then gets out
		if (--A(ix, ACTOR::LORRY_TIMER) != 0)
			return;
		uint16_t flag = ExitedLorryFlag(ix);
		if (flag != 0)
			Ram(flag) = 1;
		A(ix, ACTOR::LORRY_TIMER) = 8;
		SetActorSprColors2(ix, ID_GUARD_SLOW);
		A(ix, ACTOR::COLLISION_CFG) = 3;
		SetActorSpeed(ix, 0, 0x200);	// walks down out of the lorry
		A(ix, ACTOR::Status)++;
		return;
	}
	if (status == 1) {
		if (--A(ix, ACTOR::LORRY_TIMER) != 0)
			return;
		A(ix, ACTOR::idxPathPoint2) = 0;
		A(ix, ACTOR::PathPointsCnt) = 0xff;
		uint16_t path = static_cast<uint16_t>(A(ix, ACTOR::PointerL)
			| (A(ix, ACTOR::PointerH) << 8));
		GetPathPoint_(ix, Add16(path, 1));
		A(ix, ACTOR::Status)++;
		return;
	}
	if (status == 2) {
		// patrols; after the last point of the path, back into the lorry
		GuardLogic(ix);
		if (Ram(AlertMode) != 0) {
			TransformAlertGuard(ix);
			return;
		}
		uint8_t point = A(ix, ACTOR::idxPathPoint);
		if (point == A(ix, ACTOR::idxPathPoint2))
			return;
		A(ix, ACTOR::idxPathPoint2) = point;
		A(ix, ACTOR::PathPointsCnt)++;
		if (A(ix, ACTOR::PathPointsCnt) < A(ix, ACTOR::Cnt_NumPathPoints))
			return;
		A(ix, ACTOR::PathPointsCnt)--;
		if (A(ix, ACTOR::GuardStatus) != 0) {
			A(ix, ACTOR::Moving)++;
			A(ix, ACTOR::GuardStatus) = 0;
			SetDirToPoint(ix);
		}
		SetActorSpeed(ix, 0, 0xfe00);	// walks up into the lorry
		A(ix, ACTOR::LORRY_TIMER) = 8;
		A(ix, ACTOR::SpriteId) = 0;
		A(ix, ACTOR::Status)++;
		return;
	}
	// walks into the lorry
	if (--A(ix, ACTOR::LORRY_TIMER) != 0)
		return;
	uint16_t flag = ExitedLorryFlag(ix);
	if (flag != 0)
		Ram(flag) = 0;
	A(ix, ACTOR::COLLISION_CFG) = 0;
	SetSprColor(ix, 0, 0);	// hidden
	InitGuardLorry2(ix);
	ResetActorStatus(ix);
}
