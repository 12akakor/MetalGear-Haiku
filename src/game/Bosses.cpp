/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/tank.asm, arnold.asm, scorpion.asm,
// machinegunkid.asm, shotgunner.asm, firetropper.asm, bigboss.asm and
// cowardduck.asm.
#include "Bosses.h"

#include "Actors.h"
#include "Common.h"
#include "Dogs.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Guards.h"
#include "Text.h"
#include "Player.h"
#include "Random.h"
#include "RoomActors.h"
#include "Rom.h"
#include "Sprites.h"
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


inline uint16_t
IncLow(uint16_t address, int n = 1)
{
	return static_cast<uint16_t>((address & 0xff00) | ((address + n) & 0xff));
}


// Number of passes of a DJNZ loop started with this count.
inline int
Loops(uint8_t count)
{
	return count == 0 ? 256 : count;
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


// --- Scorpions in the desert (scorpion.asm) ------------------------------

namespace {

void
SetScorpionSprId(uint16_t ix, uint8_t direction)
{
	uint8_t sprite = static_cast<uint8_t>(direction * 2 + 0x60);
	if ((A(ix, ACTOR::ANIM_CNT) & 4) != 0)
		sprite++;
	A(ix, ACTOR::SpriteId) = sprite;
}


// True (carry) when the scorpion is near an edge of the room.
bool
ChkScorpionLimits(uint16_t ix)
{
	if (static_cast<uint8_t>(A(ix, ACTOR::Y) + 0x50) < 0x61)
		return true;
	return static_cast<uint8_t>(A(ix, ACTOR::X) + 0x10) < 0x21;
}


void
ScorpionSetSpeed(uint16_t ix)
{
	uint16_t speeds = GetPointerDE2A(static_cast<uint16_t>(SYM(ScorpionSpeedDat) - 2),
		A(ix, ACTOR::Direction));
	A(ix, ACTOR::SpeedY) = static_cast<uint8_t>(speeds);
	A(ix, ACTOR::SpeedX) = static_cast<uint8_t>(speeds >> 8);
}


void
ScorpionTurn(uint16_t ix)
{
	uint8_t direction = GetOppositeDir(A(ix, ACTOR::Direction));
	A(ix, ACTOR::Direction) = direction;
	SetScorpionSprId(ix, direction);
	ScorpionSetSpeed(ix);
}


void
ScorpionNewDir(uint16_t ix)
{
	A(ix, ACTOR::Wait) = static_cast<uint8_t>(GetRandom3(ix) + 5);
	uint8_t direction = static_cast<uint8_t>(GetRandom3(ix) + 1);
	A(ix, ACTOR::Direction) = direction;
	SetScorpionSprId(ix, direction);
	if (ChkScorpionLimits(ix))
		ScorpionTurn(ix);
	else
		ScorpionSetSpeed(ix);
}

}	// namespace


void
InitScorpion(uint16_t ix)
{
	A(ix, ACTOR::Status) = 2;	// waits
	A(ix, ACTOR::Wait) = 8;
	SetScorpionSprId(ix, static_cast<uint8_t>(((RefreshR() ^ Ram(TickCounter)) & 3) + 1));
}


void
ScorpionLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:	// wanders; near Snake it attacks
			if (GetDistancePlayer(ix) < 0x51) {
				A(ix, ACTOR::Status) = 1;
				A(ix, ACTOR::Wait) = 8;
				uint16_t speedY;
				uint16_t speedX = CalcShot(ix, &speedY);
				SetActorSpeed(ix, speedX, speedY);
				return;
			}
			if (ChkScorpionLimits(ix)) {
				ScorpionTurn(ix);
				return;
			}
			if (--A(ix, ACTOR::Wait) != 0)
				return;
			ScorpionNewDir(ix);
			return;
		case 1:	// attacks for a while
			if (!ChkScorpionLimits(ix)) {
				if (--A(ix, ACTOR::Wait) != 0)
					return;
			}
			A(ix, ACTOR::Status)++;
			A(ix, ACTOR::Wait) = 0x14;
			ResetActorSpeed(ix);
			return;
		case 2:
			if (--A(ix, ACTOR::Wait) != 0)
				return;
			A(ix, ACTOR::Status) = 0;
			A(ix, ACTOR::Moving) = 1;
			ScorpionNewDir(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "ScorpionLogic: bad status";
			return;
	}
}


// --- Machine Gun Kid (machinegunkid.asm) ----------------------------------

namespace {

void
MG_SetWalk(uint16_t ix, uint8_t direction, uint8_t speed, uint8_t status)
{
	A(ix, ACTOR2::Direction) = direction;
	A(ix, ACTOR2::SpeedX) = speed;
	A(ix, ACTOR2::SpriteId) = 0x33;	// walks
	A(ix, ACTOR2::Wait) = 8;
	A(ix, ACTOR2::Status) = status;
}


// Picks a side, or walks towards Snake; then hides.
void
MG_ChoseDir(uint16_t ix)
{
	uint8_t x = A(ix, ACTOR2::X);
	if (x == 0xe0)
		MG_SetWalk(ix, DIR_LEFT, 0xfc, 4);
	else if (x == 0x20)
		MG_SetWalk(ix, DIR_RIGHT, 4, 4);
	else if (Ram(PlayerX) < x)
		MG_SetWalk(ix, DIR_LEFT, 0xfc, 4);
	else
		MG_SetWalk(ix, DIR_RIGHT, 4, 4);
	A(ix, ACTOR2::MOVING) = 1;
}


void
MG_WalkAnim(uint16_t ix)
{
	A(ix, ACTOR2::SpriteId) = (A(ix, ACTOR2::ANIM_CNT) & 4) != 0 ? 0x34 : 0x33;
}


void
MG_ThinkMovement(uint16_t ix)
{
	if (--A(ix, ACTOR::Wait) != 0) {
		// Snake behind another wall: walks towards him
		if (static_cast<uint8_t>(Ram(PlayerX) - A(ix, ACTOR2::X) + 8) < 0x11)
			return;
		if (Ram(PlayerX) < A(ix, ACTOR2::X))
			MG_SetWalk(ix, DIR_LEFT, 0xfc, 2);
		else
			MG_SetWalk(ix, DIR_RIGHT, 4, 2);
		return;
	}
	if (((RefreshR() ^ A(ix, ACTOR2::ANIM_CNT)) & 1) != 0)
		MG_SetWalk(ix, DIR_LEFT, 0xfc, 2);
	else
		MG_SetWalk(ix, DIR_RIGHT, 4, 2);
}


// Bursts: the bullets spread 0, 1, 2, 3, 4, 3, 2, 1.
void
MG_Shooting(uint16_t ix)
{
	if (static_cast<uint8_t>(Ram(PlayerX) - A(ix, ACTOR2::X) + 0x30) >= 0x61) {
		MG_ChoseDir(ix);
		return;
	}
	if (--A(ix, ACTOR2::Wait) == 0) {
		MG_ChoseDir(ix);
		return;
	}
	A(ix, ACTOR2::SpriteId) = 0x31;
	if ((A(ix, ACTOR2::Wait) & 3) != 0)
		return;
	A(ix, ACTOR2::SpriteId) = 0x32;	// recoil
	uint16_t shot = AddEnemyShot2(ix, ID_SHOT_M_GUN_KID);
	uint8_t burst = static_cast<uint8_t>((A(ix, ACTOR2::Timer) + 1) & 7);
	A(ix, ACTOR2::Timer) = burst;
	if (burst >= 5)
		burst = static_cast<uint8_t>(8 - burst);
	Ram(static_cast<uint16_t>(shot + M_GUN_KID_SHOT::BurstDir)) = burst;
}

}	// namespace


void
InitMachGunKid(uint16_t ix)
{
	if ((Ram(MachGunStatus) & 1) != 0) {
		DismissActor(ix);	// dead
		return;
	}
	A(ix, ACTOR2::SpriteId) = 0x31;
	A(ix, ACTOR2::TIMER) = 2;
}


void
MachGunKidLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:	// "I'm Machine Gun Kid"
			if (--A(ix, ACTOR2::TIMER) != 0)
				return;
			if ((Ram(MachGunStatus) & 2) == 0) {
				Ram(MachGunStatus) |= 2;
				SetTextUnskippable(79);
			}
			A(ix, ACTOR2::MOVING) = 1;
			SetBossMusic();
			MG_ChoseDir(ix);
			return;
		case 1:
			MG_ThinkMovement(ix);
			return;
		case 2:	// walks out, then shoots
			MG_WalkAnim(ix);
			if (--A(ix, ACTOR2::Wait) != 0)
				return;
			A(ix, ACTOR2::SpriteId) = 0x31;
			A(ix, ACTOR2::Wait) = 0x28;
			A(ix, ACTOR2::Status) = 3;
			A(ix, ACTOR2::SpeedX) = 0;
			return;
		case 3:
			MG_Shooting(ix);
			return;
		case 4:	// hides behind a wall
			MG_WalkAnim(ix);
			if (--A(ix, ACTOR2::Wait) != 0)
				return;
			A(ix, ACTOR2::Status) = 1;
			A(ix, ACTOR2::SpeedX) = 0;
			A(ix, ACTOR2::Wait) = 0x2d;
			A(ix, ACTOR2::SpriteId) = 0x31;
			return;
		default:
			if (gUnported == NULL)
				gUnported = "MachGunKidLogic: bad status";
			return;
	}
}


void
InitMGunKidShot(uint16_t ix)
{
	A(ix, M_GUN_KID_SHOT::Moving) = 1;
	A(ix, M_GUN_KID_SHOT::COLLISION_CFG) = 1;
	A(ix, M_GUN_KID_SHOT::SpriteId) = 0x72;
}


void
MGunKidShotLogic(uint16_t ix)
{
	if ((A(ix, M_GUN_KID_SHOT::Configured) & 1) != 0)
		return;
	A(ix, M_GUN_KID_SHOT::Configured) = 1;
	// three RRA from a clear carry, then RL D: DE = direction * 64
	uint8_t a = A(ix, M_GUN_KID_SHOT::BurstDir);
	uint8_t e = static_cast<uint8_t>(((a & 3) << 6) | (a >> 3));
	uint8_t d = (a >> 2) & 1;
	uint16_t speed = static_cast<uint16_t>(0xff80 + ((d << 8) | e));
	A(ix, M_GUN_KID_SHOT::SpeedXdec) = static_cast<uint8_t>(speed);
	A(ix, M_GUN_KID_SHOT::SpeedX) = static_cast<uint8_t>(speed >> 8);
	A(ix, M_GUN_KID_SHOT::SpeedY) = 5;
	SetSoundEntryChk(5);
}


// --- Shoot Gunner (shotgunner.asm) -----------------------------------------

namespace {

void
SetSGunnerRollSpr(uint16_t ix)
{
	uint8_t frame = static_cast<uint8_t>((A(ix, ACTOR::ANIM_CNT) & 6) >> 1);
	A(ix, ACTOR::SpriteId) = Mem(static_cast<uint16_t>(SYM(SGunnerRollSpr) + frame));
}


void
SetSGunnerSpeed(uint16_t ix)
{
	if (Ram(PlayerX) >= A(ix, SHOT_GUNNER::X)) {
		A(ix, SHOT_GUNNER::SpeedX) = 4;
		A(ix, SHOT_GUNNER::Direction) = DIR_RIGHT;
	} else {
		A(ix, SHOT_GUNNER::SpeedX) = 0xfc;
		A(ix, SHOT_GUNNER::Direction) = DIR_LEFT;
	}
}


void
ShotGunnerStop(uint16_t ix)
{
	A(ix, ACTOR::SpeedX) = 0;
	A(ix, ACTOR::SpriteId) = 0x5d;	// stands
	A(ix, ACTOR::Wait) = 0x2d;
	A(ix, ACTOR::Status) = 2;	// shoots
	A(ix, ACTOR::COLLISION_CFG) = 3;
}

}	// namespace


void
InitShotGunner(uint16_t ix)
{
	if ((Ram(ShotGunnerStat) & 1) != 0) {
		DismissActor(ix);	// dead
		return;
	}
	A(ix, SHOT_GUNNER::SpriteId) = 0x5d;
	A(ix, SHOT_GUNNER::IntroDelay) = 2;
	A(ix, SHOT_GUNNER::Moving)++;
	SetSGunnerSpeed(ix);
	A(ix, SHOT_GUNNER::Wait) = 0x0b;
	A(ix, SHOT_GUNNER::COLLISION_CFG) = 0;
}


void
ShotGunnerLogic(uint16_t ix)
{
	switch (A(ix, SHOT_GUNNER::Status)) {
		case 0:	// "I'm Shoot Gunner"
			if (--A(ix, SHOT_GUNNER::IntroDelay) != 0)
				return;
			if ((Ram(ShotGunnerStat) & 2) == 0) {
				Ram(ShotGunnerStat) |= 2;
				SetTextUnskippable(61);
			}
			A(ix, ACTOR::Status) = 1;
			SetBossMusic();
			return;
		case 1:	// rolls (no collisions)
		{
			SetSGunnerRollSpr(ix);
			uint8_t direction = (A(ix, ACTOR::SpeedX) & 0x80) != 0 ? DIR_LEFT : DIR_RIGHT;
			if (!ChkTileCollision(ix, 0, direction)) {
				if (--A(ix, ACTOR::Wait) != 0)
					return;
			}
			ShotGunnerStop(ix);
			return;
		}
		case 2:	// shoots
			if (--A(ix, ACTOR::Wait) == 0) {
				SetSGunnerSpeed(ix);
				A(ix, ACTOR::Status) = 1;
				A(ix, ACTOR::Wait) = 0x0b;
				A(ix, ACTOR::COLLISION_CFG) = 0;
				SetSGunnerRollSpr(ix);
				return;
			}
			if (Ram(PlayerY) >= 166 && Ram(PlayerX) >= 170)
				return;	// behind the boxes
			if ((A(ix, ACTOR::ANIM_CNT) & 0x0f) != 0)
				return;
			AddEnemyShot2(ix, ID_SGUNNER_SHOT);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "ShotGunnerLogic: bad status";
			return;
	}
}


void
InitShotGunnerShot(uint16_t ix)
{
	A(ix, ACTOR::Moving)++;
	A(ix, ACTOR::SpriteId) = 0x6e;
	A(ix, ACTOR::COLLISION_CFG) = 1;
	A(ix, ACTOR::Wait) = 0;
	uint16_t speedY;
	uint16_t speedX = CalcShot2(ix, 0x90, &speedY);
	SetActorSpeed(ix, speedX, speedY);
	SetSoundEntryChk(0x0f);
}


// The shot spreads while it flies.
void
ShotGunnerShot(uint16_t ix)
{
	uint8_t time = ++A(ix, ACTOR::Wait);
	uint8_t shape = 2;
	uint8_t sprite = 0x6e;
	if (time >= 7) {
		sprite++;
		if (time >= 14) {
			sprite++;
			if (time >= 21) {
				sprite++;
				shape = 1;
			}
		}
	}
	A(ix, ACTOR::SpriteId) = sprite;
	if (ChkTileCollision(ix, shape, DIR_DOWN))
		DismissActor0(ix);
}


// --- Fire Trooper (firetropper.asm) ------------------------------------------

namespace {

void
FT_MoveToPlayer(uint16_t ix)
{
	A(ix, FIRE_TROPPER::MOVING) = 1;
	if (Ram(PlayerX) >= A(ix, FIRE_TROPPER::X)) {
		A(ix, FIRE_TROPPER::SpriteId) = 0x2c;
		SetActorSpeedX(ix, 0x200);
	} else {
		A(ix, FIRE_TROPPER::SpriteId) = 0x28;
		SetActorSpeedX(ix, 0xfe00);
	}
}


void
InitFireTrooper2(uint16_t ix)
{
	A(ix, FIRE_TROPPER::DELTA_) = 4;
	A(ix, FIRE_TROPPER::ANGLE) = 0x3f;
	A(ix, FIRE_TROPPER::FLAME_MOV_ID) = 2;
	A(ix, FIRE_TROPPER::SpriteId) = 0x2c;
	A(ix, FIRE_TROPPER::START_Y) = A(ix, FIRE_TROPPER::Y);
	A(ix, FIRE_TROPPER::START_X) = A(ix, FIRE_TROPPER::X);
	SetActorSpeed(ix, 0xfe00, 0);
}


uint16_t
FlamePointer(uint16_t ix, uint8_t offset)
{
	uint16_t hl = static_cast<uint16_t>(ix + offset);
	return static_cast<uint16_t>(Ram(hl) | (Ram(IncLow(hl)) << 8));
}


// The flames that are in the room: their count and addresses (last one
// first, through the stack) after the Fire Trooper's structure.
void
FT_GetFlames(uint16_t ix)
{
	uint16_t flames[16];
	int count = 0;
	uint16_t hl = EnemyList;
	for (int i = 0; i < 16; i++, hl = static_cast<uint16_t>(hl + 0x80)) {
		if (Ram(hl) == ID_FLAME)
			flames[count++] = hl;
	}
	if (count != 0) {
		hl = static_cast<uint16_t>(ix | FIRE_TROPPER::NUM_FLAMES);
		Ram(hl) = static_cast<uint8_t>(count);
		hl = IncLow(hl);
		for (int i = count - 1; i >= 0; i--) {
			Ram(hl) = static_cast<uint8_t>(flames[i]);
			hl = IncLow(hl);
			Ram(hl) = static_cast<uint8_t>(flames[i] >> 8);
			hl = IncLow(hl);
		}
	}
	FT_MoveToPlayer(ix);
	A(ix, FIRE_TROPPER::Status)++;
}


// Walks to his firing place; the flames gather at the flamethrower.
void
FT_Walk(uint16_t ix)
{
	Anim2FramesActor(ix, 3);
	uint8_t x = A(ix, FIRE_TROPPER::X);
	uint8_t limit;
	if (x >= 0x80)
		limit = 0x80;
	else if (x >= 0x60)
		return;
	else
		limit = 0x60;
	A(ix, FIRE_TROPPER::X) = limit;
	uint8_t y = A(ix, FIRE_TROPPER::Y);
	A(ix, FIRE_TROPPER::START_Y) = y;
	A(ix, FIRE_TROPPER::START_X) = limit;
	A(ix, FIRE_TROPPER::SpriteId) = 0x2f;	// fires down
	uint8_t flameY = static_cast<uint8_t>(y + 0x0b);
	// (!?) the flames are taken to be actors 1, 2, ...
	uint16_t hl = static_cast<uint16_t>(EnemyListEntry1 + 3);
	int n = Loops(A(ix, FIRE_TROPPER::NUM_FLAMES));
	for (int i = 0; i < n; i++) {
		Ram(hl) = flameY;
		hl = IncLow(hl, 2);
		Ram(hl) = limit;
		hl = static_cast<uint16_t>(hl + 0x7e);
	}
	SetActorSpeed(ix, 0, 0);
	SetRam16(TempData, ix);
	uint8_t flames = A(ix, FIRE_TROPPER::NUM_FLAMES);
	for (int i = 0; i < Loops(flames); i++) {
		uint8_t b = static_cast<uint8_t>(flames - i);	// DJNZ counts down
		uint16_t flame = FlamePointer(ix, static_cast<uint8_t>(b * 2 + 0x3e));
		A(flame, ACTOR::HiddenFlame) = 0;
		A(flame, ACTOR::COLLISION_CFG) = 3;
		SetSprColor(flame, 8, 0x46);	// visible
	}
	A(ix, FIRE_TROPPER::FLAME_JET_CNT) = 0x10;
	A(ix, FIRE_TROPPER::Status)++;
}


// Places the flames along the jet: gaps from the Fire Trooper.
void
FT_MoveFlames11(uint16_t ix)
{
	uint16_t gaps = static_cast<uint16_t>(ix | FIRE_TROPPER::FLAME1_GAP_Y);
	uint16_t hl = static_cast<uint16_t>((gaps & 0xff00) | ((gaps - 0x11) & 0xff));
	int n = Loops(Ram(hl));
	hl = IncLow(hl);
	for (int i = 0; i < n; i++) {
		uint16_t flame = static_cast<uint16_t>(Ram(hl) | (Ram(IncLow(hl)) << 8));
		hl = IncLow(hl, 2);
		uint8_t gapY = Ram(gaps);
		A(flame, ACTOR::GAP_Y) = gapY;
		gaps = IncLow(gaps);
		Ram(IncLow(flame, 3)) = static_cast<uint8_t>(gapY + A(ix, FIRE_TROPPER::START_Y));
		uint8_t gapX = Ram(gaps);
		A(flame, ACTOR::GAP_X) = gapX;
		gaps = IncLow(gaps);
		Ram(IncLow(flame, 5)) = static_cast<uint8_t>(gapX + A(ix, FIRE_TROPPER::START_X));
	}
}


void
FT_ThrowFlames(uint16_t ix)
{
	if (--A(ix, FIRE_TROPPER::FLAME_JET_CNT) == 0) {
		A(ix, FIRE_TROPPER::FLAME_JET_CNT) = 0x50;
		A(ix, FIRE_TROPPER::Status)++;
		return;
	}
	uint8_t gap = static_cast<uint8_t>(0x10 - A(ix, FIRE_TROPPER::FLAME_JET_CNT));
	uint16_t hl = IncLow(ix, FIRE_TROPPER::FLAME1_GAP_Y);
	int n = Loops(A(ix, FIRE_TROPPER::NUM_FLAMES));
	uint8_t a = 0;
	for (int i = 0; i < n; i++) {
		a = static_cast<uint8_t>(a + gap);
		Ram(hl) = a;
		hl = IncLow(hl);
		Ram(hl) = 0;
		hl = IncLow(hl);
	}
	FT_MoveFlames11(ix);
}


// The jet sweeps from side to side.
void
FT_MoveFlames(uint16_t ix)
{
	if (--A(ix, FIRE_TROPPER::FLAME_JET_CNT) == 0) {
		A(ix, FIRE_TROPPER::FLAME_JET_CNT)
			= static_cast<uint8_t>(A(ix, FIRE_TROPPER::NUM_FLAMES) + 1);
		A(ix, FIRE_TROPPER::Status)++;
		return;
	}
	uint8_t delta = A(ix, FIRE_TROPPER::DELTA_);
	uint8_t angle = static_cast<uint8_t>(delta + A(ix, FIRE_TROPPER::ANGLE));
	A(ix, FIRE_TROPPER::ANGLE) = angle;
	uint8_t limit = 0;
	if (angle < 0x18)
		limit = 0x18;
	else if (angle >= 0x3f)
		limit = 0x3f;
	if (limit != 0) {
		A(ix, FIRE_TROPPER::ANGLE) = limit;
		A(ix, FIRE_TROPPER::DELTA_) = static_cast<uint8_t>(-delta);
		A(ix, FIRE_TROPPER::FLAME_MOV_ID)
			= static_cast<uint8_t>((A(ix, FIRE_TROPPER::FLAME_MOV_ID) + 1) & 3);
	}
	uint8_t moving = A(ix, FIRE_TROPPER::FLAME_MOV_ID);
	uint8_t sprite = 0x2f;
	if (A(ix, FIRE_TROPPER::ANGLE) < 0x2c)
		sprite = moving == 1 || moving == 2 ? 0x2e : 0x30;
	A(ix, FIRE_TROPPER::SpriteId) = sprite;

	uint8_t sine, cosine;
	GetFlameSinCos(ix, &sine, &cosine);
	uint8_t gapX = static_cast<uint8_t>(cosine >> 4);
	if (moving == 1 || moving == 2)
		gapX = static_cast<uint8_t>(-gapX);
	A(ix, FIRE_TROPPER::FLAME1_GAP_X) = gapX;
	uint8_t gapY = static_cast<uint8_t>(sine >> 4);
	A(ix, FIRE_TROPPER::FLAME1_GAP_Y) = gapY;

	// (!?) one gap more than there are flames
	uint8_t n = A(ix, FIRE_TROPPER::NUM_FLAMES);
	int loops = Loops(n);
	uint16_t de = IncLow(ix, FIRE_TROPPER::FLAME1_GAP_Y);
	uint8_t c = Ram(de);
	de = IncLow(de, 2);
	uint8_t a = c;
	for (int i = 0; i < loops; i++) {
		a = static_cast<uint8_t>(a + c);
		Ram(de) = a;
		de = IncLow(de, 2);
	}
	de = static_cast<uint16_t>((de & 0xff00) | ((de - n * 2 + 1) & 0xff));
	c = A(ix, FIRE_TROPPER::FLAME1_GAP_X);
	a = c;
	for (int i = 0; i < loops; i++) {
		a = static_cast<uint8_t>(a + c);
		Ram(de) = a;
		de = IncLow(de, 2);
	}
	FT_MoveFlames11(ix);
}


// The flames go out one by one, then he walks again.
void
FT_BringInFlames(uint16_t ix)
{
	if (--A(ix, FIRE_TROPPER::FLAME_JET_CNT) == 0) {
		InitFireTrooper2(ix);
		FT_MoveToPlayer(ix);
		SetActorStatus(ix, 1);
		return;
	}
	uint8_t offset = static_cast<uint8_t>((A(ix, FIRE_TROPPER::FLAME_JET_CNT) - 1) * 2
		+ FIRE_TROPPER::POINTERS);
	uint16_t flame = FlamePointer(ix, offset);
	A(flame, FIRE_TROPPER::field_72) = 1;	// hidden
	A(flame, FIRE_TROPPER::COLLISION_CFG) = 0;
	SetSprColor(flame, 0, 0);
}

}	// namespace


void
InitFireTrooper(uint16_t ix)
{
	if (Ram(FireTrooper_KO) != 0) {
		DismissActor0(ix);
		return;
	}
	A(ix, FIRE_TROPPER::DUMMY_CNT) = 2;
	// (!?) counts Grey Fox, not Fire Troopers
	if (CountEnemyType(ID_GREY_FOX) >= 2) {
		DismissActor0(ix);
		return;
	}
	SetBossMusic();
	InitFireTrooper2(ix);
}


void
FireTrooperLoogic(uint16_t ix)
{
	switch (A(ix, FIRE_TROPPER::Status)) {
		case 0:	// "I'm Fire Trooper"
			if (--A(ix, FIRE_TROPPER::DUMMY_CNT) != 0)
				return;
			A(ix, FIRE_TROPPER::Status)++;
			SetTextUnskip2(108);
			return;
		case 1:
			FT_GetFlames(ix);
			return;
		case 2:
			FT_Walk(ix);
			return;
		case 3:
			FT_ThrowFlames(ix);
			return;
		case 4:
			FT_MoveFlames(ix);
			return;
		case 5:
			FT_BringInFlames(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "FireTrooperLoogic: bad status";
			return;
	}
}


// --- Big Boss (bigboss.asm): runs between the crates and shoots ------------

namespace {

// In the upper or lower corridor (the original's Z flag).
bool
BBChkUpDownCorridors(uint16_t ix)
{
	uint8_t y = A(ix, BIG_BOSS::Y);
	return y == 56 || y == 168;
}


void
BigBossSetSpr2(uint16_t ix, uint8_t direction)
{
	uint8_t sprite = static_cast<uint8_t>(direction * 2 + 0x3f);
	if ((A(ix, BIG_BOSS::ANIM_CNT) & 2) == 0)
		sprite++;
	A(ix, BIG_BOSS::SpriteId) = sprite;
}


void
BigBossSetSpr(uint16_t ix)
{
	BigBossSetSpr2(ix, A(ix, BIG_BOSS::Direction));
}


void
BigBossSetSpeed(uint16_t ix, uint8_t direction)
{
	SetWalkSpeedFast2(ix, direction);
	A(ix, BIG_BOSS::SpeedY) = static_cast<uint8_t>(A(ix, BIG_BOSS::SpeedY) << 1);
	A(ix, BIG_BOSS::SpeedX) = static_cast<uint8_t>(A(ix, BIG_BOSS::SpeedX) << 1);
}


void
BigBossSetDir(uint16_t ix, uint8_t direction)
{
	A(ix, BIG_BOSS::Direction) = direction;
	BigBossSetSpeed(ix, direction);
}


// Faces Snake from behind the crate (the sprite only).
void
BBAimToPlayer(uint16_t ix)
{
	uint8_t a, b;
	if (BBChkUpDownCorridors(ix)) {
		a = A(ix, BIG_BOSS::Y);
		b = 2;
	} else {
		a = A(ix, BIG_BOSS::X);
		b = 4;
	}
	BigBossSetSpr2(ix, static_cast<uint8_t>(b - (a >> 7)));
}


void
BigBossCalcAway(uint16_t ix)
{
	if (BBChkUpDownCorridors(ix))
		BigBossSetDir(ix, Ram(PlayerX) < A(ix, BIG_BOSS::X) ? DIR_RIGHT : DIR_LEFT);
	else
		BigBossSetDir(ix, Ram(PlayerY) < A(ix, BIG_BOSS::Y) ? DIR_DOWN : DIR_UP);
}


void
BBSetDirToPlayer(uint16_t ix)
{
	if (BBChkUpDownCorridors(ix))
		BigBossSetDir(ix, Ram(PlayerX) < A(ix, BIG_BOSS::X) ? DIR_LEFT : DIR_RIGHT);
	else
		BigBossSetDir(ix, Ram(PlayerY) < A(ix, BIG_BOSS::Y) ? DIR_UP : DIR_DOWN);
}


void
BBChkTurnCorner(uint16_t ix)
{
	if (A(ix, BIG_BOSS::SpeedY) != 0) {
		uint8_t y = A(ix, BIG_BOSS::Y);
		if (y != 56 && y != 168)
			return;
		BigBossSetDir(ix, (A(ix, BIG_BOSS::X) & 0x80) == 0 ? DIR_RIGHT : DIR_LEFT);
		return;
	}
	uint8_t x = A(ix, BIG_BOSS::X);
	if (x != 24 && x != 200)
		return;
	BigBossSetDir(ix, (A(ix, BIG_BOSS::Y) & 0x80) == 0 ? DIR_DOWN : DIR_UP);
}


void
SetRandomWait1_20(uint16_t ix)
{
	A(ix, BIG_BOSS::Wait) = static_cast<uint8_t>(((RefreshR() ^ A(ix, BIG_BOSS::ANIM_CNT)) & 0x1f)
		+ 1);
}


// Behind a crate (the original's Z flag).
bool
BBChkCovered(uint16_t ix)
{
	if (BBChkUpDownCorridors(ix)) {
		uint8_t x = A(ix, BIG_BOSS::X);
		return x == 0x30 || x == 0x70 || x == 0xb0;
	}
	uint8_t y = A(ix, BIG_BOSS::Y);
	return y == 0x58 || y == 0x98;
}


bool
BBChkPlayerNear(uint16_t ix)
{
	if (static_cast<uint8_t>(Ram(PlayerX) - A(ix, BIG_BOSS::X) + 0x48) >= 0x91)
		return false;
	return static_cast<uint8_t>(Ram(PlayerY) - A(ix, BIG_BOSS::Y) + 0x48) < 0x91;
}


// In the same row or column of crates as Snake.
bool
BBChkShoot(uint16_t ix)
{
	uint8_t distance = BBChkUpDownCorridors(ix)
		? static_cast<uint8_t>(Ram(PlayerX) - A(ix, BIG_BOSS::X))
		: static_cast<uint8_t>(Ram(PlayerY) - A(ix, BIG_BOSS::Y));
	return static_cast<uint8_t>(distance + 0x30) < 0x61;
}


void
BigBossSetRunAway(uint16_t ix)
{
	A(ix, BIG_BOSS::Status) = 2;
	A(ix, BIG_BOSS::Moving) = 1;
	BigBossCalcAway(ix);
	BigBossSetSpr(ix);
}


void
BigBossThink(uint16_t ix)
{
	if (BBChkPlayerNear(ix)) {
		BigBossSetRunAway(ix);
		return;
	}
	if (!BBChkShoot(ix)) {
		A(ix, BIG_BOSS::Status) = 2;	// runs
		A(ix, BIG_BOSS::Moving) = 1;
		A(ix, BIG_BOSS::Dummy) = 2;	// (!?) not used
		BBSetDirToPlayer(ix);
		BigBossSetSpr(ix);
		return;
	}
	if (--A(ix, BIG_BOSS::Wait) != 0)
		return;
	A(ix, BIG_BOSS::Status) = 3;	// shows up to shoot
	A(ix, BIG_BOSS::Moving) = 1;
	A(ix, BIG_BOSS::Wait) = 5;
	BBSetDirToPlayer(ix);
	BigBossSetSpr(ix);
}


void
BigBossRun(uint16_t ix)
{
	BigBossSetSpr(ix);
	BBChkTurnCorner(ix);
	if (!BBChkCovered(ix))
		return;
	if (BBChkPlayerNear(ix)) {
		BigBossSetRunAway(ix);
		return;
	}
	if (!BBChkShoot(ix))
		return;
	A(ix, BIG_BOSS::Status) = 1;
	A(ix, BIG_BOSS::Moving) = 0;
	BBAimToPlayer(ix);
}


void
BB_Shoot(uint16_t ix)
{
	if (--A(ix, BIG_BOSS::Wait) != 0)
		return;
	A(ix, BIG_BOSS::Status) = 5;	// back behind the crate
	A(ix, BIG_BOSS::Moving) = 1;
	uint8_t direction = GetOppositeDir(A(ix, BIG_BOSS::Direction));
	A(ix, BIG_BOSS::Direction) = direction;
	BigBossSetSpeed(ix, direction);
	AddEnemyShot2(ix, BBChkUpDownCorridors(ix) ? ID_BULLET_VERT : ID_BULLET_HORIZ);
}

}	// namespace


void
InitBigBoss(uint16_t ix)
{
	if ((Ram(BigBossStat) & 1) != 0) {
		DismissActor(ix);	// defeated
		return;
	}
	A(ix, BIG_BOSS::Wait) = 2;
	ResetActorSpeed(ix);
	A(ix, BIG_BOSS::Direction) = DIR_RIGHT;
	BigBossSetSpr(ix);
}


void
BigBossLogic(uint16_t ix)
{
	switch (A(ix, BIG_BOSS::Status)) {
		case 0:	// the confession
			if (--A(ix, BIG_BOSS::Wait) != 0)
				return;
			A(ix, BIG_BOSS::Status) = 1;
			SetRandomWait1_20(ix);
			if ((Ram(BigBossStat) & 2) != 0)
				return;
			Ram(BigBossStat) |= 2;
			SetTextUnskip2(147);
			return;
		case 1:
			BigBossThink(ix);
			return;
		case 2:
			BigBossRun(ix);
			return;
		case 3:	// shows up from a crate
			BigBossSetSpr(ix);
			if (--A(ix, BIG_BOSS::Wait) != 0)
				return;
			A(ix, BIG_BOSS::Status) = 4;
			A(ix, BIG_BOSS::Moving) = 0;
			A(ix, BIG_BOSS::Wait) = 5;
			BBAimToPlayer(ix);
			return;
		case 4:
			BB_Shoot(ix);
			return;
		case 5:	// takes cover
			BigBossSetSpr(ix);
			if (!BBChkCovered(ix))
				return;
			A(ix, BIG_BOSS::Status) = 1;
			A(ix, BIG_BOSS::Moving) = 0;
			SetRandomWait1_20(ix);
			BBAimToPlayer(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "BigBossLogic: bad status";
			return;
	}
}


// --- Coward Duck and his boomerangs (cowardduck.asm) ---------------------

void
InitCowardDuck(uint16_t ix)
{
	if (Ram(Card8Taken) != 0) {
		DismissActor0(ix);
		return;
	}
	A(ix, COWARD_D::SpriteId) = 0x18;	// walks
	A(ix, COWARD_D::TIMER) = 3;
	A(ix, COWARD_D::MOVING)++;
	SetBossMusic();
}


void
CowardDuckLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:	// "I'm Coward Duck"
			if (--A(ix, COWARD_D::TIMER) != 0)
				return;
			if (Ram(CowardDuckSpeech) == 0) {
				Ram(CowardDuckSpeech) = 1;
				SetTextUnskippable(139);
			}
			A(ix, COWARD_D::TIMER) = 0x10;
			A(ix, COWARD_D::Status)++;
			return;
		case 1:	// walks towards Snake's side
			if (--A(ix, COWARD_D::TIMER) != 0)
				return;
			A(ix, COWARD_D::TIMER) = 8;
			SetActorSpeedX(ix, Ram(PlayerX) >= A(ix, COWARD_D::X) ? 0x200 : 0xfe00);
			A(ix, COWARD_D::Status)++;
			return;
		case 2:	// then throws a boomerang
			Anim2FramesActor(ix, 7);
			if (--A(ix, COWARD_D::TIMER) != 0)
				return;
			A(ix, COWARD_D::TIMER) = 5;
			A(ix, COWARD_D::SpriteId) = 0x1a;
			SetActorSpeedX(ix, 0);
			A(ix, COWARD_D::Status)++;
			AddEnemy(ID_BOOMERANG, static_cast<uint8_t>(A(ix, COWARD_D::Y) - 0x10),
				A(ix, COWARD_D::X));
			return;
		case 3:	// back to the middle
			if (--A(ix, COWARD_D::TIMER) != 0)
				return;
			A(ix, COWARD_D::TIMER) = 8;
			A(ix, ACTOR::SpriteId) = 0x18;
			SetActorSpeedX(ix, A(ix, COWARD_D::X) >= 0x80 ? 0xfe00 : 0x200);
			A(ix, COWARD_D::Status)++;
			return;
		case 4:
			Anim2FramesActor(ix, 7);
			if (--A(ix, COWARD_D::TIMER) != 0)
				return;
			A(ix, COWARD_D::TIMER) = 8;
			SetActorSpeedX(ix, 0);
			A(ix, COWARD_D::Status) = 1;
			return;
		default:
			if (gUnported == NULL)
				gUnported = "CowardDuckLogic: bad status";
			return;
	}
}


// The boomerang flies along an ellipse around where it was thrown.
void
InitBoomerang(uint16_t ix)
{
	SetSoundEntryChk(6);
	A(ix, BOOMERANG::START_Y) = A(ix, BOOMERANG::Y);
	uint8_t x = A(ix, BOOMERANG::X);
	A(ix, BOOMERANG::START_X) = x;
	A(ix, BOOMERANG::COUNTER) = x >= 0x80 ? 0 : 2;	// clockwise or not
	A(ix, BOOMERANG::SHORT_RANGE) = RefreshR() & 1;
	A(ix, BOOMERANG::SpriteId) = 0x77;
	A(ix, BOOMERANG::ANGLE_Y) = 64;
	A(ix, BOOMERANG::ANGLE_DELTA_Y) = 0xfe;
	A(ix, BOOMERANG::ANGLE_X) = 64;
	A(ix, BOOMERANG::ANGLE_DELTA_X) = 0xfc;
}


void
BoomerangLogic(uint16_t ix)
{
	if ((A(ix, BOOMERANG::ANIM_CNT) & 1) == 0) {
		uint8_t spin = static_cast<uint8_t>(A(ix, BOOMERANG::SPIN_CNT) + 1);
		if (spin >= 3)
			spin = 0;
		A(ix, BOOMERANG::SPIN_CNT) = spin;
		A(ix, BOOMERANG::SpriteId) = static_cast<uint8_t>(spin + 0x77);
	}

	uint8_t angle = static_cast<uint8_t>(A(ix, BOOMERANG::ANGLE_Y)
		+ A(ix, BOOMERANG::ANGLE_DELTA_Y));
	A(ix, BOOMERANG::ANGLE_Y) = angle;
	if (angle >= 64) {
		// down, up, then back to Coward Duck. (!?) GetSinCos then gets
		// what is left in A, not the angle
		uint8_t delta = static_cast<uint8_t>(-A(ix, BOOMERANG::ANGLE_DELTA_Y));
		A(ix, BOOMERANG::ANGLE_DELTA_Y) = delta;
		angle = static_cast<uint8_t>(delta + 2);
		if (angle == 0) {
			DismissActor0(ix);
			// A as RemoveActor_ leaves it
			uint8_t sprites = Ram(static_cast<uint16_t>(ix | 0x20));
			angle = sprites == 0 ? 0
				: static_cast<uint8_t>((ix & 0xff) + 0x21 + 5 * sprites);
		}
	}
	uint8_t sine, cosine;
	GetSinCos(angle, &sine, &cosine);
	uint8_t radius = static_cast<uint8_t>(cosine >> 1);
	if (A(ix, BOOMERANG::SHORT_RANGE) != 0)
		radius >>= 1;
	A(ix, BOOMERANG::Y) = static_cast<uint8_t>(A(ix, BOOMERANG::START_Y) + radius);

	angle = static_cast<uint8_t>(A(ix, BOOMERANG::ANGLE_X) + A(ix, BOOMERANG::ANGLE_DELTA_X));
	A(ix, BOOMERANG::ANGLE_X) = angle;
	if (angle >= 0x40) {
		uint8_t delta = static_cast<uint8_t>(-A(ix, BOOMERANG::ANGLE_DELTA_X));
		A(ix, BOOMERANG::ANGLE_DELTA_X) = delta;
		A(ix, BOOMERANG::COUNTER)++;
		angle = delta;	// (!?) as above
	}
	GetSinCos(angle, &sine, &cosine);
	uint8_t offset = static_cast<uint8_t>(cosine >> 2);
	uint8_t counter = A(ix, BOOMERANG::COUNTER);
	if (counter >= 2 && counter < 4)
		offset = static_cast<uint8_t>(-offset);
	A(ix, BOOMERANG::X) = static_cast<uint8_t>(A(ix, BOOMERANG::START_X) + offset);
}
