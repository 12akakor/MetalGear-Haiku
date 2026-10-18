/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_TITLE_H
#define MG_TITLE_H


// Konami logo, Metal Gear logo and menu (logic/konamilogo.asm,
// logic/mainmenu.asm, logic/loadfont.asm).

void	InitKonamiLogo();
void	DrawKonamiLogo();
void	SetUpKonamiLogo();
void	LoadIntroGfx();
void	MenuLogoLogic();
void	SetMetalGearLogo();
void	LoadFont();
void	ClearP1_24x8();


#endif	// MG_TITLE_H
