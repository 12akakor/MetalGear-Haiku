/*
 * Basic graphics routines of the original (Banks0123.asm): palette, tile
 * decoding and upload, rectangles, tile drawing. Names follow the
 * disassembly.
 */
#ifndef MG_GFX_BASIC_H
#define MG_GFX_BASIC_H

#include <stdint.h>

/* BIOS WRTVDP */
void		WRTVDP(int reg, uint8_t value);

void		SetPaletteColor(int color, uint8_t redBlue, uint8_t green);
/* Palette data: (colour, red|blue, green) ... #FF. Returns the pointer
   after the end marker. */
uint16_t	SetPalette(uint16_t data);

void		FillRect(uint8_t color, uint8_t dx, uint8_t dy, uint8_t page, uint8_t nx,
				uint8_t ny);
void		ClearPage(uint8_t page);
void		ClearPage0();
/* HMMM; pages: bits 0-1 source page, bits 2-3 destination page. */
void		VDP_Copy_Byte(uint8_t sx, uint8_t sy, uint8_t pages, uint8_t dx, uint8_t dy,
				uint8_t nx, uint8_t ny);

/* Writes 'count' bytes from RAM/ROM at 'source' to VRAM 'dest'. */
void		RAMtoVRAM(uint16_t source, uint32_t dest, int count);

/* Tiles (8x8 pixels, 32 bytes) */
void		SetTilesetColors();
void		SetColorsIndexes(uint16_t colors);
/* Decode 3bpp tiles from 'source' to VRAM 'dest'; returns the source
   pointer after the data; 'dest' is advanced as the original does. */
uint16_t	Load3bppTiles(uint16_t source, uint16_t* dest, int count);
uint16_t	Load3bppTileFlip(uint16_t source, uint16_t* dest, int count);
void		LoadTileGfx(uint16_t source, uint16_t dest);
uint16_t	LoadTilesGfx(uint16_t source, uint16_t* dest, int count);
uint16_t	LoadTilesGfxFlip(uint16_t source, uint16_t* dest, int count);

/* Tile number to its position in VRAM page 1. */
uint16_t	TileToVramAdd(uint8_t tile);
void		GetTileXY(uint8_t tile, uint8_t* x, uint8_t* y);
void		DrawTile(uint8_t tile, uint8_t dx, uint8_t dy);
void		CalcNextCharPos(uint8_t* x, uint8_t* y);

/* Helpers */
uint8_t		GetNibbleHL_A2(uint16_t table, uint8_t position);
uint8_t		GetNibbleRoom(uint16_t table);
uint16_t	GetPointerDE2A(uint16_t table, uint8_t index);

#endif
