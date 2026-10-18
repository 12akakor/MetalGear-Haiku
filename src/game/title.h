/*
 * Konami logo, Metal Gear logo and menu (logic/konamilogo.asm,
 * logic/mainmenu.asm, logic/loadfont.asm).
 */
#ifndef MG_TITLE_H
#define MG_TITLE_H

void	InitKonamiLogo();
void	DrawKonamiLogo();
void	SetUpKonamiLogo();
void	LoadIntroGfx();
void	MenuLogoLogic();
void	SetMetalGearLogo();
void	LoadFont();
void	ClearP1_24x8();

#endif
