/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/tank.asm and arnold.asm.
#include "Bosses.h"

#include "Actors.h"
#include "Common.h"
#include "Dogs.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Guards.h"
#include "Player.h"
#include "Random.h"
#include "RoomActors.h"
#include "Rom.h"
#include "Structures.h"


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


// --- Tank -------------------------------------------------------------------

void
TankMachineGun(uint16_t ix)
{
	uint8_t y = static_cast<uint8_t>(A(ix, TANK::Y) - 0x0e);
	uint8_t offset = (A(ix, TANK::MachineGunLeftRight) & 0x80) != 0 ? 0xf0 : 16;
	uint16_t hl = AddEnemy(ID_TANK_BULLET, y, static_cast<uint8_t>(offset + A(ix, TANK::X)));
	// the spread of the bullets: 0, 1, 2, 3, 4, 3, 2, 1
	uint8_t shot = static_cast<uint8_t>((A(ix, TANK_SHOT::NumberShots) + 1) & 7);
	A(ix, TANK::NumberShots) = shot;
	if (shot >= 5)
		shot = static_cast<uint8_t>(8 - shot);
	Ram(Add16(hl, TANK_SHOT::SpeedXUnsigned)) = shot;
}


void
TankIdle(uint16_t ix)
{
	if (--A(ix, TANK::StopTime) != 0)
		return;
	A(ix, TANK::Status) = A(ix, TANK::StatusCopy);
	A(ix, TANK::Moving) = 1;
}


void
TankStatusLogic(uint16_t ix)
{
	switch (A(ix, TANK::Status)) {
		case 0:	// up and down
		{
			if (--A(ix, TANK::MovingTime) != 0)
				return;
			A(ix, TANK::MovingTime) = 0x32;
			A(ix, TANK::speedYdec) = 0x80;
			uint8_t speed = 0;
			if ((A(ix, TANK::SpeedY) & 0x80) == 0) {
				speed = 0xff;
				if (GetRandom3(ix) == 0) {
					A(ix, TANK::Status) = 1;
					A(ix, TANK::MovingTime) = 0x9a;
				}
			}
			A(ix, TANK::SpeedY) = speed;
			return;
		}
		case 1:	// a long way up, then down again
			if (--A(ix, TANK::MovingTime) != 0)
				return;
			A(ix, TANK::MovingTime) = 0x9a;
			A(ix, TANK::Status) = 0;
			A(ix, TANK::speedYdec) = 0x80;
			A(ix, TANK::SpeedY) = 0;
			return;
		case 2:
			TankIdle(ix);
			return;
		case 3:	// (!?) not used
			Ram(BossTank_KO) = 0;
			return;
		default:
			if (gUnported == NULL)
				gUnported = "TankLogic: bad status";
			return;
	}
}


void
AnimateTank(uint16_t ix)
{
	if (A(ix, TANK::Status) == 2) {
		TankIdle(ix);
		return;
	}
	A(ix, TANK::SpriteId) = (A(ix, TANK::ANIM_CNT) & 4) != 0 ? 0x17 : 0x16;
	if ((A(ix, TANK::ANIM_CNT) & 0x7f) == 0) {
		A(ix, TANK::Moving) = 0;	// stops for a while
		A(ix, TANK::StatusCopy) = A(ix, TANK::Status);
		A(ix, TANK::Status) = 2;
		A(ix, TANK::StopTime) = 0x28;
	}
	TankStatusLogic(ix);
}


// --- Arnold -----------------------------------------------------------------

void
SetArnoldSpr(uint16_t ix, uint8_t sprite)
{
	A(ix, ARNOLD::SpriteId) = static_cast<uint8_t>(sprite + 0x7c);
}


void
SetArnoldWalkSpr2(uint16_t ix, uint8_t direction, uint8_t mask)
{
	uint8_t sprite = static_cast<uint8_t>(direction * 2);
	if ((A(ix, ARNOLD::ANIM_CNT) & mask) != 0)
		sprite++;
	SetArnoldSpr(ix, sprite);
}


void
SetArnoldWalkSpr(uint16_t ix, uint8_t mask)
{
	SetArnoldWalkSpr2(ix, A(ix, ARNOLD::Direction), mask);
}


void
SetArnoldRndWait(uint16_t ix)
{
	uint8_t a = static_cast<uint8_t>((RefreshR() ^ A(ix, ACTOR::ANIM_CNT)) >> 3);
	A(ix, ARNOLD::Wait) = static_cast<uint8_t>((a >> 1) + a + 1);
}


bool
ArnoldSeesPlayer(uint16_t ix)
{
	return static_cast<uint8_t>(Ram(PlayerY) - A(ix, ARNOLD::Y) + 0x10) < 0x21;
}


void
ArnoldSeePlayer(uint16_t ix)
{
	A(ix, ARNOLD::Status) = 1;	// runs at Snake
	SetArnoldRndWait(ix);
	uint8_t direction = Ram(PlayerX) < A(ix, ARNOLD::X) ? DIR_LEFT : DIR_RIGHT;
	A(ix, ARNOLD::Direction) = direction;
	SetArnoldWalkSpr(ix, 4);
	SetChaseSpeed(ix);
}


void
ArnoldReturn2(uint16_t ix)
{
	SetArnoldSpr(ix, static_cast<uint8_t>(A(ix, ARNOLD::Direction) * 2));
	A(ix, ARNOLD::Status) = 0;
	A(ix, ARNOLD::SpeedX) = 0;
	SetArnoldRndWait(ix);
}


void
ArnoldWatch(uint16_t ix)
{
	if (ArnoldSeesPlayer(ix)) {
		ArnoldSeePlayer(ix);
		return;
	}
	if (--A(ix, ARNOLD::Wait) != 0)
		return;
	SetArnoldRndWait(ix);
	A(ix, ARNOLD::SpriteId) = static_cast<uint8_t>(((RefreshR() ^ A(ix, ARNOLD::ANIM_CNT)) & 1) * 2
		+ 0x82);	// looks left or right
}


void
ArnoldTowardsPlayer(uint16_t ix)
{
	if (!ArnoldSeesPlayer(ix)) {
		// walks back to the middle of the room
		A(ix, ARNOLD::Status) = 2;
		uint8_t x = A(ix, ARNOLD::X);
		if (x == 0x80) {
			ArnoldReturn2(ix);
			return;
		}
		uint8_t direction = x < 0x80 ? DIR_RIGHT : DIR_LEFT;
		A(ix, ARNOLD::Direction) = direction;
		SetArnoldWalkSpr2(ix, direction, DIR_RIGHT);
		SetWalkSpeed(ix);
		return;
	}
	SetArnoldWalkSpr(ix, 4);
	if (ChkTileCollision(ix, 0, A(ix, ARNOLD::Direction))) {
		ArnoldSeePlayer(ix);
		return;
	}
	if (--A(ix, ARNOLD::Wait) != 0)
		return;
	ArnoldSeePlayer(ix);
}


void
ArnoldReturn(uint16_t ix)
{
	if (ArnoldSeesPlayer(ix)) {
		ArnoldSeePlayer(ix);
		return;
	}
	SetArnoldWalkSpr(ix, 8);
	if (A(ix, ARNOLD::X) != 0x80)
		return;
	ArnoldReturn2(ix);
}


// Pushed back by a shot.
void
ArnoldBounceBack3(uint16_t ix)
{
	uint8_t direction = (A(ix, ARNOLD::SpeedX) & 0x80) != 0 ? DIR_LEFT : DIR_RIGHT;
	uint8_t speed = 0;	// against a wall: stops, not to leave the room
	if (!ChkTileCollision(ix, 0, direction)) {
		if (--A(ix, ARNOLD::PushBackCnt) != 0)
			return;
		speed = A(ix, ARNOLD::SpeedXCopy);
	}
	A(ix, ARNOLD::SpeedX) = speed;
	A(ix, ARNOLD::Status) = A(ix, ARNOLD::OldStatus);
	A(ix, ARNOLD::COLLISION_CFG) = 3;
	SetArnoldWalkSpr(ix, 4);
}


void
ArnoldBounceBack(uint16_t ix, uint8_t weapon)
{
	A(ix, ARNOLD::PushBackCnt) = static_cast<uint8_t>(weapon + 3);
	A(ix, ARNOLD::SpeedXCopy) = A(ix, ARNOLD::SpeedX);
	// (!?) by where Snake is, not where the shot comes from
	A(ix, ARNOLD::SpeedX) = Ram(PlayerX) < A(ix, ARNOLD::X) ? 2 : 0xfe;
	A(ix, ARNOLD::OldStatus) = A(ix, ARNOLD::Status);
	A(ix, ARNOLD::Status) = 3;
	A(ix, ARNOLD::COLLISION_CFG) = 1;
	A(ix, ARNOLD::TOUCH_INFO) = 0;
	ArnoldBounceBack3(ix);
}

}	// namespace


void
InitTank(uint16_t ix)
{
	if (Ram(BossTank_KO) != 0) {
		DismissActor(ix);
		return;
	}
	A(ix, TANK::Moving)++;
	A(ix, TANK::SpriteId) = 0x16;
	A(ix, TANK::speedYdec) = 0x80;
	A(ix, TANK::SpeedY) = 0;
	A(ix, TANK::MachineGunTimer) = 0x1e;
	A(ix, TANK::MachineGunOn) = 0;
	A(ix, TANK::MachineGunLeftRight) = 1;
	A(ix, TANK::NumberShots) = 0;
	A(ix, TANK::CannonlDelay) = 0x3c;
	A(ix, TANK::MovingTime) = 0x9a;
	A(ix, TANK::KO_POINTER_L) = static_cast<uint8_t>(BossTank_KO);
	A(ix, TANK::KO_POINTER_H) = static_cast<uint8_t>(BossTank_KO >> 8);
	SetBossMusic();
}


// The cannon fires when Snake is in front; the machine guns fire in
// bursts to one side, then the other.
void
TankLogic(uint16_t ix)
{
	if (--A(ix, TANK::CannonlDelay) == 0) {
		A(ix, TANK::CannonlDelay)++;	// ready
		if (static_cast<uint8_t>(Ram(PlayerX) - A(ix, TANK::X) + 4) < 9) {
			AddEnemy(ID_TANK_SHELL_BOSS, static_cast<uint8_t>(A(ix, TANK::Y) + 4),
				A(ix, TANK::X));
			A(ix, TANK::CannonlDelay) = 0x1e;
		}
	}

	if ((A(ix, TANK::MachineGunOn) & 1) == 0) {
		if (--A(ix, TANK::MachineGunTimer) != 0) {
			AnimateTank(ix);
			return;
		}
		A(ix, TANK::MachineGunTimer) = 0x2d;
		A(ix, TANK::MachineGunOn) = 1;
	}
	if (--A(ix, TANK::MachineGunTimer) != 0) {
		if ((A(ix, TANK::ANIM_CNT) & 7) == 0)
			TankMachineGun(ix);
		AnimateTank(ix);
		return;
	}
	A(ix, TANK::MachineGunTimer) = 0x1e;
	A(ix, TANK::MachineGunOn) = 0;
	A(ix, TANK::MachineGunLeftRight) = static_cast<uint8_t>(-A(ix, TANK::MachineGunLeftRight));
	AnimateTank(ix);
}


void
InitArnold(uint16_t ix)
{
	if (Ram(Card7Taken) != 0) {
		DismissActor(ix);
		return;
	}
	A(ix, ARNOLD::Moving) = 1;
	A(ix, ARNOLD::COLLISION_CFG) = 3;
	uint8_t direction = static_cast<uint8_t>(((A(ix, ARNOLD::Y) >> 6) & 1) + DIR_LEFT);
	A(ix, ARNOLD::Direction) = direction;
	SetArnoldSpr(ix, static_cast<uint8_t>(direction * 2));
	ResetActorSpeed(ix);
	SetArnoldRndWait(ix);
}


void
ArnoldLogic(uint16_t ix)
{
	uint8_t weapon = A(ix, ARNOLD::TOUCH_INFO) & 0x0f;
	if (weapon != 0) {
		ArnoldBounceBack(ix, weapon);
		return;
	}
	switch (A(ix, ARNOLD::Status)) {
		case 0:
			ArnoldWatch(ix);
			return;
		case 1:
			ArnoldTowardsPlayer(ix);
			return;
		case 2:
			ArnoldReturn(ix);
			return;
		case 3:
			ArnoldBounceBack3(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "ArnoldLogic: bad status";
			return;
	}
}
