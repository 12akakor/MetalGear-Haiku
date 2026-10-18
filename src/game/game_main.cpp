/*
 * Start up, tick and game status logic, rewritten from Banks0123.asm.
 */
#include "game_main.h"

#include "gfx_basic.h"
#include "input.h"
#include "rom.h"
#include "screen.h"
#include "sound_driver.h"
#include "title.h"

const char* g_unported = NULL;


namespace {

const uint16_t kSCRMOD = 0xfcaf;
const uint16_t kHTIMI = 0xfd9f;
const uint16_t kCLIKSW = 0xf3db;
const uint16_t kFNKSTR = 0xf87f;

/* BIOS CHGMOD 5 (GRAPHIC 4), the settings the game relies on. */
void
CHGMOD5()
{
	static const uint8_t kRegs[][2] = {
		{0, 0x06}, {1, 0x60}, {2, 0x1f}, {5, 0xef}, {6, 0x0f},
		{11, 0x00}, {8, 0x08}, {9, 0x80}
	};
	for (size_t i = 0; i < sizeof(kRegs) / sizeof(kRegs[0]); i++)
		WRTVDP(kRegs[i][0], kRegs[i][1]);
	ram(kSCRMOD) = 5;
}


void
InitHardware()
{
	ram(var::PGS_MixerVal) = 0xbf;
	g_soundDriver.WritePsg(7, 0xbf);
	DisableSprites();
	CHGMOD5();
	DisableScreen();
	ClearPage0();
	ClearPage(1);
	uint16_t data = SYM(InitVdpDat);
	for (int i = 0; i < 4; i++) {
		uint8_t reg = rom(data++);
		uint8_t value = rom(data++);
		WRTVDP(reg, value);
	}
	for (int i = 0; i <= 0xa0; i++)
		ram((uint16_t)(kFNKSTR + i)) = 0;
	ClearSprAttr();
	EnableScreen();
}


void
NextSubstatusT(uint8_t wait)
{
	ram(var::WaitCounter) = wait;
	NextSubstatus();
}


void
NextGameStatus(uint8_t wait)
{
	ram(var::WaitCounter) = wait;
	ram(var::GameStatus)++;
	ram(var::GameSubstatus) = 0;
}


void
ChkEnableMusicDemo()
{
	uint8_t trigger = StoreControls(var::FKeysHoldMenu, ReadFKeys());
	if (trigger & 0x10)
		ram(var::MusicInDemoMode) = (uint8_t)~ram(var::MusicInDemoMode);
}


/* Status 0: Konami logo, Metal Gear logo, menu. */
void
GS_KonamiLogo(uint8_t substatus)
{
	switch (substatus) {
		case 1:
			ChkEnableMusicDemo();
			SetBankInA0(9);
			DrawKonamiLogo();
			set_banks(1);
			if (ram(var::Class) == 0)
				return;
			/* RegionLock: only checks the MSX model */
			NextSubstatusT(0);
			return;

		case 2:		/* GS_LoadIntroGfx */
			ChkEnableMusicDemo();
			if (--ram(var::WaitCounter) != 0)
				return;
			SetBankInA0(9);
			LoadIntroGfx();
			set_banks(1);
			NextSubstatus();
			return;

		case 3:		/* GS_IntroMenu */
			ChkEnableMusicDemo();
			SetBankInA0(9);
			MenuLogoLogic();
			set_banks(1);
			if (ram(var::MenuStatus) != 4)
				return;
			NextGameStatus(0);
			return;

		default:	/* GS_InitKonamiLogo */
			ClearScreen();
			SetBankInA0(9);
			InitKonamiLogo();
			set_banks(1);
			NextSubstatus();
			return;
	}
}


/* Status 1: menu wait */
void
GS_WaitMenu()
{
	ChkEnableMusicDemo();
	if (--ram(var::WaitCounter) != 0)
		return;
	NextGameStatus(0x20);
}


/* Before the game starts: any key shows the menu, fire starts a game. */
void
ChkAnykeyStart()
{
	uint8_t trigger = StoreControls(var::ControlsHold_, ReadControls());
	if (trigger == 0)
		return;
	ram(var::WaitCounter) = 0;
	if (ram(var::GameStatus) != 1) {
		/* GoToMenu */
		ram(var::GameStatus) = 1;
		SetSoundEntry(0x28);
		SetBankInA0(9);
		LoadIntroGfx();
		ram(var::MenuStatus) = 4;
		MenuLogoLogic();
		set_banks(1);
		return;
	}
	if ((trigger & 0x30) == 0)
		return;
	ram(var::ControlConfig) = 0x40;
	ram(var::GameStatus) = 3;
	ram(var::GameSubstatus) = 0;
}

}	// namespace


void
NextSubstatus()
{
	ram(var::GameSubstatus)++;
}


void
SetSoundEntry(uint8_t id)
{
	g_soundDriver.SetSound(id);
}


void
SetSoundEntryChk(uint8_t id)
{
	if (ram(var::ControlConfig) & 0x40)
		SetSoundEntry(id);
}


void
Start()
{
	for (uint16_t a = 0xc000; a <= 0xf0ef; a++)
		ram(a) = 0;
	set_banks(1);
	/* RegionLock: only checks the MSX model */
	ram(var::BankIn60Fixed) = 0;
	ram(var::BankIn80Fixed) = 0;
	ram(var::BankInA0Fixed) = 0;
	InitHardware();
	ram(kHTIMI) = 0xc3;		/* JP InterruptTick, as the original */
	uint16_t tick = SYM(InterruptTick);
	ram((uint16_t)(kHTIMI + 1)) = (uint8_t)tick;
	ram((uint16_t)(kHTIMI + 2)) = (uint8_t)(tick >> 8);
	ram(kCLIKSW) = 0;
}


void
InterruptTick()
{
	g_soundDriver.Update();
	ram(var::TickInProgress) = 1;
	GameStatusLogic();
	ram(var::TickInProgress) = 0;
}


void
GameStatusLogic()
{
	ram(var::TickCounter)++;
	uint8_t status = ram(var::GameStatus);
	uint8_t substatus = ram(var::GameSubstatus);
	switch (status) {
		case 0:
			GS_KonamiLogo(substatus);
			break;
		case 1:
			GS_WaitMenu();
			break;
		default:
			if (g_unported == NULL)
				g_unported = "game status 2 and higher (demo, game)";
			return;
	}
	if (status < 3)
		ChkAnykeyStart();
}
