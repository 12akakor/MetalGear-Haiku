/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_SCREEN_RENDER_H
#define MG_SCREEN_RENDER_H


// Turns the game's screen (page shown, scroll, sprites, palette) into a
// picture of 256 x 212 pixels (0xRRGGBB), once per tick.

#include <stddef.h>
#include <stdint.h>


class Screen;

enum { kScreenWidth = 256, kScreenHeight = 212 };

void	RenderScreen(const Screen& screen, uint32_t* out);
// The same as colour numbers 0-15 (for the platform layer, which takes an
// indexed picture plus a palette). 'sprites', when given, gets 1 for every
// pixel drawn by a sprite.
void	RenderScreenIndexed(const Screen& screen, uint8_t* out, uint8_t* sprites = NULL);
// 0xRRGGBB of palette entry 'index'.
uint32_t ScreenColor(const Screen& screen, int index);


#endif	// MG_SCREEN_RENDER_H
