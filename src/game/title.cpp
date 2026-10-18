/*
 * Konami logo, Metal Gear logo and menu, rewritten from
 * logic/konamilogo.asm, logic/mainmenu.asm and logic/loadfont.asm.
 * The "_" helpers of the original call a routine and then restore banks
 * 1, 2, X (SetBanks_1_2_X).
 */
#include "title.h"

#include "game_main.h"
#include "gfx_basic.h"
#include "rom.h"


void
InitKonamiLogo()
{
	DisableScreen();
	SetBanks_1_2_X();
	SetSoundEntry(0x5c);
	SetBanks_1_2_X();
	SetDefaultPal();
	SetBanks_1_2_X();
	SetPalette(SYM(KonamiLogoPal));
	SetBanks_1_2_X();
	WRTVDP(7, 0x0f);		/* background white */
	FillRect(0, 0x28, 0x40, 1, 0xa8, 0x48);
	SetBanks_1_2_X();
	SetUpKonamiLogo();
	SetBanks_1_2_X();
	EnableScreen();
	SetBanks_1_2_X();
	ram(var::Room) = 0x3c;
	ram((uint16_t)(var::Room + 1)) = 0x31;
	ram((uint16_t)(var::Room + 2)) = 0;
}


void
SetUpKonamiLogo()
{
	set_banks(7);
	uint8_t x = 0x08, y = 0x00;
	Load1bppTiles(SYM(gfxKonamiLogo), &x, &y, 0x0d, 1);
	x = 0x70;
	y = 0x00;
	Load1bppTiles(SYM(gfxKonamiLogo2), &x, &y, 0x0d, 2);
	x = 0xd8;
	y = 0x00;
	Load1bppTiles(SYM(gfxKonami), &x, &y, 0x1a, 3);
	DrawTileList(SYM(KonamiLogoTiles), 0x40, 0x40);
}


/* One more line of the logo every two ticks; Room and Room+1 serve as
   counters here. */
void
DrawKonamiLogo()
{
	uint16_t counter = var::Room;
	ram(counter)--;
	if (ram(counter) & 1)
		return;
	if (--ram((uint16_t)(counter + 1)) == 0) {
		ram(var::Class) = 1;		/* logo finished */
		return;
	}
	uint8_t ny = (uint8_t)(49 - ram((uint16_t)(counter + 1)));
	VDP_Copy_Byte(0x28, 0x40, 1, 0x28, 0x40, 168, ny);
	SetBanks_1_2_X();
}


void
ClearP1_24x8()
{
	FillRect(0, 0, 0, 1, 0x18, 0x08);
	SetBanks_1_2_X();
}


void
LoadFont()
{
	ram(var::ACPAGE) = 1;
	ClearP1_24x8();
	uint8_t x = 0x00, y = 0x40;
	Load1bppTiles(SYM(gfxFont), &x, &y, 0x6c, 0x0e);
	SetBanks_1_2_X();
	x = 0x98;
	y = 0x58;
	Load1bppTiles(SYM(gfxFreqDigits), &x, &y, 0x0d, 0x08);
	SetBanks_1_2_X();
	x = 0x58;
	y = 0x40;
	Load1bppTiles(SYM(gfxSymbChars), &x, &y, 0x01, 0x06);
	SetBanks_1_2_X();
	for (int i = 0; i < 4; i++)
		ram((uint16_t)(var::BufferColor + i)) = rom((uint16_t)(SYM(colorsCALL) + i));
	uint16_t dest = 0xac30;
	Load2bppTile(SYM(gfxCALL), &dest, 6);
	SetBanks_1_2_X();
	ram(var::ACPAGE) = 0;
}


static void
DrawTiles(uint16_t data, uint8_t x, uint8_t y, int columns, int rows)
{
	for (int r = 0; r < rows; r++) {
		uint8_t cx = x;
		for (int c = 0; c < columns; c++) {
			DrawTile_P1(rom(data), cx, y);
			SetBanks_1_2_X();
			cx = (uint8_t)(cx + 8);
			data++;
		}
		y = (uint8_t)(y + 8);
	}
}


void
SetMetalGearLogo()
{
	for (int i = 0; i < 8; i++)
		ram((uint16_t)(var::BufferColor + i)) = rom((uint16_t)(SYM(MGLogoColors) + i));
	uint16_t dest = 0x8004;
	Load3bppTiles(SYM(gfxMetalGearLogo), &dest, 0x46);
	SetBanks_1_2_X();
	DrawTiles(SYM(MetalTilesDat), 0x20, 0x68, 0x0d, 4);	/* "METAL" */
	DrawTiles(SYM(GearTilesDat), 0x88, 0x70, 0x09, 4);	/* "GEAR" */
}


void
LoadIntroGfx()
{
	DisableScreen();
	SetBanks_1_2_X();
	ClearPage(1);
	SetBanks_1_2_X();
	ClearScreen();
	SetBanks_1_2_X();
	WRTVDP(7, 0);		/* background black */
	LoadFont();
	SetMetalGearLogo();
	ram(var::MenuCnt) = 0x60;
	ram(var::MenuStatus) = 0;
	ClearP1_24x8();
	SetPalette(SYM(MenuPalette));
	SetBanks_1_2_X();
	EnableScreen();
	SetBanks_1_2_X();
}


namespace {

void
NextMenuStatus()
{
	ram(var::MenuCnt) = 0x0c;
	ram(var::MenuStatus)++;
}


void
DrawMG_Logo()
{
	uint8_t index = (uint8_t)(ram(var::MenuCnt) - 1);
	uint16_t hl = (uint16_t)(SYM(MGLogoYpos) + index);
	SetBanks_1_2_X();
	uint8_t dy = rom(hl);
	VDP_Copy_Dot(0x20, 0x68, 0x48, 0x20, dy, 0x68, 0x20);	/* "METAL" */
	SetBanks_1_2_X();
	VDP_Copy_Dot(0x88, 0x68, 0x48, 0x88, (uint8_t)(dy + 8), 0x48, 0x20);	/* "GEAR" */
	SetBanks_1_2_X();
}


void
EraseLogoRests()
{
	ClearPage0();
	SetBanks_1_2_X();
	ram(var::MenuCnt) = 1;
	DrawMG_Logo();
	ram(var::MenuCnt) = 0;
	NextMenuStatus();
}


void
PrintPushSpace2()
{
	PrintTextXY(SYM(txtPushSpace));
	SetBanks_1_2_X();
}

}	// namespace


void
MenuLogoLogic()
{
	uint8_t status = ram(var::MenuStatus);
	SetBanks_1_2_X();		/* ADD_HL_A_ in JumpIndex2 */
	switch (status) {
		case 0:		/* LogoSfx */
			SetSoundEntry(0x47);
			SetBanks_1_2_X();
			NextMenuStatus();
			return;
		case 1:		/* LogoScroll */
			if (--ram(var::MenuCnt) == 0) {
				NextMenuStatus();
				return;
			}
			DrawMG_Logo();
			return;
		case 2:
			EraseLogoRests();
			return;
		case 3:		/* PrintPushSpace */
			if (--ram(var::MenuCnt) != 0)
				return;
			NextMenuStatus();
			PrintPushSpace2();
			return;
		case 4:		/* DrawMenuNow */
			EraseLogoRests();
			SetSoundEntry(0x4a);
			SetBanks_1_2_X();
			PrintPushSpace2();
			return;
		default:
			if (g_unported == NULL)
				g_unported = "MenuLogoLogic status above 4";
			return;
	}
}
