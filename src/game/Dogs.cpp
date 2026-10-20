/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/dog.asm, dogbasement.asm and dogspawner.asm.
#include "Dogs.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Guards.h"
#include "Player.h"
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


// --- Dogs that sleep, listen and chase (dog.asm) -------------------------

bool
ChkDogCollision(uint16_t ix, uint8_t direction)
{
	uint16_t offsets = GetPointerDE2A(static_cast<uint16_t>(SYM(DirCollisionPoints) - 2),
		direction);
	uint8_t y = static_cast<uint8_t>(static_cast<uint8_t>(offsets) + A(ix, ACTOR::Y));
	uint8_t x = static_cast<uint8_t>((offsets >> 8) + A(ix, ACTOR::X));
	return ChkTileCollision_(x, y, 1, direction);
}


void
AnimateRunningDog2(uint16_t ix, uint8_t direction)
{
	uint8_t sprite = static_cast<uint8_t>(direction * 2 + 0x1e);
	if ((A(ix, ACTOR::ANIM_CNT) & 4) == 0)
		sprite++;
	A(ix, ACTOR::SpriteId) = sprite;
}


void
DogMove4(uint16_t ix)
{
	A(ix, ACTOR::Wait) = static_cast<uint8_t>((GetRandom3(ix) + 5) * 4);
}


// Runs towards Snake, along the other axis if the way is blocked.
void
DogChase(uint16_t ix)
{
	uint8_t other;
	uint8_t direction = GetDirToPlayer(ix, &other);
	if (ChkDogCollision(ix, direction))
		direction = other;
	A(ix, ACTOR::Direction) = direction;
	AnimateRunningDog2(ix, direction);
	SetChaseSpeed(ix);
	DogMove4(ix);
}


void
DogSleep(uint16_t ix)
{
	if (--A(ix, ACTOR2::Wait) != 0)
		return;
	uint8_t time = static_cast<uint8_t>((GetRandom3(ix) + 5) * 4);
	A(ix, ACTOR2::Timer) = time;	// listening
	A(ix, ACTOR2::Wait) = static_cast<uint8_t>(time * 2);
	A(ix, ACTOR2::Status) = 1;
	A(ix, ACTOR2::SpriteId) = 0x15;
}


void
DogListen(uint16_t ix)
{
	if (--A(ix, ACTOR2::Timer) != 0)
		return;
	if (((RefreshR() ^ A(ix, ACTOR2::ANIM_CNT)) & 1) == 0) {
		A(ix, ACTOR2::Status) = 0;	// sleeps again
		A(ix, ACTOR2::SpriteId) = 0x14;
		return;
	}
	A(ix, ACTOR::Status) = 2;
	A(ix, ACTOR::Moving) = 1;
	DogChase(ix);
}


void
DogMove(uint16_t ix)
{
	AnimateRunningDog2(ix, A(ix, ACTOR::Direction));
	if (!ChkDogCollision(ix, A(ix, ACTOR::Direction))) {
		if (--A(ix, ACTOR::Wait) != 0)
			return;
		SetSoundEntryChk(3);	// bark
	}
	DogChase(ix);
}


// --- Basement dogs (dogbasement.asm) --------------------------------------

void
SetBaseDogSprSpeed(uint16_t ix, uint8_t direction, uint16_t speedX, uint16_t speedY)
{
	A(ix, DOG::Direction) = direction;
	A(ix, DOG::SpriteId) = static_cast<uint8_t>(direction * 2 + 0x20);
	SetActorSpeed(ix, speedX, speedY);
}


void
SetVertDogSpeed(uint16_t ix)
{
	if (Ram(PlayerY) < A(ix, DOG::Y))
		SetBaseDogSprSpeed(ix, 0, 0, 0xfd00);	// up
	else
		SetBaseDogSprSpeed(ix, 1, 0, 0x0300);	// down
}


void
SetDogHorSpeed(uint16_t ix)
{
	if (Ram(PlayerX) < A(ix, DOG::X))
		SetBaseDogSprSpeed(ix, 2, 0xfd00, 0);	// left
	else
		SetBaseDogSprSpeed(ix, 3, 0x0300, 0);	// right
}


void
SetDogSprColNxStat(uint16_t ix)
{
	NextActorStatus(ix);
	SetSprColor(ix, 0x0d, 0x0f);
}


// The way ahead is blocked: tries the next directions in turn.
void
ChkDogChgDir(uint16_t ix)
{
	Anim2FramesActor(ix, 7);
	uint8_t direction = A(ix, DOG::Direction);
	uint16_t speeds = Add16(SYM(DogSpeedDat), direction * 2);
	uint8_t y = static_cast<uint8_t>(Mem(speeds) + A(ix, DOG::Y));
	uint8_t x = static_cast<uint8_t>(Mem(Add16(speeds, 1)) + A(ix, DOG::X));
	uint8_t c = static_cast<uint8_t>(direction + 1);
	if (c >= 5)
		c = 0;
	if (!ChkTileCollision_(x, y, 2, c))
		return;
	uint8_t dogX = A(ix, DOG::X);
	uint8_t dogY = A(ix, DOG::Y);
	if (dogX >= 242 || dogX < 0x0c || dogY < 28 || dogY >= 180)
		return;	// leaving the room

	uint8_t next = (A(ix, ACTOR::Direction) & 2) != 0 ? 0 : 2;
	for (int tries = 0;; tries++) {
		if (tries > 1000) {
			// the original would loop forever here
			if (gUnported == NULL)
				gUnported = "ChkDogChgDir: no way out";
			return;
		}
		A(ix, DOG::NextDirection) = next;
		uint16_t de;
		switch (next) {
			case 0:
				de = 0x00e7;
				break;
			case 1:
				de = 0x0019;
				break;
			case 2:
				de = 0xe700;
				break;
			default:
				de = 0x1900;
				break;
		}
		x = static_cast<uint8_t>((de >> 8) + A(ix, DOG::X));
		y = static_cast<uint8_t>(static_cast<uint8_t>(de) + A(ix, DOG::Y));
		c = static_cast<uint8_t>(next + 1);
		if (c >= 5)
			c = DIR_UP;
		if (!ChkTileCollision_(x, y, 2, c))
			break;
		next = static_cast<uint8_t>((A(ix, DOG::NextDirection) + 1) & 3);	// ChangeDogDir
	}

	direction = static_cast<uint8_t>(c - 1);
	A(ix, DOG::Direction) = direction;
	switch (direction) {
		case 0:
			SetActorSpeed(ix, 0, 0xfd00);
			break;
		case 1:
			SetActorSpeed(ix, 0, 0x0300);
			break;
		case 2:
			SetActorSpeed(ix, 0xfd00, 0);
			break;
		default:
			SetActorSpeed(ix, 0x0300, 0);
			break;
	}
	uint8_t frame = A(ix, DOG::SpriteId) & 1;
	A(ix, DOG::SpriteId) = static_cast<uint8_t>(A(ix, DOG::Direction) * 2 + 0x20 + frame);
}


// Near Snake: the dog chases him.
void
ChkDogNearPlayer(uint16_t ix)
{
	uint8_t playerX = Ram(PlayerX);
	uint8_t playerY = Ram(PlayerY);
	uint8_t x = A(ix, DOG::X);
	uint8_t y = A(ix, DOG::Y);
	if ((A(ix, DOG::Direction) & 2) == 0) {
		uint8_t distance = static_cast<uint8_t>(playerX - x);
		if (playerX < x)
			distance = static_cast<uint8_t>(-distance);
		if (distance >= 0x30)
			return;
		uint8_t a = static_cast<uint8_t>(playerY - 0x18);
		if (a >= y)
			return;
		a = static_cast<uint8_t>(a + 0x30);
		if (a < y)
			return;
	} else {
		uint8_t distance = static_cast<uint8_t>(playerY - y);
		if (playerY < y)
			distance = static_cast<uint8_t>(-distance);
		if (distance >= 0x30)
			return;
		uint8_t a = static_cast<uint8_t>(playerX - 0x20);
		if (a >= x)
			return;
		a = static_cast<uint8_t>(a + 0x40);
		if (a < x)
			return;
	}
	A(ix, DOG::Timer) = 0x18;
	NextActorStatus(ix);
}

}	// namespace


void
SetChaseSpeed(uint16_t ix)
{
	uint16_t speeds = GetPointerDE2A(static_cast<uint16_t>(SYM(DogSpeeds) - 2),
		A(ix, ACTOR::Direction));
	A(ix, ACTOR::SpeedY) = static_cast<uint8_t>(speeds);
	A(ix, ACTOR::SpeedX) = static_cast<uint8_t>(speeds >> 8);
}


void
InitDog(uint16_t ix)
{
	A(ix, ACTOR2::Wait) = static_cast<uint8_t>(((RefreshR() ^ Ram(TickCounter)) & 3) * 8 + 0x20);
	A(ix, ACTOR2::SpriteId) = 0x14;	// lying
	A(ix, ACTOR2::COLLISION_CFG) = 3;
	ResetActorSpeed(ix);
}


void
DogLogic(uint16_t ix)
{
	switch (A(ix, ACTOR2::Status)) {
		case 0:
			DogSleep(ix);
			return;
		case 1:
			DogListen(ix);
			return;
		case 2:
			DogMove(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "DogLogic: bad status";
			return;
	}
}


void
InitDogBasement(uint16_t ix)
{
	if (CountEnemyType(ID_DOG_BASEMENT) >= 3) {
		DismissActor0(ix);
		return;
	}
	A(ix, DOG::Moving)++;
	if (Ram(NoEnemiesRoom) == 0) {
		// a dog that comes running into the room
		SetDogSprColNxStat(ix);
		if ((static_cast<uint8_t>(Ram(NextRoomDirect) - 1) & 2) != 0)
			SetDogHorSpeed(ix);
		else
			SetVertDogSpeed(ix);
		return;
	}
	A(ix, DOG::Timer) = 0x40;
	A(ix, DOG::SpriteId) = 0x14;	// lying
	A(ix, DOG::ANIM_CNT) = RefreshR();
	ResetActorSpeed(ix);
}


void
DogBasementLogic(uint16_t ix)
{
	if ((A(ix, DOG::Direction) & 2) == 0 && A(ix, DOG::X) <= 0x20)
		A(ix, DOG::X) = 0x20;	// left limit
	switch (A(ix, DOG::Status)) {
		case 0:	// sleeps
			Anim2FramesActor(ix, 0x1f);
			if (--A(ix, DOG::Timer) != 0)
				return;
			SetVertDogSpeed(ix);
			SetDogSprColNxStat(ix);
			return;
		case 1:	// runs
			ChkDogNearPlayer(ix);
			ChkDogChgDir(ix);
			return;
		case 2:	// chases Snake
			ChkDogChgDir(ix);
			if (--A(ix, DOG::Timer) != 0)
				return;
			SetSoundEntryChk(3);	// bark
			A(ix, DOG::Timer) = 0x18;
			if ((A(ix, DOG::Direction) & 2) == 0)
				SetVertDogSpeed(ix);
			else
				SetDogHorSpeed(ix);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "DogBasementLogic: bad status";
			return;
	}
}


// Dogs from the previous room follow Snake.
void
InitSpawnDog(uint16_t ix)
{
	A(ix, DOG::SpawnX) = Ram(PlayerX);
	A(ix, DOG::SpawnY) = Ram(PlayerY);
	A(ix, DOG::Timer) = 0x30;
	A(ix, DOG::COLLISION_CFG) = 0;
}


void
SpawnDogLogic(uint16_t ix)
{
	if (--A(ix, DOG::Timer) != 0)
		return;
	if (Ram(NumBasementDogs) == 0) {
		DismissActor0(ix);
		return;
	}
	Ram(NumBasementDogs)--;
	A(ix, DOG::Timer) = 0x30;
	AddEnemy(ID_DOG_BASEMENT, A(ix, DOG::SpawnY), A(ix, DOG::SpawnX));
}
