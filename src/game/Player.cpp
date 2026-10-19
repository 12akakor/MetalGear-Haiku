/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from Banks0123.asm (InitPlayerVars .. SetGrenaTargetSpr),
// logic/collisions.asm and logic/damagegas.asm.
#include "Player.h"

#include "Actors.h"
#include "Common.h"
#include "GameGfx.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "Items.h"
#include "Rom.h"
#include "Structures.h"
#include "Unported.h"


using namespace var;


static void
SetPlayerSprId(uint8_t id)
{
	Ram(SnakeSprId) = id;
}


void
InitPlayerVars()
{
	SetSnakePal();
	Ram(Life) = 0x18;
	Ram(MaxLife) = 0x18;
	Ram(Class) = 0;
	Ram(PlayerFrameNum) = 0;
	Ram(PlayerAnimation) = 0;
	Ram(PlayerControlMod) = 0;
	Ram(StopPlayerFlag) = 1;
	Ram(PlayerDirection) = 2;
	Ram(SnakeSprId) = 3;
	SetRam16(PlayerMovSpeed, 0x200);
	if ((Ram(ControlConfig) & 0x40) == 0)
		return;	// demo
	// the intro: Snake swims in from the sea
	Ram(PlayerAnimation) = 4;
	Ram(PlayerControlMod) = CONTROL_INTRO;
	Ram(SnakeSprId) = 0x25;
	SetRam16(PlayerYdec, 0xb800);
	SetRam16(PlayerXdec, 0xc000);
	SetRam16(PlayerMovSpeed, 0x100);
	Ram(IntroSceneCnt) = 0x40;
}


static void
SetAnimation()
{
	AnimatePlayer();
	ChkExitRoom();
}


static void
NormalCtrl()
{
	if (Ram(PlayerShotsList) == 7)
		return;	// too many shots
	GetPlayerDir();
	ChkWater();
	if (Ram(PlayerControlMod) == CONTROL_DEAD) {
		AnimatePlayer();
		return;
	}
	ChkRoofAirFlow();
	if (Ram(PlayerControlMod) == CONTROL_AIR_FLOW) {
		AnimatePlayer();
		return;
	}
	chkPunch();
	ChkControlPlayer();
	ChkPlayerColl();
	uint8_t animation = Ram(PlayerAnimation);
	if (animation != 1 && animation != 2 && animation != 4)
		Ram(PlayerAnimation) = Ram(SelectedItem) == SELECTED_BOX ? 7 : 0;
	SetAnimation();
}


static void
ElevatorCtrl()
{
	ChkCtrlElevator();
	if (Ram(GameMode) == GAME_MODE_ELEVATOR) {
		SetSprIdle();
		return;
	}
	GetDirLeftRight();
	ControlPlayerH();
	ChkPlayerColl();
	Ram(PlayerAnimation) = 0;
	AnimatePlayer();
	ChkLimitXElevator();
}


// The parachute sways left and right.
static void
ParachuteLogic()
{
	SetRam16(PlayerSpeedY, 0x100);
	SetRam16(PlayerSpeedX, (Ram(TickCounter) & 0x10) != 0 ? 0xff00 : 0x0100);
	Ram(PlayerAnimation) = 3;
	SetAnimation();
}


static void
AirFlowLogic()
{
	if (Ram(PlayerY) < 0x30) {
		Ram(PlayerControlMod) = 0;
		return;
	}
	Ram(PlayerY) = static_cast<uint8_t>(Ram(PlayerY) - 3);
}


static void
LaddersWalk()
{
	GetDirLeftRight();
	ControlPlayerH();
	ChkPlayerColl();
	ChkStartClimb();
	if (Ram(PlayerControlMod) == CONTROL_LADDER_CLIMB)
		return;
	Ram(PlayerAnimation) = 0;
	SetAnimation();
}


static void
LaddersClimb()
{
	GetDirUpDown();
	ControlPlayerV();
	ChkPlayerColl();
	ChkExitLadders();
	if (Ram(PlayerControlMod) == CONTROL_LADDER_WALK)
		return;
	Ram(PlayerAnimation) = 5;
	AnimatePlayer();
	ChkLadderLimits();
}


static void
IntroScene()
{
	SetBankInA0(9);
	IntroSceneLogic();
	SetBanks1_2_3();
	if (Ram(PlayerControlMod) != 0)
		return;
	SetBankInA0(6);
	ChkSaveGameStatus();
	SetBanks1_2_3();
}


void
PlayerControlLogic()
{
	switch (Ram(PlayerControlMod)) {
		case 0:
			NormalCtrl();
			break;
		case 1:	// punch
			GetDirLeftRight();
			PunchLogic2();
			AnimatePlayer();
			break;
		case 2:
			ElevatorCtrl();
			break;
		case 3:	// dead
			AnimatePlayer();
			break;
		case 4:
			ParachuteLogic();
			break;
		case 5:
			AirFlowLogic();
			break;
		case 6:
			LaddersWalk();
			break;
		case 7:
			LaddersClimb();
			break;
		case 8:
			IntroScene();
			break;
		default:
			if (gUnported == NULL)
				gUnported = "PlayerControlLogic: bad mode";
			break;
	}
}


// The direction last pressed wins; when it is released the one pressed
// before it (if still held) takes over.
static void
GetPlayerDir2(uint8_t mask)
{
	uint8_t a = Ram(ControlsTrigger) & mask;
	if (a != 0) {
		Ram(DirectionMaskOld) = Ram(DirectionMask);
		uint8_t c = a;
		if ((a = c & 1) == 0 && (a = c & 2) == 0 && (a = c & 4) == 0)
			a = c & 8;
		Ram(DirectionMask) = a;
	} else {
		uint8_t held = Ram(ControlsHold) & mask;
		a = held;
		if ((held & Ram(DirectionMask)) == 0 && (held & Ram(DirectionMaskOld)) != 0) {
			a = Ram(DirectionMaskOld);
			Ram(DirectionMaskOld) = 0;
			Ram(DirectionMask) = a;
		}
	}
	Ram(PlayerDirectionNew) = Mem(static_cast<uint16_t>(SYM(IdsDirection) + a));
}


void
GetPlayerDir()
{
	GetPlayerDir2(0x0f);
}


void
GetDirUpDown()
{
	GetPlayerDir2(0x03);
}


void
GetDirLeftRight()
{
	GetPlayerDir2(0x0c);
}


static void
ControlPlayer2()
{
	uint16_t speed = Ram16(PlayerMovSpeed);
	uint16_t sx = 0, sy = 0;
	if (Ram(StopPlayerFlag) == 0) {
		switch (Ram(PlayerDirection)) {
			case 1:
				sy = static_cast<uint16_t>(-speed);
				break;
			case 2:
				sy = speed;
				break;
			case 3:
				sx = static_cast<uint16_t>(-speed);
				break;
			default:
				sx = speed;
				break;
		}
	}
	SetRam16(PlayerSpeedY, sy);
	SetRam16(PlayerSpeedX, sx);
	if (Ram(InAirFlow) != 0)
		SetRam16(PlayerSpeedY, 0);
}


static void
ControlPlayer()
{
	Ram(StopPlayerFlag) = 0;
	if (Ram(PlayerDirectionNew) != 0)
		Ram(PlayerDirection) = Ram(PlayerDirectionNew);
	ControlPlayer2();
}


static void
SetStopPlayer()
{
	Ram(StopPlayerFlag) = 1;
	ControlPlayer2();
}


void
ControlPlayerV()
{
	Ram(InAirFlow) = 0;
	if ((Ram(ControlsHold) & 3) != 0)
		ControlPlayer();
	else
		SetStopPlayer();
}


void
ControlPlayerH()
{
	Ram(InAirFlow) = 0;
	if ((Ram(ControlsHold) & 0x0c) != 0)
		ControlPlayer();
	else
		SetStopPlayer();
}


void
ChkControlPlayer()
{
	if (Ram(PlayerControlMod) == CONTROL_PUNCH)
		return;
	if ((Ram(ControlsHold) & 0x0f) != 0)
		ControlPlayer();
	else
		SetStopPlayer();
}


// Not in water, deep water or the box.
void
chkPunch()
{
	if ((Ram(ControlsTrigger) & 0x20) == 0)
		return;
	uint8_t animation = Ram(PlayerAnimation);
	if (animation == 2 || animation == 4 || animation == 7)
		return;
	Ram(PunchCnt) = 8;
	Ram(PlayerAnimation) = 1;
	Ram(PlayerControlMod) = 1;
	ResetPlayerSpd();
}


void
ResetPlayerSpd()
{
	SetRam16(PlayerSpeedY, 0);
	SetRam16(PlayerSpeedX, 0);
}


static void
ChkPunchColl()
{
	uint16_t dy = 0, dx = 0;
	switch (Ram(PlayerDirection)) {
		case 1:
			dy = 0xfe00;
			break;
		case 2:
			dy = 0x0200;
			break;
		case 3:
			dx = 0xfe00;
			break;
		default:
			dx = 0x0200;
			break;
	}
	uint8_t y = static_cast<uint8_t>((Ram16(PlayerYdec) + dy) >> 8);
	uint8_t x = static_cast<uint8_t>((Ram16(PlayerXdec) + dx) >> 8);
	if (ChkTileCollision_(x, y, 0, Ram(PlayerDirection)))
		SetSoundEntryChk(9);	// punching a wall
}


// Stops Snake in front of solid tiles (in room 78 also with the large
// shape, the water channel).
void
ChkPlayerColl()
{
	if (Ram(PlayerControlMod) == 1) {
		ChkPunchColl();
		return;
	}
	uint8_t y = static_cast<uint8_t>((Ram16(PlayerYdec) + Ram16(PlayerSpeedY)) >> 8);
	uint8_t x = static_cast<uint8_t>((Ram16(PlayerXdec) + Ram16(PlayerSpeedX)) >> 8);
	uint8_t direction = Ram(PlayerDirection);
	if (ChkTileCollision_(x, y, 0, direction)) {
		ResetPlayerSpd();
		return;
	}
	if (Ram(Room) != 78)
		return;
	if (ChkTileCollision_(x, y, 2, direction))
		ResetPlayerSpd();
}


void
PunchLogic2()
{
	if (--Ram(PunchCnt) != 0)
		return;
	Ram(PlayerAnimation) = 0;
	Ram(PlayerControlMod) = 0;
}


// In the cabin, up or down moves the elevator (some only one way).
void
ChkCtrlElevator()
{
	if (Ram(PlayerX) >= 0x78)
		return;
	uint8_t mask = 3;
	uint8_t room = Ram(Room);
	if (room >= 243 && room < 247)
		mask = room < 245 ? 1 : 2;
	uint8_t a = Ram(ControlsHold) & mask;
	if (a == 0)
		return;
	uint8_t direction, limit;
	if ((a & 1) != 0) {
		direction = 1;
		limit = Ram(ElevatorLimitUp);
	} else {
		direction = 2;
		limit = Ram(ElevatorLimitDown);
	}
	if (limit == Ram(ElevatorY))
		return;
	Ram(ElevatorDir) = direction;
	Ram(ElevatorStatus) = 0;
	Ram(GameMode) = GAME_MODE_ELEVATOR;
}


static void
SetInWaterMode2(uint8_t animation)
{
	bool same = animation == Ram(PlayerAnimation);
	Ram(PlayerAnimation) = animation;
	if (!same) {
		Ram(PlayerAnimWaitCnt) = 0;
		Ram(PlayerFrameNum) = 0;
	}
	if (Ram(PlayerAnimation) != 4 || Ram(SelectedItem) == SELECTED_OXYGEN)
		return;
	DecrementLife_C(8);	// deep water without oxygen
}


static bool
InRoomList(uint16_t list, int count)
{
	uint8_t room = Ram(Room);
	for (int i = 0; i < count; i++) {
		if (Mem(static_cast<uint16_t>(list + i)) == room)
			return true;
	}
	return false;
}


// Water tiles: #6F-#72 shadow (deep in some rooms), #73-#74 water,
// #75-#76 deep water, #6D bricks (water).
static bool
WaterTile(uint8_t tile)
{
	if (tile < 0x6f || tile >= 0x77)
		return false;
	if (tile < 0x73) {
		SetInWaterMode2(InRoomList(SYM(RoomsWater), 6) ? 2 : 4);
		return true;
	}
	SetInWaterMode2(tile < 0x75 ? 2 : 4);
	return true;
}


void
ChkWater()
{
	if (!InRoomList(SYM(RoomsWater), 10)) {
		if (Ram(PlayerAnimation) != 7)
			Ram(PlayerAnimation) = 0;
		return;
	}
	uint16_t tiles = GetTilePlayer();
	uint8_t h = static_cast<uint8_t>(tiles >> 8);
	uint8_t l = static_cast<uint8_t>(tiles);
	if (WaterTile(h) || WaterTile(l))
		return;
	if (h == 0x6d || l == 0x6d) {
		SetInWaterMode2(2);
		return;
	}
	SetInWaterMode2(Ram(SelectedItem) == SELECTED_BOX ? 7 : 0);
}


// The air flow on the roof (room 53) pushes Snake back without the bomb
// blast suit.
void
ChkRoofAirFlow()
{
	Ram(InAirFlow) = 0;
	if (Ram(Room) != 53 || Ram(SelectedItem) == SELECTED_BOMB_SUIT)
		return;
	uint8_t y = Ram(PlayerY);
	uint8_t x = Ram(PlayerX);
	if (y < 0x50 || y >= 0x60 || x < 0x48 || x >= 0xc0)
		return;
	if ((Ram(ControlsHold) & 2) != 0) {
		Ram(InAirFlow) = 1;
		return;
	}
	Ram(PlayerControlMod) = CONTROL_AIR_FLOW;
	SetSoundEntryChk(0x11);
}


void
ChkStartClimb()
{
	if ((GetTilePlayer() >> 8) != 8)
		return;	// not at a ladder
	if ((Ram(ControlsTrigger) & 1) == 0)
		return;
	Ram(PlayerY) = 0x99;
	Ram(PlayerControlMod) = CONTROL_LADDER_CLIMB;
	Ram(PlayerAnimation) = 5;
	Ram(DirectionMaskOld) = 0;
	Ram(PlayerAnimWaitCnt) = 0;
	Ram(PlayerFrameNum) = 0;
	Ram(PlayerDirection) = 1;
	Ram(DirectionMask) = 1;
	SetRam16(PlayerMovSpeed, 0x100);
	AnimatePlayer();
}


// Only in room 224, at the bottom of the ladder.
void
ChkExitLadders()
{
	if (Ram(Room) != 224 || Ram(PlayerY) != 0x99)
		return;
	uint8_t a = Ram(ControlsTrigger) & 0x0c;
	if (a == 0)
		return;
	bool right = (a & 8) != 0;
	Ram(PlayerDirection) = right ? 4 : 3;
	Ram(DirectionMask) = right ? 8 : 4;
	Ram(PlayerY) = 0x9e;
	Ram(PlayerControlMod) = CONTROL_LADDER_WALK;
	SetRam16(PlayerMovSpeed, 0x200);
	Ram(PlayerAnimation) = 0;
	Ram(DirectionMaskOld) = 0;
	Ram(PlayerAnimWaitCnt) = 0;
	Ram(PlayerFrameNum) = 0;
	AnimatePlayer();
}


void
ChkExitRoom()
{
	uint8_t x = static_cast<uint8_t>(MovePlayerX() >> 8);
	if (x < 12) {
		ExitRoom(3);
		return;
	}
	if (x >= 244) {
		ExitRoom(4);
		return;
	}
	uint8_t y = static_cast<uint8_t>(MovePlayerY() >> 8);
	if (y < 16)
		ExitRoom(1);
	else if (y >= 186)
		ExitRoom(2);
}


void
ChkLimitXElevator()
{
	uint16_t hl = MovePlayerX();
	uint8_t x = static_cast<uint8_t>(hl >> 8);
	if (x < 104) {
		SetRam16(PlayerXdec, static_cast<uint16_t>((104 << 8) | (hl & 0xff)));
		return;
	}
	if (x >= 244)
		ExitRoom(DIR_RIGHT);
}


// The last ladder room (226) leads out of Outer Heaven: the ending.
void
ChkLadderLimits()
{
	uint16_t hl = MovePlayerY();
	uint8_t y = static_cast<uint8_t>(hl >> 8);
	uint8_t room = Ram(Room);
	if (room == 226 && y < 0x10) {
		Ram(LeavedOuterHeaven) = 1;
		return;
	}
	if (room == 224 && y >= 0x99) {
		SetRam16(PlayerYdec, static_cast<uint16_t>((0x99 << 8) | (hl & 0xff)));
		return;
	}
	if (y < 16)
		ExitRoom(DIR_UP);
	else if (y >= 186)
		ExitRoom(DIR_DOWN);
}


void
ExitRoom(uint8_t direction)
{
	Ram(NextRoomDirect) = direction;
	Ram(GameMode) = GAME_MODE_NEXT_ROOM;
}


uint16_t
MovePlayerX()
{
	uint16_t hl = static_cast<uint16_t>(Ram16(PlayerXdec) + Ram16(PlayerSpeedX));
	SetRam16(PlayerXdec, hl);
	return hl;
}


uint16_t
MovePlayerY()
{
	uint16_t hl = static_cast<uint16_t>(Ram16(PlayerYdec) + Ram16(PlayerSpeedY));
	SetRam16(PlayerYdec, hl);
	return hl;
}


static void
UpdateLevels(uint16_t rank)
{
	DrawClass();
	static const uint8_t kLife[] = { 24, 32, 40, 48 };
	uint8_t c = Ram(rank);
	uint8_t life = (c >= 1 && c <= 3) ? kLife[c] : 24;
	Ram(MaxLife) = life;
	Ram(Life) = life;
	DrawLife();
	SetMaxAmmoVals();
}


// Captured: the rescued prisoners (but Jennifer's brother) are lost and
// the rank goes down.
void
DowngradeRank()
{
	uint8_t brother = Ram(static_cast<uint16_t>(RescuedArray + 0x0d));
	for (int i = 0; i < 18; i++)
		Ram(static_cast<uint16_t>(RescuedArray + i)) = 0;
	Ram(static_cast<uint16_t>(RescuedArray + 0x0d)) = brother;
	Ram(RescuedCnt) = 0;
	if (Ram(Class) == 0)
		return;
	Ram(Class)--;
	SetSoundEntryChk(0x27);
	uint8_t life = Ram(Life);
	UpdateLevels(Class);
	if (Ram(MaxLife) < life)
		life = Ram(MaxLife);
	Ram(Life) = life;
	LimitAmmo();
	DrawWeaponHUD();
}


// Every 5 rescued prisoners: one rank up.
void
IncRescued()
{
	if (++Ram(RescuedCnt) != 5)
		return;
	Ram(RescuedCnt) = 0;
	IncClassLv();
}


void
IncClassLv()
{
	if (Ram(Class) == 3)
		return;
	Ram(Class)++;
	SetSoundEntryChk(0x26);
	UpdateLevels(Class);
}


void
SetSprIdle()
{
	Ram(PlayerAnimWaitCnt) = 0;
	Ram(PlayerFrameNum) = 0;
	uint8_t c = static_cast<uint8_t>((Ram(PlayerDirection) - 1) * 3);
	uint8_t weapon = Ram(SelectedWeapon);
	if (weapon != 0 && weapon != PLASTIC_BOMB && weapon != LAND_MINE)
		c = static_cast<uint8_t>(c + 12);
	SetPlayerSprId(static_cast<uint8_t>(Ram(PlayerFrameNum) + c));
}


static void
SetSprWalk()
{
	if (Ram(StopPlayerFlag) != 0) {
		SetSprIdle();
		return;
	}
	if (++Ram(PlayerAnimWaitCnt) == 6) {
		Ram(PlayerAnimWaitCnt) = 0;
		Ram(PlayerFrameNum)++;
	}
	if (Ram(PlayerFrameNum) == 3)
		Ram(PlayerFrameNum) = 1;
	uint8_t c = static_cast<uint8_t>((Ram(PlayerDirection) - 1) * 3);
	uint8_t weapon = Ram(SelectedWeapon);
	if (weapon != 0 && weapon != PLASTIC_BOMB && weapon != LAND_MINE)
		c = static_cast<uint8_t>(c + 12);
	SetPlayerSprId(static_cast<uint8_t>(Ram(PlayerFrameNum) + c));
}


void
AnimatePlayer()
{
	uint8_t direction = static_cast<uint8_t>(Ram(PlayerDirection) - 1);
	switch (Ram(PlayerAnimation)) {
		case 0:
			SetSprWalk();
			break;
		case 1:	// punch
			SetPlayerSprId(static_cast<uint8_t>(direction + 24));
			break;
		case 2:
		{
			// water
			uint8_t c = static_cast<uint8_t>(direction + (Ram(SelectedWeapon) != 0 ? 32 : 28));
			SetPlayerSprId(static_cast<uint8_t>(c + ((Ram(TickCounter) & 8) != 0 ? 17 : 0)));
			break;
		}
		case 3:	// parachute
			SetPlayerSprId(36);
			break;
		case 4:
			// deep water
			if (++Ram(PlayerAnimWaitCnt) != 4)
				return;
			Ram(PlayerAnimWaitCnt) = 0;
			Ram(PlayerFrameNum)++;
			SetPlayerSprId(static_cast<uint8_t>((Ram(PlayerFrameNum) & 1) + 37));
			break;
		case 5:
			// ladder
			if (Ram(StopPlayerFlag) != 0) {
				Ram(PlayerAnimWaitCnt) = 0;
			} else {
				if (++Ram(PlayerAnimWaitCnt) != 6)
					return;
				Ram(PlayerAnimWaitCnt) = 0;
				Ram(PlayerFrameNum)++;
			}
			SetPlayerSprId(static_cast<uint8_t>((Ram(PlayerFrameNum) & 1) + 39));
			break;
		case 6:
		{
			// dead: spinning, leaning, lying
			uint8_t t = Ram(DeadTimer);
			uint8_t c;
			if (t < 0x40) {
				c = 43;
			} else if (t < 0x70) {
				c = 41;
			} else {
				uint8_t x = t & 0x0c;
				c = static_cast<uint8_t>((x >> 1) + (x >> 2));
			}
			SetPlayerSprId(c);
			break;
		}
		case 7:	// box
			if (Ram(StopPlayerFlag) != 0)
				SetPlayerSprId(42);
			else
				SetPlayerSprId((Ram(TickCounter) & 8) != 0 ? 44 : 42);
			break;
		default:
			if (gUnported == NULL)
				gUnported = "AnimatePlayer: bad animation";
			break;
	}
}


// Where a grenade would land (not in the box, ladders, elevators, room 204).
void
SetGrenaTargetSpr()
{
	if (Ram(Room) == 204 || Ram(SelectedWeapon) != GRENADE_LAUNCHER)
		return;
	bool hide = Ram(PlayerAnimation) == 7 || Ram(Room) >= 224;
	if (!hide) {
		for (int i = 0; i < 16; i++)
			Ram(static_cast<uint16_t>(GrenadTargetCol + i)) = 0x0e;
		uint8_t y = Ram(PlayerY);
		uint8_t x = Ram(PlayerX);
		uint8_t direction = static_cast<uint8_t>(Ram(PlayerDirection) - 1);
		uint16_t offset = static_cast<uint16_t>(SYM(TargetXYOffsets) + static_cast<uint8_t>(direction * 2));
		Ram(GrenaTargetAtt) = static_cast<uint8_t>(Mem(offset) + y);
		Ram(static_cast<uint16_t>(GrenaTargetAtt + 1)) = static_cast<uint8_t>(Mem(static_cast<uint16_t>(offset + 1)) + x);
		Ram(static_cast<uint16_t>(GrenaTargetAtt + 2)) = 0x18;
		switch (direction) {
			case 0:
				hide = y < 0x38;
				break;
			case 1:
				hide = y >= 0x80;
				break;
			case 2:
				hide = x < 0x50;
				break;
			default:
				hide = x >= 0xc0;
				break;
		}
	}
	if (hide)
		Ram(GrenaTargetAtt) = 225;
}


bool
ChkTileCollision(uint16_t ix, uint8_t shape, uint8_t direction)
{
	return ChkTileCollision_(Ram(static_cast<uint16_t>(ix + ACTOR::X)),
		Ram(static_cast<uint16_t>(ix + ACTOR::Y)), shape, direction);
}


// Two points per side and shape (BoxColliderDat); CollisionTiles bit 0.
bool
ChkTileCollision_(uint8_t x, uint8_t y, uint8_t shape, uint8_t direction)
{
	Ram(TempData) = y;
	Ram(static_cast<uint16_t>(TempData + 1)) = x;
	uint8_t index = static_cast<uint8_t>((shape * 4 + direction) * 4);
	uint16_t hl = static_cast<uint16_t>(SYM(BoxColliderDat) - 4 + index);
	for (int i = 0; i < 2; i++) {
		uint8_t py = static_cast<uint8_t>(Ram(TempData) + Mem(hl++));
		uint8_t px = static_cast<uint8_t>(Ram(static_cast<uint16_t>(TempData + 1)) + Mem(hl++));
		uint8_t tile = Ram(CoordToBuffTile(px, py));
		if ((Ram(static_cast<uint16_t>(CollisionTiles + tile)) & 1) != 0)
			return true;
	}
	return false;
}


uint16_t
GetTilePlayer()
{
	return GetTileInXY(static_cast<uint8_t>(Ram(PlayerX) - 4), Ram(PlayerY));
}


uint16_t
GetTileInXY(uint8_t x, uint8_t y)
{
	uint16_t hl = CoordToBuffTile(x, y);
	return static_cast<uint16_t>((Ram(hl) << 8) | Ram(static_cast<uint16_t>(hl + 1)));
}


// Gas rooms hurt without the gas mask.
void
ChkGasRooms()
{
	if (!InRoomList(SYM(GasRooms), 9) || Ram(SelectedItem) == SELECTED_GAS_MASK)
		return;
	DecrementLife_C(0x10);
}


void
DecrementLife_C(uint8_t delay)
{
	if (Ram(DamageDelayTimer) != 0)
		return;
	Ram(DamageDelayTimer) = delay;
	DecrementLife_2();
}


void
DecrementLife_2()
{
	DecrementLife_B(2);
}
