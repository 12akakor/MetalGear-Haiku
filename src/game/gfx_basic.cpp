/*
 * Basic graphics routines, rewritten from Banks0123.asm.
 */
#include "gfx_basic.h"

#include "rom.h"
#include "screen.h"


void
WRTVDP(int reg, uint8_t value)
{
	g_screen.SetRegister(reg, value);
}


/* SetPaletteColor: palette entry plus its copy in VRAM at #F680. */
void
SetPaletteColor(int color, uint8_t redBlue, uint8_t green)
{
	g_screen.SetPalette(color, redBlue, green);
	uint16_t a = (uint16_t)(0xf600 | (uint8_t)(0x80 + color * 2));
	g_screen.Write(a, redBlue);
	g_screen.Write((uint32_t)a + 1, green);
}


uint16_t
SetPalette(uint16_t data)
{
	for (;;) {
		uint8_t color = rom(data++);
		if (color == 0xff)
			return data;
		uint8_t rb = rom(data++);
		uint8_t g = rom(data++);
		SetPaletteColor(color, rb, g);
	}
}


/* FillRect: HMMV. nx or ny 0 means 256. */
void
FillRect(uint8_t color, uint8_t dx, uint8_t dy, uint8_t page, uint8_t nx, uint8_t ny)
{
	g_screen.FillBytes(dx, dy + (page & 3) * 256, nx == 0 ? 256 : nx, ny == 0 ? 256 : ny,
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
	g_screen.CopyBytes(sx, sy + (pages & 3) * 256, dx, dy + ((pages >> 2) & 3) * 256, nx, ny);
}


void
RAMtoVRAM(uint16_t source, uint32_t dest, int count)
{
	for (int i = 0; i < count; i++)
		g_screen.Write(dest + (uint32_t)i, rom((uint16_t)(source + i)));
}


void
SetColorsIndexes(uint16_t colors)
{
	for (int i = 0; i < 8; i++)
		ram((uint16_t)(var::BufferColor + i)) = rom((uint16_t)(colors + i));
}


void
SetTilesetColors()
{
	SetColorsIndexes(SYM(ColorsTileset));
}


/* Decode3bppTile: 8 lines of 3 bytes into BufferGfx (32 bytes). */
static uint16_t
decode_3bpp_tile(uint16_t source)
{
	uint16_t out = var::BufferGfx;
	for (int line = 0; line < 8; line++) {
		uint8_t e = rom(source++);
		uint8_t d = rom(source++);
		uint8_t c = rom(source++);
		for (int i = 0; i < 4; i++) {
			uint8_t pixel[2];
			for (int p = 0; p < 2; p++) {
				int index = ((c >> 7) << 2) | ((d >> 7) << 1) | (e >> 7);
				c = (uint8_t)(c << 1);
				d = (uint8_t)(d << 1);
				e = (uint8_t)(e << 1);
				pixel[p] = ram((uint16_t)(var::BufferColor + index));
			}
			ram(out++) = (uint8_t)((pixel[0] << 4) | pixel[1]);
		}
	}
	return source;
}


/* LoadTileGfx: one tile from RAM (linear, 32 bytes) to VRAM. */
void
LoadTileGfx(uint16_t source, uint16_t dest)
{
	for (int line = 0; line < 8; line++) {
		RAMtoVRAM(source, dest, 4);
		source = (uint16_t)(source + 4);
		dest = (uint16_t)(dest + 0x80);
	}
}


static void
next_tile_address(uint16_t* dest)
{
	uint8_t e = (uint8_t)(*dest + 4);
	uint8_t d = (uint8_t)(*dest >> 8);
	if (e == 0x80) {
		d = (uint8_t)(d + 4);
		e = 0;
	}
	*dest = (uint16_t)((d << 8) | e);
}


uint16_t
LoadTilesGfx(uint16_t source, uint16_t* dest, int count)
{
	do {
		LoadTileGfx(source, *dest);
		source = (uint16_t)(source + 32);
		next_tile_address(dest);
	} while (--count > 0);
	return source;
}


uint16_t
LoadTilesGfxFlip(uint16_t source, uint16_t* dest, int count)
{
	do {
		uint16_t buffer = (uint16_t)(var::DecodeTileBuf + 3);
		for (int line = 0; line < 8; line++) {
			for (int i = 0; i < 4; i++) {
				uint8_t a = rom(source++);
				ram(buffer--) = (uint8_t)((a >> 4) | (a << 4));
			}
			buffer = (uint16_t)(buffer + 8);
		}
		LoadTileGfx(var::DecodeTileBuf, *dest);
		next_tile_address(dest);
	} while (--count > 0);
	return source;
}


uint16_t
Load3bppTiles(uint16_t source, uint16_t* dest, int count)
{
	do {
		source = decode_3bpp_tile(source);
		LoadTilesGfx(var::BufferGfx, dest, 1);
	} while (--count > 0);
	return source;
}


uint16_t
Load3bppTileFlip(uint16_t source, uint16_t* dest, int count)
{
	do {
		source = decode_3bpp_tile(source);
		LoadTilesGfxFlip(var::BufferGfx, dest, 1);
	} while (--count > 0);
	return source;
}


uint16_t
TileToVramAdd(uint8_t tile)
{
	uint16_t hl = (uint16_t)((tile & 0xe0) * 32);
	hl = (uint16_t)(hl + (tile & 0x1f) * 4);
	return (uint16_t)(hl + 0x8000);
}


void
GetTileXY(uint8_t tile, uint8_t* x, uint8_t* y)
{
	*x = (uint8_t)((tile & 0x1f) * 8);
	*y = (uint8_t)((tile & 0xe0) >> 2);
}


/* DrawTile: tile from page 1 to page 0. */
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
	*x = (uint8_t)(*x + 8);
	if (*x == 0)
		*y = (uint8_t)(*y + 8);
}


uint8_t
GetNibbleHL_A2(uint16_t table, uint8_t position)
{
	uint8_t a = rom((uint16_t)(table + (position >> 1)));
	if ((position & 1) == 0)
		a = (uint8_t)(a >> 4);
	return (uint8_t)(a & 0x0f);
}


uint8_t
GetNibbleRoom(uint16_t table)
{
	return GetNibbleHL_A2(table, ram(var::Room));
}


uint16_t
GetPointerDE2A(uint16_t table, uint8_t index)
{
	return rom16((uint16_t)(table + index * 2));
}
