/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/doors (drawdoors, enterdoor, opendoor, erasedoor).
#include "Doors.h"

#include "Common.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "Rom.h"
#include "Sprites.h"
#include "Structures.h"


using namespace var;


namespace {

void
PutDoorCollTiles(uint8_t columns, uint8_t rows, uint8_t x, uint8_t y)
{
	DrawTileBlkTimp2(SYM(DoorClosedTiles), rows, columns, x, y);
}


// Doors are drawn from page 1 (colour 0 transparent); invisible solid
// tiles close the way.
void
DrawDoorType(uint8_t type, uint8_t x, uint8_t y)
{
	switch (type) {
		case 1:	// north
		case 5:	// elevator
			VDP_Copy_Dot(0xc4, type == 1 ? 0xa0 : 0xc0, 0x48, x, y, 0x18, 0x20);
			PutDoorCollTiles(4, 4, static_cast<uint8_t>(x - 4), y);
			return;
		case 2:	// south
			VDP_Copy_Dot(0xe0, 0xc0, 0x48, x, y, 0x20, 0x08);
			PutDoorCollTiles(4, 1, x, y);
			return;
		case 3:
		case 4:
		{
			// west and east: slanted, one column at a time
			uint8_t dx = x;
			uint8_t dy = type == 3 ? y : static_cast<uint8_t>(y + 28);
			uint8_t sx = type == 3 ? 0xe0 : 0xe8;
			for (int i = 0; i < 8; i++) {
				VDP_Copy_Dot(sx, 0xa0, 0x48, dx, dy, 1, 0x20);
				dy = static_cast<uint8_t>(type == 3 ? dy + 4 : dy - 4);
				dx++;
				sx++;
			}
			uint8_t cx = type == 3 ? static_cast<uint8_t>(x - 8) : x;
			PutDoorCollTiles(2, 4, cx, static_cast<uint8_t>(y + 0x20));
			return;
		}
		case 6:	// dummy
			return;
		default:
		{
			// walls: tile blocks, the background saved for when they break
			static const sym::Id kWalls[] = {
				sym::TilesBasemWall60, sym::TilesBasemWall61, sym::TilesBasemWall59,
				sym::TilesBasemWall58, sym::TilesBasemWall63, sym::TilesWallPrison2,
				sym::TilesWallPrison2, sym::TilesWallPrison1, sym::TilesWallPrison,
				sym::TilesWallBld3_108, sym::TilesBasemWall93, sym::TilesBasemWall100,
				sym::TilesBasemWall112
			};
			int index = type - 7;
			if (index < 0 || index >= static_cast<int>(sizeof(kWalls) / sizeof(kWalls[0]))) {
				if (gUnported == NULL)
					gUnported = "DrawDoorType: bad type";
				return;
			}
			uint16_t tiles = RomSymbol(kWalls[index]);
			SaveBackgTiles(tiles, x, y);
			DrawTileBlkTimp(tiles, x, y);
			return;
		}
	}
}

}	// namespace


void
DrawDoors()
{
	uint8_t count = Ram(DoorsInRoom);
	if (count == 0)
		return;
	uint16_t hl = DoorsList;
	for (int i = 0; i < count; i++, hl = static_cast<uint16_t>(hl + 0x10)) {
		if (Ram(static_cast<uint16_t>(hl + 1)) == 0)
			continue;	// open
		uint8_t type = Ram(static_cast<uint16_t>(hl + 3));
		uint8_t y = Ram(static_cast<uint16_t>(hl + 5));
		uint8_t x = Ram(static_cast<uint16_t>(hl + 6));
		Ram(TempData) = y;
		Ram(static_cast<uint16_t>(TempData + 1)) = x;
		DrawDoorType(type, x, y);
	}
}


// Opening (closed doors) or walking through (open ones).
void
ChkDoors()
{
	uint16_t hl = DoorsList;
	uint8_t count = Ram(DoorsInRoom);
	if (count == 0)
		return;
	for (uint8_t c = 0; c < count; c++, hl = static_cast<uint16_t>(hl + 0x10)) {
		uint8_t id = Ram(hl);
		Ram(TempData) = id;
		uint16_t logic = static_cast<uint16_t>(hl + 2);
		if (Ram(static_cast<uint16_t>(hl + 1)) != 0) {
			// closed; after Metal Gear the door back (#62) stays shut
			if (Ram(MetalGear_KO) != 0 && id == 0x62)
				continue;
			if (!ChkOpenDoor(logic))
				continue;
			Ram(idxDoorOpen) = c;
			Ram(GameMode) = GAME_MODE_OPEN_DOOR;
			Ram(OpenDoorStatus) = 0;
			UpdateSnakeSpr();
			return;
		}
		// ChkEnterDoor
		if (id == 0x40 || id == 0x6c)
			continue;	// hidden doors to room 204
		uint8_t type = Ram(logic) & 0x1f;
		if ((type == 0x0f || type == 0x10) && (Ram(logic) & 0x20) == 0)
			continue;	// a broken wall inside a room
		if (!ChkTouchDoor2(static_cast<uint16_t>(logic + 9)))
			continue;
		Ram(IdDoorEnter) = Ram(static_cast<uint16_t>(DoorsList + HL_4xA(c) * 4));
		Ram(OpenDoorStatus) = 0;
		Ram(GameMode) = 1;
		return;
	}
	Ram(IdDoorEnter) = 0;
}


namespace {

bool
ChkTouchDoor3(uint16_t area, uint8_t x, uint8_t y)
{
	uint8_t dy = static_cast<uint8_t>(y - Mem(area));
	if (dy >= Mem(static_cast<uint16_t>(area + 1)))
		return false;
	uint8_t dx = static_cast<uint8_t>(x - Mem(static_cast<uint16_t>(area + 2)));
	return dx < Mem(static_cast<uint16_t>(area + 3));
}


// type: the door's render type (DoorsList + 3); the open area follows at +4.
bool
ChkTouchDoor(uint16_t type)
{
	return ChkTouchDoor2(static_cast<uint16_t>(type + 4));
}


bool
ChkBombLocation(uint16_t area)
{
	return ChkTouchDoor3(area, Ram(PlayerShot1X), Ram(PlayerShot1Y));
}


bool
PlayBreakableSfx()
{
	SetSoundEntryChk(0x0a);
	return false;
}


uint8_t
PunchWallDir(uint16_t type)
{
	return Mem(static_cast<uint16_t>(SYM(PunchWallDirs) + static_cast<uint8_t>(Ram(type) - 7)));
}


bool
PlasticBombExploding()
{
	return Ram(PlayerShotsList) == PLASTIC_BOMB && Ram(PlayerShot1Stat) == 2;
}

}	// namespace


bool
ChkTouchDoor2(uint16_t area)
{
	return ChkTouchDoor3(area, Ram(PlayerX), Ram(PlayerY));
}


// Door logic 1-16: elevator, cards 1-8, punch, lorry, building 2 entrance,
// compass room, Big Boss, prison walls, basement walls. The render type
// doubles as the direction Snake must face.
bool
ChkOpenDoor(uint16_t logic)
{
	if (Ram(PlayerAnimation) == 7)
		return false;	// not in the box
	uint8_t kind = Ram(logic) & 0x1f;
	uint16_t type = static_cast<uint16_t>(logic + 1);
	uint8_t direction = Ram(PlayerDirection);
	switch (kind) {
		case 1:
			if (Ram(type) == 5) {
				if (direction != DIR_UP)
					return false;
			} else if (direction != DIR_RIGHT) {
				return false;
			}
			return ChkTouchDoor(type);
		case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
			if (Ram(SelectedItem) != SELECTED_CARD1 + kind - 2 || direction != Ram(type))
				return false;
			return ChkTouchDoor(type);
		case 10:
			// Shot Gunner's door: punch it
			if (direction != Ram(type) || Ram(PlayerControlMod) != 1)
				return false;
			return ChkTouchDoor(type);
		case 11:
			// lorry: a plastic bomb or punches
			if (PlasticBombExploding())
				return ChkBombLocation(static_cast<uint16_t>(type + 4));
			if (Ram(PlayerControlMod) != 1 || direction != Ram(type))
				return false;
			if (!ChkTouchDoor(type))
				return false;
			if (Ram(PunchCnt) != 8)
				return false;
			return PlayBreakableSfx();
		case 12:
			// building 2 entrance: the guards open it
			if (Ram(Room) == 73) {
				if (direction != DIR_DOWN)
					return false;
				return ChkTouchDoor(type);
			}
			if (Ram(DoorBuild2LockedF) == 0)
				return false;
			Ram(DoorBuild2LockedF) = 0;
			return true;
		case 13:
			// Jennifer opens the compass room
			if (Ram(JeniOpenDoorF) == 0)
				return false;
			Ram(JeniOpenDoorF) = 0;
			return true;
		case 14:
			if (Ram(OpenBigBossDoor) == 0)
				return false;
			Ram(OpenBigBossDoor) = 0;
			return true;
		case 15:
		{
			// prison walls: punched many times
			if (direction != PunchWallDir(type) || Ram(PlayerControlMod) != 1)
				return false;
			if (!ChkTouchDoor(type))
				return false;
			uint16_t life = Ram(TempData) == 0x0c ? PrisonWall2Life : PrisonWall1Life;
			if (--Ram(life) != 0)
				return PlayBreakableSfx();
			return true;
		}
		case 16:
			// basement walls: a plastic bomb or one punch
			if (PlasticBombExploding())
				return ChkBombLocation(static_cast<uint16_t>(type + 4));
			if (direction != PunchWallDir(type) || Ram(PlayerControlMod) != 1)
				return false;
			if (!ChkTouchDoor(type))
				return false;
			return PlayBreakableSfx();
		default:
			if (gUnported == NULL)
				gUnported = "ChkOpenDoor: bad logic";
			return false;
	}
}


uint16_t
GetOpeningDoorDat()
{
	return static_cast<uint16_t>(DoorsList + static_cast<uint8_t>(static_cast<uint8_t>(Ram(idxDoorOpen) * 8) * 2));
}


namespace {

void
InitOpenDoor()
{
	SetBankInA0(0x0f);
	SwapSprBuffer();
	SetBanks1_2_3();
	uint16_t hl = static_cast<uint16_t>(GetOpeningDoorDat() + 3);
	uint8_t type = Ram(hl);
	Ram(static_cast<uint16_t>(hl + 1)) = Mem(static_cast<uint16_t>(SYM(EraseDoorDat)
		+ static_cast<uint8_t>(type - 1)));
	Ram(OpenDoorStatus) = type;
	uint8_t index = static_cast<uint8_t>(type - 1);
	uint8_t sfx = index >= 6 ? 0x1e : Mem(static_cast<uint16_t>(SYM(DoorOpenSfxs) + index));
	SetSoundEntry(sfx);
}


void
SetOpenDoorTiles(uint8_t columns, uint8_t rows, uint8_t x, uint8_t y)
{
	DrawTileBlkTimp2(SYM(DoorOpenTiles), rows, columns, x, y);
}


// counter: DoorsList entry + 4. Marks the door open (walls also free
// their entry), clears the solid tiles.
void
SetDoorOpen(uint16_t counter)
{
	uint16_t door = static_cast<uint16_t>(counter - 4);
	Ram(counter) = 0;
	uint8_t type = Ram(static_cast<uint16_t>(door + 3));
	uint8_t logic = Ram(static_cast<uint16_t>(door + 2));
	Ram(static_cast<uint16_t>(door + 1)) = 0;
	if ((logic & 0xc0) == 0) {
		Ram(static_cast<uint16_t>(DoorOpenArray + static_cast<uint8_t>(Ram(door) - 1))) = 0;
		if (type == 7 || type == 0x0a || type == 0x0b)
			Ram(door) = 0;
	}
	uint8_t y = Ram(static_cast<uint16_t>(door + 5));
	uint8_t x = Ram(static_cast<uint16_t>(door + 6));
	if (type == 3)
		x = static_cast<uint8_t>(x - 8);
	Ram(GameMode) = 0;
	switch (type) {
		case 1:
		case 5:
		case 6:
			SetOpenDoorTiles(4, 4, x, y);
			break;
		case 2:
			SetOpenDoorTiles(4, 1, x, y);
			break;
		case 3:
		case 4:
			SetOpenDoorTiles(2, 4, x, static_cast<uint8_t>(y + 0x20));
			break;
		default:
			break;	// walls
	}
}


void
RestoreSavedTiles(uint8_t columns, uint8_t rows)
{
	uint16_t door = GetOpeningDoorDat();
	uint8_t y = Ram(static_cast<uint16_t>(door + 5));
	uint8_t x = Ram(static_cast<uint16_t>(door + 6));
	DrawTileBlkTimp2(SavedTilesBuffer, rows, columns, x, y);
	SetDoorOpen(static_cast<uint16_t>(door + 4));
}

}	// namespace


// The opening animations, one line a tick; walls come down at once.
void
EraseDoorLogic()
{
	uint8_t status = Ram(OpenDoorStatus);
	if (status == 0) {
		InitOpenDoor();
		return;
	}
	uint16_t door = GetOpeningDoorDat();
	uint16_t counter = static_cast<uint16_t>(door + 4);
	uint8_t y = Ram(static_cast<uint16_t>(door + 5));
	uint8_t x = Ram(static_cast<uint16_t>(door + 6));
	switch (status) {
		case 1:	// north: from the left
			if (++Ram(counter) == 24) {
				SetDoorOpen(counter);
				return;
			}
			DrawLineVert(static_cast<uint8_t>(x + Ram(counter)), y, 0x20, 0);
			return;
		case 2:	// south: from the right
			if (--Ram(counter) == 0) {
				SetDoorOpen(counter);
				return;
			}
			DrawLineVert(static_cast<uint8_t>(x + Ram(counter) - 1), y, 8, 0);
			return;
		case 3:
		{
			if (--Ram(counter) == 0) {
				SetDoorOpen(counter);
				return;
			}
			uint8_t dy = static_cast<uint8_t>(y + Ram(counter));
			for (int i = 0; i < 8; i++) {
				DrawLineVert(x, dy, 1, 0);
				dy = static_cast<uint8_t>(dy + 4);
				x++;
			}
			return;
		}
		case 4:
		{
			if (++Ram(counter) == 35) {
				SetDoorOpen(counter);
				return;
			}
			uint8_t dy = static_cast<uint8_t>(y + Ram(counter) + 28);
			for (int i = 0; i < 8; i++) {
				DrawLineVert(x, dy, 1, 0);
				dy = static_cast<uint8_t>(dy - 4);
				x++;
			}
			return;
		}
		case 5:
		{
			// elevator: two slides open to the sides
			if (--Ram(counter) == 0) {
				SetDoorOpen(counter);
				return;
			}
			uint8_t open = Ram(counter);
			DrawLineVert(static_cast<uint8_t>(x + open - 1), y, 0x20, 0);
			// (!?) the original draws the right line at Y 0: DrawLineVert
			// leaves the colour (0) in L
			uint8_t right = static_cast<uint8_t>(0x0c + x + static_cast<uint8_t>(0x0c - open));
			DrawLineVert(right, 0, 0x20, 0);
			return;
		}
		case 6:
		case 12:
			return;	// nothing to draw
		default:
		{
			// broken walls: the saved background comes back
			static const uint8_t kWallSize[][2] = {
				{ 4, 6 }, { 4, 1 }, { 3, 0x0d }, { 5, 0x0d }, { 5, 0x0c }, { 0, 0 },
				{ 4, 1 }, { 3, 0x0d }, { 2, 0x0c }, { 8, 0x0a }, { 2, 0x0c }, { 5, 0x0c },
				{ 6, 5 }
			};
			if (status >= 7 && status <= 19) {
				RestoreSavedTiles(kWallSize[status - 7][0], kWallSize[status - 7][1]);
				return;
			}
			if (gUnported == NULL)
				gUnported = "EraseDoorLogic: bad status";
			return;
		}
	}
}
