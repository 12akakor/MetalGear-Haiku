/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_SCREENS_H
#define MG_SCREENS_H


// The transceiver (radio), the weapon screen, back to the room
// (RenderScreen), palette fading (Banks0123.asm).

#include <stdint.h>


void		RadioLogic();
void		DrawRadioFreq();
void		DrawRadioLeds();
void		ExitRadio();
// True when nobody answers.
bool		ChkRadioReply(uint16_t person);

void		MenuWeapon();
void		RenderScreen();
void		GetWeaponCursor();
// Menu cursor index of the selected entry (or of the first empty one).
void		GetMenuCursor(uint16_t selected, uint8_t count, uint16_t table);
void		EraseArrow();
void		DrawArrow();
void		SelectWeapon2(uint8_t weapon);

void		CopyPalToRAM();
// True when all colours are black.
bool		FadeOutLogic();
void		FadeOutColors();

// LDIR: count bytes (0 = 65536) from source to dest, one by one.
void		Ldir(uint16_t source, uint16_t dest, uint16_t count);


#endif	// MG_SCREENS_H
