/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Screen.h"

#include <string.h>

#include "GameRam.h"


Screen gScreen;


namespace {

const uint16_t kRG0SAV = 0xf3df;
const uint16_t kRG8SAV = 0xffe7;

inline uint32_t
Address(int x, int y)
{
	return static_cast<uint32_t>((((y & 1023) << 7) | ((x & 255) >> 1)) & 0x1ffff);
}

inline uint8_t
Logic(uint8_t dst, uint8_t src, int op)
{
	if (((op & 8) != 0) && src == 0)
		return dst;
	switch (op & 7) {
		case 0: return src;
		case 1: return static_cast<uint8_t>(dst & src);
		case 2: return static_cast<uint8_t>(dst | src);
		case 3: return static_cast<uint8_t>(dst ^ src);
		case 4: return static_cast<uint8_t>(~src & 0x0f);
		default: return dst;
	}
}

inline int
ClipWidth(int x, int nx)
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
	memset(fVram, 0, sizeof(fVram));
	// MSX2 power-on fPalette (R, G, B, 0..7), as the BIOS sets it.
	static const uint8_t kDefault[16][3] = {
		{0,0,0}, {0,0,0}, {1,6,1}, {3,7,3}, {1,1,7}, {2,3,7}, {5,1,1}, {2,6,7},
		{7,1,1}, {7,3,3}, {6,6,1}, {6,6,4}, {1,4,1}, {6,2,5}, {5,5,5}, {7,7,7}
	};
	for (int i = 0; i < 16; i++)
		SetPalette(i, static_cast<uint8_t>((kDefault[i][0] << 4) | kDefault[i][2]), kDefault[i][1]);
	memset(fRegister, 0, sizeof(fRegister));
}


uint8_t
Screen::Point(int x, int y) const
{
	uint8_t b = fVram[Address(x, y)];
	return ((x & 1) != 0) ? static_cast<uint8_t>(b & 0x0f) : static_cast<uint8_t>(b >> 4);
}


void
Screen::Pset(int x, int y, uint8_t color, int op)
{
	uint8_t& b = fVram[Address(x, y)];
	if ((x & 1) != 0)
		b = static_cast<uint8_t>((b & 0xf0) | Logic(b & 0x0f, color & 0x0f, op));
	else
		b = static_cast<uint8_t>((b & 0x0f)
			| (Logic(static_cast<uint8_t>(b >> 4), color & 0x0f, op) << 4));
}


void
Screen::FillBytes(int dx, int dy, int nx, int ny, uint8_t value)
{
	if (nx == 0)
		nx = 512;
	if (ny == 0)
		ny = 1024;
	int width = ClipWidth(dx, nx);
	for (int line = 0; line < ny; line++) {
		int y = (dy + line) & 1023;
		for (int i = 0, x = dx & ~1; i < width; i += 2, x += 2)
			fVram[Address(x, y)] = value;
	}
}


void
Screen::CopyBytes(int sx, int sy, int dx, int dy, int nx, int ny)
{
	if (fOnCopyBytes)
		fOnCopyBytes(sx, sy, dx, dy, nx, ny);
	if (nx == 0)
		nx = 512;
	if (ny == 0)
		ny = 1024;
	int width = ClipWidth(dx, nx);
	int sw = ClipWidth(sx, nx);
	if (sw < width)
		width = sw;
	for (int line = 0; line < ny; line++) {
		int ys = (sy + line) & 1023, yd = (dy + line) & 1023;
		for (int i = 0, xs = sx & ~1, xd = dx & ~1; i < width; i += 2, xs += 2, xd += 2)
			fVram[Address(xd, yd)] = fVram[Address(xs, ys)];
	}
}


void
Screen::CopyPixels(int sx, int sy, int dx, int dy, int nx, int ny, int op)
{
	if (nx == 0)
		nx = 512;
	if (ny == 0)
		ny = 1024;
	int width = ClipWidth(dx, nx);
	int sw = ClipWidth(sx, nx);
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
	int width = ClipWidth(dx, nx);
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
	fRegister[reg] = value;
	if (reg < 8)
		Ram(static_cast<uint16_t>(kRG0SAV + reg)) = value;
	else if (reg < 24)
		Ram(static_cast<uint16_t>(kRG8SAV + reg - 8)) = value;
}


void
Screen::SetPalette(int index, uint8_t redBlue, uint8_t green)
{
	fPalette[index & 15][0] = static_cast<uint8_t>(redBlue & 0x77);
	fPalette[index & 15][1] = static_cast<uint8_t>(green & 0x07);
}
