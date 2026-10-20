/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from Banks0123.asm (SetupEnemyRoom .. Multiply8),
// logic/actors/hideguards.asm, guard.asm (ChangeGuardSprDir),
// helperdirections.asm (GetDirToPlayer).
#include "Actors.h"

#include "Common.h"
#include "Bosses.h"
#include "Cameras.h"
#include "Dogs.h"
#include "Guards.h"
#include "Hazards.h"
#include "GfxBasic.h"
#include "Random.h"
#include "Rom.h"
#include "Shots.h"
#include "Soldiers.h"
#include "RoomActors.h"
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
IncLow(uint16_t address, int n = 1)
{
	return static_cast<uint16_t>((address & 0xff00) | ((address + n) & 0xff));
}


typedef void (*ActorInit)(uint16_t ix);

const ActorInit kActorInit[] = {
	InitBridge, InitBridge, InitEllenVoice, InitGuardSlow, InitGuardMedium, InitCamera,
	InitMines, InitGas, InitTank, InitGuardAlert, InitGuardAlert, InitTankShell, InitShooter,
	InitGuardElevat, InitRollingBarrel, InitPitfall,	// #10
	InitMetalGear, InitBulldozer, InitGuardLorry, InitJetpackTakeoff, InitJetpackSwitch,
	InitJetpack, InitTankShellBoss, InitGuardSwitch,	// #18
	InitDog, InitArnold, InitDogBasement, InitLorryShooter, InitSpawnDog, InitGuardFast,
	InitScorpion, InitBigBoss,	// #20
	InitShotGunner, InitMachGunKid, InitLaserRoom, InitFireTrooper, InitFlame, InitHindD,
	InitSpawnTankShell, InitSpawnGuardElev,	// #28
	InitCowardDuck, InitDummy, InitShotGunnerShot, InitPowerSwitch, InitCaptureScene,
	InitDesertSecurity, InitGuardShot, InitSentinel,	// #30
	InitPrisoner, InitPrisoner, InitPrisoner, InitPrisoner, InitCameraLaser, InitLaserShot,
	InitPrisoner, InitPrisoner,	// #38
	InitGuardSilencer, InitBulletHor, InitBulletVert, InitMGunKidShot, InitShotToPlayer,
	InitTankShot, InitBoomerang, InitSnoringSymbol
};


void
SetupActor(uint16_t ix)
{
	uint16_t hl = Ram16(TempData3);
	Ram(hl) = Ram(TempData);
	hl = IncLow(hl);
	Ram(hl) = 0;	// status
	hl = IncLow(hl);
	Ram(hl) = 0;
	hl = IncLow(hl);
	Ram(hl) = Ram(TempData2);	// Y
	hl = IncLow(hl);
	Ram(hl) = 0;
	hl = IncLow(hl);
	Ram(hl) = Ram(static_cast<uint16_t>(TempData2 + 1));	// X
	hl = IncLow(hl);
	Ram(hl) = 0;
	A(ix, ACTOR::ANIM_CNT) = 0;
	A(ix, ACTOR::PunchesCnt) = 0;
	A(ix, ACTOR::COLLISION_CFG) = 3;
	A(ix, ACTOR::TOUCH_INFO) = 0;
	A(ix, ACTOR::ZzzSignSprIdx) = 1;
	A(ix, ACTOR::StunnedCnt) = 0;
	SetBanks_4_5_6();
	SetActorSprCols(ix, static_cast<uint16_t>(SYM(idxActorSprCols) - 2));
	A(ix, ACTOR::LIFE) = Mem(static_cast<uint16_t>(SYM(idxActorLife) - 1 + Ram(TempData)));
	SetBanks1_2_3();
	uint8_t index = static_cast<uint8_t>(A(ix, ACTOR::ID) - 1);
	if (index < sizeof(kActorInit) / sizeof(kActorInit[0]))
		kActorInit[index](ix);
}


// Reserves sprite layers (free ones hold Y = #E0) and sets up the actor.
void
AddEnemy4(uint16_t hl)
{
	SetRam16(TempData3, hl);
	uint16_t ix = hl;
	SetBanks_4_5_6();
	uint8_t sprites = Mem(static_cast<uint16_t>(SYM(NumSprEnemies) - 1 + Ram(TempData)));
	A(ix, ACTOR::NumSprites) = sprites;
	SetBanks1_2_3();
	if (sprites == 0) {
		SetupActor(ix);
		return;
	}
	uint8_t needed = sprites;
	uint8_t reserved = 0;
	uint8_t layer = 0;
	uint16_t attr = EnemySprAttRAM;
	for (int i = 0; i < 22; i++) {
		if (Ram(attr) == 0xe0) {
			Ram(attr)++;
			SetSpriteLayer(layer, reserved);
			reserved++;
			if (--needed == 0) {
				SetupActor(ix);
				return;
			}
		}
		layer++;
		attr = IncLow(attr, 4);
	}
	// not enough free sprites: give back the ones reserved
	uint8_t count = static_cast<uint8_t>(A(ix, ACTOR::NumSprites) - needed);
	if (count == 0)
		return;
	SetSpritesFree(IncLow(static_cast<uint16_t>(ix | 0x20)), count);
}

}	// namespace


void
SetupEnemyRoom()
{
	ChkAlarmEnd();
	Ram(AlertModeCopy) = Ram(AlertMode);
	Ram(PowerSwitchOn) = 0;
	Ram(AlertSignNotOnScreen) = 0;
	Ram(AlertIconTimer) = 0;
	Ram(static_cast<uint16_t>(AlertIconTimer + 1)) = 0;
	Ram(PowerSwitchY) = 0;
	Ram(static_cast<uint16_t>(PowerSwitchY + 1)) = 0;
	if (Ram(Room) == 83)
		Ram(ArnoldsCnt) = 2;
	if (Ram(Room) == 150)
		Ram(GuardSilencerCnt) = 4;
	Ram(NumBasementDogs) = CountEnemyType(ID_DOG_BASEMENT);
	for (int i = 0; i < 0x800; i++)
		Ram(static_cast<uint16_t>(EnemyList + i)) = 0;
	HideEnemySprs();
	SetBanks_4_5_6();
	Ram(NoEnemiesRoom) = 1;
	uint8_t room = Ram(Room);
	if (room >= 222) {
		SetBanks1_2_3();
		return;
	}
	uint16_t hl = GetPointerDE2A(SYM(idxActorsRooms), room);
	uint8_t count = Mem(hl) & 0x0f;
	if (count == 0) {
		SetBanks1_2_3();
		return;
	}
	hl++;
	for (int i = 0; i < count; i++) {
		SetBanks_4_5_6();
		uint8_t id = Mem(hl++);
		uint8_t y = Mem(hl++);
		uint8_t x = Mem(hl++);
		AddEnemy(id, y, x);
	}
	Ram(NoEnemiesRoom) = 0;
	SetBanks1_2_3();
}


uint16_t
AddEnemy(uint8_t id, uint8_t y, uint8_t x)
{
	Ram(TempData) = id;
	Ram(TempData2) = y;
	Ram(static_cast<uint16_t>(TempData2 + 1)) = x;
	uint16_t hl = EnemyList;
	for (int i = 0; i < 16; i++) {
		if (Ram(hl) == 0) {
			AddEnemy4(hl);
			return hl;
		}
		hl = static_cast<uint16_t>(hl + 0x80);
	}
	return hl;
}


uint8_t
CountEnemyType(uint8_t id)
{
	uint8_t count = 0;
	for (int i = 0; i < 16; i++) {
		if (Ram(static_cast<uint16_t>(EnemyList + i * 0x80)) == id)
			count++;
	}
	return count;
}


// Room x 3 into RespawnInfo: enemy ID, two places (high nibble Y, low X).
static uint16_t
RespawnInfoOfRoom()
{
	return static_cast<uint16_t>(Ram(Room) * 3 + SYM(RespawnInfo));
}


// In an alert, now and then another enemy comes in.
void
ChkRespawnEnemy()
{
	if (Ram(AlertMode) == 0 || Ram(AlertRespawnTimer) == 0)
		return;
	if (--Ram(AlertRespawnTimer) != 0)
		return;
	Ram(AlertRespawnTimer) = static_cast<uint8_t>(((RefreshR() ^ Ram(TickCounter)) & 0x0f) + 0x14);
	if (Ram(Room) >= 188)
		return;
	SetBanks_4_5_6();
	uint16_t hl = RespawnInfoOfRoom();
	uint8_t id = Mem(hl);
	if (id == 0) {
		SetBanks1_2_3();
		return;
	}
	uint8_t limit = (id == ID_GUARD_REDALERT || id == ID_JETPACK) ? 3 : 4;
	if (CountEnemyType(id) >= limit) {
		SetBanks1_2_3();
		return;
	}
	hl++;
	if ((Ram(TickCounter) & 1) != 0)
		hl++;
	uint8_t place = Mem(hl);
	SetBanks1_2_3();
	AddEnemy(id, place & 0xf0, static_cast<uint8_t>(place << 4));
}


void
ChkAlarmEnd()
{
	if (Ram(TransmiTaken) != 0 || Ram(AlertMode) == 0)
		return;
	uint8_t room = Ram(Room);
	if (room >= 0xf0) {
		StopAlert();
		return;
	}
	if (Ram(AlertRespawnTimer) != 0) {
		if (Ram(static_cast<uint16_t>(AlertRespawnTimer + 1)) != 0)
			return;
		Ram(AlertRespawnTimer) = 0;
		Ram(RoomAlert) = Ram(Room);
		return;
	}
	if (room != Ram(RoomAlert)) {
		StopAlert();
		return;
	}
	uint8_t id;
	if (room == 216) {
		id = ID_GUARD_REDALERT;
	} else {
		if (room >= 188) {
			StopAlert();
			return;
		}
		SetBanks_4_5_6();
		id = Mem(RespawnInfoOfRoom());
		SetBanks1_2_3();
	}
	uint8_t count = CountEnemyType(id);
	if (count == 0x10 || count == 0)
		StopAlert();
}


void
StopAlert()
{
	for (int i = 0; i < 6; i++)
		Ram(static_cast<uint16_t>(AlertMode + i)) = 0;
	if ((Ram(ControlConfig) & 0x40) == 0)
		return;
	SetAreaMusic2();
}


void
TransformAlertGuard(uint16_t ix)
{
	if (Ram(Room) >= 188)
		return;
	SetBanks_4_5_6();
	uint8_t id = Mem(RespawnInfoOfRoom());
	SetBanks1_2_3();
	TransformAlertGuard2(ix, id);
}


// The alert guard turns into the room's respawn enemy and comes for Snake.
void
TransformAlertGuard2(uint16_t ix, uint8_t id)
{
	A(ix, ACTOR::ID) = id;
	A(ix, ACTOR::Status) = 0;
	ResetActorSpeed(ix);
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::Wait) = 1;
	A(ix, ACTOR::COLLISION_CFG) = 3;
	A(ix, ACTOR::Y) &= 0xfe;
	A(ix, ACTOR::X) &= 0xfe;
	uint8_t other;
	A(ix, ACTOR::Direction) = GetDirToPlayer(ix, &other);
	AnimateGuard2(ix, A(ix, ACTOR::Direction));
	SetActorSprColors(ix);
}


void
InitGuardFast(uint16_t ix)
{
	InitGuard(ix, 8);
}


void
InitGuardMedium(uint16_t ix)
{
	InitGuard(ix, 4);
}


void
InitGuardSlow(uint16_t ix)
{
	InitGuard(ix, 0);
}


// Guards in rooms 26, 85 and 138 fall asleep.
void
InitGuard(uint16_t ix, uint8_t speed)
{
	A(ix, ACTOR::IdxGuardSpeed) = speed;
	uint8_t room = Ram(Room);
	Ram(SleepyGuardFlag) = (room == 26 || room == 85 || room == 138) ? 1 : 0;
	A(ix, ACTOR::AwakeTime) = (RefreshR() & 1) == 0 ? 5 : 0x40;
	InitGuardPath(ix);
	ChangeGuardSprDir(ix);
}


// The path is chosen by the number of guards and cameras before this one.
void
InitGuardPath(uint16_t ix)
{
	A(ix, ACTOR::Moving)++;
	uint8_t count = CountEnemyType(ID_GUARD_SLOW);
	count = static_cast<uint8_t>(count + CountEnemyType(ID_CAMERA));
	count = static_cast<uint8_t>(count + CountEnemyType(ID_SENTINEL));
	count = static_cast<uint8_t>(count + CountEnemyType(ID_GUARD_MEDIUM));
	count = static_cast<uint8_t>(count + CountEnemyType(ID_GUARD_FAST));
	InitGuardPath2(ix, count);
}


namespace {

// Set by HideGuardSpr: SetSprColor leaves B = 0, so InitGuardPath2 goes
// on with path index 0 (a path from the bytes before the room's list).
bool sHiddenGuard = false;


// HideGuardRoom...: guards that would stand right where Snake comes in
// are removed. They return what the original leaves in A, which the
// chain in InitGuardPath2 goes on comparing with.
uint8_t
HideGuardSpr(uint16_t ix)
{
	A(ix, ACTOR::REMOVE_GUARD) = 1;
	A(ix, ACTOR::COLLISION_CFG) = 0;
	A(ix, ACTOR::NumSprites) = 0;
	HideSprColor(ix);	// with 0 sprites it does nothing
	sHiddenGuard = true;
	return 0;
}


uint8_t
HideGuardChkCoord(uint16_t ix, uint8_t a, uint8_t c)
{
	if (a != c)
		return a;
	return HideGuardSpr(ix);
}


uint8_t
HideGuardChkCoordN(uint16_t ix, uint8_t a, uint8_t c)
{
	if (a == c)
		return a;
	return HideGuardSpr(ix);
}


uint8_t
HideGuardRoom1(uint16_t ix, uint8_t d)
{
	return HideGuardChkCoord(ix, A(ix, ACTOR::Y), d == 1 ? 0xb0 : 0x18);
}


uint8_t
HideGuardRoom13(uint16_t ix, uint8_t e)
{
	if (e != 137)
		return A(ix, ACTOR::X);
	return HideGuardChkCoord(ix, A(ix, ACTOR::X), 0x88);
}


uint8_t
HideGuardRoom15(uint16_t ix, uint8_t d)
{
	if (d != 4)
		return A(ix, ACTOR::X);
	return HideGuardChkCoord(ix, A(ix, ACTOR::X), 0x10);
}


uint8_t
HideGuardRoom17(uint16_t ix, uint8_t d)
{
	if (d != 1)
		return HideGuardChkCoord(ix, A(ix, ACTOR::Y), 0x30);
	return HideGuardChkCoordN(ix, A(ix, ACTOR::Y), 0x30);
}


uint8_t
HideGuardRoom18(uint16_t ix, uint8_t d)
{
	if (d != 1)
		return HideGuardChkCoord(ix, A(ix, ACTOR::Y), 0x13);
	return HideGuardChkCoordN(ix, A(ix, ACTOR::Y), 0x13);
}


uint8_t
HideGuardRoom19(uint16_t ix, uint8_t d, uint8_t e)
{
	if (e == 141)
		return HideGuardChkCoord(ix, A(ix, ACTOR::Y), 0x78);
	if (d != 1)
		return A(ix, ACTOR::Y);
	return HideGuardChkCoord(ix, A(ix, ACTOR::Y), 0xa8);
}


uint8_t
HideGuardRoom22(uint16_t ix, uint8_t e)
{
	return HideGuardChkCoord(ix, A(ix, ACTOR::X), e == 20 ? 0x20 : 0xf0);
}


uint8_t
HideGuardRoom35(uint16_t ix, uint8_t e)
{
	if (e == 156)
		return HideGuardChkCoord(ix, A(ix, ACTOR::Y), 0x28);
	if (e != 0x21)
		return A(ix, ACTOR::X);
	return HideGuardChkCoord(ix, A(ix, ACTOR::X), 0x18);
}

}	// namespace


uint8_t
HideGuardRoom39(uint16_t ix)
{
	uint8_t c = Ram(PreviousRoom) == 242 ? 72 : 176;
	return HideGuardChkCoord(ix, A(ix, ACTOR::Y), c);
}


void
InitGuardPath2(uint16_t ix, uint8_t index)
{
	uint8_t d = Ram(NextRoomDirect);
	uint8_t e = Ram(PreviousRoom);
	sHiddenGuard = false;
	uint8_t a = static_cast<uint8_t>(Ram(Room) - 1);
	if (a == 0)
		a = HideGuardRoom1(ix, d);
	a = static_cast<uint8_t>(a - 12);
	if (a == 0)
		a = HideGuardRoom13(ix, e);
	a = static_cast<uint8_t>(a - 2);
	if (a == 0)
		a = HideGuardRoom15(ix, d);
	a = static_cast<uint8_t>(a - 2);
	if (a == 0)
		a = HideGuardRoom17(ix, d);
	a = static_cast<uint8_t>(a - 1);
	if (a == 0)
		a = HideGuardRoom18(ix, d);
	a = static_cast<uint8_t>(a - 1);
	if (a == 0)
		a = HideGuardRoom19(ix, d, e);
	a = static_cast<uint8_t>(a - 3);
	if (a == 0)
		a = HideGuardRoom22(ix, e);
	if (a == 13)
		HideGuardRoom35(ix, e);
	A(ix, ACTOR::SpriteId) = 0;
	if (sHiddenGuard)
		index = 0;
	GetPathPoints(ix, index);
}


static uint16_t
RoomPath(uint8_t index)
{
	uint16_t paths = Mem16(static_cast<uint16_t>(SYM(idxRoomPaths) + Ram(Room) * 2));
	return Mem16(static_cast<uint16_t>(paths + static_cast<uint8_t>((index - 1) * 2)));
}


void
GetPathPoints(uint16_t ix, uint8_t index)
{
	A(ix, ACTOR::idxPathPoint) = 0;
	SetBanks_4_5_6();
	uint16_t path = RoomPath(index);
	A(ix, ACTOR::PointerL) = static_cast<uint8_t>(path);
	A(ix, ACTOR::PointerH) = static_cast<uint8_t>(path >> 8);
	A(ix, ACTOR::Cnt_NumPathPoints) = Mem(path);
	GetPathPoint(ix, static_cast<uint16_t>(path + 1));
}


void
GetPathPoint(uint16_t ix, uint16_t point)
{
	A(ix, ACTOR::DestinationY) = Mem(point);
	A(ix, ACTOR::DestinationX) = Mem(static_cast<uint16_t>(point + 1));
	A(ix, ACTOR::NextPointOffset) = 1;
	SetDirToPoint(ix);
}


void
GetPathPoint_(uint16_t ix, uint16_t point)
{
	SetBanks_4_5_6();
	GetPathPoint(ix, point);
}


// Direction and speed (WalkSpeeds by the guard's speed) to the destination.
void
SetDirToPoint(uint16_t ix)
{
	uint8_t e = 0, b = 0, d = 0, c = 0;
	uint8_t y = A(ix, ACTOR::Y);
	uint8_t destY = A(ix, ACTOR::DestinationY);
	if (y != destY) {
		if (y > destY) {
			e = 2;
		} else {
			e = 1;
			b = 1;
		}
	}
	uint8_t x = A(ix, ACTOR::X);
	uint8_t destX = A(ix, ACTOR::DestinationX);
	if (x != destX) {
		if (x > destX) {
			d = 2;
			c = 2;
		} else {
			d = 1;
			c = 3;
		}
	}
	A(ix, ACTOR::Direction) = b | c;
	uint16_t table = static_cast<uint16_t>(SYM(WalkSpeeds) - 2);
	uint16_t speedX = 0;
	if (d != 0)
		speedX = Mem16(static_cast<uint16_t>(table + static_cast<uint8_t>(d * 2
			+ A(ix, ACTOR::IdxGuardSpeed))));
	uint16_t speedY = 0;
	if (e != 0)
		speedY = Mem16(static_cast<uint16_t>(table + static_cast<uint8_t>(e * 2
			+ A(ix, ACTOR::IdxGuardSpeed))));
	SetBanks1_2_3();
	SetActorSpeed(ix, speedX, speedY);
}


// At a path point: on to the next one (back to the first after the last);
// guards may stop there for a while.
bool
UpdateActorPath(uint16_t ix)
{
	SetBanks_4_5_6();
	A(ix, ACTOR::Y) = A(ix, ACTOR::DestinationY);
	A(ix, ACTOR::Ydec) = 0;
	A(ix, ACTOR::X) = A(ix, ACTOR::DestinationX);
	A(ix, ACTOR::Xdec) = 0;
	uint8_t count = A(ix, ACTOR::Cnt_NumPathPoints);
	unsigned next = A(ix, ACTOR::idxPathPoint) + A(ix, ACTOR::NextPointOffset);
	uint8_t index;
	if (next > 0xff)
		index = static_cast<uint8_t>(count - 1);
	else if (next == count)
		index = 0;
	else
		index = static_cast<uint8_t>(next);
	A(ix, ACTOR::idxPathPoint) = index;
	uint16_t point = static_cast<uint16_t>((A(ix, ACTOR::PointerL) | (A(ix, ACTOR::PointerH) << 8))
		+ 1 + static_cast<uint8_t>(index * 2));
	A(ix, ACTOR::DestinationY) = Mem(point);
	A(ix, ACTOR::DestinationX) = Mem(static_cast<uint16_t>(point + 1));
	SetDirToPoint(ix);

	uint8_t id = A(ix, ACTOR::ID);
	if (id == ID_CAMERA) {
		// SetCamRndWait
		A(ix, ACTOR::Moving) = 0;
		uint8_t wait = static_cast<uint8_t>(RefreshR() >> 1);
		A(ix, ACTOR::Wait) = wait == 0 ? 1 : wait;
		SetBanks1_2_3();
		return false;
	}
	if (id != ID_GUARD_SLOW && id != ID_GUARD_MEDIUM && id != ID_GUARD_EXIT_LORRY
		&& id != ID_GUARD_FAST)
		return false;	// (banks 4-6 stay selected, as in the original)
	// ChkWaitPathPoint
	if (id == ID_GUARD_EXIT_LORRY && static_cast<uint8_t>(A(ix, ACTOR::idxPathPoint) - 1) == 0)
		return false;
	if ((RefreshR() & 1) == 0)
		return false;
	A(ix, ACTOR::Moving)--;
	A(ix, ACTOR::Wait) = 0x10;
	A(ix, ACTOR::GuardStatus) = 1;
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>((RefreshR() & 6) + A(ix, ACTOR::Direction));
	SetBanks1_2_3();
	return true;
}


// Sentinels turn to the directions in their path data.
void
GetSentinelLookDirs(uint16_t ix)
{
	SetBanks_4_5_6();
	uint8_t index = CountEnemyType(ID_SENTINEL);
	A(ix, ACTOR::IDX_SAME_ID) = index;
	uint16_t path = RoomPath(index);
	A(ix, ACTOR::Cnt_NumPathPoints) = Mem(path);
	path++;
	A(ix, ACTOR::LookDirPointerLow) = static_cast<uint8_t>(path);
	A(ix, ACTOR::LookDirPointerHigh) = static_cast<uint8_t>(path >> 8);
	SetSentinelLookDir(ix, path);
}


void
SetSentinelLookDir(uint16_t ix, uint16_t pointer)
{
	uint8_t direction = Mem(pointer);
	A(ix, ACTOR::Direction) = direction;
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(direction + 8);
	A(ix, ACTOR::SENTINEL_WAIT) = RefreshR();
	SetBanks1_2_3();
}


void
SetSentinelLookDir_(uint16_t ix, uint16_t pointer)
{
	SetBanks_4_5_6();
	SetSentinelLookDir(ix, pointer);
}


void
SetActorSpeedY(uint16_t ix, uint16_t speed)
{
	A(ix, ACTOR::speedYdec) = static_cast<uint8_t>(speed);
	A(ix, ACTOR::SpeedY) = static_cast<uint8_t>(speed >> 8);
}


void
SetActorSpeedX(uint16_t ix, uint16_t speed)
{
	A(ix, ACTOR::SpeedXdec) = static_cast<uint8_t>(speed);
	A(ix, ACTOR::SpeedX) = static_cast<uint8_t>(speed >> 8);
}


uint16_t
AddActorSpeedX(uint16_t ix, uint16_t speed)
{
	speed = static_cast<uint16_t>(speed + (A(ix, ACTOR::SpeedXdec) | (A(ix, ACTOR::SpeedX) << 8)));
	SetActorSpeedX(ix, speed);
	return speed;
}


void
SetActorSpeed(uint16_t ix, uint16_t speedX, uint16_t speedY)
{
	SetActorSpeedX(ix, speedX);
	SetActorSpeedY(ix, speedY);
}


void
ResetActorSpeed(uint16_t ix)
{
	SetActorSpeed(ix, 0, 0);
}


void
Anim2FramesActor(uint16_t ix, uint8_t mask)
{
	if ((A(ix, ACTOR::ANIM_CNT) & mask) != 0)
		return;
	A(ix, ACTOR::SpriteId) ^= 1;
}


void
AnimZzzSign(uint16_t ix, uint8_t mask, uint8_t firstSprite)
{
	if ((A(ix, ACTOR::ANIM_CNT) & mask) != 0)
		return;
	uint8_t b = static_cast<uint8_t>(A(ix, ACTOR::Direction) * 3);
	uint8_t index = --A(ix, ACTOR::ZzzSignSprIdx);
	if (index == 0) {
		index = 4;
		A(ix, ACTOR::ZzzSignSprIdx) = 4;
	}
	uint8_t frame = Mem(static_cast<uint16_t>(SYM(AnimZzzFrames) - 1 + index));
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(frame + b + firstSprite);
}


void
NextActorStatus(uint16_t ix)
{
	A(ix, ACTOR::Status)++;
}


void
ResetActorStatus(uint16_t ix)
{
	A(ix, ACTOR::Status) = 0;
}


void
SetActorStatus(uint16_t ix, uint8_t status)
{
	A(ix, ACTOR::Status) = status;
}


// Walk sprites: direction x 2, keeping the animation frame.
void
ChangeGuardSprDir(uint16_t ix)
{
	uint8_t direction = A(ix, ACTOR::Direction);
	uint8_t base = static_cast<uint8_t>((direction << 1) | (direction >> 7));	// RLCA
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>((A(ix, ACTOR::SpriteId) & 1) | base);
}


uint8_t
GetDirToPlayer(uint16_t ix, uint8_t* other)
{
	uint8_t d = DIR_DOWN;
	uint8_t dy = static_cast<uint8_t>(Ram(PlayerY) - A(ix, ACTOR::Y));
	if (Ram(PlayerY) < A(ix, ACTOR::Y)) {
		d--;
		dy = static_cast<uint8_t>(-dy);
	}
	uint8_t e = DIR_RIGHT;
	uint8_t dx = static_cast<uint8_t>(Ram(PlayerX) - A(ix, ACTOR::X));
	if (Ram(PlayerX) < A(ix, ACTOR::X)) {
		e--;
		dx = static_cast<uint8_t>(-dx);
	}
	if (dx < dy) {
		*other = e;
		return d;
	}
	*other = d;
	return e;
}


uint8_t
GetOppositeDir(uint8_t direction)
{
	return static_cast<uint8_t>(((direction - 1) ^ 1) + 1);
}


void
GetSinCos(uint8_t angle, uint8_t* sine, uint8_t* cosine)
{
	SetBanks_4_5_6();
	uint8_t e = angle & 0x3f;
	uint8_t c = static_cast<uint8_t>(0x3f - e);
	*sine = Mem(static_cast<uint16_t>(SYM(SinTable) + e));
	*cosine = Mem(static_cast<uint16_t>(SYM(SinTable) + c));
	SetBanks1_2_3();
}


void
GetFlameSinCos(uint16_t ix, uint8_t* sine, uint8_t* cosine)
{
	GetSinCos(A(ix, FIRE_TROPPER::ANGLE), sine, cosine);
}


uint16_t
ChangeSignHL(uint16_t value)
{
	return static_cast<uint16_t>(-value);
}


uint16_t
CalcShot(uint16_t ix, uint16_t* speedY)
{
	return CalcShot2(ix, 0x80, speedY);
}


uint16_t
CalcShot2(uint16_t ix, uint8_t speed, uint16_t* speedY)
{
	uint8_t y = A(ix, ACTOR::Y);
	uint8_t x = A(ix, ACTOR::X);
	Ram(ShotSpeed) = static_cast<uint8_t>(Ram(Dificulty) * 8 + speed);
	CalcQuadrantDegree(x, y);
	uint8_t e = Ram(QuadranDegree);
	uint8_t c = static_cast<uint8_t>(0x3f - e);
	SetBanks_4_5_6();
	uint8_t sine = Mem(static_cast<uint16_t>(SYM(SinTable) + e));
	Ram(TmpShotCos) = Mem(static_cast<uint16_t>(SYM(SinTable) + c));
	SetBanks1_2_3();
	uint16_t sy = CalShootSpeed(sine);
	if (Ram(ShotDirectionV) != 0)
		sy = ChangeSignHL(sy);
	SetRam16(TmpShootSpeedY, sy);
	uint16_t sx = CalShootSpeed(Ram(TmpShotCos));
	if (Ram(ShotDirectionH) != 0)
		sx = ChangeSignHL(sx);
	*speedY = Ram16(TmpShootSpeedY);
	return sx;
}


// Angle to the player from distance blocks of 32 pixels.
void
CalcQuadrantDegree(uint8_t x, uint8_t y)
{
	Ram(TempData) = Ram(PlayerY);
	Ram(static_cast<uint16_t>(TempData + 1)) = Ram(PlayerX);
	Ram(ShotDirectionV) = 0;
	uint8_t dy = static_cast<uint8_t>(Ram(TempData) - y);
	if (Ram(TempData) < y) {
		dy = static_cast<uint8_t>(-dy);
		Ram(ShotDirectionV)++;
	}
	Ram(ShotDirectionH) = 0;
	uint8_t e = static_cast<uint8_t>((dy & 0xe0) >> 2);
	uint8_t target = Ram(static_cast<uint16_t>(TempData + 1));
	uint8_t dx = static_cast<uint8_t>(target - x);
	if (target < x) {
		dx = static_cast<uint8_t>(-dx);
		Ram(ShotDirectionH)++;
	}
	uint8_t index = static_cast<uint8_t>((dx >> 5) + e);
	uint16_t hl = static_cast<uint16_t>(SYM(QuadrantDegrees) + index);
	SetBanks_4_5_6();
	Ram(QuadranDegree) = Mem(hl);
	SetBanks1_2_3();
}


uint16_t
CalShootSpeed(uint8_t factor)
{
	uint32_t product = static_cast<uint32_t>(Multiply8(Ram(ShotSpeed), factor)) << 3;
	return static_cast<uint16_t>(product >> 8);
}


uint16_t
Multiply8(uint8_t a, uint8_t b)
{
	return static_cast<uint16_t>(a * b);
}


void
InitDummy(uint16_t)
{
}


void
InitMetalGear(uint16_t ix)
{
	SetBankInA0(0x0c);
	InitMetalGear_(ix);
	SetBanks1_2_3();
}


// Laser beams of rooms 24, 25 and 72 (not in an alert). Entries of 7
// bytes: status, Y, X, buffer Y, buffer X, length, vertical; the
// background under each beam is saved to page 1.
void
InitLaserRoom(uint16_t ix)
{
	if (Ram(AlertMode) != 0) {
		DismissActor(ix);
		return;
	}
	uint8_t room = Ram(Room);
	uint16_t de = room == 24 ? SYM(LasersRoom24) : room == 25 ? SYM(LasersRoom25)
		: SYM(LasersRoom72);
	SetBanks_A_B_C();
	uint16_t hl = EnemyList;
	uint8_t count = Mem(de);
	Ram(NumEnemies) = count;
	de++;
	int n = count == 0 ? 256 : count;
	for (int i = 0; i < n; i++) {
		uint16_t actor = hl;
		uint16_t data = de;
		uint8_t status = Mem(data);
		uint8_t y = Mem(static_cast<uint16_t>(data + 1));
		uint8_t x = Mem(static_cast<uint16_t>(data + 2));
		uint8_t dy = Mem(static_cast<uint16_t>(data + 3));
		uint8_t dx = Mem(static_cast<uint16_t>(data + 4));
		uint8_t length = Mem(static_cast<uint16_t>(data + 5));
		uint8_t vertical = Mem(static_cast<uint16_t>(data + 6));
		Ram(actor) = ID_LASER;
		A(actor, 1) = status;
		Ram(TempData) = status;
		A(actor, 2) = 0;
		A(actor, 3) = y;
		A(actor, 4) = 0;
		A(actor, 5) = x;
		Ram(static_cast<uint16_t>(TempData + 1)) = y;
		Ram(static_cast<uint16_t>(TempData + 2)) = x;
		A(actor, 6) = 0;
		A(actor, 13) = 1;
		A(actor, 14) = status;
		A(actor, 16) = 0;
		A(actor, 19) = dy;
		A(actor, 20) = dx;
		Ram(static_cast<uint16_t>(TempData2 + 1)) = dy;
		Ram(static_cast<uint16_t>(TempData2 + 2)) = dx;
		A(actor, 21) = length;
		A(actor, 22) = vertical;
		uint8_t nx = length, ny = 2;
		if ((vertical & 1) == 0) {
			nx = 2;
			ny = length;
		}
		VDP_Copy_Byte(x, y, 4, dx, dy, nx, ny);
		de = static_cast<uint16_t>(data + 7);
		hl = static_cast<uint16_t>(actor + 0x80);
	}
	SetBanks1_2_3();
}
