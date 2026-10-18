#include "screen.h"

#include "game_ram.h"

#include <string.h>

Screen g_screen;


namespace {

const uint16_t kRG0SAV = 0xf3df;
const uint16_t kRG8SAV = 0xffe7;

inline uint32_t
address(int x, int y)
{
	return (uint32_t)((((y & 1023) << 7) | ((x & 255) >> 1)) & 0x1ffff);
}

inline uint8_t
logic(uint8_t dst, uint8_t src, int op)
{
	if ((op & 8) && src == 0)
		return dst;
	switch (op & 7) {
		case 0: return src;
		case 1: return (uint8_t)(dst & src);
		case 2: return (uint8_t)(dst | src);
		case 3: return (uint8_t)(dst ^ src);
		case 4: return (uint8_t)(~src & 0x0f);
		default: return dst;
	}
}

inline int
clip_width(int x, int nx)
{
	int edge = 256 - (x & 255);
	return nx < edge ? nx : edge;
}

}	// namespace


Screen::Screen()
{
	Reset();
}


void
Screen::Reset()
{
	memset(vram, 0, sizeof(vram));
	memset(palette, 0, sizeof(palette));
	memset(reg_, 0, sizeof(reg_));
}


uint8_t
Screen::Point(int x, int y) const
{
	uint8_t b = vram[address(x, y)];
	return (x & 1) ? (uint8_t)(b & 0x0f) : (uint8_t)(b >> 4);
}


void
Screen::Pset(int x, int y, uint8_t color, int op)
{
	uint8_t& b = vram[address(x, y)];
	if (x & 1)
		b = (uint8_t)((b & 0xf0) | logic(b & 0x0f, color & 0x0f, op));
	else
		b = (uint8_t)((b & 0x0f) | (logic((uint8_t)(b >> 4), color & 0x0f, op) << 4));
}


void
Screen::FillBytes(int dx, int dy, int nx, int ny, uint8_t value)
{
	if (nx == 0)
		nx = 512;
	if (ny == 0)
		ny = 1024;
	int width = clip_width(dx, nx);
	for (int line = 0; line < ny; line++) {
		int y = (dy + line) & 1023;
		for (int i = 0, x = dx & ~1; i < width; i += 2, x += 2)
			vram[address(x, y)] = value;
	}
}


void
Screen::CopyBytes(int sx, int sy, int dx, int dy, int nx, int ny)
{
	if (onCopyBytes)
		onCopyBytes(sx, sy, dx, dy, nx, ny);
	if (nx == 0)
		nx = 512;
	if (ny == 0)
		ny = 1024;
	int width = clip_width(dx, nx);
	int sw = clip_width(sx, nx);
	if (sw < width)
		width = sw;
	for (int line = 0; line < ny; line++) {
		int ys = (sy + line) & 1023, yd = (dy + line) & 1023;
		for (int i = 0, xs = sx & ~1, xd = dx & ~1; i < width; i += 2, xs += 2, xd += 2)
			vram[address(xd, yd)] = vram[address(xs, ys)];
	}
}


void
Screen::CopyPixels(int sx, int sy, int dx, int dy, int nx, int ny, int op)
{
	if (nx == 0)
		nx = 512;
	if (ny == 0)
		ny = 1024;
	int width = clip_width(dx, nx);
	int sw = clip_width(sx, nx);
	if (sw < width)
		width = sw;
	for (int line = 0; line < ny; line++) {
		int ys = (sy + line) & 1023, yd = (dy + line) & 1023;
		for (int i = 0; i < width; i++)
			Pset(dx + i, yd, Point(sx + i, ys), op);
	}
}


void
Screen::FillPixels(int dx, int dy, int nx, int ny, uint8_t color, int op)
{
	if (nx == 0)
		nx = 512;
	if (ny == 0)
		ny = 1024;
	int width = clip_width(dx, nx);
	for (int line = 0; line < ny; line++)
		for (int i = 0; i < width; i++)
			Pset(dx + i, (dy + line) & 1023, color, op);
}


void
Screen::Line(int x, int y, int length, bool vertical, uint8_t color)
{
	for (int i = 0; i < length; i++) {
		Pset(x, y, color, 0);
		if (vertical)
			y = (y + 1) & 1023;
		else if (++x > 255)
			break;
	}
}


void
Screen::SetRegister(int reg, uint8_t value)
{
	reg &= 63;
	reg_[reg] = value;
	if (reg < 8)
		ram((uint16_t)(kRG0SAV + reg)) = value;
	else if (reg < 24)
		ram((uint16_t)(kRG8SAV + reg - 8)) = value;
}


void
Screen::SetPalette(int index, uint8_t redBlue, uint8_t green)
{
	palette[index & 15][0] = (uint8_t)(redBlue & 0x77);
	palette[index & 15][1] = (uint8_t)(green & 0x07);
}
