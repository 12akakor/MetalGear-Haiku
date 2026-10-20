/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/camera.asm: surveillance cameras that sweep
// along a path and raise the alarm, and the laser cameras that move from
// side to side and fire at Snake.
#include "Cameras.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "Guards.h"
#include "Random.h"
#include "Rom.h"
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
Add16(uint16_t address, int n)
{
	return static_cast<uint16_t>(address + n);
}


void
InitCamera2(uint16_t ix, uint8_t id)
{
	A(ix, ACTOR::IDX_SAME_ID) = CountEnemyType(id);
	A(ix, ACTOR::Wait) = RefreshR();

	// CPIR over the room list: C counts down from 11 (0 if not found)
	uint8_t room = Ram(Room);
	uint8_t c = 11;
	uint16_t rooms = SYM(RoomsWithCamera);
	for (int i = 0; i < 11; i++) {
		c--;
		if (Mem(Add16(rooms, i)) == room)
			break;
	}
	uint16_t types = Mem16(Add16(SYM(RoomCamTypes), c * 2));
	uint8_t direction = Mem(Add16(types, static_cast<uint8_t>(A(ix, ACTOR::IDX_SAME_ID) - 1)));
	A(ix, ACTOR::BASE_SPR_ID) = direction;
	A(ix, ACTOR::SpriteId) = static_cast<uint8_t>(direction + 0x1c);
	A(ix, ACTOR::Moving)++;
	GetPathPoints(ix, A(ix, ACTOR::IDX_SAME_ID));
}


void
CameraMove(uint16_t ix)
{
	if (Ram(AlertMode) != 0) {
		RenderCamera(ix);	// drawn into the room in an alert
		return;
	}
	uint8_t direction = A(ix, ACTOR::BASE_SPR_ID);
	uint16_t offsets = Add16(SYM(CameraDrawOffsets), static_cast<uint8_t>(direction * 2));
	uint8_t y = static_cast<uint8_t>(A(ix, ACTOR::Y) + Mem(offsets));
	uint8_t x = static_cast<uint8_t>(A(ix, ACTOR::X) + Mem(Add16(offsets, 1)));
	ChkSeePlayer(ix, direction, x, y);
	if (A(ix, ACTOR::SEE_PLAYER_F) != 0) {
		A(ix, ACTOR::Moving) = 0;
		A(ix, ACTOR::Wait) = 0x20;	// blinks red
		NextActorStatus(ix);
		return;
	}
	if (A(ix, ACTOR::Moving) != 0) {
		ChkReachPoint(ix);
		return;
	}
	if (--A(ix, ACTOR::Wait) != 0)
		return;
	A(ix, ACTOR::Moving) = 1;
	A(ix, ACTOR::Wait) = RefreshR();
	SetDirToPoint(ix);
}


void
CamAlertAnim(uint16_t ix)
{
	if (--A(ix, ACTOR::Wait) == 0) {
		RenderCamera(ix);
		return;
	}
	if ((A(ix, ACTOR::Wait) & 4) != 0)
		SetSprColor(ix, 4, 0x4b);	// red
	else
		SetSprColor(ix, 2, 0x4d);
}


// The laser camera sees Snake below it: it stops and fires. Returns the
// original's A: #FF when it fires.
uint8_t
LaserCamChkShot(uint16_t ix)
{
	uint8_t playerY = Ram(PlayerY);
	if (playerY < A(ix, ACTOR::Y))
		return playerY;	// behind the camera
	uint8_t a = static_cast<uint8_t>(Ram(PlayerX) - 4);
	uint8_t x = A(ix, ACTOR::X);
	if (a >= x)
		return a;
	a = static_cast<uint8_t>(a + 8);
	if (a < x)
		return a;
	if (A(ix, ACTOR::Status) == 0)
		NextActorStatus(ix);
	A(ix, ACTOR::LASER_WAIT) = 0x20;
	SetActorSpeedX(ix, 0);
	AddEnemy(ID_LASER_SHOT, A(ix, ACTOR::Y), A(ix, ACTOR::X));
	return 0xff;
}


void
CameraChkContinue(uint16_t ix)
{
	uint8_t x = A(ix, ACTOR::X);
	if (Ram(PlayerX) == x) {
		SetActorSpeedX(ix, 0);
		return;
	}
	uint16_t speed = A(ix, ACTOR::DestinationX) < x ? 0xff00 : 0x0100;
	A(ix, ACTOR::KO_POINTER_H) = 0;
	SetActorSpeedX(ix, speed);
}


// After a shot: the camera follows Snake for a while (the original's
// logic here is odd).
void
LaserCameraShot(uint16_t ix)
{
	ChkReachPoint(ix);
	if (A(ix, ACTOR::LASER_WAIT) != 0) {
		if (--A(ix, ACTOR::LASER_WAIT) != 0)
			return;
	}
	if (static_cast<uint8_t>(LaserCamChkShot(ix) + 1) == 0)
		return;	// fires again
	uint8_t x = A(ix, ACTOR::X);
	uint8_t distance = static_cast<uint8_t>(Ram(PlayerX) - x);
	if (Ram(PlayerX) < x)
		distance = static_cast<uint8_t>(-distance);
	if (distance >= 0x60) {
		GetPathPoints(ix, A(ix, ACTOR::IDX_SAME_ID));
		ResetActorStatus(ix);	// back on its path
		return;
	}
	if (A(ix, ACTOR::KO_POINTER_H) != 0) {
		CameraChkContinue(ix);
		return;
	}
	// X limits of the cameras of room 115, in the bytes after the JP at
	// LaserCameraShot9
	uint16_t limits = Add16(SYM(LaserCameraShot9) + 1,
		static_cast<uint8_t>(A(ix, ACTOR::IDX_SAME_ID) * 2));
	uint8_t c;
	if (x == Mem(limits))
		c = 1;
	else if (x == Mem(Add16(limits, 1)))
		c = 0xff;
	else {
		CameraChkContinue(ix);
		return;
	}
	A(ix, ACTOR::KO_POINTER_H) = c;
	SetActorSpeedX(ix, 0);
}

}	// namespace


void
InitCamera(uint16_t ix)
{
	InitCamera2(ix, ID_CAMERA);
}


void
InitCameraLaser(uint16_t ix)
{
	if (Ram(Room) == 118 && Ram(MetalGear_KO) != 0) {
		DismissActor0(ix);	// off after Metal Gear is destroyed
		return;
	}
	A(ix, ACTOR::COLLISION_CFG) = 0;
	InitCamera2(ix, ID_CAMERA_LASER);
}


void
CameraLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:
			CameraMove(ix);
			return;
		case 1:
			CamAlertAnim(ix);
			return;
		case 2:
			return;
		default:
			if (gUnported == NULL)
				gUnported = "CameraLogic: bad status";
			return;
	}
}


void
LaserCameraLogic(uint16_t ix)
{
	switch (A(ix, ACTOR::Status)) {
		case 0:
			ChkReachPoint(ix);
			LaserCamChkShot(ix);
			return;
		case 1:
			LaserCameraShot(ix);
			return;
		case 2:
			return;
		default:
			if (gUnported == NULL)
				gUnported = "LaserCameraLogic: bad status";
			return;
	}
}
