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
/* The same as colour numbers 0-15 (for the platform layer, which takes an
   indexed picture plus a palette). */
void	screen_render_indexed(const Screen& screen, uint8_t* out);
/* 0xRRGGBB of palette entry 'index'. */
uint32_t screen_rgb(const Screen& screen, int index);

#endif
