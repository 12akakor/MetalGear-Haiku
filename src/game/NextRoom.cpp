/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Changing rooms: by the sides, doors, elevators, the parachute jump from
// the roof; where Snake appears (logic/nextroom.asm).
#include "NextRoom.h"

#include "Actors.h"
#include "Common.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "Rom.h"
#include "Sprites.h"
#include "Structures.h"


using namespace var;


static void
NormalGameMode()
{
	Ram(GameMode) = 0;
}


static void
SetLandingRoom()
{
	uint8_t from = Ram(JumpRoomId);
	Ram(Room) = from == 1 ? 5 : from == 2 ? 6 : 10;
	Ram(PreviousRoom) = 204;
	Ram(JumpRoomId) = 0;
	NormalGameMode();
}


// Jumping from the roof (or the bridges): with the parachute down along
// the brick wall (room 204), without it Snake dies.
static void
ChkParachute(uint8_t from)
{
	if (Ram(SelectedItem) != SELECTED_PARACHUTE) {
		SetLandingRoom();
		Ram(PreviousRoom) = 0;
		Ram(IdDoorEnter) = 0xff;
		DecrementLife_B(0xff);
		return;
	}
	Ram(Room) = 204;
	Ram(PreviousRoom) = from;
	Ram(HeightParachuteCnt) = 2;
	StopAlert();
	NormalGameMode();
}


static uint16_t
FindDoor(uint8_t id)
{
	uint16_t hl = DoorsList;
	for (int i = 0; i < 256 && Ram(hl) != id; i++)
		hl = static_cast<uint16_t>(hl + 0x10);
	return hl;
}


static void
SetElevatorPosY()
{
	uint8_t playerY, elevatorY;
	GetElevatorPosY(&playerY, &elevatorY);
	SetRam16(PlayerYdec, static_cast<uint16_t>(playerY << 8));
	SetRam16(PlayerXdec, static_cast<uint16_t>((0xd8 << 8) | 0));
	Ram(ElevatorY) = elevatorY;
	Ram(static_cast<uint16_t>(ElevatorY + 1)) = 0x70;
	SetHUDSprMask();
	NormalGameMode();
}


static void
ChkDoorDestination()
{
	uint8_t id = Ram(IdDoorEnter);
	if (id == 0x91) {
		ChkParachute(117);	// from the roof
		return;
	}
	if (id == 0x5b) {
		ChkParachute(45);	// from the bridge
		return;
	}
	if (id == 0x81) {
		ChkParachute(46);
		return;
	}
	uint8_t destination = Ram(static_cast<uint16_t>(FindDoor(id) + 15));
	Ram(PreviousRoom) = Ram(Room);
	Ram(Room) = destination;
	if (destination >= 0xf0) {
		SetElevatorPosY();
		return;
	}
	NormalGameMode();
}


static void
SetNextRoomElev()
{
	uint8_t next = GetNextRoomNum();
	uint8_t previous = Ram(Room);
	Ram(Room) = next;
	Ram(PreviousRoom) = previous;
	GetElevatorRoomDat();
	uint8_t y = Ram(NextRoomDirect) == 1 ? 0xd0 : 0x18;
	Ram(ElevatorY) = y;
	SetRam16(PlayerYdec, static_cast<uint16_t>(static_cast<uint8_t>(y - 4) << 8));
	SetHUDSprMask();
	Ram(ElevatorStatus) = 0;
	Ram(GameMode) = GAME_MODE_ELEVATOR;
}


void
SetNextRoom()
{
	if (Ram(Room) == 204) {
		// NextParachuteRoom: two floors of falling
		if (--Ram(HeightParachuteCnt) == 0) {
			SetLandingRoom();
			return;
		}
		Ram(Room) = 204;
		Ram(PreviousRoom) = 204;
		NormalGameMode();
		return;
	}
	if (Ram(IdDoorEnter) != 0) {
		ChkDoorDestination();
		return;
	}
	if (Ram(NextRoomDirect) == 0)
		return;
	uint8_t next = GetNextRoomNum();
	if (next >= 0xf0) {
		SetNextRoomElev();
		return;
	}
	uint8_t previous = Ram(Room);
	Ram(Room) = next;
	if (previous == 103 && Ram(SelectedItem) != SELECTED_COMPASS
		&& Ram(PlayerDirection) != DIR_DOWN) {
		// lost in the desert without the compass
		Ram(Room) = 103;
		previous = 103;
	}
	Ram(PreviousRoom) = previous;
	NormalGameMode();
}


static void
DisableControls()
{
	Ram(DirectionMask) = 0;
	Ram(DirectionMaskOld) = 0;
}


static void
SetLandingPos2()
{
	switch (Ram(Room)) {
		case 5:
			SetRam16(PlayerYdec, 0x3800);
			SetRam16(PlayerXdec, 0x6800);
			break;
		case 6:
			SetRam16(PlayerYdec, 0xa800);
			SetRam16(PlayerXdec, 0x8000);
			break;
		default:
			SetRam16(PlayerYdec, 0x8000);
			SetRam16(PlayerXdec, 0xa000);
			break;
	}
}


static void
SetParachuteStartY()
{
	SetRam16(PlayerYdec, 0x3000);
}


static void
SetParachuteMode()
{
	Ram(PlayerAnimation) = 3;
	Ram(PlayerControlMod) = 4;
	DisableControls();
	SetRam16(PlayerXdec, 0x8000);
	SetParachuteStartY();
}


static void
SetPlayerInDoor2()
{
	uint16_t door = FindDoor(Ram(IdDoorEnter));
	uint8_t type = Ram(static_cast<uint16_t>(door + 3));
	uint16_t de = static_cast<uint16_t>(door + 5);
	uint16_t hl = static_cast<uint16_t>(SYM(PlayerInDoorDat) + static_cast<uint8_t>((type - 1) * 3));
	uint8_t y = static_cast<uint8_t>(Mem(de) + Mem(hl));
	uint8_t x = static_cast<uint8_t>(Mem(static_cast<uint16_t>(de + 1))
		+ Mem(static_cast<uint16_t>(hl + 1)));
	SetRam16(PlayerYdec, static_cast<uint16_t>(y << 8));
	SetRam16(PlayerXdec, static_cast<uint16_t>(x << 8));
	Ram(PlayerDirection) = Mem(static_cast<uint16_t>(hl + 2));
}


static void
SetPlayerInDoor()
{
	DisableControls();
	Ram(PlayerControlMod) = 0;
	SetPlayerInDoor2();
}


void
LocatePlayerEntry()
{
	uint8_t id = Ram(IdDoorEnter);
	if (id == 0xff) {
		SetLandingPos2();	// fell without the parachute
		return;
	}
	if (id == 0x91 || id == 0x5b || id == 0x81) {
		SetParachuteMode();
		return;
	}
	if (id == 0x6b) {
		// SetLadderRoomEntry
		if (Ram(Room) == 119) {
			SetPlayerInDoor();
			return;
		}
		Ram(PlayerControlMod) = 6;
		Ram(PlayerDirection) = 3;
		DisableControls();
		SetRam16(PlayerYdec, 0x9e00);
		SetRam16(PlayerXdec, 0xd800);
		return;
	}
	if (id != 0) {
		// SetDoorOrElev
		if (Ram(Room) >= 0xf0) {
			Ram(PlayerControlMod) = 2;
			Ram(PlayerDirection) = 3;
			DisableControls();
			return;
		}
		if (Ram(PreviousRoom) >= 0xf0)
			SetPlayerInDoor();
		else
			SetPlayerInDoor2();
		return;
	}
	if (Ram(PreviousRoom) == 204) {
		// ChkParachuteXY
		uint8_t room = Ram(Room);
		if (room != 10 && room != 5 && room != 6) {
			SetParachuteStartY();
			return;
		}
		// SetLandingXY
		DisableControls();
		Ram(PlayerControlMod) = 0;
		Ram(PlayerAnimation) = 0;
		Ram(PlayerFrameNum) = 0;
		Ram(SnakeSprId) = 0;
		Ram(PlayerAnimWaitCnt) = 0;
		Ram(PlayerDirection) = 1;
		SetLandingPos2();
		return;
	}
	if (Ram(Room) >= 240)
		return;
	// SetRoomEntryXY: on the opposite side
	uint8_t side = static_cast<uint8_t>(Ram(NextRoomDirect) - 1);
	uint16_t position = GetPointerDE2A(SYM(EntryRoomXY), side);
	if (side < 2)
		SetRam16(PlayerYdec, position);
	else
		SetRam16(PlayerXdec, position);
}
