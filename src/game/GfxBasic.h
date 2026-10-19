/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_GFX_BASIC_H
#define MG_GFX_BASIC_H


// Basic graphics routines of the original (Banks0123.asm): palette, tile
// decoding and upload, rectangles, tile drawing. Names follow the
// disassembly.

#include <stdint.h>


// BIOS WRTVDP
void		WRTVDP(int reg, uint8_t value);

void		SetPaletteColor(int color, uint8_t redBlue, uint8_t green);
// Palette data: (colour, red|blue, green) ... #FF. Returns the pointer
// after the end marker.
uint16_t	SetPalette(uint16_t data);

void		FillRect(uint8_t color, uint8_t dx, uint8_t dy, uint8_t page, uint8_t nx,
				uint8_t ny);
void		ClearPage(uint8_t page);
void		ClearPage0();
// HMMM; pages: bits 0-1 source page, bits 2-3 destination page.
void		VDP_Copy_Byte(uint8_t sx, uint8_t sy, uint8_t pages, uint8_t dx, uint8_t dy,
				uint8_t nx, uint8_t ny);

// Writes 'count' bytes from RAM/ROM at 'source' to VRAM 'dest'.
void		RAMtoVRAM(uint16_t source, uint32_t dest, int count);

// Tiles (8x8 pixels, 32 bytes)
void		SetTilesetColors();
void		SetColorsIndexes(uint16_t colors);
// Decode 3bpp tiles from 'source' to VRAM 'dest'; returns the source
// pointer after the data; 'dest' is advanced as the original does.
uint16_t	Load3bppTiles(uint16_t source, uint16_t* dest, int count);
uint16_t	Load3bppTileFlip(uint16_t source, uint16_t* dest, int count);
void		LoadTileGfx(uint16_t source, uint16_t dest);
uint16_t	LoadTilesGfx(uint16_t source, uint16_t* dest, int count);
uint16_t	LoadTilesGfxFlip(uint16_t source, uint16_t* dest, int count);

// Tile number to its position in VRAM page 1.
uint16_t	TileToVramAdd(uint8_t tile);
void		GetTileXY(uint8_t tile, uint8_t* x, uint8_t* y);
void		DrawTile(uint8_t tile, uint8_t dx, uint8_t dy);
void		CalcNextCharPos(uint8_t* x, uint8_t* y);

// Helpers
uint8_t		GetNibbleHL_A2(uint16_t table, uint8_t position);
uint8_t		GetNibbleRoom(uint16_t table);
uint16_t	GetPointerDE2A(uint16_t table, uint8_t index);

#endif

// More drawing routines (title, text, sprites)
void		VDP_Copy_Dot(uint8_t sx, uint8_t sy, uint8_t pagesOp, uint8_t dx, uint8_t dy,
				uint8_t nx, uint8_t ny);
void		DrawTile_P1(uint8_t tile, uint8_t dx, uint8_t dy);
// Tile list: tiles, #FE offset = next row (x += offset, y += 8), #FF end.
void		DrawTileList(uint16_t data, uint8_t x, uint8_t y);
void		DrawChar(uint8_t c, uint8_t color, uint8_t x, uint8_t y);
// Text: XY, characters, #FE XY, ... #FF. 'erase' draws blanks.
void		PrintTextXY(uint16_t text);
// Text at x, y; c = 0 erases (as the original's register C).
void		PrintText(uint16_t text, uint8_t x, uint8_t y, uint8_t c);
void		EraseTextXY(uint16_t text);
void		Decode1bppTile(uint16_t source, uint8_t color);
void		Load1bppTile(uint16_t source, uint8_t x, uint8_t y, uint8_t color);
// Returns the source after the data; x, y advanced.
uint16_t	Load1bppTiles(uint16_t source, uint8_t* x, uint8_t* y, int count, uint8_t color);
uint16_t	Load2bppTile(uint16_t source, uint16_t* dest, int count);
void		FillVRAM(uint32_t address, int count, uint8_t value);
void		ClearSprAttr();
void		EnableScreen();
void		DisableScreen();
void		EnableSprites();
void		DisableSprites();
void		ClearScreen();
void		SetDefaultPal();
// Banks 1, 2 and (BankInA0Fixed or 3): what the "_" helpers restore.
void		SetBanks_1_2_X();
void		SetBankInA0(uint8_t bank);
