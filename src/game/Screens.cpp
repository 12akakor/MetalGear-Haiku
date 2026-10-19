/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from Banks0123.asm (RadioLogic .. FadeOutColors).
#include "Screens.h"

#include "Actors.h"
#include "Common.h"
#include "GameGfx.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "RoomGfx.h"
#include "Rom.h"
#include "Sprites.h"
#include "Structures.h"
#include "Text.h"
#include "Unported.h"


using namespace var;


void
Ldir(uint16_t source, uint16_t dest, uint16_t count)
{
	int n = count == 0 ? 0x10000 : count;
	for (int i = 0; i < n; i++)
		Ram(dest++) = Mem(source++);
}


static void
ErasePrintTxt(uint16_t print, uint16_t erase)
{
	EraseTextXY(erase);
	PrintTextXY(print);
}


static void
NextRadioStat()
{
	Ram(EquipRadioStatus)++;
}


static void
DrawRadio()
{
	DisableScreen();
	ClearSprAttr();
	ClearPage0();
	SetRadioPal();
	Ram(RadioCallFlag) = 2;
	Ram(TilesetBank) = 1;
	DrawTilesBlock(SYM(RadioTilesMap), 0x30, 0x18);
	DrawTilesBlock(SYM(SnakeTilesMap), 0xc8, 0x28);
	Ram(RadioCmd) = 0;
	Ldir(RadioCmd, RadioLedCnt, 0x10);
	PrintTransceiv();
	DrawRadioFreq();
	RenderHUD();
	NextRadioStat();
	if (Ram(EndingStatus) == 0)
		SetSoundEntryChk(0x50);	// radio noise
	EnableScreen();
}


// Left and right tune (BCD 00-99), held keys repeat.
static void
ChgRadioFreq()
{
	uint8_t a = Ram(ControlsTrigger) & 0x0c;
	if (a != 0) {
		Ram(AutoReplyDone) = 0;
		Ram(static_cast<uint16_t>(AutoReplyDone + 1)) = 0;
		Ram(ControlHoldWait) = 8;
	} else {
		a = Ram(ControlsHold) & 0x0c;
		if (a == 0 || --Ram(ControlHoldWait) != 0)
			return;
		Ram(ControlHoldWait) = 2;
	}
	uint8_t freq = Ram(RadioFreq);
	if ((a & 4) != 0) {
		if (freq == 0)
			return;
		Ram(RadioFreq) = (freq & 0x0f) == 0 ? static_cast<uint8_t>(freq - 7)
			: static_cast<uint8_t>(freq - 1);
	} else {
		if (freq == 0x99)
			return;
		Ram(RadioFreq) = (freq & 0x0f) == 9 ? static_cast<uint8_t>(freq + 7)
			: static_cast<uint8_t>(freq + 1);
	}
}


// Somebody on this frequency: either answers at once (auto reply) or
// after Snake called (up = SEND).
static void
ChkRadioReceiv()
{
	DrawRadioFreq();
	uint8_t count = Ram(NumRadioPersons);
	if (count == 0)
		return;
	uint8_t freq = Ram(RadioFreq);
	uint16_t hl = RadioPersonsDat;
	for (int i = 0; i < count; i++, hl = static_cast<uint16_t>(hl + 0x20)) {
		if (Ram(hl) != freq)
			continue;
		if ((Ram(static_cast<uint16_t>(hl + 2)) & 1) == 0) {
			if (Ram(AutoReplyDone) != 0)
				return;
		} else if (Ram(ReplyRequested) == 0) {
			continue;
		}
		if (ChkRadioReply(hl))
			return;
		SetRam16(ReplyRadioPerson, hl);
		Ram(EquipRadioStatus) = 2;
		Ram(RadioLedDelay) = 0x10;
		return;
	}
}


static void
RadioIdle()
{
	if (Ram(RadioCmd) != 0 && Ram(EndingStatus) == 0)
		SetSoundEntryChk(0x50);
	ErasePrintTxt(SYM(txtRecv), SYM(txtSend));
	if ((Ram(ControlsTrigger) & 1) != 0) {
		// SEND
		Ram(RadioCmd) = 1;
		Ram(ReplyRequested) = 1;
		ErasePrintTxt(SYM(txtSend), SYM(txtRecv));
		SetSoundEntryChk(0x5c);
		SetText(0x0a);	// This is Solid Snake... Your reply, please.
		return;
	}
	Ram(RadioCmd) = 0;
	ChgRadioFreq();
	ChkRadioReceiv();
}


static void
RadioSignalUp()
{
	if (--Ram(RadioLedDelay) != 0)
		return;
	Ram(RadioLedDelay) = 2;
	Ram(RadioLedCnt)++;
	DrawRadioLeds();
	if (Ram(RadioLedCnt) != 12)
		return;
	NextRadioStat();
}


static void
SetupRadioReply()
{
	Ram(EquipRadioStatus) = 4;
	SetSoundEntryChk(0x5c);
	SetText(Ram(static_cast<uint16_t>(Ram16(ReplyRadioPerson) + 3)));
}


static void
RadioSignalOFF()
{
	Ram(ReplyRequested) = 0;
	Ram(RadioLedCnt) = 0;
	Ram(AutoReplyDone) = 1;
	Ram(EquipRadioStatus) = 1;
	if (Ram(EndingStatus) == 0)
		SetSoundEntryChk(0x50);
	DrawRadioLeds();
}


void
RadioLogic()
{
	if ((Ram(FKeysTrigger) & 8) != 0) {
		ExitRadio();
		return;
	}
	switch (Ram(EquipRadioStatus)) {
		case 0:
			DrawRadio();
			break;
		case 1:
			RadioIdle();
			break;
		case 2:
			RadioSignalUp();
			break;
		case 3:
			SetupRadioReply();
			break;
		case 4:
			RadioSignalOFF();
			break;
		default:
			if (gUnported == NULL)
				gUnported = "RadioLogic: bad status";
			break;
	}
}


// Who answers what: without the antenna nobody far away; Big Boss tells
// Snake to switch off the MSX, or that he is bugged; Schneider not once
// captured; Jennifer only at rank 3 and while her brother lives.
bool
ChkRadioReply(uint16_t hl)
{
	uint16_t text = static_cast<uint16_t>(hl + 3);
	uint8_t id = Ram(text);
	if (Ram(MapZone) >= 5 && Ram(AntennaTaken) == 0)
		return true;
	uint8_t freq = Ram(hl);
	bool bigBoss = freq == FREQ_BIGBOSS || freq == FREQ_BIGBOSS_BUILDING2;
	if (bigBoss && Ram(SwitchOffMSXF) != 0) {
		Ram(text) = 136;
		return false;
	}
	if (bigBoss && Ram(TransmiTaken) != 0 && Ram(MapZone) != 4) {
		Ram(text) = 50;
		return false;
	}
	if (freq == FREQ_SCHNEIDER || freq == FREQ_SCHNEIDER_BUILDING2)
		return Ram(SchneiderCaptured) != 0;
	if (freq == FREQ_JENIFFER)
		return Ram(Class) != 3 || Ram(JennifBrotherDead) != 0;
	if (id != 15)
		return false;
	return Ram(MadnarMoved) != 0;
}


void
DrawRadioFreq()
{
	uint8_t x = 0x98;
	PrintDigit(static_cast<uint8_t>(Ram(RadioFreq) >> 4), 1, &x, 0x21);
	PrintDigit(Ram(RadioFreq), 1, &x, 0x21);
}


// Two rows of leds per tile column.
static void
DrawRadioLeds4(uint8_t sx, uint8_t sy, uint8_t count, uint8_t* x, uint8_t y)
{
	int n = count == 0 ? 256 : count;
	for (int i = 0; i < n; i++) {
		VDP_Copy_Byte(sx, sy, 1, *x, y, 8, 8);
		VDP_Copy_Byte(sx, sy, 1, *x, static_cast<uint8_t>(y + 8), 8, 8);
		*x = static_cast<uint8_t>(*x + 8);
	}
}


// 12 leds, two per tile: on-on, on-off, off-off.
void
DrawRadioLeds()
{
	uint8_t x = 0x40;
	const uint8_t y = 0x20;
	uint8_t count = Ram(RadioLedCnt);
	if ((count >> 1) != 0)
		DrawRadioLeds4(0x18, 0x90, static_cast<uint8_t>(count >> 1), &x, y);
	if ((count & 1) != 0)
		DrawRadioLeds4(0x10, 0x90, 1, &x, y);
	if (count >= 11)
		return;
	uint8_t c = static_cast<uint8_t>((count & 1) + (count >> 1));
	DrawRadioLeds4(0x08, 0x90, static_cast<uint8_t>(6 - c), &x, y);
}


void
ExitRadio()
{
	Ram(RestoreSoundData) = 1;
	SetSnakePal();
	RenderScreen();
}


void
SelectWeapon2(uint8_t weapon)
{
	Ram(SelectedWeapon) = weapon;
	Ram(WeaponInUse) = weapon;
	DrawWeaponHUD();
}


// Left column 1-4, right column 5-7.
static void
CtrlMenuWeapon()
{
	uint8_t a = Ram(ControlsTrigger) & 0x0f;
	if (a == 0) {
		a = Ram(ControlsHold) & 0x0f;
		if (a == 0 || --Ram(ControlHoldWait) != 0)
			return;
	}
	Ram(ControlHoldWait) = 8;
	uint8_t index = Ram(SelectIdx);
	uint8_t next;
	if ((a & 1) != 0) {
		if (index == 1 || index == 5)
			return;
		next = static_cast<uint8_t>(index - 1);
	} else if ((a & 2) != 0) {
		if (index == 4 || index == 7)
			return;
		next = static_cast<uint8_t>(index + 1);
	} else if ((a & 4) != 0) {
		if (index < 5)
			return;
		next = static_cast<uint8_t>(index - 4);
	} else {
		if (index >= 4)
			return;
		next = static_cast<uint8_t>(index + 4);
	}
	SetSoundEntryChk(0x20);
	EraseArrow();
	Ram(SelectIdx) = next;
	CalcCursorXYWeapon();
	DrawArrow();
	if (Ram(EquipRemoved) != 0) {
		SelectWeapon2(0);
		return;
	}
	SelectWeapon2(Ram(static_cast<uint16_t>(Weapons + DEC_A_HL_4xA(Ram(SelectIdx)))));
}


void
MenuWeapon()
{
	if ((Ram(FKeysTrigger) & 2) != 0) {
		RenderScreen();
		return;
	}
	if (Ram(EquipRadioStatus) == 1) {
		CtrlMenuWeapon();
		return;
	}
	DisableScreen();
	ClearSprAttr();
	ClearPage0();
	SetMenuWeaponPal();
	Ram(EquipRadioStatus)++;
	CompactWeapons();
	DrawWeaponMenu();
	GetWeaponCursor();
	CalcCursorXYWeapon();
	DrawArrow();
	RenderHUD();
	EnableScreen();
}


// Back to the room after a menu or the radio.
void
RenderScreen()
{
	Ram(GameMode) = 0;
	DisableScreen();
	ClearSprAttr();
	LoadSprProjectile();
	for (int i = 0; i < 0x18; i++)
		Ram(static_cast<uint16_t>(SprShootsAtt + i)) = 0xe0;
	Ram(PlayerShotsList) = 0;
	Ldir(PlayerShotsList, PlayerShot1Stat, 0x17f);
	Ram(LaserRoomCnt) = 0;
	RenderRoom();
	SetRoomPal();
	SetSprPal();
	DrawDoors();
	if (Ram(IntroSceneStatus) >= 0x0c) {
		SetBankInA0(6);
		DrawRoomItems();
		SetBanks1_2_3();
		SetBankInA0(0x0f);
		DrawLaserBeams();
		SetBanks1_2_3();
		SetBankInA0(0x0c);
		ChkDrawMetalGear();
		SetBanks1_2_3();
	}
	DrawMines();
	ChkDrawHindD();
	DrawOpenPitfalls();
	DrawCameras();
	ChkDrawDestroyPS(EnemyList + 0x800);	// IX after DrawCameras (to check)
	RenderHUD();
	EnableScreen();
}


void
GetWeaponCursor()
{
	GetMenuCursor(SelectedWeapon, 7, Weapons);
}


void
GetMenuCursor(uint16_t selected, uint8_t count, uint16_t table)
{
	uint8_t c = 1;
	uint8_t id = Ram(selected);
	uint16_t hl = table;
	int n = count == 0 ? 256 : count;
	for (int i = 0; i < n; i++) {
		if (Ram(hl) == id) {
			Ram(SelectIdx) = c;
			return;
		}
		hl = static_cast<uint16_t>(hl + 4);
		c++;
	}
	if (id == 0)
		Ram(SelectIdx) = 1;	// no empty entry either
}


void
EraseArrow()
{
	DrawChar(0, 0xff, Ram(static_cast<uint16_t>(MenuCursorXY + 1)), Ram(MenuCursorXY));
}


void
DrawArrow()
{
	DrawChar(0x3c, 0xff, Ram(static_cast<uint16_t>(MenuCursorXY + 1)), Ram(MenuCursorXY));
}


// Colour number, red-blue, green per colour, #FF at the end.
void
CopyPalToRAM()
{
	uint16_t de = CurrentPal;
	uint16_t hl = 0xf680;
	for (int i = 0; i < 16; i++) {
		Ram(de++) = static_cast<uint8_t>(i);
		de = VRAMtoRAM(hl, de, 2);
		hl = static_cast<uint16_t>(hl + 2);
	}
	Ram(de) = 0xff;
}


bool
FadeOutLogic()
{
	FadeOutColors();
	SetPalette(CurrentPal);
	uint8_t a = 0;
	uint16_t hl = static_cast<uint16_t>(CurrentPal + 1);
	for (int i = 0; i < 16; i++) {
		a |= Ram(hl);
		a |= Ram(static_cast<uint16_t>(hl + 1));
		hl = static_cast<uint16_t>(hl + 3);
	}
	return a == 0;
}


void
FadeOutColors()
{
	if ((Ram(TickCounter) & 3) != 0)
		return;
	uint16_t hl = static_cast<uint16_t>(CurrentPal + 1);
	for (int i = 0; i < 16; i++) {
		uint8_t rb = Ram(hl);
		if ((rb & 0xf0) != 0)
			rb = static_cast<uint8_t>(rb - 0x10);
		if ((rb & 0x0f) != 0)
			rb--;
		Ram(hl) = rb;
		uint16_t g = static_cast<uint16_t>(hl + 1);
		Ram(g) &= 0x0f;
		if (Ram(g) != 0)
			Ram(g)--;
		hl = static_cast<uint16_t>(hl + 3);
	}
}
