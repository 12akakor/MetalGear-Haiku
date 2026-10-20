/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/shooter.asm, jetpack.asm, guardswitch.asm,
// powerswitch.asm and guardelevator.asm.
#include "Soldiers.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Text.h"
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



// --- Alert sign over a guard (guardelevator.asm) ---------------------------

// (!?) The original also drops a return address here (see the callers).
void
DrawAlertSign(uint16_t ix, uint8_t sx)
{
	uint8_t y = static_cast<uint8_t>((A(ix, ACTOR::Y) - 0x18) & 0xf8);
	uint8_t x = static_cast<uint8_t>((A(ix, ACTOR::X) + 8) & 0xf8);
	Ram(XY_AlertIcon) = y;
	Ram(static_cast<uint16_t>(XY_AlertIcon + 1)) = x;
	SaveAlertIconBacknd(x, y);
	Draw16x16(sx, 0x80, x, y);
	SetAlertRoom();
}


// --- The guard at the power switch (guardswitch.asm) ----------------------

void
GuardSwTurn(uint16_t ix, uint8_t direction)
{
	A(ix, ACTOR::Direction) = direction;
	AnimateGuard2(ix, direction);
	SetWalkSpeed(ix);
}


// (!?) DrawAlertSign drops the return address of its call, so the rest of
// the original's GuardSwAlarm (wait, look at Snake) never runs: the code
// goes on after the call of GuardSwChkSeeY, or returns from the logic.
void
GuardSwAlarm(uint16_t ix)
{
	SetAlertModeRespawn(ix, 0);
	DrawAlertSign(ix, 0);
}


void
GuardSwChkBox(uint16_t ix)
{
	if (Ram(PlayerAnimation) == 7) {
		// in the box: seen only when it moves
		if ((Ram(PlayerSpeedY) | Ram(static_cast<uint16_t>(PlayerSpeedY + 1))) == 0
			&& (Ram(PlayerSpeedX) | Ram(static_cast<uint16_t>(PlayerSpeedX + 1))) == 0)
			return;
	}
	GuardSwAlarm(ix);
}


void
GuardSwChkSeeY(uint16_t ix)
{
	if ((Ram(PlayerY) & 0x80) == 0)
		return;	// not in line with the guard
	GuardSwChkBox(ix);
}


void
SetStatGoToSwitch(uint16_t ix)
{
	A(ix, ACTOR::Status) = 2;
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::SpeedX) = 0xfd;
	A(ix, ACTOR::Direction) = DIR_LEFT;
	AnimateGuard2(ix, DIR_LEFT);
}


void
GuardSwPatrol(uint16_t ix)
{
	if (Ram(AlertMode) != 0) {
		SetStatGoToSwitch(ix);
		return;
	}
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
	GuardSwChkSeeY(ix);
	uint8_t x = A(ix, ACTOR::X);
	if (x == 0x50) {
		GuardSwTurn(ix, DIR_RIGHT);	// left end of the patrol
		return;
	}
	if (x != 0x98 && x != 0xd0)
		return;
	A(ix, ACTOR::Status) = 1;	// looks north
	A(ix, ACTOR::Moving) = 0;
	A(ix, ACTOR::Wait) = 0x1e;
	A(ix, ACTOR::SpriteId) = 8;
}


void
GuardSwLookNorth(uint16_t ix)
{
	if (Ram(AlertMode) != 0) {
		SetStatGoToSwitch(ix);
		return;
	}
	if (--A(ix, ACTOR::Wait) == 0) {
		A(ix, ACTOR::Status) = 0;
		A(ix, ACTOR::Moving) = 1;
		if (A(ix, ACTOR::X) == 0xd0) {
			GuardSwTurn(ix, DIR_LEFT);
			return;
		}
		AnimateGuard2(ix, A(ix, ACTOR::Direction));
		return;
	}
	if (A(ix, ACTOR::X) == 0xd0) {
		if (Ram(PlayerX) >= 0xc0)
			GuardSwChkBox(ix);	// both at the right
		else
			GuardSwChkSeeY(ix);
		return;
	}
	if (static_cast<uint8_t>(Ram(PlayerX) - A(ix, ACTOR::X) + 8) < 0x11) {
		GuardSwChkBox(ix);
		return;
	}
	GuardSwChkSeeY(ix);
}


void
GuardSwGoToSw(uint16_t ix)
{
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
	if (A(ix, ACTOR::X) >= 0x25)
		return;
	A(ix, ACTOR::Status)++;
	A(ix, ACTOR::Moving) = 0;
	A(ix, ACTOR::Wait) = 0x0a;
	A(ix, ACTOR::SpriteId) = 8;
	SetSoundEntryChk(0x15);	// click
	AddEnemy(ID_POWER_SWITCH, 0x70, 0x24);	// the floor is electrified
}


void
GuardSwWait(uint16_t ix)
{
	if (--A(ix, ACTOR::Wait) != 0)
		return;
	A(ix, ACTOR::Status)++;
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::Wait) = 0x10;
	A(ix, ACTOR::Direction) = DIR_RIGHT;
	// (!?) the original stores the direction but animates with what A
	// holds: the low byte JumpIndex left (its table entry for status 3)
	AnimateGuard2(ix, static_cast<uint8_t>(SYM(GuardSwitchLogic) + 6 + 3 * 2));
	SetWalkSpeed(ix);
}


void
GuardSwRight(uint16_t ix)
{
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
	if (--A(ix, ACTOR::Wait) != 0)
		return;
	A(ix, ACTOR::Status)++;
	A(ix, ACTOR::Moving) = 0;
	A(ix, ACTOR::SpriteId) = 0x0b;	// right
	A(ix, ACTOR::Wait) = 1;
}


void
GuardSwShot(uint16_t ix)
{
	if ((Ram(PlayerY) & 0x80) == 0)
		return;	// Snake is in the upper part
	uint8_t other;
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(GetDirToPlayer(ix, &other) + 7);
	if (--A(ix, ACTOR::Wait) != 0)
		return;
	A(ix, ACTOR::Wait) = 0x10;
	AddEnemyShot2(ix, ID_BULLET);
}


// --- Power switch: the electrified floor glows ----------------------------

void
ChkRevertFade(uint16_t ix, uint8_t bright)
{
	A(ix, POWER_SWITCH::BRIGHT) = bright == 0xff ? 1 : 6;
	A(ix, POWER_SWITCH::BRIGHT_DELTA) = A(ix, POWER_SWITCH::BRIGHT_DELTA) == 1 ? 0xff : 1;
}


// --- Guards of the elevator halls (guardelevator.asm) ---------------------

// Both guards look at Snake; the alert. (!?) The original drops the
// return address of RunEnemyLogic here, see gEnemiesLogicAbort.
void
GuardElevSetAlert(uint16_t ix)
{
	uint8_t sprite = Ram(PlayerX) < A(ix, ELEVATOR_GUARD::X) ? 0x0a : 0x0b;
	uint16_t second = static_cast<uint16_t>(ix + 0x80);
	for (uint16_t guard : {ix, second}) {
		A(guard, ELEVATOR_GUARD::Status) = 3;
		A(guard, ELEVATOR_GUARD::Moving) = 0;
		A(guard, ELEVATOR_GUARD::SpriteId) = sprite;
		A(guard, ELEVATOR_GUARD::AlertDelay) = 0x0f;
	}
	if (Ram(AlertMode) != 0) {
		// the original returns to the address IX here
		if (gUnported == NULL)
			gUnported = "GuardElevSetAlert: already in an alert";
		return;
	}
	SetAlertModeRespawn(second, 0x3c);
	DrawAlertSign(ix, 0x10);	// "!!"
	gEnemiesLogicAbort = true;
}


void
GuardReachElevator(uint16_t ix)
{
	A(ix, ELEVATOR_GUARD::Wait) = 0;
	A(ix, ELEVATOR_GUARD::Status) = 1;
	A(ix, ELEVATOR_GUARD::Moving) = 0;
	A(ix, ELEVATOR_GUARD::LookDirTimer) = 0x1e;
	A(ix, ELEVATOR_GUARD::Direction) = DIR_DOWN;
	A(ix, ELEVATOR_GUARD::SpriteId) = 9;
}


void
GuardElevatorWalk(uint16_t ix)
{
	if (Ram(PlayerY) < 0x3d) {
		GuardElevSetAlert(ix);
		return;
	}
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
	if (A(ix, ELEVATOR_GUARD::X) == A(ix, ELEVATOR_GUARD::DestinationX)) {
		GuardReachElevator(ix);
		return;
	}
	if (A(ix, ELEVATOR_GUARD::DestinationX) == 0x90)
		return;	// the second guard
	if (--A(ix, ELEVATOR_GUARD::Wait) != 0)
		return;
	uint16_t guard = AddEnemy(ID_GUARD_ELEVATOR, 0x30, 0xf2);
	Ram(static_cast<uint16_t>(guard + ELEVATOR_GUARD::DestinationX)) = 0x90;
}


// Looks left, right or down now and then.
void
GuardRandomDir(uint16_t ix)
{
	if (--A(ix, ELEVATOR_GUARD::LookDirTimer) != 0)
		return;
	A(ix, ELEVATOR_GUARD::LookDirTimer) = static_cast<uint8_t>((RefreshR() & 0x0f) + 0x20);
	uint8_t b = (RefreshR() ^ A(ix, ACTOR::ANIM_CNT)) & 1;
	uint8_t direction = A(ix, ELEVATOR_GUARD::Direction);
	uint8_t a = static_cast<uint8_t>(((direction & 1) + b + direction) & 3);
	if (a == 0)
		return;	// not up
	a++;
	A(ix, ELEVATOR_GUARD::Direction) = a;
	GuardLookDirection2(ix, a);
}


void
GuardElevIdle(uint16_t ix)
{
	if (Ram(PlayerY) < 0x3d) {
		GuardElevSetAlert(ix);
		return;
	}
	if (--A(ix, ELEVATOR_GUARD::Wait) != 0) {
		GuardRandomDir(ix);
		return;
	}
	A(ix, ELEVATOR_GUARD::Status) = 2;	// leaves
	A(ix, ELEVATOR_GUARD::Moving) = 1;
	A(ix, ELEVATOR_GUARD::Direction) = DIR_RIGHT;
	AnimateGuard2(ix, DIR_RIGHT);
	SetWalkSpeed(ix);
	if (A(ix, ELEVATOR_GUARD::DestinationX) == 0x50)
		SetTextUnskippable(1);	// Relieve
}


void
GuardElevLeave(uint16_t ix)
{
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
	if (Ram(PlayerY) >= 0x3d)
		return;	// not in the same corridor
	if (static_cast<uint8_t>(Ram(PlayerX) + 0x10) >= A(ix, ELEVATOR_GUARD::X))
		GuardElevSetAlert(ix);
}


// The first guard turns into an alert guard, the second one leaves. (!?)
// The original leaves IX on the second guard.
void
GuardElevAlert(uint16_t ix)
{
	if (--A(ix, ELEVATOR_GUARD::AlertDelay) != 0)
		return;
	TransformAlertGuard(EnemyListEntry1);
	uint16_t second = EnemyListEntry2;
	A(second, ELEVATOR_GUARD::Status) = 4;
	A(second, ELEVATOR_GUARD::Moving) = 1;
	A(second, ELEVATOR_GUARD::Direction) = DIR_RIGHT;
	A(second, ELEVATOR_GUARD::SpeedX) = DIR_RIGHT;
	AnimateGuard2(second, DIR_RIGHT);
	gEnemyIX = second;
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


// --- The guard at the power switch (room 16) ----------------------------

void
InitGuardSwitch(uint16_t ix)
{
	if (Ram(PreviousRoom) == 139) {
		DismissActor0(ix);	// Snake comes from the goggles room
		return;
	}
	A(ix, ACTOR::Moving)++;
	A(ix, ACTOR::Direction) = DIR_RIGHT;
	AnimateGuard2(ix, DIR_RIGHT);
	ResetActorSpeed(ix);
	SetWalkSpeed(ix);
}


void
GuardSwitchLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:
			GuardSwPatrol(ix);
			return;
		case 1:
			GuardSwLookNorth(ix);
			return;
		case 2:
			GuardSwGoToSw(ix);
			return;
		case 3:
			GuardSwWait(ix);
			return;
		case 4:
			GuardSwRight(ix);
			return;
		case 5:
			GuardSwShot(ix);
			return;
		case 6:
			if (--A(ix, ACTOR::Wait) != 0)
				return;
			SetStatGoToSwitch(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "GuardSwitchLogic: bad status";
			return;
	}
}


void
InitPowerSwitch(uint16_t ix)
{
	A(ix, POWER_SWITCH::BRIGHT) = 4;
	A(ix, POWER_SWITCH::BRIGHT_DELTA) = 1;	// fades in
	Ram(PowerSwitchOn) = 1;
}


void
PowerSwitchLogic(uint16_t ix)
{
	if ((A(ix, POWER_SWITCH::ANIM_CNT) & 3) != 0)
		return;
	uint8_t bright = static_cast<uint8_t>(A(ix, POWER_SWITCH::BRIGHT)
		+ A(ix, POWER_SWITCH::BRIGHT_DELTA));
	A(ix, POWER_SWITCH::BRIGHT) = bright;
	if (bright >= 7)
		ChkRevertFade(ix, bright);
	bright = A(ix, POWER_SWITCH::BRIGHT);
	uint8_t room = Ram(Room);
	uint8_t color = room == 116 || room == 40 ? 5 : 9;
	SetPaletteColor(color, static_cast<uint8_t>((bright << 4) | (bright >> 4) | bright), bright);
}


// --- Guards of the elevator halls ------------------------------------------

void
InitGuardElevat(uint16_t ix)
{
	if (A(ix, ELEVATOR_GUARD::X) == 0xf2) {
		// the relief comes in from the right
		A(ix, ELEVATOR_GUARD::Status) = 0;
		A(ix, ELEVATOR_GUARD::Moving) = 1;
		A(ix, ELEVATOR_GUARD::Direction) = DIR_LEFT;
		AnimateGuard2(ix, DIR_LEFT);
		ResetActorSpeed(ix);
		SetWalkSpeed(ix);
		if (A(ix, ELEVATOR_GUARD::DestinationX) == 0x90)
			return;
		A(ix, ELEVATOR_GUARD::Wait) = 0x40;	// then the second one
		A(ix, ELEVATOR_GUARD::DestinationX) = 0x50;
		return;
	}
	if (Ram(PreviousRoom) == 0xf0) {
		DismissActor0(ix);	// not there when Snake comes from the elevator
		return;
	}
	A(ix, ELEVATOR_GUARD::Wait) = 0;
	A(ix, ELEVATOR_GUARD::Status) = 1;
	A(ix, ELEVATOR_GUARD::Moving) = 0;
	A(ix, ELEVATOR_GUARD::DestinationX) = A(ix, ELEVATOR_GUARD::X);
	A(ix, ELEVATOR_GUARD::LookDirTimer) = 0x1e;
	A(ix, ELEVATOR_GUARD::Direction) = DIR_DOWN;
	A(ix, ELEVATOR_GUARD::SpriteId) = 9;
}


void
GuardElevator(uint16_t ix)
{
	uint8_t status = A(ix, ELEVATOR_GUARD::Status);
	if (status < 3 && Ram(AlertMode) != 0) {
		TransformAlertGuard(ix);
		return;
	}
	switch (status) {
		case 0:
			GuardElevatorWalk(ix);
			return;
		case 1:
			GuardElevIdle(ix);
			return;
		case 2:
			GuardElevLeave(ix);
			return;
		case 3:
			GuardElevAlert(ix);
			return;
		case 4:	// the second guard walks out to the right
			AnimateGuard2(ix, A(ix, ACTOR::Direction));
			return;
		default:
			if (gUnported == NULL)
				gUnported = "GuardElevator: bad status";
			return;
	}
}
