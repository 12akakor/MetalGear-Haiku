/*
 * Turns the game's screen (page shown, scroll, sprites, palette) into a
 * picture of 256 x 212 pixels (0xRRGGBB), once per tick.
 */
#ifndef MG_SCREEN_RENDER_H
#define MG_SCREEN_RENDER_H

#include <stdint.h>

class Screen;

enum { kScreenWidth = 256, kScreenHeight = 212 };

void	screen_render(const Screen& screen, uint32_t* out);
/* 0xRRGGBB of palette entry 'index'. */
uint32_t screen_rgb(const Screen& screen, int index);

#endif
