/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/drawlaserbeams.asm, logic/actors/rendercameras.asm,
// mine.asm, pitfall.asm, hindd.asm, tank.asm (SetBossMusic) and
// BanksABC.asm (InitMetalGear_ .. RemoveMetalGear).
#include "RoomActors.h"

#include "Actors.h"
#include "Common.h"
#include "GameGfx.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
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


// Number of passes of a DJNZ loop started with this count.
inline int
Loops(uint8_t count)
{
	return count == 0 ? 256 : count;
}

}	// namespace


// --- Laser beams (rooms 24, 25 and 72) --------------------------------------

// The beams are visible with the goggles (not in an alert).
void
DrawLaserBeams()
{
	if (Ram(SelectedItem) != SELECTED_GOGGLES)
		return;
	if (Ram(AlertMode) != 0)
		return;
	uint8_t room = Ram(Room);
	if (room != 24 && room != 25 && room != 72)
		return;
	uint16_t hl = EnemyList;
	int count = Loops(Ram(NumEnemies));
	for (int i = 0; i < count; i++, hl = Add16(hl, 0x80)) {
		if ((Mem(Add16(hl, 1)) & 1) != 0) {
			uint8_t y = Mem(Add16(hl, 3));
			uint8_t x = Mem(Add16(hl, 5));
			uint8_t length = Mem(Add16(hl, 0x15));
			if (Mem(Add16(hl, 0x16)) == 0) {
				DrawLineVert(x, y, length, 8);
				SetBanks_1_2_X();
			} else {
				DrawLineHoriz(x, y, length, 8);
				SetBanks_1_2_X();
			}
		}
		SetBanks_1_2_X();	// ADD_HL_A_
	}
}


void
RestLasersBack()
{
	uint16_t hl = EnemyList;
	int count = Loops(Ram(NumEnemies));
	for (int i = 0; i < count; i++, hl = Add16(hl, 0x80)) {
		if ((Mem(Add16(hl, 1)) & 1) != 0) {
			uint8_t dy = Mem(Add16(hl, 3));
			uint8_t dx = Mem(Add16(hl, 5));
			SetBanks_1_2_X();	// ADD_HL_A_
			uint8_t horizontal = Mem(Add16(hl, 0x16));
			uint8_t length = Mem(Add16(hl, 0x15));
			uint8_t sx = Mem(Add16(hl, 0x14));
			uint8_t sy = Mem(Add16(hl, 0x13));
			uint8_t nx = length;
			uint8_t ny = 2;	// beam width
			if ((horizontal & 1) == 0) {
				nx = 2;
				ny = length;
			}
			VDP_Copy_Byte(sx, sy, 1, dx, dy, nx, ny);
			SetBanks_1_2_X();
		}
		SetBanks_1_2_X();	// ADD_HL_A_
	}
}


// --- Cameras: drawn into the room in an alert ------------------------------

void
RenderCamera(uint16_t ix)
{
	A(ix, ACTOR::Status) = 2;	// does nothing
	A(ix, ACTOR::Moving) = 0;
	A(ix, ACTOR::COLLISION_CFG) = 0;
	HideSprColor(ix);
	DrawCamera(ix);
}


void
DrawCamera(uint16_t ix)
{
	uint8_t sx = static_cast<uint8_t>((A(ix, ACTOR::BASE_SPR_ID) << 4) + 0x20);
	uint8_t dy = static_cast<uint8_t>(A(ix, ACTOR::Y) - 8);
	uint8_t dx = static_cast<uint8_t>(A(ix, ACTOR::X) - 8);
	VDP_Copy_Dot(sx, 0x80, 0x48, dx, dy, 0x10, 0x10);
}


void
DrawCameras()
{
	if (Ram(AlertModeCopy) == 0)
		return;
	uint16_t hl = EnemyList;
	for (int i = 0; i < 16; i++, hl = Add16(hl, 0x80)) {
		if (Ram(hl) == ID_CAMERA)
			DrawCamera(hl);
	}
}


// --- Mines: seen with the mine detector ------------------------------------

void
InitMines(uint16_t ix)
{
	A(ix, ACTOR::IDX_SAME_ID) = static_cast<uint8_t>(CountEnemyType(ID_LAND_MINE) - 1);
	uint8_t dx;
	uint16_t source = GetMineBackXY(ix, A(ix, ACTOR::X), A(ix, ACTOR::Y), &dx);
	// saves the background in page 1
	VDP_Copy_Dot(static_cast<uint8_t>(source >> 8), static_cast<uint8_t>(source), 0x10, dx,
		160, 0x10, 0x10);
	if (Ram(SelectedItem) != SELECTED_MINE_DETECTOR)
		return;
	DrawMine(ix);
}


void
DrawMine(uint16_t ix)
{
	uint8_t dx = static_cast<uint8_t>(A(ix, ACTOR::X) - 3);
	uint8_t dy = static_cast<uint8_t>(A(ix, ACTOR::Y) - 3);
	VDP_Copy_Dot(0x95, 0x65, 0x48, dx, dy, 5, 5);
}


uint16_t
GetMineBackXY(uint16_t ix, uint8_t x, uint8_t y, uint8_t* sx)
{
	uint8_t index = A(ix, ACTOR::IDX_SAME_ID);
	*sx = static_cast<uint8_t>((index << 4) | (index >> 4));	// RLCA x4
	return static_cast<uint16_t>((static_cast<uint8_t>(x - 8) << 8) | static_cast<uint8_t>(y - 8));
}


void
DrawMines()
{
	if (Ram(SelectedItem) != SELECTED_MINE_DETECTOR)
		return;
	uint16_t ix = EnemyList;
	for (int i = 0; i < 16; i++, ix = Add16(ix, 0x80)) {
		if (A(ix, ACTOR::ID) == ID_LAND_MINE)
			DrawMine(ix);
	}
}


// --- Pitfalls ---------------------------------------------------------------

namespace {

// Page 1 holds the pitfall fully open; the hole grows from the middle.
void
OpenPitfall(uint16_t ix)
{
	uint8_t hole = static_cast<uint8_t>(A(ix, PITFALL::HOLE_SIZE) + 2);
	A(ix, PITFALL::HOLE_SIZE) = hole;
	if (hole >= 0x40) {
		A(ix, PITFALL::DestinationX) = 0x16;
		A(ix, PITFALL::RENDER_SX) = 0x40;
		A(ix, PITFALL::RENDER_SY) = 0xa0;
		A(ix, PITFALL::RENDER_DY) &= 0xfe;
		A(ix, PITFALL::RENDER_DX) &= 0xfe;
		NextActorStatus(ix);	// fully open
		return;
	}
	uint8_t dy = --A(ix, PITFALL::RENDER_DY);
	uint8_t dx = --A(ix, PITFALL::RENDER_DX);
	uint8_t sy = --A(ix, PITFALL::RENDER_SY);
	uint8_t sx = --A(ix, PITFALL::RENDER_SX);
	VDP_Copy_Byte(sx, sy, 1, dx, dy, hole, 1);	// top line
	VDP_Copy_Byte(sx, static_cast<uint8_t>(sy + hole), 1, dx, static_cast<uint8_t>(dy + hole),
		hole, 1);	// bottom line
	if ((hole & 2) != 0)
		return;	// the sides every other step
	VDP_Copy_Byte(sx, sy, 1, dx, dy, 2, hole);	// left side
	uint8_t offset = static_cast<uint8_t>(hole - 2);
	VDP_Copy_Byte(static_cast<uint8_t>(sx + offset), sy, 1, static_cast<uint8_t>(dx + offset),
		dy, 2, hole);	// right side
}

}	// namespace


void
InitPitfall(uint16_t ix)
{
	SetColorsIndexes2(SYM(ColorsPitfall), 4);
	SetupPitfall(SYM(GfxPitfall), GfxPitfallBuffer, 0x0c);
	A(ix, ACTOR::COLLISION_CFG) = 5;	// touches the player, closed
	if (Ram(Room) != 190)
		return;
	// (!?) there is no pitfall in room 190, only a prisoner
	A(ix, PITFALL::Status) = 2;
	A(ix, PITFALL::HOLE_SIZE) = 0x3f;
	A(ix, PITFALL::RENDER_DX) = static_cast<uint8_t>(A(ix, PITFALL::X) - 0x20);
	A(ix, PITFALL::RENDER_DY) = static_cast<uint8_t>(A(ix, PITFALL::Y) - 0x20);
	A(ix, PITFALL::RENDER_SX) = 0x40;
	A(ix, PITFALL::RENDER_SY) = 0xa0;
	RenderPitfallP0(ix);
}


void
PitfallLogic(uint16_t ix)
{
	uint8_t status = A(ix, PITFALL::Status);
	if (status == 0) {
		if ((A(ix, PITFALL::TOUCH_INFO) & 0x80) == 0)
			return;	// not stepped on
		A(ix, PITFALL::COLLISION_CFG) = 1;
		A(ix, PITFALL::Visible) = 1;
		A(ix, PITFALL::RENDER_DX) = A(ix, PITFALL::X);
		A(ix, PITFALL::RENDER_DY) = A(ix, PITFALL::Y);
		A(ix, PITFALL::RENDER_SX) = 0x60;
		A(ix, PITFALL::RENDER_SY) = 0xc0;
		A(ix, PITFALL::HOLE_SIZE) = 0;
		SetSoundEntryChk(7);
		NextActorStatus(ix);
		return;
	}
	if (status == 1)
		OpenPitfall(ix);
}


void
DrawOpenPitfalls()
{
	Ram(TempData) = 0;	// page 1 does not hold the pitfall yet
	uint16_t hl = EnemyList;
	for (int i = 0; i < 16; i++, hl = Add16(hl, 0x80)) {
		if (Ram(hl) != ID_PITFALL)
			continue;
		if (Ram(TempData)++ == 0)
			DrawPitfall_();
		if (A(hl, PITFALL::Visible) != 0)
			RenderPitfallP0(hl);
	}
}


void
RenderPitfallP0(uint16_t ix)
{
	uint8_t dx = static_cast<uint8_t>(A(ix, PITFALL::RENDER_DX) & 0xfe);
	uint8_t dy = A(ix, PITFALL::RENDER_DY);
	uint8_t sx = static_cast<uint8_t>(A(ix, PITFALL::RENDER_SX) & 0xfe);
	uint8_t sy = A(ix, PITFALL::RENDER_SY);
	if (A(ix, PITFALL::HOLE_SIZE) >= 0x40)
		A(ix, PITFALL::HOLE_SIZE) = 0x3f;
	uint8_t size = A(ix, PITFALL::HOLE_SIZE);
	VDP_Copy_Byte(sx, sy, 1, dx, dy, size, static_cast<uint8_t>(size + 1));
}


// --- Hind D -----------------------------------------------------------------

void
SetBossMusic()
{
	if (Ram(GameMode) == GAME_MODE_BINOCULARS)
		return;
	Ram(AreaMusic) = 0x3e;	// Mercenary
	SetSoundEntryChk(0x3e);
}


void
InitHindD(uint16_t ix)
{
	if (Ram(BossHindD_KO) != 0) {
		DismissActor(ix);
		return;
	}
	A(ix, HIND_D::ShootDelay) = 5;
	A(ix, HIND_D::NumberShots) = 5;
	A(ix, HIND_D::KO_POINTER_L) = static_cast<uint8_t>(BossHindD_KO);
	A(ix, HIND_D::KO_POINTER_H) = static_cast<uint8_t>(BossHindD_KO >> 8);
	A(ix, HIND_D::SpriteId) = 0x86;	// propeller
	SetBossMusic();
	ChkDrawHindD();
}


void
ChkDrawHindD()
{
	if (Ram(EnemyList) != ID_HIND_D)
		return;
	DrawTileBlkTimp(SYM(HindDTileMap), 0x40, 0);
}


// Turns the propeller, fires bursts of five bullets.
void
HindDLogic(uint16_t ix)
{
	uint8_t frame = static_cast<uint8_t>(A(ix, HIND_D::AnimationCnt) + 1);
	if (frame >= 3)
		frame = 0;
	A(ix, HIND_D::AnimationCnt) = frame;
	A(ix, HIND_D::SpriteId) = static_cast<uint8_t>(frame + 0x86);
	if ((A(ix, HIND_D::ANIM_CNT) & 3) == 0)
		SetSoundEntry(2);	// propeller

	if ((A(ix, HIND_D::Status) & 1) != 0) {
		// waits between bursts
		if (--A(ix, HIND_D::ShootDelay) != 0)
			return;
		A(ix, HIND_D::ShootDelay) = 5;
		A(ix, HIND_D::NumberShots) = 5;
		A(ix, HIND_D::Status) = 0;
		return;
	}
	if (--A(ix, HIND_D::ShootDelay) != 0)
		return;
	A(ix, HIND_D::ShootDelay) = 5;
	if (--A(ix, HIND_D::NumberShots) == 0) {
		A(ix, HIND_D::Status) = 1;
		A(ix, HIND_D::ShootDelay) = 0x11;
		return;
	}
	AddEnemy(ID_BULLET, A(ix, HIND_D::Y), A(ix, HIND_D::X));
}


void
RemoveHindD(uint16_t ix)
{
	DrawTileBlkTimp(SYM(HindDTileMap2), 0x40, 0);
	DismissActor(ix);
}


void
DummyLogic2(uint16_t)
{
}


// --- Metal Gear -------------------------------------------------------------

void
InitMetalGear_(uint16_t ix)
{
	if (Ram(MetalGear_KO) != 0) {
		DismissActor(ix);
		SetBanks_1_2_X();
		return;
	}
	A(ix, ACTOR::BASE_SPR_ID) = static_cast<uint8_t>(MetalGear_KO);
	A(ix, ACTOR::KO_POINTER_H) = static_cast<uint8_t>(MetalGear_KO >> 8);
	A(ix, ACTOR::COLLISION_CFG) = 3;
	ChkDrawMetalGear(ix);
}


// 'ix' is the original's IX, which may be any actor here.
void
ChkDrawMetalGear(uint16_t ix)
{
	if (Ram(EnemyList) != ID_METAL_GEAR)
		return;
	A(ix, ACTOR::Status) = 0;
	DrawMetalGear(0x60, 0x20);
	SetBanks_1_2_X();
}


void
RemoveMetalGear(uint16_t)
{
	EraseMetalGear(0x60, 0x20);
	SetBanks_1_2_X();
}
