/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/common.asm (with touchenemy, punchenemy,
// laserbeams, pitfall, damagelaser, bridge, damageelectric and
// damagetoenemy), incomingcall.asm and destructiontimer.asm.
#include "Contact.h"

#include "Actors.h"
#include "Common.h"
#include "Doors.h"
#include "GameMain.h"
#include "Hud.h"
#include "Items.h"
#include "Player.h"
#include "Rom.h"
#include "Structures.h"
#include "Weapons.h"


using namespace var;


namespace {

inline uint8_t&
A(uint16_t ix, int field)
{
	return Ram(static_cast<uint16_t>(ix + field));
}


uint16_t
Add16(uint16_t a, int b)
{
	return static_cast<uint16_t>(a + b);
}


// |a - b| as the original's SUB / NEG.
uint8_t
Distance(uint8_t a, uint8_t b)
{
	return a >= b ? static_cast<uint8_t>(a - b) : static_cast<uint8_t>(b - a);
}


// Is the point inside the area (Y offset, Y radius, X offset, X radius;
// banks as the caller has them) of the actor?
bool
ChkArea(uint16_t area, uint16_t ix, uint8_t x, uint8_t y)
{
	uint8_t dy = Distance(static_cast<uint8_t>(A(ix, ACTOR::Y) + Mem(area)), y);
	if (dy >= Mem(Add16(area, 1)))
		return false;
	uint8_t dx = Distance(static_cast<uint8_t>(A(ix, ACTOR::X) + Mem(Add16(area, 2))), x);
	return dx < Mem(Add16(area, 3));
}


void
PlayDamageSfx()
{
	SetSoundEntry(0x10);
}


// Damage by the actor's kind; the armour halves bullets.
void
TouchPlayer(uint16_t ix)
{
	uint8_t id = A(ix, ACTOR::ID);
	if (id >= ID_PRISONER1) {
		if (id < ID_CAMERA_LASER || id == ID_FAKE_MADNAR)
			return;	// prisoners, Ellen, Grey Fox, Madnar
	}
	if (Ram(DamageDelayTimer) != 0)
		return;
	PlayDamageSfx();
	Ram(DamageDelayTimer) = 0x20;
	SetRam16(TempData, SYM(ActorTouchDamage));
	uint8_t damage = GetTempPHL_A(static_cast<uint8_t>(id - 1));
	bool bullet = id == ID_SGUNNER_SHOT || id == ID_GUARD_BULLET
		|| (id >= ID_BULLET_HORIZ && id < ID_BOOMERANG);
	if (bullet && Ram(SelectedItem) == SELECTED_ARMOR)
		damage >>= 1;
	DecrementLife_B(damage);
}


void
ChkPunchEnemy(uint16_t ix, uint8_t x, uint8_t y)
{
	uint8_t id = A(ix, ACTOR::ID);
	uint16_t list = SYM(PunchEnemies);
	bool punchable = false;
	for (int i = 0; i < 12; i++) {
		if (Mem(Add16(list, i)) == id) {
			punchable = true;
			break;
		}
	}
	if (!punchable)
		return;
	sym::Id area;
	switch (Ram(PlayerDirection)) {
		case 1:		area = sym::PunchUpDat; break;
		case 2:		area = sym::PunchDownDat; break;
		case 3:		area = sym::PunchLeftDat; break;
		default:	area = sym::PunchRightDat; break;
	}
	if (!ChkArea(RomSymbol(area), ix, x, y))
		return;
	if (A(ix, ACTOR::StunnedCnt) >= 0x38)
		return;	// not too fast
	A(ix, ACTOR::StunnedCnt) = 0;
	A(ix, ACTOR::TOUCH_INFO) |= 0x40;
	SetSoundEntry(8);
}


// A laser beam touched: the alarm and all beams go off.
void
ChkTouchLaser(uint16_t ix, uint8_t x, uint8_t y)
{
	if ((A(ix, ACTOR::Status) & 1) == 0)
		return;
	uint8_t half = static_cast<uint8_t>(A(ix, ACTOR::DestinationY) >> 1);
	if ((A(ix, ACTOR::DestinationX) & 1) == 0) {
		// a vertical beam at X
		if (Distance(x, A(ix, ACTOR::X)) >= 4)
			return;
		uint8_t centre = static_cast<uint8_t>(half + 8 + A(ix, ACTOR::Y));
		if (Distance(centre, y) >= half)
			return;
	} else {
		if (Distance(y, A(ix, ACTOR::Y)) >= 4)
			return;
		uint8_t centre = static_cast<uint8_t>(half + A(ix, ACTOR::X));
		if (Distance(centre, x) >= half)
			return;
	}
	A(ix, ACTOR::TOUCH_INFO) |= 0x80;
	SetAlertModeRespawn(ix, 0x5a);
	RestLaserBack_();
	RemoveLaserBeans();
}


void
ChkPitfall(uint16_t ix, uint8_t x, uint8_t y)
{
	if (A(ix, PITFALL::COLLISION_CFG) & 4) {
		// closed: does Snake come near?
		if (Distance(A(ix, PITFALL::Y), y) >= 40)
			return;
		if (Distance(A(ix, PITFALL::X), x) >= 40)
			return;
		A(ix, PITFALL::TOUCH_INFO) |= 0x80;
		return;
	}
	uint8_t half = static_cast<uint8_t>(A(ix, PITFALL::HOLE_SIZE) >> 1);
	if (Distance(static_cast<uint8_t>(A(ix, PITFALL::RENDER_DY) + half), y) >= half)
		return;
	if (Distance(static_cast<uint8_t>(A(ix, PITFALL::RENDER_DX) + half), x) >= half)
		return;
	DecrementLife_B(0xff);
}


// The cameras' laser shots; the two cameras in room 111 shoot short.
void
ChkLaserShot(uint16_t ix, uint8_t x, uint8_t y)
{
	if (Distance(A(ix, ACTOR::X), x) >= 8)
		return;
	uint8_t length = A(ix, ACTOR::ANIM_CNT);
	if (Ram(Room) == 111 && length >= 8)
		length = 7;
	if (length == 0x0c || length == 0)
		return;
	uint8_t index = static_cast<uint8_t>(length - 1);
	uint8_t d = index >= 0x0c ? 0x0c : Mem(Add16(SYM(LaserLenghts), index));
	d = static_cast<uint8_t>(d * 8);
	if (Distance(static_cast<uint8_t>(d + A(ix, ACTOR::Y)), y) >= d)
		return;
	TouchPlayer(ix);
}


void
ChkTouchEnemy(uint16_t ix, uint8_t x, uint8_t y)
{
	A(ix, ACTOR::TOUCH_INFO) &= 0x1f;
	uint8_t id = A(ix, ACTOR::ID);
	if (id == ID_LASER) {
		ChkTouchLaser(ix, x, y);
		return;
	}
	if ((A(ix, ACTOR::COLLISION_CFG) & 1) == 0)
		return;
	if (id == ID_PITFALL) {
		ChkPitfall(ix, x, y);
		return;
	}
	if (id == ID_LASER_SHOT) {
		ChkLaserShot(ix, x, y);
		return;
	}
	if (Ram(ControlsTrigger) & 0x20) {
		if (Ram(PlayerAnimation) == 1)
			ChkPunchEnemy(ix, x, y);
		if (A(ix, ACTOR::TOUCH_INFO) & 0x40)
			return;
	}
	uint8_t shape = GET_HL_A_DEC(SYM(ActorsShapeTouch), A(ix, ACTOR::ID));
	if (shape == 0xff)
		return;
	if (!ChkArea(GetShapeInfo(static_cast<uint8_t>(shape + 1)), ix, x, y))
		return;
	if (A(ix, ACTOR::ID) == ID_BRIDGE) {
		Ram(isOnBridge) = 1;
		A(ix, ACTOR::TOUCH_INFO) |= 0x80;
		return;
	}
	if (A(ix, ACTOR::StunnedCnt) != 0)
		return;
	A(ix, ACTOR::TOUCH_INFO) |= 0x80;
	if (A(ix, ACTOR::ID) == ID_LAND_MINE)
		A(ix, ACTOR::LIFE) = 0;
	if (A(ix, ACTOR::ID) == ID_SCORPION) {
		Ram(Poisoned) = 1;
		PlayDamageSfx();
		return;
	}
	TouchPlayer(ix);
}


// Plastic bombs on Metal Gear's legs must follow an order.
void
ChkEneHitByShot(uint16_t ix, uint16_t iy)
{
	if ((A(iy, ACTOR::COLLISION_CFG) & 2) == 0)
		return;
	uint8_t shape = GET_HL_A_DEC(Ram16(TempData2), A(iy, ACTOR::ID));
	if (shape == 0xff)
		return;
	uint16_t area = GetShapeInfo(static_cast<uint8_t>(shape + 1));
	A(iy, ACTOR::TOUCH_INFO) &= 0xe0;
	uint8_t dy = Distance(static_cast<uint8_t>(A(iy, ACTOR::Y) + Ram(area)),
		A(ix, PLAYER_SHOT::Y));
	if (dy >= Ram(Add16(area, 1)))
		return;
	uint8_t dx = Distance(static_cast<uint8_t>(A(iy, ACTOR::X) + Ram(Add16(area, 2))),
		A(ix, PLAYER_SHOT::X));
	if (dx >= Ram(Add16(area, 3)))
		return;
	uint8_t shot = A(ix, PLAYER_SHOT::ID);
	if ((shot == ROCKET_LAUNCHER || shot == LAND_MINE || shot == MISSILE)
		&& A(ix, PLAYER_SHOT::status) == 0)
		A(ix, PLAYER_SHOT::status)++;	// explodes
	A(iy, ACTOR::TOUCH_INFO) |= A(ix, PLAYER_SHOT::ID);
	uint8_t damage = GetTempPHL_A(static_cast<uint8_t>(A(iy, ACTOR::ID) - 1));
	bool hurt = damage != 0xff;
	if (hurt && Ram(WeaponInUse) == PLASTIC_BOMB && A(iy, ACTOR::ID) == ID_METAL_GEAR) {
		hurt = false;
		if (A(ix, PLAYER_SHOT::status) == 1) {
			uint8_t leg = A(iy, ACTOR::X) >= A(ix, PLAYER_SHOT::X) ? B_LEFT : B_RIGHT;
			for (int i = 0x0f; i > 0; i--)
				Ram(Add16(BombOrderBuffer, i)) = Ram(Add16(BombOrderBuffer, i - 1));
			Ram(BombOrderBuffer) = leg;
			hurt = true;
			for (int i = 0; i < 0x10; i++) {
				if (Ram(Add16(BombOrderBuffer, i)) != Mem(Add16(SYM(PlasticBombOrder), i))) {
					hurt = false;
					break;
				}
			}
			if (hurt) {
				Ram(Add16(EnemyListEntry1, 0x0d)) = 0;	// the laser cameras
				Ram(Add16(EnemyListEntry2, 0x0d)) = 0;
				damage = 2;
			}
		}
	}
	if (hurt) {
		uint8_t life = A(iy, ACTOR::LIFE);
		A(iy, ACTOR::LIFE) = life >= damage ? static_cast<uint8_t>(life - damage) : 0;
	}
	if (A(ix, PLAYER_SHOT::ID) < GRENADE_LAUNCHER)
		RemoveShot(ix);	// bullets end; the rest explode
}

}	// namespace


void
CommonLogic()
{
	if (Ram(GameMode) == GAME_MODE_DEAD)
		return;
	ChkPlayerShots();
	ChkTouchEnemies();
	ChkOnBridge();
	ChkElectricFloor();
	ChkGasRooms();
	if (Ram(GameMode) == GAME_MODE_DEAD)
		return;
	ChkDoors();
	ChkTakeItems();
	// captured (once) in room 8
	if (Ram(EquipBagTaken) != 0 || Ram(Room) != 8)
		return;
	uint8_t x = Ram(PlayerX);
	if (x >= 0xd0 || x < 0xc0)
		return;
	Ram(GameMode) = GAME_MODE_CAPTURED;
	Ram(CaptureStatus) = 0;
}


void
ChkPlayerShots()
{
	uint8_t weapon = Ram(WeaponInUse);
	if (weapon == 0)
		return;
	bool projectile = weapon < GRENADE_LAUNCHER || weapon == ROCKET_LAUNCHER
		|| weapon == MISSILE;
	SetRam16(TempData2, projectile ? SYM(ActorShapeProject) : SYM(ActorShapeExpl));
	uint8_t count = GetWeaponDamages(weapon);
	uint16_t ix = PlayerShotsList;
	do {
		if (A(ix, PLAYER_SHOT::ID) != 0 && (A(ix, PLAYER_SHOT::KILL_BY_CONTACT) & 1)) {
			uint16_t iy = EnemyList;
			for (int i = 0; i < 16; i++, iy = Add16(iy, 0x80)) {
				if (A(iy, ACTOR::ID) != 0)
					ChkEneHitByShot(ix, iy);
			}
		}
		ix = Add16(ix, 0x40);
	} while (--count != 0);
}


void
ChkTouchEnemies()
{
	Ram(isOnBridge) = 0;
	uint8_t y = Ram(PlayerY);
	uint8_t x = Ram(PlayerX);
	uint16_t ix = EnemyList;
	for (int i = 0; i < 16; i++, ix = Add16(ix, 0x80)) {
		uint8_t id = A(ix, ACTOR::ID);
		if ((id & 0x80) == 0 && id != 0)
			ChkTouchEnemy(ix, x, y);
	}
}


// Off the moving parts of the bridges (rooms 45, 46): a fall.
void
ChkOnBridge()
{
	uint8_t room = Ram(Room);
	if (room != 45 && room != 46)
		return;
	if (Ram(isOnBridge) != 0)
		return;
	uint16_t tiles = GetTilePlayer();
	if (tiles != 0x0101)
		return;
	Ram(IdDoorEnter) = room == 45 ? 0x5b : 0x81;
	Ram(JumpRoomId) = room == 45 ? 1 : 2;
	Ram(OpenDoorStatus) = 0;
	Ram(GameMode) = 1;
}


void
ChkElectricFloor()
{
	uint8_t room = Ram(Room);
	uint8_t b, c;
	if (room == 16 || room == 37 || room == 110) {
		b = 0x60;
		c = 0x61;
	} else if (room == 40) {
		b = 0x45;
		c = 0x46;
	} else if (room == 116) {
		b = 0x40;
		c = 0x41;
	} else {
		return;
	}
	if (Ram(PowerSwitchOn) == 0)
		return;
	uint16_t tiles = GetTilePlayer();
	uint8_t right = static_cast<uint8_t>(tiles >> 8);
	uint8_t left = static_cast<uint8_t>(tiles);
	if (right != b && right != c && left != b && left != c)
		return;
	if (Ram(DamageDelayTimer) != 0)
		return;
	SetSoundEntry(0x18);
	Ram(DamageDelayTimer) = 8;
	DecrementLife_2();
}


void
RemoveLaserBeans()
{
	uint16_t ix = EnemyList;
	for (int i = 0; i < 16; i++, ix = Add16(ix, 0x80)) {
		if (A(ix, ACTOR::ID) != 0)
			DismissActor0(ix);
	}
}


// A call waits, rings #58 frames, then stops.
void
ChkIncomingCall()
{
	if (Ram(IncomingCallTimer) == 0)
		return;
	uint8_t flag = Ram(RadioCallFlag);
	if (flag == 2)
		return;
	if (flag != 1) {
		if (--Ram(IncomingCallTimer) != 0)
			return;
		Ram(IncomingCallTimer) = 0x58;
		Ram(RadioCallFlag) = 1;
	}
	if (--Ram(IncomingCallTimer) == 0)
		Ram(RadioCallFlag) = 2;
}


namespace {

uint8_t
SubBcd(uint8_t a, bool* borrow)
{
	// SUB 1 / DAA
	uint8_t r = static_cast<uint8_t>(a - 1);
	bool c = a == 0;
	bool half = (a & 0x0f) == 0;
	uint8_t adjust = 0;
	if (half || (r & 0x0f) > 9)
		adjust = 0x06;
	if (c || r > 0x99) {
		adjust |= 0x60;
		c = true;
	}
	*borrow = c;
	return static_cast<uint8_t>(r - adjust);
}

}	// namespace


void
DecNukeTimer()
{
	if (Ram(DestructionTimerOn) == 0)
		return;
	bool borrow;
	Ram(DestructTimer) = SubBcd(Ram(DestructTimer), &borrow);
	if (borrow) {
		bool b2;
		Ram(Add16(DestructTimer, 1)) = SubBcd(Ram(Add16(DestructTimer, 1)), &b2);
	}
	if ((Ram(DestructTimer) | Ram(Add16(DestructTimer, 1))) != 0)
		return;
	DrawCallTimer();
	Ram(Life) = 0;
	SetDead();
}
