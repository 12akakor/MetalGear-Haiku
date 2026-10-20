/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/elevatorroom.asm.
#include "ElevatorRoom.h"

#include "Common.h"
#include "GfxBasic.h"
#include "Player.h"
#include "Rom.h"
#include "Structures.h"


using namespace var;


namespace {

inline uint16_t
IncLow(uint16_t address)
{
	return static_cast<uint16_t>((address & 0xff00) | ((address + 1) & 0xff));
}


void
ElevatorNextStatus()
{
	Ram(ElevatorStatus)++;
}


void
ElevatorNextStatus_()
{
	SetElevatorSpr();
	ElevatorNextStatus();
}


// The elevator moves up or down and stops at the floors; in the long
// shafts it goes on while the direction is held.
void
MoveElevator()
{
	GetDirLeftRight();
	SetBanks_1_2_X();
	uint8_t room = Ram(Room);
	uint8_t controls = Ram(ControlsHold);
	if (Ram(ElevatorDir) == 1) {
		Ram(PlayerY)--;
		uint8_t y = --Ram(ElevatorY);
		if (y < 24) {
			Ram(ElevatorStatus) += 2;	// leaves the room
			return;
		}
		if ((room == 248 || room == 249 || room == 250) && (controls & 1) != 0) {
			SetElevatorSpr();
			return;
		}
		if (y == 0x38) {
			ElevatorNextStatus_();
			return;
		}
		if (room == 247 && (controls & 1) != 0) {
			SetElevatorSpr();
			return;
		}
		if (y == 0x78 || y == 0xb8)
			ElevatorNextStatus_();
		else
			SetElevatorSpr();
		return;
	}
	Ram(PlayerY)++;
	uint8_t y = ++Ram(ElevatorY);
	if (y >= 208) {
		Ram(ElevatorStatus) += 2;
		return;
	}
	if ((room == 247 || room == 248 || room == 249) && (controls & 2) != 0) {
		SetElevatorSpr();
		return;
	}
	if (y == 0xb8) {
		ElevatorNextStatus_();
		return;
	}
	if (room == 0xfa && (controls & 1) != 0) {
		SetElevatorSpr();
		return;
	}
	if (y == 0x78 || y == 0x38)
		ElevatorNextStatus_();
	else
		SetElevatorSpr();
}

}	// namespace


void
ElevatorRoomLogic()
{
	uint8_t status = Ram(ElevatorStatus);
	if (status == 1) {
		// at a floor
		Ram(GameMode) = 0;
		uint8_t controls = Ram(ControlsHold) & 0x0c;
		uint8_t direction = 0;
		if (controls != 0)
			direction = (controls & 4) == 0 ? 3 : 4;
		Ram(PlayerDirectionNew) = direction;
		return;
	}
	if (status == 2) {
		Ram(NextRoomDirect) = Ram(ElevatorDir);
		Ram(GameMode) = 1;
		return;
	}
	MoveElevator();
}


// Twelve sprites, and their colors line by line.
void
SetElevatorSpr()
{
	uint16_t hl = SYM(SprElevatorDat);
	uint16_t de = SprElevatorAttr;
	for (int i = 0; i < 12; i++) {
		Ram(de) = static_cast<uint8_t>(Ram(ElevatorY) + Mem(hl++));
		de = IncLow(de);
		Ram(de) = static_cast<uint8_t>(Ram(ElevatorX) + Mem(hl++));
		de = IncLow(de);
		Ram(de) = Mem(hl++);	// LDI
		de++;
		hl++;
		de = IncLow(de);
	}
	de = SprElevatorCol;
	hl = static_cast<uint16_t>(SYM(SprElevatorDat) + 3);
	for (int i = 0; i < 12; i++) {
		uint8_t color = Mem(hl);
		for (int line = 0; line < 16; line++)
			Ram(de++) = color;
		hl = static_cast<uint16_t>(hl + 4);
	}
}
