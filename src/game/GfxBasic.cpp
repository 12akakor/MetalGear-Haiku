/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Basic graphics routines, rewritten from Banks0123.asm.
#include "GfxBasic.h"

#include "Rom.h"
#include "Screen.h"


void
WRTVDP(int reg, uint8_t value)
{
	gScreen.SetRegister(reg, value);
}


// SetPaletteColor: palette entry plus its copy in VRAM at #F680.
void
SetPaletteColor(int color, uint8_t redBlue, uint8_t green)
{
	gScreen.SetPalette(color, redBlue, green);
	uint16_t a = static_cast<uint16_t>(0xf600 | static_cast<uint8_t>(0x80 + color * 2));
	gScreen.Write(a, redBlue);
	gScreen.Write(static_cast<uint32_t>(a) + 1, green);
}


uint16_t
SetPalette(uint16_t data)
{
	for (;;) {
		uint8_t color = Rom(data++);
		if (color == 0xff)
			return data;
		uint8_t rb = Rom(data++);
		uint8_t g = Rom(data++);
		SetPaletteColor(color, rb, g);
	}
}


// FillRect: HMMV. nx or ny 0 means 256.
void
FillRect(uint8_t color, uint8_t dx, uint8_t dy, uint8_t page, uint8_t nx, uint8_t ny)
{
	gScreen.FillBytes(dx, dy + (page & 3) * 256, nx == 0 ? 256 : nx, ny == 0 ? 256 : ny,
		color);
}


void
ClearPage(uint8_t page)
{
	FillRect(0, 0, 0, page, 0, 0);
	WRTVDP(23, 0);
}


void
ClearPage0()
{
	ClearPage(0);
}


void
VDP_Copy_Byte(uint8_t sx, uint8_t sy, uint8_t pages, uint8_t dx, uint8_t dy, uint8_t nx,
	uint8_t ny)
{
	gScreen.CopyBytes(sx, sy + (pages & 3) * 256, dx, dy + ((pages >> 2) & 3) * 256, nx, ny);
}


void
RAMtoVRAM(uint16_t source, uint32_t dest, int count)
{
	for (int i = 0; i < count; i++)
		gScreen.Write(dest + static_cast<uint32_t>(i), Rom(static_cast<uint16_t>(source + i)));
}


void
SetColorsIndexes(uint16_t colors)
{
	for (int i = 0; i < 8; i++)
		Ram(static_cast<uint16_t>(var::BufferColor + i)) = Rom(static_cast<uint16_t>(colors + i));
}


void
SetColorsIndexes2(uint16_t colors, uint8_t count)
{
	for (int i = 0; i < count; i++)
		Ram(static_cast<uint16_t>(var::BufferColor + i)) = Rom(static_cast<uint16_t>(colors + i));
}


void
SetTilesetColors()
{
	SetColorsIndexes(SYM(ColorsTileset));
}


// Decode3bppTile: 8 lines of 3 bytes into BufferGfx (32 bytes).
static uint16_t
Decode3bppTile(uint16_t source)
{
	uint16_t out = var::BufferGfx;
	for (int line = 0; line < 8; line++) {
		uint8_t e = Rom(source++);
		uint8_t d = Rom(source++);
		uint8_t c = Rom(source++);
		for (int i = 0; i < 4; i++) {
			uint8_t pixel[2];
			for (int p = 0; p < 2; p++) {
				int index = ((c >> 7) << 2) | ((d >> 7) << 1) | (e >> 7);
				c = static_cast<uint8_t>(c << 1);
				d = static_cast<uint8_t>(d << 1);
				e = static_cast<uint8_t>(e << 1);
				pixel[p] = Ram(static_cast<uint16_t>(var::BufferColor + index));
			}
			Ram(out++) = static_cast<uint8_t>((pixel[0] << 4) | pixel[1]);
		}
	}
	return source;
}


// LoadTileGfx: one tile from RAM (linear, 32 bytes) to VRAM.
void
LoadTileGfx(uint16_t source, uint16_t dest)
{
	for (int line = 0; line < 8; line++) {
		RAMtoVRAM(source, dest, 4);
		source = static_cast<uint16_t>(source + 4);
		dest = static_cast<uint16_t>(dest + 0x80);
	}
}


static void
NextTileAddress(uint16_t* dest)
{
	uint8_t e = static_cast<uint8_t>(*dest + 4);
	uint8_t d = static_cast<uint8_t>(*dest >> 8);
	if (e == 0x80) {
		d = static_cast<uint8_t>(d + 4);
		e = 0;
	}
	*dest = static_cast<uint16_t>((d << 8) | e);
}


uint16_t
LoadTilesGfx(uint16_t source, uint16_t* dest, int count)
{
	do {
		LoadTileGfx(source, *dest);
		source = static_cast<uint16_t>(source + 32);
		NextTileAddress(dest);
	} while (--count > 0);
	return source;
}


uint16_t
LoadTilesGfxFlip(uint16_t source, uint16_t* dest, int count)
{
	do {
		uint16_t buffer = static_cast<uint16_t>(var::DecodeTileBuf + 3);
		for (int line = 0; line < 8; line++) {
			for (int i = 0; i < 4; i++) {
				uint8_t a = Rom(source++);
				Ram(buffer--) = static_cast<uint8_t>((a >> 4) | (a << 4));
			}
			buffer = static_cast<uint16_t>(buffer + 8);
		}
		LoadTileGfx(var::DecodeTileBuf, *dest);
		NextTileAddress(dest);
	} while (--count > 0);
	return source;
}


uint16_t
Load3bppTiles(uint16_t source, uint16_t* dest, int count)
{
	do {
		source = Decode3bppTile(source);
		LoadTilesGfx(var::BufferGfx, dest, 1);
	} while (--count > 0);
	return source;
}


uint16_t
Load3bppTileFlip(uint16_t source, uint16_t* dest, int count)
{
	do {
		source = Decode3bppTile(source);
		LoadTilesGfxFlip(var::BufferGfx, dest, 1);
	} while (--count > 0);
	return source;
}


uint16_t
TileToVramAdd(uint8_t tile)
{
	uint16_t hl = static_cast<uint16_t>((tile & 0xe0) * 32);
	hl = static_cast<uint16_t>(hl + (tile & 0x1f) * 4);
	return static_cast<uint16_t>(hl + 0x8000);
}


void
GetTileXY(uint8_t tile, uint8_t* x, uint8_t* y)
{
	*x = static_cast<uint8_t>((tile & 0x1f) * 8);
	*y = static_cast<uint8_t>((tile & 0xe0) >> 2);
}


// DrawTile: tile from page 1 to page 0.
void
DrawTile(uint8_t tile, uint8_t dx, uint8_t dy)
{
	uint8_t x, y;
	GetTileXY(tile, &x, &y);
	VDP_Copy_Byte(x, y, 1, dx, dy, 8, 8);
}


void
CalcNextCharPos(uint8_t* x, uint8_t* y)
{
	*x = static_cast<uint8_t>(*x + 8);
	if (*x == 0)
		*y = static_cast<uint8_t>(*y + 8);
}


uint8_t
GetNibbleHL_A2(uint16_t table, uint8_t position)
{
	uint8_t a = Rom(static_cast<uint16_t>(table + (position >> 1)));
	if ((position & 1) == 0)
		a = static_cast<uint8_t>(a >> 4);
	return static_cast<uint8_t>(a & 0x0f);
}


uint8_t
GetNibbleRoom(uint16_t table)
{
	return GetNibbleHL_A2(table, Ram(var::Room));
}


uint16_t
GetPointerDE2A(uint16_t table, uint8_t index)
{
	return Rom16(static_cast<uint16_t>(table + index * 2));
}


// VDP_Copy_Dot: LMMM; pagesOp = OODDLLLL (source page, destination page,
// logical operation).
void
VDP_Copy_Dot(uint8_t sx, uint8_t sy, uint8_t pagesOp, uint8_t dx, uint8_t dy, uint8_t nx,
	uint8_t ny)
{
	int sourcePage = (pagesOp >> 6) & 3;
	int destPage = (pagesOp >> 4) & 3;
	gScreen.CopyPixels(sx, sy + sourcePage * 256, dx, dy + destPage * 256, nx, ny,
		pagesOp & 0x0f);
}


// DrawTile_P1: tile within page 1.
void
DrawTile_P1(uint8_t tile, uint8_t dx, uint8_t dy)
{
	uint8_t x, y;
	GetTileXY(tile, &x, &y);
	VDP_Copy_Byte(x, y, 5, dx, dy, 8, 8);
}


void
DrawTileList(uint16_t data, uint8_t x, uint8_t y)
{
	uint8_t rowX = x, rowY = y;
	for (;;) {
		uint8_t c = Rom(data++);
		if (c == 0xff)
			break;
		if (c == 0xfe) {
			rowX = static_cast<uint8_t>(rowX + Rom(data++));
			rowY = static_cast<uint8_t>(rowY + 8);
			x = rowX;
			y = rowY;
			continue;
		}
		DrawTile_P1(c, x, y);
		CalcNextCharPos(&x, &y);
	}
	SetBanks(1);	// DrawTileList4
}


// DrawChar: font in page 1 from y #30 (tile = char + #10); char 0 or
// colour 0 copies the blank tile at (0,0) of page 1.
void
DrawChar(uint8_t c, uint8_t color, uint8_t x, uint8_t y)
{
	uint8_t sx = 0, sy = 0;
	if (c != 0 && color != 0) {
		GetTileXY(static_cast<uint8_t>(c + 0x10), &sx, &sy);
		sy = static_cast<uint8_t>(sy + 0x30);
	}
	VDP_Copy_Byte(sx, sy, 1, x, y, 8, 8);
}


// PrintText: text from 'text' at x, y (#FE x y: new position, #FF: end);
// c = 0 erases. Returns the position after the text.
void
PrintText(uint16_t text, uint8_t x, uint8_t y, uint8_t c)
{
	for (;;) {
		uint8_t ch = Rom(text++);
		if (ch == 0xff)
			return;
		if (ch == 0xfe) {
			x = Rom(text++);
			y = Rom(text++);
			continue;
		}
		DrawChar(ch, c, x, y);
		x = static_cast<uint8_t>(x + ((ch == 0x97 || ch == 0x98) ? 4 : 8));
	}
}


static void
PrintText(uint16_t text, uint8_t color)
{
	uint8_t x = Rom(text++);
	uint8_t y = Rom(text++);
	PrintText(text, x, y, color);
}


void
PrintTextXY(uint16_t text)
{
	PrintText(text, 0xff);
}


void
EraseTextXY(uint16_t text)
{
	PrintText(text, 0);
}


// Decode1bppTile: 8 bytes, set bits in 'color', into DecodeTileBuf.
void
Decode1bppTile(uint16_t source, uint8_t color)
{
	uint16_t out = var::DecodeTileBuf;
	for (int line = 0; line < 8; line++) {
		uint8_t bits = Rom(source++);
		for (int i = 0; i < 4; i++) {
			uint8_t hi = ((bits & 0x80) != 0) ? color : 0;
			uint8_t lo = ((bits & 0x40) != 0) ? color : 0;
			bits = static_cast<uint8_t>(bits << 2);
			Ram(out++) = static_cast<uint8_t>(((hi & 0x0f) << 4) | (lo & 0x0f));
		}
	}
}


void
Load1bppTile(uint16_t source, uint8_t x, uint8_t y, uint8_t color)
{
	Decode1bppTile(source, color);
	uint16_t dest = static_cast<uint16_t>((((y << 8) | x) >> 1) + 0x8000);
	LoadTileGfx(var::DecodeTileBuf, dest);
}


uint16_t
Load1bppTiles(uint16_t source, uint8_t* x, uint8_t* y, int count, uint8_t color)
{
	do {
		Load1bppTile(source, *x, *y, color);
		source = static_cast<uint16_t>(source + 8);
		CalcNextCharPos(x, y);
	} while (--count > 0);
	return source;
}


static uint16_t
Decode2bppTile(uint16_t source)
{
	uint16_t out = var::BufferGfx;
	for (int line = 0; line < 8; line++) {
		uint8_t e = Rom(source++);
		uint8_t d = Rom(source++);
		for (int i = 0; i < 4; i++) {
			uint8_t pixel[2];
			for (int p = 0; p < 2; p++) {
				int index = ((d >> 7) << 1) | (e >> 7);
				d = static_cast<uint8_t>(d << 1);
				e = static_cast<uint8_t>(e << 1);
				pixel[p] = Ram(static_cast<uint16_t>(var::BufferColor + index));
			}
			Ram(out++) = static_cast<uint8_t>((pixel[0] << 4) | pixel[1]);
		}
	}
	return source;
}


uint16_t
Load2bppTile(uint16_t source, uint16_t* dest, int count)
{
	do {
		source = Decode2bppTile(source);
		LoadTilesGfx(var::BufferGfx, dest, 1);
	} while (--count > 0);
	return source;
}


// FillVRAM: 'count' bytes (0 means 65536).
void
FillVRAM(uint32_t address, int count, uint8_t value)
{
	if (count == 0)
		count = 0x10000;
	for (int i = 0; i < count; i++)
		gScreen.Write(address + static_cast<uint32_t>(i), value);
}


// Both sprite attribute buffers: Y = #E0 hides the sprite.
void
ClearSprAttr()
{
	FillVRAM(0xf600, 0x80, 0xe0);
	FillVRAM(0xf200, 0x80, 0xe0);
}


void
EnableSprites()
{
	WRTVDP(8, static_cast<uint8_t>(Ram(var::RG08SAV) & 0xfd));
}


void
DisableSprites()
{
	WRTVDP(8, static_cast<uint8_t>(Ram(var::RG08SAV) | 2));
}


void
EnableScreen()
{
	WRTVDP(1, static_cast<uint8_t>(Ram(var::RG1SAV) | 0x40));
	EnableSprites();
}


void
DisableScreen()
{
	WRTVDP(1, static_cast<uint8_t>(Ram(var::RG1SAV) & 0xbf));
	DisableSprites();
}


void
ClearScreen()
{
	ClearSprAttr();
	DisableScreen();
	ClearPage0();
	EnableScreen();
}


void
SetDefaultPal()
{
	uint16_t p = SYM(DefaultPalette);
	for (int i = 0; i < 16; i++) {
		uint8_t rb = Rom(p++);
		uint8_t g = Rom(p++);
		SetPaletteColor(i, rb, g);
	}
}


void
SetBanks_1_2_X()
{
	SetBanks(1);
	uint8_t fixed = Ram(var::BankInA0Fixed);
	if (fixed != 0)
		Ram(var::BankInA0) = fixed;
}


void
SetBankInA0(uint8_t bank)
{
	Ram(var::BankInA0) = bank;
	Ram(var::BankInA0Fixed) = bank;
}
