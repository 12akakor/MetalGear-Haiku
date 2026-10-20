/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/shooter.asm and jetpack.asm.
#include "Soldiers.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "Guards.h"
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


// --- Shooters: guards that step out, shoot and hide again -----------------

void
SetRandomWait(uint16_t ix)
{
	A(ix, SHOOTER::Wait) = static_cast<uint8_t>(((RefreshR() ^ A(ix, SHOOTER::ANIM_CNT)) & 0x3f)
		+ 1);
}


uint8_t
LookUpOrDown(uint16_t ix)
{
	return (A(ix, SHOOTER::Y) & 0x80) != 0 ? DIR_UP : DIR_DOWN;
}


void
SetShooterDir(uint16_t ix)
{
	if ((A(ix, SHOOTER::Y) & 0x80) == 0)
		A(ix, SHOOTER::Direction) = DIR_DOWN;
	GuardLookDirection(ix);
	ResetActorSpeed(ix);
	A(ix, SHOOTER::StartX) = A(ix, SHOOTER::X);
	SetRandomWait(ix);
}


void
ShooterWait(uint16_t ix)
{
	if (--A(ix, SHOOTER::Wait) != 0)
		return;
	if (--A(ix, SHOOTER::TransformTimer) == 0) {
		TransformAlertGuard2(ix, ID_GUARD_ALERT);	// after three times
		return;
	}
	A(ix, SHOOTER::Status)++;
	A(ix, SHOOTER::Moving) = 1;
	A(ix, SHOOTER::Wait) = 0x0a;
	uint8_t direction = A(ix, SHOOTER::NextDirection);
	if (direction == 0)
		direction = static_cast<uint8_t>(((RefreshR() ^ A(ix, SHOOTER::ANIM_CNT)) & 1) + 3);
	A(ix, SHOOTER::Direction) = direction;
	AnimateGuard2(ix, direction);
	SetWalkSpeedFast(ix);
}


void
ShooterWalk(uint16_t ix)
{
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
	if (--A(ix, SHOOTER::Wait) != 0)
		return;
	A(ix, SHOOTER::Status)++;
	A(ix, SHOOTER::Moving) = 0;
	SetRandomWait(ix);
	GuardLookDirection2(ix, LookUpOrDown(ix));
}


void
ShooterShot(uint16_t ix)
{
	if (--A(ix, SHOOTER::Wait) == 0) {
		// turns back
		A(ix, SHOOTER::Status)++;
		A(ix, SHOOTER::SpeedX) = static_cast<uint8_t>(-A(ix, SHOOTER::SpeedX));
		A(ix, SHOOTER::Moving) = 1;
		uint8_t direction = GetOppositeDir(A(ix, SHOOTER::Direction));
		A(ix, SHOOTER::Direction) = direction;
		AnimateGuard2(ix, direction);
		return;
	}
	if ((A(ix, SHOOTER::ANIM_CNT) & 0x0f) != 0)
		return;
	AddEnemyShot2(ix, ID_BULLET_VERT);
}


void
ShooterHide(uint16_t ix)
{
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
	if (A(ix, SHOOTER::X) != A(ix, SHOOTER::StartX))
		return;
	A(ix, SHOOTER::Status) = 0;
	A(ix, SHOOTER::Moving) = 0;
	GuardLookDirection2(ix, LookUpOrDown(ix));
	SetRandomWait(ix);
}


// --- Jetpack soldiers ---------------------------------------------------------

void
SetJetpackFlySpr2(uint16_t ix, uint8_t direction)
{
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(direction + 0x58);
}


void
SetJetpackFlySpr(uint16_t ix)
{
	SetJetpackFlySpr2(ix, A(ix, ACTOR::Direction));
}


void
JetPackMove(uint16_t ix)
{
	bool late = (A(ix, ACTOR::ANIM_CNT) & 0x80) != 0;
	uint16_t speedY = static_cast<uint16_t>(A(ix, ACTOR::speedYdec) | (A(ix, ACTOR::SpeedY) << 8));
	speedY = static_cast<uint16_t>(speedY + (A(ix, ACTOR::Y) < (late ? 0x70 : 0x60) ? 0x18 : -0x18));
	A(ix, ACTOR::speedYdec) = static_cast<uint8_t>(speedY);
	A(ix, ACTOR::SpeedY) = static_cast<uint8_t>(speedY >> 8);
	uint16_t speedX = static_cast<uint16_t>(A(ix, ACTOR::SpeedXdec) | (A(ix, ACTOR::SpeedX) << 8));
	speedX = static_cast<uint16_t>(speedX + (A(ix, ACTOR::X) < (late ? 0x88 : 0x78) ? 0x30 : -0x30));
	A(ix, ACTOR::SpeedXdec) = static_cast<uint8_t>(speedX);
	A(ix, ACTOR::SpeedX) = static_cast<uint8_t>(speedX >> 8);
}


void
JetPackAlert(uint16_t ix)
{
	if (Ram(AlertMode) != 0)
		return;
	SetAlertModeRespawn(ix, 0x1e);
}


void
InitJetpackTakeoff2(uint16_t ix)
{
	ResetActorSpeed(ix);
	A(ix, JETPACK::Moving) = 0;
	A(ix, JETPACK::Wait) = 0x20;	// take off height
	A(ix, JETPACK::NextDirection) = 0;
	A(ix, JETPACK::TakeoffSpeed) = 0xff;
	A(ix, JETPACK::COLLISION_CFG) = 2;	// only the player's shots
	A(ix, JETPACK::Ydec_) = A(ix, JETPACK::Ydec);
	A(ix, JETPACK::Y_) = A(ix, JETPACK::Y);
	A(ix, JETPACK::Xdec_) = A(ix, JETPACK::Xdec);
	A(ix, JETPACK::X_) = A(ix, JETPACK::X);
	uint8_t id = A(ix, JETPACK::ID);
	A(ix, JETPACK::ID) = ID_JETPACK_TAKEOFF;
	// the shadow is updated, except for the one from the switch
	A(ix, JETPACK::NumSprites) = id == ID_JETPACK_SWITCH ? 4 : 6;
}

}	// namespace


// --- Shooters (rooms 206, 88, 90 and 91) -------------------------------------

void
InitShooter(uint16_t ix)
{
	A(ix, SHOOTER::Direction) = 1;	// up
	A(ix, SHOOTER::TransformTimer) = 3;
	uint8_t room = Ram(Room);
	if (room == 206) {
		A(ix, SHOOTER::NextDirection) = 0;
		if ((Ram(PlayerY) & 0x80) != 0)
			A(ix, SHOOTER::Y) = 0x38;	// north
		SetShooterDir(ix);
		return;
	}
	Ram(AlertMode) = 1;	// the roof of building 2: always an alert
	SetRam16(AlertRespawnTimer, 0x0a80);
	if (room == 90) {
		A(ix, SHOOTER::NextDirection) = 0;
		if ((Ram(PlayerY) & 0x80) == 0)
			A(ix, SHOOTER::Y) = 0x90;
		SetShooterDir(ix);
		return;
	}
	uint8_t direction;
	if (room == 91) {
		// the middle one of four goes right, the others left
		direction = A(ix, SHOOTER::X) == 0x78 ? DIR_RIGHT : DIR_LEFT;
	} else {
		if ((Ram(PlayerY) & 0x80) == 0)
			A(ix, SHOOTER::Y) = 0xa8;	// south
		direction = A(ix, SHOOTER::X) == 0x98 ? DIR_LEFT : DIR_RIGHT;
	}
	A(ix, SHOOTER::NextDirection) = direction;
	SetShooterDir(ix);
}


void
ShooterLogic(uint16_t ix)
{
	if (static_cast<uint8_t>(Ram(PlayerY) - A(ix, SHOOTER::Y) + 0x10) < 0x21) {
		TransformAlertGuard2(ix, ID_GUARD_ALERT);	// Snake is too close
		return;
	}
	switch (A(ix, SHOOTER::Status)) {
		case 0:
			ShooterWait(ix);
			return;
		case 1:
			ShooterWalk(ix);
			return;
		case 2:
			ShooterShot(ix);
			return;
		case 3:
			ShooterHide(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "ShooterLogic: bad status";
			return;
	}
}


// --- Jetpack soldiers ---------------------------------------------------------

void
InitJetpackTakeoff(uint16_t ix)
{
	uint8_t other;
	uint8_t direction = GetDirToPlayer(ix, &other);
	A(ix, JETPACK::Direction) = direction;
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(direction + 0x54);	// on the floor
	InitJetpackTakeoff2(ix);
}


void
JetpacTakeoff(uint16_t ix)
{
	SetJetpackFlySpr(ix);
	if (--A(ix, JETPACK::Wait) == 0) {
		A(ix, JETPACK::ID) = ID_JETPACK;
		A(ix, JETPACK::Moving) = 1;
		A(ix, JETPACK::Wait) = 0x2d;
		A(ix, JETPACK::NumSprites) = 6;
		JetPackAlert(ix);
		return;
	}
	A(ix, JETPACK::Y) = static_cast<uint8_t>(A(ix, JETPACK::TakeoffSpeed) + A(ix, JETPACK::Y));
	A(ix, JETPACK::NumSprites) = 4;
}


// The soldier at the power switch (not there when Snake comes from the
// mine detector room).
void
InitJetpackSwitch(uint16_t ix)
{
	if (Ram(PreviousRoom) == 160) {
		DismissActor0(ix);
		return;
	}
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::COLLISION_CFG) = 2;
	A(ix, ACTOR::Direction) = DIR_UP;
	SetJetpackFlySpr2(ix, DIR_UP);
	ResetActorSpeed(ix);
}


void
JetpackSwitchLogic(uint16_t ix)
{
	if ((A(ix, ACTOR::Status) & 1) == 0) {
		A(ix, ACTOR::NumSprites) = 4;
		A(ix, ACTOR::Status) = 1;
		A(ix, ACTOR::SpeedY) = 2;
		SetJetpackFlySpr(ix);
		if (Ram(AlertMode) != 0)
			return;
		SetAlertModeRespawn(ix, 0x5a);
		return;
	}
	if (A(ix, ACTOR::Y) < 0x86)
		return;	// not at the switch yet
	SetSoundEntryChk(0x15);	// click
	AddEnemy(ID_POWER_SWITCH, 0x70, 0x44);
	uint8_t other;
	uint8_t direction = GetDirToPlayer(ix, &other);
	A(ix, ACTOR::Direction) = direction;
	SetJetpackFlySpr2(ix, direction);
	InitJetpackTakeoff2(ix);
}


void
InitJetpack(uint16_t ix)
{
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::COLLISION_CFG) = 2;
	A(ix, ACTOR::Direction) = DIR_LEFT;
	SetJetpackFlySpr2(ix, DIR_LEFT);
	ResetActorSpeed(ix);
	JetPackMove(ix);
	A(ix, ACTOR::Wait) = 0x2d;
	SetRespawnTime();
	JetPackAlert(ix);
}


void
JetpackLogic(uint16_t ix)
{
	uint8_t other;
	uint8_t direction = GetDirToPlayer(ix, &other);
	A(ix, ACTOR::Direction) = direction;
	SetJetpackFlySpr2(ix, direction);
	JetPackMove(ix);
	if (--A(ix, ACTOR::Wait) != 0)
		return;
	A(ix, ACTOR::Wait) = static_cast<uint8_t>(((RefreshR() ^ A(ix, ACTOR::ANIM_CNT)) & 0x0f) + 0x1e);
	AddEnemyShot2(ix, ID_BULLET);
}
