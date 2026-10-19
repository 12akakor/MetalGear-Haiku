/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from Banks0123.asm (GetNextRoomNum .. AddDoorsData).
#include "Common.h"

#include "GameMain.h"
#include "GfxBasic.h"
#include "Rom.h"
#include "Structures.h"


using namespace var;


uint8_t
Mem(uint16_t address)
{
	return Rom(address);
}


uint16_t
Mem16(uint16_t address)
{
	return Rom16(address);
}


void
SetBanks1_2_3()
{
	SetBanks(1);
}


void
SetBanks_4_5_6()
{
	SetBanks(4);
}


void
SetBanks_7_8_9()
{
	SetBanks(7);
}


void
SetBanks_A_B_C()
{
	SetBanks(0x0a);
}


void
SetBanks_D_E_F()
{
	SetBanks(0x0d);
}


uint16_t
HL_4xA(uint8_t a)
{
	return static_cast<uint16_t>(a * 4);
}


uint16_t
DEC_A_HL_4xA(uint8_t a)
{
	return HL_4xA(static_cast<uint8_t>(a - 1));
}


static uint16_t
AddHL_A(uint16_t hl, uint8_t a)
{
	return static_cast<uint16_t>(hl + a);
}


// Rooms 0-125 and 208-227 have exits to the four sides; lorries, rooms
// with only a door and the first elevator (240) do not.
uint8_t
GetNextRoomNum()
{
	SetBanks_4_5_6();
	uint8_t next = 0xff;
	uint8_t room = Ram(Room);
	bool connected = true;
	if (room >= 126) {
		if (room < 208)
			connected = false;
		else if (room < 228)
			room = static_cast<uint8_t>(room - 82);
		else if (room < 241)
			connected = false;
		else
			room = static_cast<uint8_t>(room - 13 - 82);
	}
	if (connected) {
		uint16_t hl = static_cast<uint16_t>(HL_4xA(room) + SYM(RoomConnections));
		hl = AddHL_A(hl, static_cast<uint8_t>(Ram(NextRoomDirect) - 1));
		next = Mem(hl);
	}
	SetBanks1_2_3();
	return next;
}


uint16_t
GetElevatorRoomDat()
{
	SetBanks_4_5_6();
	uint16_t hl = GetPointerDE2A(SYM(idxElevatorRoom), static_cast<uint8_t>(Ram(Room) - 0xf0));
	Ram(ElevatorLimitUp) = Mem(hl);
	hl++;
	Ram(ElevatorLimitDown) = Mem(hl);
	SetBanks1_2_3();
	return hl;
}


// The elevator data lists the rooms it connects, 3 bytes each: room,
// player Y, elevator Y.
void
GetElevatorPosY(uint8_t* playerY, uint8_t* elevatorY)
{
	uint16_t hl = GetElevatorRoomDat();
	SetBanks_4_5_6();
	hl++;
	uint8_t room = Ram(PreviousRoom);
	while (Mem(hl) != room)
		hl = static_cast<uint16_t>(hl + 3);
	*playerY = Mem(static_cast<uint16_t>(hl + 1));
	*elevatorY = Mem(static_cast<uint16_t>(hl + 2));
	SetBanks1_2_3();
}


// Bits 7-6 of the door logic: #80 = open at the start.
void
SetDefaultDoorLock()
{
	SetBanks_D_E_F();
	uint16_t logic = SYM(IdDoorsLogic);
	for (int i = 0; i < 160; i++) {
		uint8_t open = static_cast<uint8_t>((Mem(static_cast<uint16_t>(logic + i)) & 0xc0) - 0x80);
		Ram(static_cast<uint16_t>(DoorOpenArray + i)) = open == 0 ? 0 : 1;
	}
	SetBanks1_2_3();
}


// No binoculars there, and shooting does not trigger the alarm.
void
ChkIsolatedRoom()
{
	SetBanks_A_B_C();
	Ram(IsolatedRoom) = Mem(AddHL_A(SYM(RoomsMusic), Ram(Room))) & 7;
	SetBanks1_2_3();
}


// Zones 5 and more need the antenna.
void
SetRadioArea()
{
	SetBanks_A_B_C();
	Ram(MapZone) = GetNibbleRoom(SYM(idxMapZones));
	SetBanks1_2_3();
}


uint8_t
GetWeaponDamages(uint8_t weapon)
{
	SetBanks_4_5_6();
	uint16_t de = GetPointerDE2A(SYM(idxWeaponPow), static_cast<uint8_t>(weapon - 1));
	SetRam16(TempData, de);
	uint8_t shots = Mem(static_cast<uint16_t>(de - 1));
	SetBanks1_2_3();
	return shots;
}


uint8_t
GetTempPHL_A(uint8_t a)
{
	SetBanks_4_5_6();
	uint8_t value = Mem(AddHL_A(Ram16(TempData), a));
	SetBanks1_2_3();
	return value;
}


uint8_t
GET_HL_A_DEC(uint16_t hl, uint8_t a)
{
	SetBanks_4_5_6();
	uint8_t value = Mem(AddHL_A(hl, static_cast<uint8_t>(a - 1)));
	SetBanks1_2_3();
	return value;
}


uint8_t
GetItemAmount(uint8_t item)
{
	SetBanks_4_5_6();
	uint8_t value = Mem(AddHL_A(SYM(ItemTakeAmount), item));
	SetBanks1_2_3();
	return value;
}


uint16_t
GetShapeInfo(uint8_t shape)
{
	SetBanks_4_5_6();
	uint16_t source = static_cast<uint16_t>(DEC_A_HL_4xA(shape) + SYM(ImpactAreasInfo));
	for (int i = 0; i < 4; i++)
		Ram(static_cast<uint16_t>(TempImpactInfo + i)) = Mem(static_cast<uint16_t>(source + i));
	SetBanks1_2_3();
	return TempImpactInfo;
}


static void
CalcCursorXY(uint16_t table)
{
	SetBanks_4_5_6();
	SetRam16(MenuCursorXY, GetPointerDE2A(table, Ram(SelectIdx)));
	SetBanks1_2_3();
}


void
CalcCursorXYWeapon()
{
	CalcCursorXY(SYM(WeaponArrowXY));
}


void
CalcCursorXYEquip()
{
	CalcCursorXY(SYM(ItemEquipXY));
}


// Entries of 4 bytes: +0 ID, +1 tens/units, +2 hundreds.
static bool
GetInventoryAdd(uint8_t id, uint16_t table, int count, uint16_t* address, uint8_t* index)
{
	uint16_t hl = table;
	uint8_t c = 0;
	for (int i = 0; i < count; i++) {
		if (Ram(hl) == id) {
			*address = hl;
			*index = c;
			return true;
		}
		hl = static_cast<uint16_t>(hl + 4);
		c++;
	}
	*address = hl;
	*index = c;
	return false;
}


bool
GetItemInvAdd(uint8_t id, uint16_t* address, uint8_t* index)
{
	return GetInventoryAdd(id, Equipment, 25, address, index);
}


bool
GetWeaponInvAdd(uint8_t id, uint16_t* address, uint8_t* index)
{
	return GetInventoryAdd(id, Weapons, 7, address, index);
}


// DoorsList entries (16 bytes): 0 ID, 1 open, 2 open logic, 3 type,
// 4 counter, 5 draw Y, 6 draw X, 7-10 open area (Y, NY, X, NX), 11-14
// enter area, 15 destination room.
void
AddDoorsData()
{
	Ram(DoorsInRoom) = 0;
	uint8_t room = Ram(Room);
	if (room >= 225) {
		if (room < 240)
			return;
		room = static_cast<uint8_t>(room - 15);
	}
	SetBanks_D_E_F();
	uint16_t de = GetPointerDE2A(SYM(idxDoors), room);
	uint16_t hl = DoorsList;
	for (;;) {
		uint8_t id = Mem(de);
		if (id == 0xff)
			break;
		Ram(DoorsInRoom)++;
		Ram(hl++) = id;
		de++;
		Ram(hl++) = Ram(AddHL_A(DoorOpenArray, static_cast<uint8_t>(id - 1)));
		Ram(hl++) = Mem(AddHL_A(SYM(IdDoorsLogic), static_cast<uint8_t>(id - 1)));
		uint8_t type = Mem(de);
		Ram(hl) = type;
		if (type == 6)
			Ram(static_cast<uint16_t>(hl - 2)) = 0;	// dummy door: open
		hl++;
		de++;
		Ram(hl++) = 0;
		uint8_t drawY = Mem(de);
		Ram(hl++) = drawY;
		de++;
		uint8_t drawX = Mem(de);
		Ram(hl++) = drawX;
		uint16_t iy = static_cast<uint16_t>(DEC_A_HL_4xA(type) * 2 + SYM(DoorOpenEnterDat));
		Ram(hl++) = static_cast<uint8_t>(drawY + Mem(iy));
		Ram(hl++) = Mem(static_cast<uint16_t>(iy + 1));
		Ram(hl++) = static_cast<uint8_t>(drawX + Mem(static_cast<uint16_t>(iy + 2)));
		Ram(hl++) = Mem(static_cast<uint16_t>(iy + 3));
		de++;
		Ram(hl++) = static_cast<uint8_t>(drawY + Mem(static_cast<uint16_t>(iy + 4)));
		Ram(hl++) = Mem(static_cast<uint16_t>(iy + 5));
		Ram(hl++) = static_cast<uint8_t>(drawX + Mem(static_cast<uint16_t>(iy + 6)));
		Ram(hl++) = Mem(static_cast<uint16_t>(iy + 7));
		Ram(hl++) = Mem(de);
		de++;
	}
	SetBanks1_2_3();
}


// #E000 + (y / 8) * 32 + x / 8: the room's tile map.
uint16_t
CoordToBuffTile(uint8_t x, uint8_t y)
{
	return static_cast<uint16_t>(RoomTileBuffer + (y >> 3) * 32 + (x >> 3));
}


void
GetTileXY_B2(uint8_t tile, uint8_t* x, uint8_t* y)
{
	GetTileXY(tile, x, y);
	*y = static_cast<uint8_t>(*y + 0x80);
}


// From page 1 (tile bank 1 or 2) to page 0, colour 0 transparent.
void
DrawTileTIMP(uint8_t tile, uint8_t x, uint8_t y)
{
	uint8_t sx, sy;
	if ((Ram(TilesetBank) & 1) != 0)
		GetTileXY_B2(tile, &sx, &sy);
	else
		GetTileXY(tile, &sx, &sy);
	VDP_Copy_Dot(sx, sy, 0x48, x, y, 8, 8);
}


static void
DrawTileBlkTimp3(uint16_t hl, uint8_t rows, uint8_t columns, uint8_t x, uint8_t y)
{
	SetBanks_D_E_F();
	uint16_t map = CoordToBuffTile(x, y);
	for (; rows > 0; rows--) {
		uint16_t row = map;
		uint8_t dx = x;
		for (int i = 0; i < columns; i++) {
			uint8_t tile = Mem(hl++);
			Ram(map++) = tile;
			DrawTileTIMP(tile, dx, y);
			dx = static_cast<uint8_t>(dx + 8);
		}
		map = static_cast<uint16_t>(row + 0x20);
		y = static_cast<uint8_t>(y + 8);
	}
	SetBanks1_2_3();
}


// With the current tile bank.
void
DrawTilesBlock(uint16_t data, uint8_t x, uint8_t y)
{
	SetBanks_D_E_F();
	uint8_t rows = Mem(data);
	uint8_t columns = Mem(static_cast<uint16_t>(data + 1));
	DrawTileBlkTimp3(static_cast<uint16_t>(data + 2), rows, columns, x, y);
}


void
DrawTileBlkTimp(uint16_t data, uint8_t x, uint8_t y)
{
	SetBanks_D_E_F();
	uint8_t rows = Mem(data);
	uint8_t columns = Mem(static_cast<uint16_t>(data + 1));
	Ram(TilesetBank) = 0;
	DrawTileBlkTimp3(static_cast<uint16_t>(data + 2), rows, columns, x, y);
}


void
DrawMetalGear(uint8_t x, uint8_t y)
{
	DrawTileBlkTimp(SYM(MetalGearTileMap), x, y);
}


void
EraseMetalGear(uint8_t x, uint8_t y)
{
	DrawTileBlkTimp(SYM(MetalGearTileMap2), x, y);
}


// Copies the room's tiles under a block to SavedTilesBuffer.
void
SaveBackgTiles(uint16_t data, uint8_t x, uint8_t y)
{
	SetBanks_D_E_F();
	uint8_t rows = Mem(data);
	uint8_t columns = Mem(static_cast<uint16_t>(data + 1));
	SetBanks1_2_3();
	uint16_t de = SavedTilesBuffer;
	uint16_t hl = CoordToBuffTile(x, y);
	for (; rows > 0; rows--) {
		for (int i = 0; i < columns; i++)
			Ram(de++) = Ram(static_cast<uint16_t>(hl + i));
		hl = static_cast<uint16_t>(hl + 0x20);
	}
}


void
SetAreaMusic()
{
	Ram(AlertMode) = 0;
	SetAreaMusic2();
}


// Rooms where entering with the transmitter does not start the alert.
static bool
IsRoomNoAlert(uint8_t room)
{
	uint16_t list = SYM(RoomsNoAlert);
	for (int i = 0; i < 0x11; i++) {
		if (Mem(static_cast<uint16_t>(list + i)) == room)
			return true;
	}
	return false;
}


// The music of the room (or the alert), and whether a radio call comes in.
// The room table (RoomsMusic): bits 7-4 music, bit 3 incoming call, bits
// 2-0 isolated room.
void
SetAreaMusic2()
{
	if (Ram(GameMode) == GAME_MODE_BINOCULARS)
		return;
	uint8_t zone = Ram(MapZone);
	uint8_t room = Ram(Room);
	// Basement of building 1, desert to building 3 and later, some rooms,
	// the desert to building 2 and the elevators: no alert
	bool noAlert = zone == 4 || zone >= 8 || IsRoomNoAlert(room)
		|| (room >= 64 && room < 69) || room >= 0xf0;
	if (noAlert || Ram(TransmiTaken) != 0) {
		// with the transmitter (the bug) the enemy knows where Snake is
		Ram(AlertMode) = noAlert ? 0 : 1;
		Ram(AlertRespawnTimer) = 0x1e;
	}
	if (Ram(static_cast<uint16_t>(SoundWorkAreaSfx + 2)) == 0x22)
		SetSoundEntryChk(0x28);	// stop the incoming call sound
	SetBanks_A_B_C();
	uint16_t hl = AddHL_A(SYM(RoomsMusic), Ram(Room));
	uint8_t music;
	bool check = true;
	if (Ram(DestructionTimerOn) != 0) {
		music = 0x3b;	// Escape beyond Big Boss
	} else {
		if (Ram(AlertMode) != 0
			&& (Ram(AreaMusic) == 0x2f || Ram(AreaMusic) == 0x32))
			check = false;
		switch (Mem(hl) & 0xf0) {
			case 0x00: music = 0x2c; break;	// Theme of Tara
			case 0x10: music = 0x35; break;	// Sneaking mission
			case 0x20: music = 0x38; break;	// TX-55 Metal Gear
			case 0x30: music = 0x3b; break;	// Escape beyond Big Boss
			default: music = 0x32; break;	// Red alert
		}
	}
	if (check) {
		uint8_t a;
		bool set = true;
		if (Ram(AlertMode) != 0)
			a = 0x32;
		else if (Ram(AreaMusic) == music)
			set = false;
		else
			a = music;
		if (set) {
			if (Ram(DestructionTimerOn) != 0)
				a = 0x3b;
			Ram(AreaMusic) = a;
			Ram(MusicToSet) = a;
		}
	}

	// ChkRadioCalls: 1 = start an incoming call, 2 = stop it
	uint8_t flag = 2;
	uint8_t freq = Ram(RadioPersonsDat);
	bool calls = true;
	if (Ram(SchneiderCaptured) != 0
		&& (freq == FREQ_SCHNEIDER || freq == FREQ_SCHNEIDER_BUILDING2))
		calls = false;	// Schneider never calls
	if (calls && freq == FREQ_JENIFFER
		&& (Ram(Class) != 3 || Ram(JennifBrotherDead) != 0))
		calls = false;
	if (calls && Ram(MapZone) >= 5 && Ram(AntennaTaken) == 0)
		calls = false;
	if (calls) {
		uint8_t call = Mem(hl) & 8;
		if (call != 0) {
			Ram(IncomingCallTimer) = static_cast<uint8_t>(call * 4);
			flag = 0;
		}
	}
	Ram(RadioCallFlag) = flag;
	SetBanks1_2_3();
}


void
RemoveItem(uint16_t address)
{
	for (int i = 0; i < 4; i++)
		Ram(static_cast<uint16_t>(address + i)) = 0;
}


void
SetActorSprColors(uint16_t actor)
{
	SetActorSprColors2(actor, Ram(static_cast<uint16_t>(actor + ACTOR::ID)));
}


// Colours of the actor's sprites from its colour list.
void
SetActorSprColors2(uint16_t actor, uint8_t id)
{
	SetBanks_4_5_6();
	uint16_t table = static_cast<uint16_t>(SYM(idxActorSprCols) - 2
		+ static_cast<uint8_t>(id * 2));
	uint16_t colors = Mem16(table);
	uint16_t hl = static_cast<uint16_t>(actor + ACTOR::Spr1Color);
	int count = Ram(static_cast<uint16_t>(actor + ACTOR::NumSprites));
	if (count == 0)
		count = 256;	// djnz
	for (int i = 0; i < count; i++) {
		Ram(hl) = Mem(colors++);
		hl = static_cast<uint16_t>((hl & 0xff00) | ((hl + 5) & 0xff));
	}
	SetBanks1_2_3();
}
