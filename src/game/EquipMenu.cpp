/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/menuequipment.asm: choosing and using items, and
// going to the binoculars.
#include "EquipMenu.h"

#include "Common.h"
#include "GameGfx.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "Items.h"
#include "Rom.h"
#include "Screens.h"
#include "Sprites.h"
#include "Structures.h"


using namespace var;


namespace {

// ADD A,B then DAA.
uint8_t
BcdAdd(uint8_t a, uint8_t b)
{
	unsigned sum = a + b;
	bool half = ((a & 0x0f) + (b & 0x0f)) > 0x0f;
	bool carry = sum > 0xff;
	sum &= 0xff;
	unsigned correction = 0;
	if (half || (sum & 0x0f) > 9)
		correction |= 0x06;
	if (carry || sum > 0x99)
		correction |= 0x60;
	return static_cast<uint8_t>(sum + correction);
}


void
Fill(uint16_t address, uint8_t value, int count)
{
	for (int i = 0; i < count; i++)
		Ram(static_cast<uint16_t>(address + i)) = value;
}


void
UseItemSfx()
{
	SetSoundEntryChk(0x21);
}


// Rations fill the life (not in deep water), the transmitter is dropped,
// the antidote cures, cigarettes add 2000 to the destruction timer.
void
ChkUseItem()
{
	uint8_t selected = Ram(SelectedItem);
	if (selected == 0)
		return;
	uint16_t item;
	uint8_t index;
	GetItemInvAdd(selected, &item, &index);
	// (!?) in deep water A holds 4 here, which is no item to use
	uint8_t id = Ram(PlayerAnimation) == 4 ? 4 : Mem(item);
	uint16_t units = static_cast<uint16_t>(item + 1);
	if (id == SELECTED_RATION) {
		DecItemUnits(units, 2);
		Ram(Life) = Ram(MaxLife);
		DrawLife();
		UseItemSfx();
		return;
	}
	if (id == SELECTED_TRANSMITTER) {
		DecItemUnits(units, 2);
		Ram(TransmiTaken) = 0;
		UseItemSfx();
		return;
	}
	if (id == SELECTED_ANTIDOTE) {
		Ram(Poisoned) = 0;
		UseItemSfx();
		return;
	}
	if (id != SELECTED_CIGARETTES || Ram(DestructionTimerOn) == 0)
		return;
	DecItemUnits(units, 2);
	Ram(DestructTimerH) = BcdAdd(0x20, Ram(DestructTimerH));
	DrawCallTimer();
	UseItemSfx();
}


void
MenuEquipMove(uint8_t index)
{
	SetSoundEntryChk(0x20);
	EraseArrow();
	Ram(SelectIdx) = index;
	CalcCursorXYEquip();
	DrawArrow();
	uint8_t item = 0;
	if (Ram(EquipRemoved) == 0)
		item = Ram(static_cast<uint16_t>(Equipment + DEC_A_HL_4xA(Ram(SelectIdx))));
	Ram(SelectedItem) = item;
	DrawItemHUD();
}


// Three columns of 9, 9 and 7 items.
void
MenuEquipLogic()
{
	if ((Ram(ControlsTrigger) & 0x10) != 0) {
		ChkUseItem();
		return;
	}
	uint8_t a = Ram(ControlsTrigger) & 0x0f;
	if (a == 0) {
		a = Ram(ControlsHold) & 0x0f;
		if (a == 0 || --Ram(ControlHoldWait) != 0)
			return;
	}
	Ram(ControlHoldWait) = 8;
	uint8_t index = Ram(SelectIdx);
	if ((a & 1) != 0) {
		if (index == 1 || index == 10 || index == 19)
			return;
		MenuEquipMove(static_cast<uint8_t>(index - 1));
	} else if ((a & 2) != 0) {
		if (index == 9 || index == 18 || index == 25)
			return;
		MenuEquipMove(static_cast<uint8_t>(index + 1));
	} else if ((a & 4) != 0) {
		if (index < 10)
			return;
		MenuEquipMove(static_cast<uint8_t>(index - 9));
	} else {
		if (index >= 17)
			return;
		MenuEquipMove(static_cast<uint8_t>(index + 9));
	}
}


// Back to the room, or the binoculars when they are selected.
void
ExitEquipMenu()
{
	if (Ram(IsolatedRoom) != 0 || Ram(SelectedItem) != SELECTED_BINOCULARS) {
		RenderScreen();
		return;
	}
	Ram(GameMode) = GAME_MODE_BINOCULARS;
	Ram(BinoculStatus) = 0;
	Ram(BinocularDir) = 1;
	DisableScreen();
	for (int i = 0; i < 0x800; i++)
		Ram(static_cast<uint16_t>(EnemyListCopy + i)) = Ram(static_cast<uint16_t>(EnemyList + i));
	Ram(PowerSwitchOnCopy) = Ram(PowerSwitchOn);
	SetRam16(RadioCallFlagCopy, Ram16(RadioCallFlag));
	Ram(AlertModeCopy2) = Ram(AlertModeCopy);
	Fill(static_cast<uint16_t>(SprAttRAM + 0x0e), 0xe0, 0x58);	// sprites hidden
	for (int i = 0; i < 0x18; i++)
		Ram(static_cast<uint16_t>(SprShootsAtt + i)) = Mem(static_cast<uint16_t>(SYM(BinocularSprAtt)
			+ i));
	Fill(BinocularSprCol, 0x0e, 0x40);	// white
	LoadSprTarget();
}

}	// namespace


void
MenuEquip()
{
	if ((Ram(FKeysTrigger) & 4) != 0) {
		ExitEquipMenu();
		return;
	}
	if (Ram(EquipRadioStatus) == 1) {
		MenuEquipLogic();
		return;
	}
	DisableScreen();
	ClearSprAttr();
	ClearPage0();
	SetMenuWeaponPal();
	Ram(EquipRadioStatus)++;
	CompactEquipment();
	DrawEquipMenu();
	GetMenuCursor(SelectedItem, 25, Equipment);
	CalcCursorXYEquip();
	DrawArrow();
	RenderHUD();
	EnableScreen();
}
