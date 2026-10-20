/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/guardshot.asm, shottoplayer.asm,
// bullethv.asm, snoringsymbol.asm, bigexplosion.asm, tankshot.asm,
// tankshell.asm, shellspawner.asm, gas.asm, lasershot.asm and flame.asm.
#include "Shots.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Random.h"
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


void
AimAtPlayer(uint16_t ix)
{
	A(ix, ACTOR::Moving)++;
	A(ix, ACTOR::COLLISION_CFG) = 1;
	A(ix, ACTOR::SpriteId) = 0x72;	// bullet
	uint16_t speedY;
	uint16_t speedX = CalcShot2(ix, 0x90, &speedY);
	SetActorSpeed(ix, speedX, speedY);
}


void
InitBulletVert2(uint16_t ix, uint16_t speedX, uint16_t speedY)
{
	SetActorSpeed(ix, speedX, speedY);
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::COLLISION_CFG) = 1;
	A(ix, ACTOR::SpriteId) = 0x72;
	SetSoundEntryChk(5);
}


// The laser is a row of sprites: 'length' picks the one whose color is
// set.
void
LaserSetSprColor2(uint16_t ix, uint8_t length, uint8_t color)
{
	uint16_t hl = Add16(ix, ACTOR::NumSprites);
	int n = length == 0 ? 256 : length;
	for (int i = 0; i < n; i++)
		hl = Add16(hl, 5);
	Ram(hl) = color;
}


void
LaserSetSprColor(uint16_t ix)
{
	LaserSetSprColor2(ix, static_cast<uint8_t>(0x0b - A(ix, ACTOR::COUNTER)), 8);	// red
}


void
LaserIncrease(uint16_t ix)
{
	if (--A(ix, ACTOR::LASER_WAIT) != 0)
		return;
	A(ix, ACTOR::LASER_WAIT) = 1;
	if (--A(ix, ACTOR::COUNTER) != 0) {
		LaserSetSprColor(ix);
		return;
	}
	A(ix, ACTOR::COUNTER) = 0x0b;
	A(ix, ACTOR::LASER_WAIT) = 1;
	NextActorStatus(ix);
}


void
LaserDecrease(uint16_t ix)
{
	// (!?) the original picks white or red here, but LaserSetSprColor
	// always sets red
	LaserSetSprColor(ix);
	if (--A(ix, ACTOR::LASER_WAIT) != 0)
		return;
	A(ix, ACTOR::LASER_WAIT) = 1;
	if (--A(ix, ACTOR::COUNTER) == 0) {
		DismissActor0(ix);
		return;
	}
	LaserSetSprColor2(ix, static_cast<uint8_t>(0x0b - A(ix, ACTOR::COUNTER)), 0);	// erased
}

}	// namespace


// --- Bullets ----------------------------------------------------------------

void
InitGuardShot(uint16_t ix)
{
	AimAtPlayer(ix);
	SetSoundEntry(5);
}


void
InitShotToPlayer(uint16_t ix)
{
	AimAtPlayer(ix);
}


// Flies across the screen, away from the side it starts on.
void
InitBulletHor(uint16_t ix)
{
	uint8_t random = static_cast<uint8_t>(RefreshR() & 0x7f);
	uint16_t speedY = static_cast<uint16_t>(0xffc0 + random);
	uint16_t speedX = (A(ix, ACTOR::X) & 0x80) != 0 ? 0xfb00 : 0x0500;
	InitBulletVert2(ix, speedX, speedY);
}


void
InitBulletVert(uint16_t ix)
{
	uint8_t random = static_cast<uint8_t>(RefreshR() & 0x7f);
	uint16_t speedX = static_cast<uint16_t>(0xffc0 + random);
	uint16_t speedY = (A(ix, ACTOR::Y) & 0x80) != 0 ? 0xfb00 : 0x0500;
	InitBulletVert2(ix, speedX, speedY);
}


// --- Zzz over a sleeping guard, explosions ---------------------------------

void
InitSnoringSymbol(uint16_t ix)
{
	A(ix, ACTOR::ANIM_CNT) = RefreshR();
	A(ix, ACTOR::COLLISION_CFG) = 0;
	A(ix, ACTOR::COUNTER) = 0xe0;
	A(ix, ACTOR::SpriteId) = 0x3a;
}


void
SnoringSymbolLogic(uint16_t ix)
{
	if (Ram(AlertMode) != 0 || Ram(Add16(ix, -0x80)) == 0) {
		DismissActor0(ix);	// the guard is awake or gone
		return;
	}
	AnimZzzSign(ix, 0x0f, 0x3a);
	if (Ram(Room) == 140)
		return;	// these guards do not wake up
	if (--A(ix, ACTOR::COUNTER) != 0)
		return;
	if (Ram(SleepyGuardFlag) != 0)
		return;
	DismissActor0(ix);
}


void
BigExplosionLogic(uint16_t ix)
{
	if (--A(ix, ACTOR::Wait) == 0) {
		DismissActor0(ix);
		return;
	}
	uint8_t wait = A(ix, ACTOR::Wait);
	uint8_t sprite = wait >= 0x0d ? 0x73 : wait >= 8 ? 0x74 : 0x75;
	A(ix, ACTOR::SpriteId) = sprite;
	uint16_t colors = GetPointerDE2A(SYM(BigExplSprCols), static_cast<uint8_t>(sprite - 0x73));
	A(ix, ACTOR::Spr1Color) = static_cast<uint8_t>(colors);
	A(ix, ACTOR::Spr2Color) = static_cast<uint8_t>(colors >> 8);
	A(ix, ACTOR::Spr3Color) = static_cast<uint8_t>(colors);
	A(ix, ACTOR::Spr4Color) = static_cast<uint8_t>(colors >> 8);
}


// --- Tank shots and shells ---------------------------------------------------

void
InitTankShellBoss(uint16_t ix)
{
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::COLLISION_CFG) = 1;
	A(ix, ACTOR::SpriteId) = 0x7e;
	ResetActorSpeed(ix);
	A(ix, ACTOR::SpeedY) = 6;
	SetSoundEntryChk(0x13);
}


void
InitTankShot(uint16_t ix)
{
	A(ix, TANK_SHOT::SpriteId) = 0x72;
	A(ix, TANK_SHOT::COLLISION_CFG) = 1;
	ResetActorSpeed(ix);
}


void
TankShotLogic(uint16_t ix)
{
	if ((A(ix, TANK_SHOT::Status) & 1) != 0)
		return;
	A(ix, TANK_SHOT::Status) = 1;
	A(ix, TANK_SHOT::Moving) = 1;
	A(ix, TANK_SHOT::SpeedY) = 6;
	A(ix, TANK_SHOT::SpeedX) = static_cast<uint8_t>(A(ix, TANK_SHOT::SpeedXUnsigned) - 2);
	SetSoundEntryChk(5);
}


// Shells falling in the desert.
void
InitTankShell(uint16_t ix)
{
	A(ix, SHELL_DESERT::Moving) = 1;
	A(ix, SHELL_DESERT::COLLISION_CFG) = 1;
	A(ix, SHELL_DESERT::SpriteId) = 0x7e;
	uint8_t time = static_cast<uint8_t>(((RefreshR() ^ Ram(TickCounter)) & 0x0f) + 0x0a);
	A(ix, SHELL_DESERT::Timer) = time;
	A(ix, SHELL_DESERT::LeftRight) = time & 1;
	ResetActorSpeed(ix);
	A(ix, SHELL_DESERT::SpeedY) = 6;
}


void
ThankShellLogic(uint16_t ix)
{
	uint16_t speed = (A(ix, SHELL_DESERT::LeftRight) & 1) != 0 ? 0xffe8 : 0x0018;
	AddActorSpeedX(ix, speed);
	if (--A(ix, SHELL_DESERT::Timer) != 0)
		return;
	A(ix, SHELL_DESERT::Timer) = 0x12;
	A(ix, SHELL_DESERT::ID) = ID_BIG_EXPLOSION;
	A(ix, SHELL_DESERT::Moving) = 0;
	SetSoundEntryChk(0x1a);
}


void
InitSpawnTankShell(uint16_t ix)
{
	if (Ram(BossTank_KO) != 0) {
		DismissActor(ix);
		return;
	}
	A(ix, ACTOR::COLLISION_CFG) = 0;
	A(ix, ACTOR::Wait) = 0x14;
}


void
SpawnTankShell(uint16_t ix)
{
	if (--A(ix, ACTOR::Wait) != 0)
		return;
	if ((A(ix, ACTOR::Status) & 1) == 0) {
		A(ix, ACTOR::Status) = 1;
		A(ix, ACTOR::Wait) = 0x14;
		SetSoundEntryChk(0x0b);	// the shot, then the shell
		return;
	}
	A(ix, ACTOR::Status) = 0;
	A(ix, ACTOR::Wait) = 0x32;
	uint8_t x;
	if ((A(ix, ACTOR::ANIM_CNT) & 3) == 0) {
		x = Ram(PlayerX);
	} else {
		uint8_t a = static_cast<uint8_t>((RefreshR() ^ A(ix, ACTOR::ANIM_CNT)) >> 1);
		x = static_cast<uint8_t>((a >> 1) + a + 0x20);
	}
	AddEnemy(ID_TANK_SHELL_AIR, 0, x);
}


// --- Gas ----------------------------------------------------------------------

void
InitGas(uint16_t ix)
{
	A(ix, GAS::COLLISION_CFG) = 0;
	A(ix, GAS::TIMER) = RefreshR();
	A(ix, GAS::SpriteId) = 0x38;
}


void
GasLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:
			if (--A(ix, GAS::TIMER) != 0)
				return;
			A(ix, GAS::TIMER) = 0x20;
			SetSprColor(ix, 2, 0x4d);
			NextActorStatus(ix);
			return;
		case 1:
			Anim2FramesActor(ix, 7);
			if (--A(ix, GAS::TIMER) != 0)
				return;
			HideSprColor(ix);
			InitGas(ix);
			ResetActorStatus(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "GasLogic: bad status";
			return;
	}
}


// --- Laser shots of the laser cameras ------------------------------------

void
InitLaserShot(uint16_t ix)
{
	if (Ram(Room) == 111) {
		// shorter lasers
		A(ix, ACTOR::NumSprites) = 3;
		SetSpritesFree(Add16(ix, ACTOR::NumSprites + 0x10), 7);
	}
	A(ix, ACTOR::SpriteId) = 0x61;
	A(ix, ACTOR::COUNTER) = 0x0b;
	A(ix, ACTOR::LASER_WAIT) = 1;
	SetSoundEntryChk(4);
}


void
LaserShotLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:
			LaserIncrease(ix);
			return;
		case 1:
			LaserDecrease(ix);
			return;
		case 2:
			return;
		default:
			if (gUnported == NULL)
				gUnported = "LaserShotLogic: bad status";
			return;
	}
}


// --- Flames of the Fire Trooper -------------------------------------------

void
InitFlame(uint16_t ix)
{
	if (Ram(REMOVE_FLAMES) != 0) {
		DismissActor0(ix);
		return;
	}
	// (!?) counts Madnars, not flames
	if (CountEnemyType(ID_MADNAR) >= 5) {
		DismissActor0(ix);
		return;
	}
	A(ix, ACTOR::SpriteId) = 0x10;
	A(ix, ACTOR::ANIM_CNT) = RefreshR();
	A(ix, ACTOR::Moving)++;
	SetActorSpeed(ix, 0, 0);
}


void
FlamesLogic(uint16_t ix)
{
	if (Ram(FireTrooper_KO) != 0) {
		RemoveActor_(ix);
		return;
	}
	if (CountEnemyType(ID_FIRE_TROOPER) == 0) {
		DismissActor0(ix);
		return;
	}
	Anim2FramesActor(ix, 7);
	if (A(ix, ACTOR::Spr1Color) != 0)
		return;
	uint8_t x = A(ix, ACTOR::X);
	if (A(ix, ACTOR::NegativeGap) != 0) {
		if (x >= 0x80)
			return;
	} else if (x < 0x80) {
		return;
	}
	if (A(ix, ACTOR::HiddenFlame) != 0)
		return;
	SetSprColor(ix, 8, 0x46);
}


void
FlameOutScreen(uint16_t ix)
{
	A(ix, ACTOR::NegativeGap) = static_cast<uint8_t>(A(ix, ACTOR::GAP_X) >> 7);
	SetSprColor(ix, 0, 0);
}
