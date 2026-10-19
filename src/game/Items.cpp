/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/items.asm, addroomitems.asm, drawitemsinroom.asm,
// spawnitem.asm, shrinkitems.asm and maxammo.asm.
#include "Items.h"

#include "Common.h"
#include "GameData.h"
#include "GameGfx.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "Rom.h"
#include "Screens.h"
#include "Structures.h"
#include "Text.h"


using namespace var;


namespace {

uint16_t
Add16(uint16_t a, int b)
{
	return static_cast<uint16_t>(a + b);
}


// ADD A,n then DAA.
uint8_t
AddBcd(uint8_t a, uint8_t b, bool* carry)
{
	unsigned sum = static_cast<unsigned>(a) + b;
	bool half = (a & 0x0f) + (b & 0x0f) > 0x0f;
	bool c = sum > 0xff;
	uint8_t r = static_cast<uint8_t>(sum);
	uint8_t adjust = 0;
	if (half || (r & 0x0f) > 9)
		adjust = 0x06;
	if (c || r > 0x99) {
		adjust |= 0x60;
		c = true;
	}
	*carry = c;
	return static_cast<uint8_t>(r + adjust);
}


// Clamps the amount at entry + 1 (BCD, 16 bits) to 'max'.
void
ChkMaxAmount(uint16_t entry, uint16_t max)
{
	uint16_t amount = Ram16(Add16(entry, 1));
	if (amount <= max)
		return;
	SetRam16(Add16(entry, 1), max);
}


void
LimitWeapon(uint8_t weapon, uint16_t max)
{
	uint16_t entry;
	uint8_t index;
	if (GetWeaponInvAdd(weapon, &entry, &index))
		ChkMaxAmount(entry, Ram16(max));
}

}	// namespace


uint16_t
AddRoomItems2(uint16_t list, uint16_t entry)
{
	for (;;) {
		uint8_t id = Mem(list);
		if (id == 0xff)
			return entry;
		if (id == ROCKET_LAUNCHER && Ram(JeniRocketF) == 0)
			return entry;	// not before speaking to Schneider
		uint16_t taken = id < 9
			? Add16(WeaponsTaken, id - 1) : Add16(ItemsTaken, id - 9);
		if (Ram(taken) != 0) {
			list = Add16(list, 3);
			continue;
		}
		Ram(entry) = id;
		Ram(Add16(entry, 2)) = Mem(Add16(list, 1));
		Ram(Add16(entry, 3)) = Mem(Add16(list, 2));
		Ram(Add16(entry, 4)) = 0;
		list = Add16(list, 3);
		entry = Add16(entry, 0x10);
	}
}


// Only the isolated rooms (122-217) hold items.
void
AddRoomItems()
{
	for (int i = 0; i < 0x30; i++)
		Ram(Add16(ItemsInTheRoom, i)) = 0;
	uint8_t room = Ram(Room);
	if (room >= 218 || room < 122)
		return;
	uint8_t index = Mem(Add16(SYM(idxRoomItemsIdx), room - 122));
	if (index == 0)
		return;
	uint16_t list = GetPointerDE2A(SYM(idxRoomItems), static_cast<uint8_t>(index - 1));
	AddRoomItems2(list, ItemsInTheRoom);
}


// Saves the background (page 1, X #A0, Y #B0 + 16 * index) and draws
// the item from page 1.
void
DrawRoomItems2(uint16_t ix, uint8_t count, uint8_t index)
{
	do {
		uint8_t id = Ram(ix);
		if (id != 0) {
			uint16_t gfx = id < 9 ? SYM(WeaponGfxXY) : SYM(ItemGfxXY);
			uint8_t n = id < 9 ? id : static_cast<uint8_t>(id - 8);
			gfx = Add16(gfx, static_cast<uint8_t>(static_cast<uint8_t>(n - 1) * 2));
			uint8_t narrow = Mem(gfx) & 1;
			Ram(Add16(ix, ITEM::status)) = narrow;
			uint8_t buffer = index == 0 ? 0xb0 : index == 1 ? 0xc0 : 0xd0;
			uint8_t width = narrow ? 0x10 : 0x20;
			uint8_t y = Ram(Add16(ix, ITEM::Y));
			uint8_t x = Ram(Add16(ix, ITEM::X));
			VDP_Copy_Dot(x, y, 0x18, 0xa0, buffer, width, 0x10);
			VDP_Copy_Dot(Mem(gfx) & 0xfe, Mem(Add16(gfx, 1)), 0x48, x, y, width, 0x10);
		}
		ix = Add16(ix, 0x10);
		index++;
	} while (--count != 0);
}


void
DrawRoomItems()
{
	DrawRoomItems2(ItemsInTheRoom, 3, 0);
}


// One item at a time, and only in a room without items.
void
SpawnItem(uint8_t drop, uint8_t y, uint8_t x)
{
	uint8_t id = Mem(Add16(SYM(ListDropItems), drop));
	Ram(SpawnItemData) = id;
	Ram(Add16(SpawnItemData, 1)) = y;
	Ram(Add16(SpawnItemData, 2)) = x;
	Ram(Add16(SpawnItemData, 3)) = 0xff;
	uint8_t amount = id == RATION || id == AMMO_CRATE ? 1 : 0;
	if (Ram(SpawnedItems) != 0)
		return;
	Ram(SpawnedItems) = 1;
	if (Ram(ItemsInTheRoom) != 0)
		return;
	uint16_t entry = AddRoomItems2(SpawnItemData, ItemsInTheRoom);
	Ram(Add16(entry, -0x0c)) = amount;
	DrawRoomItems2(ItemsInTheRoom, 1, 0);
	SetSoundEntryChk(0x25);
	SetBanks_1_2_X();
}


namespace {

void
SetItemAsTaken(uint8_t id)
{
	if (id >= PLASTIC_BOMB) {
		if (id < SUPRESSOR || id == RATION || id == AMMO_CRATE)
			return;
	}
	if (id < ARMOR)
		Ram(Add16(WeaponsTaken, id - 1)) = 1;
	else
		Ram(Add16(ItemsTaken, id - 9)) = 1;
}


void
AddAmmo(uint16_t amount, uint8_t units)
{
	bool carry;
	Ram(amount) = AddBcd(units, Ram(amount), &carry);
	if (carry)
		Ram(Add16(amount, 1))++;
	LimitAmmo();
}


void
AddItemAmount(uint16_t entry)
{
	AddAmmo(Add16(entry, 1), GetItemAmount(static_cast<uint8_t>(Ram(Add16(TempData, 1)) - 1)));
}


void
EraseTakenItem(uint16_t ix)
{
	uint8_t index = Ram(TempData);
	uint8_t buffer = index == 0 ? 0xb0 : index == 1 ? 0xc0 : 0xd0;
	uint8_t width = (Ram(Add16(ix, ITEM::status)) & 1) ? 0x10 : 0x20;
	VDP_Copy_Dot(0xa0, buffer, 0x48, Ram(Add16(ix, ITEM::X)), Ram(Add16(ix, ITEM::Y)),
		width, 0x10);
	uint8_t id = Ram(ix);
	Ram(ix) = 0;
	SetSoundEntry(0x24);
	if (Ram(TempData2) != 0)
		return;	// a dropped ration or ammunition
	uint8_t text = GET_HL_A_DEC(SYM(ItemTakeText), id);
	if (text == 0)
		return;
	// (!?) the English version only shows the text of the equipment bag
	if (LoadedRomVersion() != kRomJapanese && text != 62)
		return;
	SetText(text);
}


void
PickAmmoCrate(uint16_t ix)
{
	static const uint8_t kWeapon[] = { HAND_GUN, SUB_MACHINE_GUN, GRENADE_LAUNCHER,
		ROCKET_LAUNCHER };
	static const uint8_t kUnits[] = { 0x20, 0x20, 6, 2 };
	uint16_t entry;
	uint8_t index;
	for (int i = 0; i < 4; i++) {
		if (GetWeaponInvAdd(kWeapon[i], &entry, &index))
			AddAmmo(Add16(entry, 1), kUnits[i]);
	}
	uint8_t selected = Ram(SelectedWeapon);
	if (selected != 0) {
		GetWeaponInvAdd(selected, &entry, &index);
		RenderAmmoHUD(Add16(entry, 2));
	}
	EraseTakenItem(ix);
}


void
PickUpWeapon(uint16_t ix)
{
	uint8_t id = Ram(Add16(TempData, 1));
	SetItemAsTaken(id);
	uint16_t entry;
	uint8_t index;
	if (GetWeaponInvAdd(id, &entry, &index)) {
		AddItemAmount(entry);
		if (Ram(SelectedWeapon) == id)
			RenderAmmoHUD(Add16(entry, 2));
		EraseTakenItem(ix);
		return;
	}
	// a new weapon; the first one is selected (but the grenade launcher)
	entry = Weapons;
	uint8_t b = 7;
	while (Ram(entry) != 0) {
		entry = Add16(entry, 4);
		if (--b == 0)
			return;
	}
	Ram(entry) = id;
	if (b == 7 && id != GRENADE_LAUNCHER) {
		Ram(WeaponInUse) = id;
		Ram(SelectedWeapon) = id;
		LoadSprProjectile();
		for (int i = 0; i < 0x18; i++)
			Ram(Add16(SprShootsAtt, i)) = 0xe0;
		Ram(PlayerShotsList) = 0;
		Ldir(PlayerShotsList, PlayerShot1Stat, 0x17f);
	}
	AddItemAmount(entry);
	DrawWeaponHUD();
	EraseTakenItem(ix);
}


// The bag with the equipment taken at the capture: the transmitter
// comes along.
void
RecoverEquipment(uint16_t ix)
{
	Ram(EquipRemoved) = 0;
	Ram(EquipBagTaken) = 1;
	uint16_t entry = Equipment;
	while (Ram(entry) != 0)
		entry = Add16(entry, 4);
	Ram(entry) = SELECTED_TRANSMITTER;
	Ram(Add16(entry, 1)) = 1;
	Ram(TransmiTaken) = 1;
	DrawWeaponHUD();
	DrawItemHUD();
	EraseTakenItem(ix);
}


void
ChkTakeItem(uint16_t ix, uint8_t index, uint8_t playerX, uint8_t playerY)
{
	Ram(TempData) = index;
	bool narrow = (Ram(Add16(ix, 1)) & 1) != 0;
	uint8_t halfWidth = narrow ? 0x08 : 0x10;
	uint8_t radiusX = narrow ? 0x0c : 0x14;
	uint8_t dy = static_cast<uint8_t>(Ram(Add16(ix, 2)) + 16 - playerY);
	if (static_cast<uint8_t>(Ram(Add16(ix, 2)) + 16) < playerY)
		dy = static_cast<uint8_t>(-dy);
	if (dy >= 16)
		return;
	uint8_t centre = static_cast<uint8_t>(Ram(Add16(ix, 3)) + halfWidth);
	uint8_t dx = static_cast<uint8_t>(centre - playerX);
	if (centre < playerX)
		dx = static_cast<uint8_t>(-dx);
	if (dx >= radiusX)
		return;
	Ram(TempData2) = Ram(Add16(ix, 4));
	uint8_t id = Ram(ix);
	Ram(Add16(TempData, 1)) = id;
	if (id == AMMO_CRATE) {
		PickAmmoCrate(ix);
		return;
	}
	if (id < SUPRESSOR) {
		PickUpWeapon(ix);
		return;
	}
	if (id == SUPRESSOR) {
		SetItemAsTaken(id);
		Ram(InvSupressor) = SUPRESSOR;
		EraseTakenItem(ix);
		return;
	}
	SetItemAsTaken(id);
	uint8_t item = static_cast<uint8_t>(id - SUPRESSOR);
	if (item == 0x1a) {
		RecoverEquipment(ix);
		return;
	}
	uint16_t entry;
	uint8_t slot;
	if (GetItemInvAdd(item, &entry, &slot)) {
		AddItemAmount(entry);
		EraseTakenItem(ix);
		return;
	}
	entry = Equipment;
	for (int i = 0; Ram(entry) != 0; i++) {
		if (i == 24)
			return;	// full
		entry = Add16(entry, 4);
	}
	Ram(entry) = item;
	if (item == SELECTED_ANTENNA) {
		Ram(IncomingCallTimer) = 0x10;
		Ram(RadioCallFlag) = 0;	// pending calls come now
	}
	AddItemAmount(entry);
	EraseTakenItem(ix);
}

}	// namespace


void
ChkTakeItems()
{
	uint8_t y = Ram(PlayerY);
	uint8_t x = Ram(PlayerX);
	uint16_t entry = ItemsInTheRoom;
	uint8_t b = 3;
	uint8_t index = 0;
	do {
		if (Ram(entry) != 0)
			ChkTakeItem(entry, index, x, y);
		if (Ram(GameMode) == GAME_MODE_TEXT_BOX)
			b = 1;	// a description is shown: no more items
		entry = Add16(entry, 0x10);
		index++;
	} while (--b != 0);
}


void
LimitAmmo()
{
	LimitWeapon(HAND_GUN, MaxAmmoGun);
	LimitWeapon(SUB_MACHINE_GUN, MaxAmmoSMG);
	LimitWeapon(GRENADE_LAUNCHER, MaxAmmoGrenade);
	LimitWeapon(ROCKET_LAUNCHER, MaxAmmonRocket);
	LimitWeapon(PLASTIC_BOMB, MaxAmmoBomb);
	LimitWeapon(LAND_MINE, MaxAmmoMine);
	LimitWeapon(MISSILE, MaxAmmoMissile);
	uint16_t entry;
	uint8_t index;
	if (GetItemInvAdd(SELECTED_RATION, &entry, &index))
		ChkMaxAmount(entry, Ram16(MaxRations));
}


// The limits depend on the class; INTRUDER and ISOLATION raise them.
void
SetMaxAmmoVals()
{
	Ram(MaxAmmoGun) = 0xff;
	Ldir(MaxAmmoGun, Add16(MaxAmmoGun, 1), 0x4f);
	static const sym::Id kTables[] = { sym::MaxAmmoLv1, sym::MaxAmmoLv2, sym::MaxAmmoLv3,
		sym::MaxAmmoLv4 };
	static const uint8_t kRations[] = { 3, 6, 9, 0x12 };
	uint8_t level = Ram(Class) > 3 ? 3 : Ram(Class);
	uint16_t table = RomSymbol(kTables[level]);
	for (int i = 0; i < 0x10; i++)
		Ram(Add16(MaxAmmoGun, i)) = Mem(Add16(table, i));
	SetRam16(MaxRations, kRations[level]);
	uint8_t cheats = Ram(MaxAmmoRatioF);
	if (cheats & 1) {
		for (int i = 0; i < 0x10; i++)
			Ram(Add16(MaxAmmoGun, i)) = Mem(Add16(SYM(MaxAmmoVals), i));
	}
	if (cheats & 2)
		SetRam16(MaxRations, 0x999);

	// (!?) the carry is the other way round: only absent rations and
	// weapons are clamped (the entry after the list)
	uint16_t entry;
	uint8_t index;
	if (!GetItemInvAdd(SELECTED_RATION, &entry, &index))
		ChkMaxAmount(entry, Ram16(MaxRations));
	for (uint8_t i = 0; i < 8; i++) {
		if (!GetWeaponInvAdd(static_cast<uint8_t>(HAND_GUN + i), &entry, &index))
			ChkMaxAmount(entry, Ram16(Add16(MaxAmmoGun, i * 2)));
	}
}


namespace {

// (!?) The loop looking for a used slot only looks at the next one (it
// should skip PUSH DE / POP HL), so gaps of two stay.
void
CompactSlots(uint16_t de, uint8_t b)
{
	for (;;) {
		// FindEmptySlot
		while (Ram(de) != 0) {
			de = Add16(de, 4);
			if (--b == 0)
				return;
		}
		// FindUsedSlot
		uint16_t hl;
		for (;;) {
			hl = Add16(de, 4);
			if (Ram(hl) != 0)
				break;
			if (--b == 0)
				return;
		}
		for (int i = 0; i < 4; i++) {
			Ram(de) = Ram(hl);
			Ram(hl) = 0;
			hl = Add16(hl, 1);
			de = Add16(de, 1);
		}
		if (--b == 0)
			return;
	}
}

}	// namespace


void
CompactWeapons()
{
	CompactSlots(Weapons, 6);
}


void
CompactEquipment()
{
	CompactSlots(Equipment, 24);
}
