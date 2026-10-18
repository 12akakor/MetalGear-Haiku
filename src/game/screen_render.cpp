#include "screen_render.h"

#include "screen.h"

#include <string.h>


uint32_t
screen_rgb(const Screen& screen, int index)
{
	int r = (screen.PaletteRB(index) >> 4) & 7;
	int b = screen.PaletteRB(index) & 7;
	int g = screen.PaletteG(index) & 7;
	return (uint32_t)(((r * 255 / 7) << 16) | ((g * 255 / 7) << 8) | (b * 255 / 7));
}


/* Sprite mode 2: 16x16 (or 8x8), a colour per line, up to 8 per line,
   colour mixing of linked sprites (CC bit). */
static void
render_sprites(const Screen& s, int line, uint8_t* colors, bool* set)
{
	if (s.Register(8) & 0x02)
		return;
	uint32_t colorTable = (uint32_t)(((s.Register(11) & 3) << 15) | ((s.Register(5) & 0xf8) << 7));
	uint32_t attrTable = colorTable + 0x200;
	uint32_t patternTable = (uint32_t)((s.Register(6) & 0x3f) << 11);
	bool big = (s.Register(1) & 0x02) != 0;
	bool mag = (s.Register(1) & 0x01) != 0;
	int size = (big ? 16 : 8) * (mag ? 2 : 1);
	int vline = (line + s.Register(23)) & 0xff;
	const uint8_t* vram = s.vram;

	int visible[8];
	int count = 0;
	for (int n = 0; n < 32; n++) {
		uint8_t y = vram[(attrTable + n * 4) & 0x1ffff];
		if (y == 216)
			break;
		int row = (vline - y - 1) & 0xff;
		if (row >= size)
			continue;
		if (count == 8)
			break;
		visible[count++] = n;
	}

	int8_t owner[256];
	memset(owner, -1, sizeof(owner));
	bool groupActive = false;
	bool tp = (s.Register(8) & 0x20) != 0;
	for (int k = 0; k < count; k++) {
		int n = visible[k];
		uint32_t at = attrTable + n * 4;
		uint8_t y = vram[at & 0x1ffff];
		int x = vram[(at + 1) & 0x1ffff];
		int pattern = vram[(at + 2) & 0x1ffff];
		int row = (vline - y - 1) & 0xff;
		if (mag)
			row >>= 1;
		uint8_t colorByte = vram[(colorTable + n * 16 + row) & 0x1ffff];
		bool cc = (colorByte & 0x40) != 0;
		if (colorByte & 0x80)
			x -= 32;
		uint8_t color = colorByte & 0x0f;
		if (big)
			pattern &= 0xfc;
		uint32_t pa = patternTable + pattern * 8 + row;
		uint16_t bits = (uint16_t)(vram[pa & 0x1ffff] << 8);
		if (big)
			bits |= vram[(pa + 16) & 0x1ffff];
		int width = big ? 16 : 8;
		if (!cc)
			groupActive = true;
		else if (!groupActive)
			continue;
		for (int i = 0; i < width; i++) {
			if (!(bits & (0x8000 >> i)))
				continue;
			for (int m = 0; m < (mag ? 2 : 1); m++) {
				int px = x + i * (mag ? 2 : 1) + m;
				if (px < 0 || px > 255)
					continue;
				if (cc) {
					if (owner[px] >= 0 && set[px])
						colors[px] |= color;
					else if (!set[px] && (color != 0 || tp)) {
						colors[px] = color;
						set[px] = true;
						owner[px] = (int8_t)n;
					}
				} else if (!set[px]) {
					if (color == 0 && !tp)
						continue;
					colors[px] = color;
					set[px] = true;
					owner[px] = (int8_t)n;
				}
			}
		}
	}
}


void
screen_render(const Screen& s, uint32_t* out)
{
	static uint8_t indexed[kScreenWidth * kScreenHeight];
	screen_render_indexed(s, indexed);
	uint32_t palette[16];
	for (int i = 0; i < 16; i++)
		palette[i] = screen_rgb(s, i);
	for (int i = 0; i < kScreenWidth * kScreenHeight; i++)
		out[i] = palette[indexed[i] & 15];
}


void
screen_render_indexed(const Screen& s, uint8_t* out)
{
	uint8_t backdrop = (uint8_t)(s.Register(7) & 0x0f);
	int lines = (s.Register(9) & 0x80) ? 212 : 192;
	bool enabled = (s.Register(1) & 0x40) != 0;
	uint32_t page = (uint32_t)((s.Register(2) & 0x60) << 10);
	bool tp = (s.Register(8) & 0x20) != 0;
	for (int line = 0; line < kScreenHeight; line++) {
		uint8_t* row = out + line * kScreenWidth;
		if (line >= lines || !enabled) {
			for (int x = 0; x < kScreenWidth; x++)
				row[x] = backdrop;
			continue;
		}
		int vline = (line + s.Register(23)) & 0xff;
		const uint8_t* src = s.vram + page + vline * 128;
		uint8_t spriteColors[256];
		bool spriteSet[256];
		memset(spriteSet, 0, sizeof(spriteSet));
		render_sprites(s, line, spriteColors, spriteSet);
		for (int x = 0; x < kScreenWidth; x++) {
			uint8_t c;
			if (spriteSet[x])
				c = spriteColors[x];
			else {
				uint8_t b = src[x >> 1];
				c = (x & 1) ? (b & 0x0f) : (uint8_t)(b >> 4);
			}
			row[x] = (c == 0 && !tp) ? backdrop : c;
		}
	}
}
