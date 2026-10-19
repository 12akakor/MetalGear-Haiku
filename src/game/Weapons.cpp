/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/weaponuse.asm, logic/weapon/*.asm,
// checkweaponalert.asm and setalert.asm.
#include "Weapons.h"

#include "Common.h"
#include "GameMain.h"
#include "Hud.h"
#include "Player.h"
#include "Rom.h"
#include "Sprites.h"
#include "Structures.h"


using namespace var;


namespace {

inline uint8_t&
S(uint16_t ix, int field)
{
	return Ram(static_cast<uint16_t>(ix + field));
}


uint16_t
Add16(uint16_t a, int b)
{
	return static_cast<uint16_t>(a + b);
}


// Free shot structure, as the original's GetEmptyShotDat (carry = found).
bool
GetEmptyShotDat(uint16_t* ix)
{
	uint16_t hl = PlayerShotsList;
	for (int i = 0; i < 6; i++, hl = Add16(hl, 0x40)) {
		if (Ram(hl) == 0) {
			*ix = hl;
			return true;
		}
	}
	return false;
}


// Takes the free sprites of layers 4-9 for the shot.
bool
ReserveShotSpr(uint16_t ix)
{
	uint8_t count = Mem(Add16(SYM(NumSprShot), static_cast<uint8_t>(Ram(SelectedWeapon) - 1)));
	uint16_t iy = Add16(ix, 0x20);
	Ram(iy) = count;
	iy++;
	uint16_t hl = SprShootsAtt;
	uint8_t layer = 4;
	for (int i = 0; i < 6; i++) {
		if (Ram(hl) == 0xe0) {
			Ram(iy) = layer;
			Ram(hl) = 0xe1;
			if (--count == 0) {
				S(ix, 0) = Ram(SelectedWeapon);
				S(ix, 1) = 0;
				S(ix, 2) = 0;
				return true;
			}
			iy = Add16(iy, 5);
		}
		layer++;
		hl = Add16(hl, 4);
	}
	RemoveShot(ix);
	return false;
}


void
MovePlayerShotX(uint16_t ix)
{
	uint16_t x = static_cast<uint16_t>(S(ix, PLAYER_SHOT::Xdec) | S(ix, PLAYER_SHOT::X) << 8);
	uint16_t speed = static_cast<uint16_t>(S(ix, PLAYER_SHOT::speedXdec)
		| S(ix, PLAYER_SHOT::speedX) << 8);
	x = static_cast<uint16_t>(x + speed);
	S(ix, PLAYER_SHOT::Xdec) = static_cast<uint8_t>(x);
	S(ix, PLAYER_SHOT::X) = static_cast<uint8_t>(x >> 8);
}


void
AddSpeedY(uint16_t ix, int decField, int field)
{
	uint16_t y = static_cast<uint16_t>(S(ix, decField) | S(ix, field) << 8);
	uint16_t speed = static_cast<uint16_t>(S(ix, PLAYER_SHOT::speedYdec)
		| S(ix, PLAYER_SHOT::speedY) << 8);
	y = static_cast<uint16_t>(y + speed);
	S(ix, decField) = static_cast<uint8_t>(y);
	S(ix, field) = static_cast<uint8_t>(y >> 8);
}


void
MovePlayerShot(uint16_t ix)
{
	AddSpeedY(ix, PLAYER_SHOT::Ydec, PLAYER_SHOT::Y);
	MovePlayerShotX(ix);
}


void
MovePlayerShotAlt2(uint16_t ix)
{
	AddSpeedY(ix, PLAYER_SHOT::Ydec_Alt, PLAYER_SHOT::Y_Alt);
}


bool
ChkShotCollision(uint16_t ix, uint8_t y)
{
	return ChkTileCollision_(S(ix, PLAYER_SHOT::X), y, 2, S(ix, PLAYER_SHOT::Direction));
}


// Both points (the shot and Snake's height) against the walls.
bool
ShotHitsWall(uint16_t ix)
{
	if (!ChkShotCollision(ix, S(ix, PLAYER_SHOT::Y)))
		return false;
	return ChkShotCollision(ix, S(ix, PLAYER_SHOT::Y_Alt));
}


void
ChkShotBoundaries(uint16_t ix)
{
	if (S(ix, PLAYER_SHOT::Y) >= 184 || S(ix, PLAYER_SHOT::X) >= 248
		|| S(ix, PLAYER_SHOT::X) < 9)
		RemoveShot(ix);
}


// The weapon's ammunition: the amount's address, or 0 when there is none.
uint16_t
GetAmmo(uint8_t weapon)
{
	uint16_t entry;
	uint8_t index;
	GetWeaponInvAdd(weapon, &entry, &index);
	uint16_t amount = Add16(entry, 1);
	SetRam16(TempData, amount);
	return (Ram(amount) | Ram(Add16(amount, 1))) != 0 ? amount : 0;
}


bool
FirePressed()
{
	return (Ram(ControlsTrigger) & 0x10) != 0;
}


// From Snake's position: Y (and the second Y at 'altY' above it).
void
SetShotStart(uint16_t ix, uint8_t timer, uint8_t kill)
{
	if (timer != 0)
		S(ix, PLAYER_SHOT::Timer) = timer;
	S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = kill;
}


void
SetShotDirSpeed(uint16_t ix, sym::Id table)
{
	uint8_t direction = Ram(PlayerDirection);
	S(ix, PLAYER_SHOT::Direction) = direction;
	uint16_t hl = Add16(RomSymbol(table), static_cast<uint8_t>((direction - 1) * 2));
	S(ix, PLAYER_SHOT::speedYdec) = 0;
	S(ix, PLAYER_SHOT::speedY) = Mem(hl);
	S(ix, PLAYER_SHOT::speedXdec) = 0;
	S(ix, PLAYER_SHOT::speedX) = Mem(Add16(hl, 1));
}


// Bullets (hand gun and SMG) start 14 pixels above Snake's feet.
void
SetBulletStart(uint16_t ix, uint8_t above)
{
	uint8_t y = Ram(PlayerY);
	S(ix, PLAYER_SHOT::Ydec) = 0;
	S(ix, PLAYER_SHOT::Ydec_Alt) = 0;
	S(ix, PLAYER_SHOT::Y_Alt) = y;
	S(ix, PLAYER_SHOT::Y) = static_cast<uint8_t>(y - above);
	S(ix, PLAYER_SHOT::Xdec) = 0;
	S(ix, PLAYER_SHOT::X) = Ram(PlayerX);
}


// Shots without the suppressor can be heard.
void
GunSfx(uint16_t ix, uint8_t sfx, bool check)
{
	bool silent = Ram(InvSupressor) != 0;
	uint8_t id = silent ? 0x0e : sfx;
	if (check)
		SetSoundEntryChk(id);
	else
		SetSoundEntry(id);
	if (!silent)
		ChkAlertTrigger(ix);
}


void
ChkHandGunShot()
{
	if (!FirePressed())
		return;
	uint16_t ammo = GetAmmo(HAND_GUN);
	if (ammo == 0) {
		SetSoundEntryChk(0x15);	// click
		return;
	}
	uint16_t ix;
	if (!GetEmptyShotDat(&ix) || !ReserveShotSpr(ix))
		return;
	DecItemUnits(Ram16(TempData), 0);
	SetShotStart(ix, 0x10, 1);
	SetBulletStart(ix, 14);
	SetShotDirSpeed(ix, sym::ShootDirSpeeds);
	S(ix, PLAYER_SHOT::SpriteID) = 0;
	GunSfx(ix, 0x0c, false);
	SetShotSpr(ix);
}


// Bursts: a shot every 2 frames, 8 slightly different directions.
void
ChkSMGShot()
{
	uint16_t timer = SubMachGunTimer;
	if ((Ram(ControlsHold) & 0x10) == 0) {
		Ram(timer) = 0;
		Ram(Add16(timer, 1)) = 0;
		return;
	}
	if (++Ram(timer) != 2)
		return;
	Ram(timer) = 0;
	if (++Ram(Add16(timer, 1)) == 9)
		Ram(Add16(timer, 1)) = 1;
	if (GetAmmo(SUB_MACHINE_GUN) == 0) {
		// (!?) clears the hundreds and the byte after them, not the timers
		uint16_t hl = Add16(Ram16(TempData), 1);
		SetSoundEntryChk(0x15);
		Ram(hl) = 0;
		Ram(Add16(hl, 1)) = 0;
		return;
	}
	uint16_t ix;
	if (!GetEmptyShotDat(&ix) || !ReserveShotSpr(ix))
		return;
	DecItemUnits(Ram16(TempData), 0);
	SetShotStart(ix, 0x10, 1);
	SetBulletStart(ix, 14);
	uint8_t direction = Ram(PlayerDirection);
	S(ix, PLAYER_SHOT::Direction) = direction;
	uint16_t hl = Add16(SYM(SMG_BulletSpeeds), static_cast<uint8_t>((direction - 1) * 16) * 2);
	hl = Add16(hl, static_cast<uint8_t>((Ram(BurstCnt) - 1) * 2) * 2);
	S(ix, PLAYER_SHOT::speedYdec) = Mem(hl);
	S(ix, PLAYER_SHOT::speedY) = Mem(Add16(hl, 1));
	S(ix, PLAYER_SHOT::speedXdec) = Mem(Add16(hl, 2));
	S(ix, PLAYER_SHOT::speedX) = Mem(Add16(hl, 3));
	S(ix, PLAYER_SHOT::SpriteID) = 0;
	GunSfx(ix, 0x0d, true);
	SetShotSpr(ix);
}


void
ChkGrenadeShot()
{
	if (!FirePressed() || GetAmmo(GRENADE_LAUNCHER) == 0)
		return;
	uint16_t ix;
	if (!GetEmptyShotDat(&ix) || !ReserveShotSpr(ix))
		return;
	DecItemUnits(Ram16(TempData), 0);
	SetShotStart(ix, 0x18, 0);
	uint8_t y = static_cast<uint8_t>(Ram(PlayerY) - 16);
	S(ix, PLAYER_SHOT::Ydec) = 0;
	S(ix, PLAYER_SHOT::Ydec_Alt) = 0;
	S(ix, PLAYER_SHOT::Y) = y;
	S(ix, PLAYER_SHOT::Y_Alt) = y;
	S(ix, PLAYER_SHOT::Xdec) = 0;
	S(ix, PLAYER_SHOT::X) = Ram(PlayerX);
	SetShotDirSpeed(ix, sym::GrenadeSpeeds);
	S(ix, PLAYER_SHOT::SpriteID) = 1;
	SetSoundEntryChk(0x12);
	SetShotSpr(ix);
}


// Rockets, plastic bombs and missiles: one at a time, in the first slot.
bool
FirstShotFree(uint8_t weapon)
{
	if (!FirePressed() || GetAmmo(weapon) == 0)
		return false;
	if (Ram(PlayerShotsList) != 0)
		return false;
	return ReserveShotSpr(PlayerShotsList);
}


void
ChkFireRocket()
{
	if (!FirstShotFree(ROCKET_LAUNCHER))
		return;
	uint16_t ix = PlayerShotsList;
	DecItemUnits(Ram16(TempData), 0);
	S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 1;
	SetBulletStart(ix, 16);
	SetShotDirSpeed(ix, sym::RocketSpeeds);
	S(ix, PLAYER_SHOT::SpriteID) = 2;
	SetSoundEntryChk(0x13);
	LoadSprMissilRocket(ix, SYM(idxSprRocket));
	SetShotSpr(ix);
}


void
ChkPBombShot()
{
	if (!FirstShotFree(PLASTIC_BOMB))
		return;
	uint16_t ix = PlayerShotsList;
	DecItemUnits(Ram16(TempData), 1);
	SetShotStart(ix, 0x30, 0);
	uint16_t de = Add16(SYM(PBombDirOffset), static_cast<uint8_t>((Ram(PlayerDirection) - 1) * 4));
	uint16_t y = Add16(Ram16(PlayerYdec), Mem(de) | Mem(Add16(de, 1)) << 8);
	S(ix, PLAYER_SHOT::Ydec) = static_cast<uint8_t>(y);
	S(ix, PLAYER_SHOT::Y) = static_cast<uint8_t>(y >> 8);
	uint16_t x = Add16(Ram16(PlayerXdec), Mem(Add16(de, 2)) | Mem(Add16(de, 3)) << 8);
	S(ix, PLAYER_SHOT::Xdec) = static_cast<uint8_t>(x);
	S(ix, PLAYER_SHOT::X) = static_cast<uint8_t>(x >> 8);
	S(ix, PLAYER_SHOT::SpriteID) = 6;
	SetSoundEntryChk(0x17);
	SetShotSpr(ix);
}


void
ChkLMineShot()
{
	if (!FirePressed() || GetAmmo(LAND_MINE) == 0)
		return;
	uint16_t ix;
	if (!GetEmptyShotDat(&ix) || !ReserveShotSpr(ix))
		return;
	DecItemUnits(Ram16(TempData), 1);
	S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 1;
	S(ix, PLAYER_SHOT::Y) = Ram(PlayerY);
	S(ix, PLAYER_SHOT::Ydec) = 0;
	S(ix, PLAYER_SHOT::X) = Ram(PlayerX);
	S(ix, PLAYER_SHOT::Xdec) = 0;
	S(ix, PLAYER_SHOT::SpriteID) = 0x0a;
	SetSoundEntryChk(0x17);
	SetShotSpr(ix);
}


void
SetMissileSpr(uint16_t ix, uint8_t direction)
{
	S(ix, PLAYER_SHOT::Direction) = direction;
	uint16_t hl = Add16(SYM(MissileIniSpeed), static_cast<uint8_t>((direction - 1) * 2));
	S(ix, PLAYER_SHOT::speedYdec) = 0;
	S(ix, PLAYER_SHOT::speedY) = Mem(hl);
	S(ix, PLAYER_SHOT::speedXdec) = 0;
	S(ix, PLAYER_SHOT::speedX) = Mem(Add16(hl, 1));
	S(ix, PLAYER_SHOT::SpriteID) = 0x0b;
	LoadSprMissilRocket(ix, SYM(idxSprMissile));
	SetShotSpr(ix);
}


void
ChkMissileShot()
{
	if (!FirstShotFree(MISSILE))
		return;
	uint16_t ix = PlayerShotsList;
	DecItemUnits(Ram16(TempData), 1);
	S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 1;
	SetBulletStart(ix, 16);
	SetSoundEntryChk(0x14);
	SetMissileSpr(ix, Ram(PlayerDirection));
}


// Shot logic

void
PlayerBulletLogic(uint16_t ix)
{
	if (--S(ix, PLAYER_SHOT::Timer) == 0) {
		RemoveShot(ix);
		return;
	}
	MovePlayerShot(ix);
	MovePlayerShotAlt2(ix);
	if (ShotHitsWall(ix)) {
		RemoveShot(ix);
		return;
	}
	ChkShotBoundaries(ix);
	SetShotSpr(ix);
}


// Explosions: three frames from 'sprite'.
void
ExplosionLogic(uint16_t ix, uint8_t sprite)
{
	S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 0;
	if (--S(ix, PLAYER_SHOT::Timer) == 0) {
		RemoveShot(ix);
		return;
	}
	uint8_t timer = S(ix, PLAYER_SHOT::Timer);
	if (timer < 0x0a)
		sprite++;
	if (timer < 5)
		sprite++;
	S(ix, PLAYER_SHOT::SpriteID) = sprite;
	SetShotSpr(ix);
}


void
PlayerGrenadeLogic(uint16_t ix)
{
	if (S(ix, PLAYER_SHOT::status) != 0) {
		ExplosionLogic(ix, 3);
		return;
	}
	if (--S(ix, PLAYER_SHOT::Timer) == 0) {
		SetSoundEntryChk(0x1a);
		S(ix, PLAYER_SHOT::status)++;
		S(ix, PLAYER_SHOT::Timer) = 0x0f;
		S(ix, PLAYER_SHOT::SpriteID) = 3;
		S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 1;
		ChkAlertTrigger(ix);
		return;
	}
	// a parabola: the visible Y over the real one
	MovePlayerShotX(ix);
	MovePlayerShotAlt2(ix);
	uint8_t offset = Mem(Add16(SYM(GrenadeYOffsets), S(ix, PLAYER_SHOT::Timer)));
	uint16_t y = Add16(offset << 8,
		S(ix, PLAYER_SHOT::Ydec_Alt) | S(ix, PLAYER_SHOT::Y_Alt) << 8);
	S(ix, PLAYER_SHOT::Ydec) = static_cast<uint8_t>(y);
	S(ix, PLAYER_SHOT::Y) = static_cast<uint8_t>(y >> 8);
	ChkShotBoundaries(ix);
	SetShotSpr(ix);
}


void
PlayerRocketLogic(uint16_t ix)
{
	switch (S(ix, PLAYER_SHOT::status)) {
		case 0:
			MovePlayerShot(ix);
			MovePlayerShotAlt2(ix);
			if (ShotHitsWall(ix)) {
				S(ix, PLAYER_SHOT::status)++;
				return;
			}
			ChkShotBoundaries(ix);
			SetShotSpr(ix);
			return;
		case 1:
			SetSoundEntryChk(0x1a);
			S(ix, PLAYER_SHOT::Timer) = 15;
			S(ix, PLAYER_SHOT::SpriteID) = 7;
			S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 0;
			ChkAlertTrigger(ix);
			S(ix, PLAYER_SHOT::status)++;
			return;
		case 2:
			ExplosionLogic(ix, 7);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "PlayerRocketLogic: bad status";
			return;
	}
}


void
PlayerPBombLogic(uint16_t ix)
{
	switch (S(ix, PLAYER_SHOT::status)) {
		case 0:
			if (--S(ix, PLAYER_SHOT::Timer) != 0)
				return;
			S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 1;
			S(ix, PLAYER_SHOT::status)++;
			return;
		case 1:
			SetSoundEntryChk(0x1c);
			S(ix, PLAYER_SHOT::Timer) = 0x0f;
			S(ix, PLAYER_SHOT::SpriteID) = 7;
			ChkAlertTrigger(ix);
			S(ix, PLAYER_SHOT::status)++;
			return;
		case 2:
			ExplosionLogic(ix, 7);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "PlayerPBombLogic: bad status";
			return;
	}
}


void
PlayerMineLogic(uint16_t ix)
{
	switch (S(ix, PLAYER_SHOT::status)) {
		case 0:
			return;	// waits for an enemy
		case 1:
			SetSoundEntryChk(0x1c);
			S(ix, PLAYER_SHOT::status)++;
			S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 0;
			S(ix, PLAYER_SHOT::Timer) = 0x0f;
			S(ix, PLAYER_SHOT::SpriteID) = 3;
			ChkAlertTrigger(ix);
			return;
		case 2:
			ExplosionLogic(ix, 3);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "PlayerMineLogic: bad status";
			return;
	}
}


// Missiles turn with the cursor keys.
void
PlayerMissileLogic(uint16_t ix)
{
	switch (S(ix, PLAYER_SHOT::status)) {
		case 0:
		{
			uint8_t keys = Ram(ControlsTrigger);
			for (uint8_t d = 0; d < 4; d++) {
				if (keys & (1 << d)) {
					SetMissileSpr(ix, static_cast<uint8_t>(DIR_UP + d));
					break;
				}
			}
			MovePlayerShot(ix);
			MovePlayerShotAlt2(ix);
			if (ShotHitsWall(ix)) {
				S(ix, PLAYER_SHOT::status)++;
				return;
			}
			ChkShotBoundaries(ix);
			LoadSprMissilRocket(ix, SYM(idxSprMissile));
			SetShotSpr(ix);
			return;
		}
		case 1:
			SetSoundEntryChk(0x1a);
			S(ix, PLAYER_SHOT::KILL_BY_CONTACT) = 0;
			S(ix, PLAYER_SHOT::Timer) = 0x0f;
			S(ix, PLAYER_SHOT::SpriteID) = 7;
			ChkAlertTrigger(ix);
			S(ix, PLAYER_SHOT::status)++;
			return;
		case 2:
			ExplosionLogic(ix, 7);
			return;
		default:
			if (gUnported == NULL)
				gUnported = "PlayerMissileLogic: bad status";
			return;
	}
}

}	// namespace


// Not in elevators, water, the box or the brick wall room.
void
ChkWeaponShot()
{
	uint8_t weapon = Ram(SelectedWeapon);
	if (weapon == 0)
		return;
	uint8_t room = Ram(Room);
	if (room == 204 || room >= 224)
		return;
	uint8_t animation = Ram(PlayerAnimation);
	if (animation == 2 || animation == 4 || animation == 7)
		return;
	switch (weapon) {
		case HAND_GUN:			ChkHandGunShot(); return;
		case SUB_MACHINE_GUN:	ChkSMGShot(); return;
		case GRENADE_LAUNCHER:	ChkGrenadeShot(); return;
		case ROCKET_LAUNCHER:	ChkFireRocket(); return;
		case PLASTIC_BOMB:		ChkPBombShot(); return;
		case LAND_MINE:			ChkLMineShot(); return;
		case MISSILE:			ChkMissileShot(); return;
		default:
			if (gUnported == NULL)
				gUnported = "ChkWeaponShot: bad weapon";
			return;
	}
}


void
PlayerShotsLogic()
{
	uint16_t hl = PlayerShotsList;
	for (int i = 0; i < 6; i++, hl = Add16(hl, 0x40)) {
		uint8_t id = Ram(hl);
		if (id == 0)
			continue;
		switch (id) {
			case 0xff:				RemoveShot(hl); break;
			case HAND_GUN:
			case SUB_MACHINE_GUN:	PlayerBulletLogic(hl); break;
			case GRENADE_LAUNCHER:	PlayerGrenadeLogic(hl); break;
			case ROCKET_LAUNCHER:	PlayerRocketLogic(hl); break;
			case PLASTIC_BOMB:		PlayerPBombLogic(hl); break;
			case LAND_MINE:			PlayerMineLogic(hl); break;
			case MISSILE:			PlayerMissileLogic(hl); break;
			default:
				if (gUnported == NULL)
					gUnported = "PlayerShotsLogic: bad shot";
				break;
		}
	}
}


// Copies the shots' sprites (Y, X, pattern; one colour for all lines).
void
SetPlayerShotSpr()
{
	uint16_t shot = PlayerShotsList;
	for (int i = 0; i < 6; i++, shot = Add16(shot, 0x40)) {
		if (Ram(shot) == 0)
			continue;
		uint16_t hl = Add16(shot, 0x20);
		uint8_t count = Ram(hl);
		hl++;
		do {
			uint8_t offset = static_cast<uint8_t>(Ram(hl) * 4);
			uint16_t de = Add16(SprAttRAM, offset);
			hl++;
			for (int j = 0; j < 3; j++)
				Ram(Add16(de, j)) = Ram(Add16(hl, j));
			hl = Add16(hl, 3);
			uint16_t colors = Add16(SpritesColors, HL_4xA(offset));
			uint8_t color = Ram(hl);
			hl++;
			for (int j = 0; j < 16; j++)
				Ram(Add16(colors, j)) = color;
		} while (--count != 0);
	}
}


void
RemoveShot(uint16_t ix)
{
	S(ix, PLAYER_SHOT::ID) = 0;
	uint16_t de = Add16(ix, 0x20);	// PLAYER_SHOT.NumSprites
	uint8_t count = Ram(de);
	de++;
	do {
		Ram(Add16(SprAttRAM, static_cast<uint8_t>(Ram(de) * 4))) = 0xe0;
		de = Add16(de, 5);
	} while (--count != 0);
}


// The number of guards coming depends on the highest card taken.
void
SetAlertModeRespawn(uint16_t ix, uint8_t timer)
{
	Ram(AlertRespawnTimer) = timer;
	SetAlertMode(ix);
}


void
SetAlertMode(uint16_t ix)
{
	Ram(AlertMode) = 1;
	Ram(AlertModeCopy) = 1;
	uint8_t room = Ram(Room);
	Ram(RoomAlert) = room;
	uint8_t guards = 0;
	if (room != 216) {
		uint8_t b = 8;
		uint16_t hl = Card8Taken;
		while ((Ram(hl) & 1) == 0) {
			hl--;
			if (--b == 0)
				break;
		}
		guards = static_cast<uint8_t>(b + 3);
	}
	Ram(NumRespawnGuards) = guards;
	if (Ram(DestructionTimerOn) != 0)
		return;
	if (Ram(Add16(SoundWorkArea, 2)) == 0x44)
		return;	// game over music
	uint8_t id = S(ix, ACTOR::ID);
	uint8_t music = id == ID_CAMERA || id == ID_LASER ? 0x2f : 0x32;
	if ((Ram(ControlConfig) & 0x40) == 0 && Ram(MusicInDemoMode) != 0)
		return;
	Ram(AreaMusic) = music;
	SetSoundEntry(music);
}


// Some rooms can hear nothing.
void
ChkAlertTrigger(uint16_t ix)
{
	if (Ram(IsolatedRoom) == 1)
		return;
	uint8_t room = Ram(Room);
	for (int i = 0; i < 55; i++) {
		if (Mem(Add16(SYM(RoomShotSecure), i)) == room)
			return;
	}
	if (Ram(AlertMode) != 0)
		return;
	SetAlertModeRespawn(ix, 0x5a);
}
