/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from Banks0123.asm (GetText, DecodeText, SetText ..
// GetTextBoxXYSize) and logic/textboxappear.asm.
#include "Text.h"

#include "Common.h"
#include "GameData.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "Rom.h"
#include "Screens.h"
#include "Structures.h"
#include "Unported.h"


using namespace var;


static bool
Japanese()
{
	return LoadedRomVersion() == kRomJapanese;
}


static void
NextTextStatus()
{
	Ram(TextWindowStatus)++;
}


static void
SetText2(uint8_t id, uint8_t mode)
{
	Ram(TextId) = id;
	Ram(SkipTextMode) = mode;
	// LDIR from PrevGameMode to TextWindowStatus: clears #1F bytes
	Ram(PrevGameMode) = 0;
	for (int i = 0; i < 0x1e; i++)
		Ram(static_cast<uint16_t>(TextWindowStatus + i))
			= Ram(static_cast<uint16_t>(PrevGameMode + i));
	Ram(PrevGameMode) = Ram(GameMode);
	Ram(GameMode) = GAME_MODE_TEXT_BOX;
}


void
SetText(uint8_t id)
{
	SetText2(id, 0);
}


void
SetTextUnskip2(uint8_t id)
{
	SetText2(id, 1);
}


void
SetTextUnskippable(uint8_t id)
{
	SetText2(id, 2);
}


uint8_t
GetText()
{
	SetBanks_A_B_C();
	Ram(SkipTextF) = 0;
	uint16_t text = GetPointerDE2A(SYM(idxTexts), static_cast<uint8_t>(Ram(TextId) - 1));
	SetRam16(TextPointer, text);
	uint8_t type = Mem(text);
	Ram(TextBoxType) = type;
	SetBanks1_2_3();
	return type;
}


// Codes: #FF end, #FE new line, #FD next page, #A1 and up a word from the
// dictionary.
uint8_t
DecodeText(uint16_t* source, uint16_t* dest)
{
	SetBanks_A_B_C();
	uint16_t de = TextBuffer;
	uint16_t hl = Ram16(TextPointer);
	uint8_t result;
	for (;;) {
		uint8_t c = Mem(hl);
		if (c == 0xff) {
			result = 0;
			break;
		}
		if (c == 0xfd) {
			result = 1;
			break;
		}
		if (c != 0xfe && c >= 0xa1) {
			if (Japanese() && c == 0xd9)
				Ram(flagTxtItem) = 1;
			hl++;
			uint16_t word = GetPointerDE2A(SYM(idxDictionary), static_cast<uint8_t>(c - 0xa1));
			for (;;) {
				uint8_t w = Mem(word);
				if (w == 0xff)
					break;
				Ram(de++) = w;
				word++;
			}
			continue;
		}
		Ram(de++) = c;
		hl++;
	}
	SetBanks1_2_3();
	*source = hl;
	*dest = de;
	return result;
}


static void
DrawTextBoxIn3(uint8_t color)
{
	DrawRect(Ram(TextBoxEff_DX), Ram(TextBoxEff_DY), Ram(TextBoxEff_NX),
		static_cast<uint8_t>(Ram(TextBoxEff_NY) + 1), color);
}


bool
DrawTextBoxIn()
{
	if (--Ram(TextBoxEff_Cnt) == 0)
		return true;
	DrawTextBoxIn3(0);	// erase the bottom line of the interior
	Ram(TextBoxEff_DY) = static_cast<uint8_t>(Ram(TextBoxEff_DY) - Ram(TextBoxStepY));
	Ram(TextBoxEff_NY) = static_cast<uint8_t>(Ram(TextBoxStepY) * 2 + Ram(TextBoxEff_NY));
	Ram(TextBoxEff_DX) = static_cast<uint8_t>(Ram(TextBoxEff_DX) - Ram(TextBoxStepX));
	Ram(TextBoxEff_NX) = static_cast<uint8_t>(Ram(TextBoxStepX) * 2 + Ram(TextBoxEff_NX));
	FillRect(0, static_cast<uint8_t>(Ram(TextBoxEff_DX) + 1),
		static_cast<uint8_t>(Ram(TextBoxEff_DY) + 1), 0,
		static_cast<uint8_t>(Ram(TextBoxEff_NX) - 2),
		static_cast<uint8_t>(Ram(TextBoxEff_NY) - 2));
	DrawTextBoxIn3(Ram(TextBoxEff_Col));
	return false;
}


static void
GetTextBoxXYSize2(uint16_t hl, uint16_t* de, uint16_t* bc)
{
	*de = Mem16(hl);
	*bc = Mem16(static_cast<uint16_t>(hl + 2));
}


void
GetTextBoxXYSize(uint16_t* xy, uint16_t* size)
{
	uint8_t type = Ram(TextBoxType) & 0x0f;
	GetTextBoxXYSize2(static_cast<uint16_t>(SYM(TextBoxXYSize) + type * 4), xy, size);
}


static void
TW_Init()
{
	uint8_t type = GetText() & 0x0f;
	uint16_t effect = static_cast<uint16_t>(SYM(TextBoxEffectDat) + static_cast<uint8_t>(type * 8));
	for (int i = 0; i < 8; i++)
		Ram(static_cast<uint16_t>(TextBoxEff_Cnt + i)) = Mem(static_cast<uint16_t>(effect + i));
	uint16_t hl = static_cast<uint16_t>(SYM(TextXYSize) + static_cast<uint8_t>(type * 6));
	uint16_t de, bc;
	GetTextBoxXYSize2(hl, &de, &bc);
	SetRam16(TextY, de);
	SetRam16(TextNY, bc);
	SetRam16(PromptXY, Mem16(static_cast<uint16_t>(hl + 4)));
	if (Ram(TextId) != 2 && Ram(PrevGameMode) != GAME_MODE_RADIO
		&& Ram(PlayerControlMod) != CONTROL_INTRO) {
		// save the background under the window to page 1
		GetTextBoxXYSize(&de, &bc);
		VDP_Copy_Byte(static_cast<uint8_t>(de >> 8), static_cast<uint8_t>(de), 4, 0, 0xa0,
			static_cast<uint8_t>(bc >> 8), static_cast<uint8_t>(bc));
	}
	NextTextStatus();
}


static void
TextBoxAppear()
{
	if (!DrawTextBoxIn())
		return;
	SetRam16(TextPointer, static_cast<uint16_t>(Ram16(TextPointer) + 1));
	Ram(TextWindowStatus) = 4;
	Ram(PendingTextFlag) = 1;
}


static void
DrawSnakeFrame2(uint16_t picture)
{
	if (Ram(PrevGameMode) != GAME_MODE_RADIO || Ram(EndingStatus) >= 8)
		return;
	Ram(TilesetBank) = 1;
	DrawTilesBlock(picture, 0xd0, 0x30);
}


// Snake's portrait in the radio moves its mouth while he talks.
void
DrawSnakeFrame()
{
	uint8_t a = Ram(TickCounter) & 0x1c;
	if (a == 0)
		DrawSnakeFrame2(SYM(SnakePicture1));
	else if ((a & 4) != 0)
		DrawSnakeFrame2(SYM(SnakePicture2));
	else
		DrawSnakeFrame1();
}


void
DrawSnakeFrame1()
{
	DrawSnakeFrame2(SYM(SnakePicture0));
}


static void
EraseWinText()
{
	uint16_t xy = Ram16(TextY);
	uint16_t size = Ram16(TextNY);
	FillRect(0, static_cast<uint8_t>(xy >> 8), static_cast<uint8_t>(xy), 0,
		static_cast<uint8_t>(size >> 8), static_cast<uint8_t>(size));
	NextTextStatus();
}


static void
SkipText()
{
	Ram(SkipTextF) = 1;
	DrawSnakeFrame1();
	NextTextStatus();
	EraseWinText();
}


static bool
SkipKeys()
{
	if ((Ram(ControlsTrigger) & 0x20) != 0 || (Ram(FKeysTrigger) & 0x20) != 0)
		return true;	// M/N or RETURN
	return Japanese() && Ram(flagTxtItem) != 0 && (Ram(ControlsHold) & 0x0f) != 0;
}


static void
TW_PrintChar()
{
	for (;;) {
		uint8_t mode = Ram(SkipTextMode);
		if (mode != 1 && mode != 2 && SkipKeys()) {
			SkipText();
			return;
		}
		if (!(Japanese() && Ram(flagTxtItem) != 0)) {
			uint8_t mask = Ram(TextId) == 45 ? 7 : 3;
			if ((Ram(TickCounter) & mask) != 0)
				return;
		}
		uint16_t hl = Ram16(pTexBuffer);
		uint8_t y = Ram(TextCharXY);
		uint8_t x = Ram(static_cast<uint16_t>(TextCharXY + 1));
		for (;;) {
			uint8_t c = Ram(hl);
			bool newLine = false;
			if (c == 0xff) {
				// TW_TextEnd
				DrawSnakeFrame1();
				Ram(WaitTextCnt) = 0x60;
				NextTextStatus();
				return;
			}
			if (c == 0xfe) {
				hl++;
				newLine = true;
			} else if (static_cast<uint8_t>(Ram(TextX) + Ram(TextNX) - 8) < x) {
				newLine = true;
			}
			if (newLine) {
				Ram(TextCharXY) = static_cast<uint8_t>(Ram(TextCharXY) + 12);
				y = Ram(TextCharXY);
				x = Ram(TextX);
				continue;
			}
			DrawChar(c, 1, x, y);
			uint8_t width = (c == 0x97 || c == 0x98) ? 4 : 8;
			if (c != 0 && Ram(EndingStatus) != 10)
				SetSoundEntry(0x23);
			x = static_cast<uint8_t>(x + width);
			hl++;
			SetRam16(pTexBuffer, hl);
			Ram(TextCharXY) = y;
			Ram(static_cast<uint16_t>(TextCharXY + 1)) = x;
			break;
		}
		if (Japanese() && Ram(flagTxtItem) != 0)
			continue;
		if (Ram(TextId) == 10 || Ram(TextId) == 155)
			DrawSnakeFrame();
		return;
	}
}


static void
TW_Wait()
{
	uint8_t mode = Ram(SkipTextMode);
	if (mode != 2) {
		if (SkipKeys()) {
			EraseWinText();
			return;
		}
		if (mode != 1 && Ram(TextId) != 10) {
			if (Ram(PendingTextFlag) == 0)
				return;
			if ((Ram(TextBoxType) & 0xf0) != 0) {
				// DrawEnterIcon (blinks)
				uint8_t c = (Ram(TickCounter) & 0x10) != 0 ? 1 : 0;
				DrawChar(0x3f, c, Ram(static_cast<uint16_t>(PromptXY + 1)), Ram(PromptXY));
				return;
			}
		}
	}
	if (--Ram(WaitTextCnt) != 0)
		return;
	EraseWinText();
}


static void
TW_GetTextPage()
{
	if (Japanese())
		Ram(flagTxtItem) = Ram(TextId) == 62 ? 1 : 0;
	if (Ram(PendingTextFlag) == 0) {
		NextTextStatus();
		return;
	}
	uint16_t hl, de;
	Ram(PendingTextFlag) = DecodeText(&hl, &de);
	SetRam16(TextPointer, static_cast<uint16_t>(hl + 1));
	Ram(TextWindowStatus) = 2;
	Ram(de) = 0xff;
	SetRam16(pTexBuffer, TextBuffer);
	SetRam16(TextCharXY, Ram16(TextY));
}


static void
TextBox_End()
{
	uint16_t xy, size;
	GetTextBoxXYSize(&xy, &size);
	uint8_t x = static_cast<uint8_t>(xy >> 8);
	uint8_t y = static_cast<uint8_t>(xy);
	uint8_t nx = static_cast<uint8_t>(size >> 8);
	uint8_t ny = static_cast<uint8_t>(size);
	if (Ram(TextId) == 2 || Ram(PrevGameMode) == GAME_MODE_RADIO || Ram(EndingStatus) != 0) {
		FillRect(0, x, y, 0, nx, ny);	// EraseTextBox
	} else {
		VDP_Copy_Byte(0, 0xa0, 1, x, y, nx, ny);	// restore the background
		DrawOpenPitfalls();
	}
	TextBoxExit();
}


void
TextBoxExit()
{
	Ram(GameMode) = Ram(PrevGameMode);
	if (Ram(SkipTextF) != 0)
		return;	// a skipped text does not count as read
	uint8_t id = Ram(TextId);
	if (id == 117)
		Ram(JeniRocketF) = 1;
	else if (id == 118)
		Ram(JeniOpenDoorF) = 1;
	else if (id == 138)
		Ram(SchneiderCaptured) = 1;
}


void
TextBoxLogic()
{
	if (Ram(PrevGameMode) == GAME_MODE_RADIO && (Ram(FKeysTrigger) & 8) != 0) {
		TextBoxExit();
		ExitRadio();
		return;
	}
	switch (Ram(TextWindowStatus)) {
		case 0:
			TW_Init();
			break;
		case 1:
			TextBoxAppear();
			break;
		case 2:
			TW_PrintChar();
			break;
		case 3:
			TW_Wait();
			break;
		case 4:
			TW_GetTextPage();
			break;
		case 5:
			TextBox_End();
			break;
		default:
			if (gUnported == NULL)
				gUnported = "TextBoxLogic: bad status";
			break;
	}
}
