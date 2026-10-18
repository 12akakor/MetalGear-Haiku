/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Start up, tick and game status logic, rewritten from Banks0123.asm.
#include "GameMain.h"

#include "GfxBasic.h"
#include "Input.h"
#include "Rom.h"
#include "Screen.h"
#include "SoundDriver.h"
#include "Title.h"


const char* gUnported = NULL;


namespace {

const uint16_t kSCRMOD = 0xfcaf;
const uint16_t kHTIMI = 0xfd9f;
const uint16_t kCLIKSW = 0xf3db;
const uint16_t kFNKSTR = 0xf87f;

// BIOS CHGMOD 5 (GRAPHIC 4), the settings the game relies on.
void
CHGMOD5()
{
	static const uint8_t kRegs[][2] = {
		{0, 0x06}, {1, 0x60}, {2, 0x1f}, {5, 0xef}, {6, 0x0f},
		{11, 0x00}, {8, 0x08}, {9, 0x80}
	};
	for (size_t i = 0; i < sizeof(kRegs) / sizeof(kRegs[0]); i++)
		WRTVDP(kRegs[i][0], kRegs[i][1]);
	Ram(kSCRMOD) = 5;
}


void
InitHardware()
{
	Ram(var::PGS_MixerVal) = 0xbf;
	gSoundDriver.WritePsg(7, 0xbf);
	DisableSprites();
	CHGMOD5();
	DisableScreen();
	ClearPage0();
	ClearPage(1);
	uint16_t data = SYM(InitVdpDat);
	for (int i = 0; i < 4; i++) {
		uint8_t reg = Rom(data++);
		uint8_t value = Rom(data++);
		WRTVDP(reg, value);
	}
	for (int i = 0; i <= 0xa0; i++)
		Ram(static_cast<uint16_t>(kFNKSTR + i)) = 0;
	ClearSprAttr();
	EnableScreen();
}


void
NextSubstatusT(uint8_t wait)
{
	Ram(var::WaitCounter) = wait;
	NextSubstatus();
}


void
NextGameStatus(uint8_t wait)
{
	Ram(var::WaitCounter) = wait;
	Ram(var::GameStatus)++;
	Ram(var::GameSubstatus) = 0;
}


void
ChkEnableMusicDemo()
{
	uint8_t trigger = StoreControls(var::FKeysHoldMenu, ReadFKeys());
	if ((trigger & 0x10) != 0)
		Ram(var::MusicInDemoMode) = static_cast<uint8_t>(~Ram(var::MusicInDemoMode));
}


// Status 0: Konami logo, Metal Gear logo, menu.
void
GS_KonamiLogo(uint8_t substatus)
{
	switch (substatus) {
		case 1:
			ChkEnableMusicDemo();
			SetBankInA0(9);
			DrawKonamiLogo();
			SetBanks(1);
			if (Ram(var::Class) == 0)
				return;
			// RegionLock: only checks the MSX model
			NextSubstatusT(0);
			return;

		case 2:	// GS_LoadIntroGfx
			ChkEnableMusicDemo();
			if (--Ram(var::WaitCounter) != 0)
				return;
			SetBankInA0(9);
			LoadIntroGfx();
			SetBanks(1);
			NextSubstatus();
			return;

		case 3:	// GS_IntroMenu
			ChkEnableMusicDemo();
			SetBankInA0(9);
			MenuLogoLogic();
			SetBanks(1);
			if (Ram(var::MenuStatus) != 4)
				return;
			NextGameStatus(0);
			return;

		default:	// GS_InitKonamiLogo
			ClearScreen();
			SetBankInA0(9);
			InitKonamiLogo();
			SetBanks(1);
			NextSubstatus();
			return;
	}
}


// Status 1: menu wait
void
GS_WaitMenu()
{
	ChkEnableMusicDemo();
	if (--Ram(var::WaitCounter) != 0)
		return;
	NextGameStatus(0x20);
}


// Before the game starts: any key shows the menu, fire starts a game.
void
ChkAnykeyStart()
{
	uint8_t trigger = StoreControls(var::ControlsHold_, ReadControls());
	if (trigger == 0)
		return;
	Ram(var::WaitCounter) = 0;
	if (Ram(var::GameStatus) != 1) {
		// GoToMenu
		Ram(var::GameStatus) = 1;
		SetSoundEntry(0x28);
		SetBankInA0(9);
		LoadIntroGfx();
		Ram(var::MenuStatus) = 4;
		MenuLogoLogic();
		SetBanks(1);
		return;
	}
	if ((trigger & 0x30) == 0)
		return;
	Ram(var::ControlConfig) = 0x40;
	Ram(var::GameStatus) = 3;
	Ram(var::GameSubstatus) = 0;
}

}	// namespace


void
NextSubstatus()
{
	Ram(var::GameSubstatus)++;
}


void
SetSoundEntry(uint8_t id)
{
	gSoundDriver.SetSound(id);
}


void
SetSoundEntryChk(uint8_t id)
{
	if ((Ram(var::ControlConfig) & 0x40) != 0)
		SetSoundEntry(id);
}


void
Start()
{
	for (uint16_t a = 0xc000; a <= 0xf0ef; a++)
		Ram(a) = 0;
	SetBanks(1);
	// RegionLock: only checks the MSX model
	Ram(var::BankIn60Fixed) = 0;
	Ram(var::BankIn80Fixed) = 0;
	Ram(var::BankInA0Fixed) = 0;
	InitHardware();
	Ram(kHTIMI) = 0xc3;	// JP InterruptTick, as the original
	uint16_t tick = SYM(InterruptTick);
	Ram(static_cast<uint16_t>(kHTIMI + 1)) = static_cast<uint8_t>(tick);
	Ram(static_cast<uint16_t>(kHTIMI + 2)) = static_cast<uint8_t>(tick >> 8);
	Ram(kCLIKSW) = 0;
}


void
InterruptTick()
{
	gSoundDriver.Update();
	Ram(var::TickInProgress) = 1;
	GameStatusLogic();
	Ram(var::TickInProgress) = 0;
}


void
GameStatusLogic()
{
	Ram(var::TickCounter)++;
	uint8_t status = Ram(var::GameStatus);
	uint8_t substatus = Ram(var::GameSubstatus);
	switch (status) {
		case 0:
			GS_KonamiLogo(substatus);
			break;
		case 1:
			GS_WaitMenu();
			break;
		default:
			if (gUnported == NULL)
				gUnported = "game status 2 and higher (demo, game)";
			return;
	}
	if (status < 3)
		ChkAnykeyStart();
}
