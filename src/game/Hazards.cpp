/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/actors/bridge.asm, rollingbarrels.asm and
// bulldozer.asm.
#include "Hazards.h"

#include "Actors.h"
#include "Common.h"
#include "GameMain.h"
#include "Rom.h"
#include "RoomActors.h"
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


// The bridge control finds the parts of the bridge and gives each its
// speed (the original saves them through the stack: last one first).
void
SetupBridges(uint16_t ix)
{
	uint16_t parts[16];
	int count = 0;
	uint16_t hl = EnemyList;
	for (int i = 0; i < 16; i++, hl = static_cast<uint16_t>(hl + 0x80)) {
		if (Ram(hl) == ID_BRIDGE)
			parts[count++] = hl;
	}
	if (count == 0) {
		NextActorStatus(ix);
		return;
	}
	hl = static_cast<uint16_t>(ix | BRIDGE::NumberOfBridges);
	Ram(hl) = static_cast<uint8_t>(count);
	hl = IncLow(hl);
	for (int i = count - 1; i >= 0; i--) {
		Ram(hl) = static_cast<uint8_t>(parts[i]);
		hl = IncLow(hl);
		Ram(hl) = static_cast<uint8_t>(parts[i] >> 8);
		hl = IncLow(hl);
	}

	uint16_t speeds = SYM(BridgesSpeeds);
	uint8_t n = A(ix, BRIDGE::NumberOfBridges);
	hl = IncLow(ix, BRIDGE::PointersToBridges);
	do {
		uint16_t part = static_cast<uint16_t>(Ram(hl) | (Ram(IncLow(hl)) << 8));
		hl = IncLow(hl, 2);
		A(ix, BRIDGE::P_NextBridge_L) = static_cast<uint8_t>(hl);
		A(ix, BRIDGE::P_NextBridge_H) = static_cast<uint8_t>(hl >> 8);
		uint16_t p = IncLow(part, BRIDGE::speedYdec);
		Ram(p) = 0;
		p = IncLow(p);
		Ram(p) = 0;	// no speed Y
		p = IncLow(p);
		for (int i = 0; i < 2; i++) {
			Ram(p) = Mem(speeds++);	// speed X
			p = IncLow(p);
		}
		hl = static_cast<uint16_t>(A(ix, BRIDGE::P_NextBridge_L)
			| (A(ix, BRIDGE::P_NextBridge_H) << 8));
	} while (--n != 0);
	NextActorStatus(ix);
}


// The parts turn round now and then.
void
BridgeCtrlLogic(uint16_t ix)
{
	if (--A(ix, BRIDGE::MOVEMENT_CNT) != 0)
		return;
	A(ix, BRIDGE::MOVEMENT_CNT) = 0x20;
	uint8_t n = A(ix, BRIDGE::NumberOfBridges);
	uint16_t hl = IncLow(ix, BRIDGE::PointersToBridges);
	do {
		uint16_t part = static_cast<uint16_t>(Ram(hl) | (Ram(IncLow(hl)) << 8));
		hl = IncLow(hl, 2);
		A(ix, BRIDGE::P_NextBridge_L) = static_cast<uint8_t>(hl);
		A(ix, BRIDGE::P_NextBridge_H) = static_cast<uint8_t>(hl >> 8);
		uint16_t p = IncLow(part, BRIDGE::SpeedXdec);
		uint16_t speed = static_cast<uint16_t>(Ram(p) | (Ram(IncLow(p)) << 8));
		speed = ChangeSignHL(speed);
		Ram(IncLow(p)) = static_cast<uint8_t>(speed >> 8);
		Ram(p) = static_cast<uint8_t>(speed);
		hl = static_cast<uint16_t>(A(ix, BRIDGE::P_NextBridge_L)
			| (A(ix, BRIDGE::P_NextBridge_H) << 8));
	} while (--n != 0);
}


void
ChangeBarrelDir(uint16_t ix, uint8_t x, uint8_t direction, uint16_t speed)
{
	A(ix, ROLLING_BARREL::X) = x;
	A(ix, ROLLING_BARREL::Direction) = direction;
	SetActorSpeedX(ix, speed);
	SetSoundEntryChk(0x1d);	// hits the wall
}


void
ChkBarrelBounce(uint16_t ix)
{
	uint8_t x = A(ix, ROLLING_BARREL::X);
	if (x >= 200) {
		ChangeBarrelDir(ix, 199, DIR_DOWN, 0xff80);
		return;
	}
	if (x >= 56)
		return;
	ChangeBarrelDir(ix, 57, DIR_LEFT, 0x0080);
}


void
StopBulldozer(uint16_t ix)
{
	SetActorSpeedY(ix, 0);
}


void
BulldozerStop(uint16_t ix, uint16_t speed)
{
	if (--A(ix, BULLDOZER::Timer) != 0)
		return;
	SetActorSpeedY(ix, speed);
	A(ix, BULLDOZER::Timer) = 0x30;
	NextActorStatus(ix);
}

}	// namespace


// --- Moving bridges -------------------------------------------------------

void
InitBridge(uint16_t ix)
{
	A(ix, BRIDGE::MOVING)++;
	A(ix, BRIDGE::SpriteId) = 0x35;
	A(ix, BRIDGE::MOVEMENT_CNT) = 0x10;
	A(ix, BRIDGE::COLLISION_CFG) = 1;
}


void
BridgeLogic(uint16_t ix)
{
	if (A(ix, BRIDGE::Status) == 1)
		BridgeCtrlLogic(ix);
	else
		SetupBridges(ix);
}


// --- Rolling barrels --------------------------------------------------------

void
InitRollingBarrel(uint16_t ix)
{
	A(ix, ROLLING_BARREL::START_X) = Ram(PlayerX);	// (!?) not used
	SetActorSpeed(ix, Ram(PlayerX) < 0x80 ? 0x0080 : 0xff80, 0);
	A(ix, ROLLING_BARREL::MOVING)++;
	A(ix, ROLLING_BARREL::SpriteId) = 0x36;
}


void
RollingBarrelLogic(uint16_t ix)
{
	Anim2FramesActor(ix, 3);
	ChkBarrelBounce(ix);
	if (A(ix, ROLLING_BARREL::Status) != 1) {
		// rolls faster and faster
		AddActorSpeedX(ix, (A(ix, ACTOR::Direction) & 1) != 0 ? 8 : 0xfff8);
		return;
	}
	// (!?) status 1 is not used
	if (A(ix, ROLLING_BARREL::START_X) >= 0x80) {
		uint8_t x = A(ix, ROLLING_BARREL::X);
		if (x >= 200 || x < 56)
			NextActorStatus(ix);
		return;
	}
	if (Ram(PlayerY) >= 0x80)
		return;
	if (Ram(PlayerX) < A(ix, ACTOR::X)) {
		A(ix, ACTOR::Direction) = DIR_DOWN;
		SetActorSpeedX(ix, 0x80);
	} else {
		A(ix, ACTOR::Direction) = DIR_LEFT;
		SetActorSpeedX(ix, 0xff80);
	}
	NextActorStatus(ix);
}


// --- Bulldozer: moves a bit, stops, and comes on faster -----------------

void
InitBulldozer(uint16_t ix)
{
	if (Ram(Bulldozer_KO) != 0) {
		DismissActor(ix);
		return;
	}
	// (!?) the tank's flag, not used
	A(ix, BULLDOZER::KO_POINTER_L) = static_cast<uint8_t>(BossTank_KO);
	A(ix, BULLDOZER::KO_POINTER_H) = static_cast<uint8_t>(BossTank_KO >> 8);
	A(ix, BULLDOZER::Timer) = 0x30;
	A(ix, BULLDOZER::SpriteId) = 0x13;
	A(ix, BULLDOZER::Moving)++;
	SetActorSpeed(ix, 0, 0x60);
	SetBossMusic();
}


void
BulldozerLogic(uint16_t ix)
{
	if (A(ix, BULLDOZER::Y) >= 160) {
		StopBulldozer(ix);	// at the bottom of the room
		return;
	}
	switch (A(ix, BULLDOZER::Status)) {
		case 0:
		case 2:
		case 4:
			if (--A(ix, BULLDOZER::Timer) != 0)
				return;
			StopBulldozer(ix);
			A(ix, BULLDOZER::Timer) = 0x10;
			NextActorStatus(ix);
			return;
		case 1:
			BulldozerStop(ix, 0x80);
			return;
		case 3:
			BulldozerStop(ix, 0xc0);
			return;
		case 5:
			BulldozerStop(ix, 0xe0);
			return;
		case 6:
			return;
		default:
			if (gUnported == NULL)
				gUnported = "BulldozerLogic: bad status";
			return;
	}
}


void
SetBulldozerKO()
{
	Ram(Bulldozer_KO) = 1;
	SetAreaMusic();
}
