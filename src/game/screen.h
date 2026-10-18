/*
 * The game's screen memory and display settings.
 *
 * The game draws in four 256x256 pages of 16 colours (4 bits per pixel,
 * two pixels per byte, 128 bytes per line): page 0 is shown, pages 1 to 3
 * hold tiles and other graphics that are copied to page 0. The sprite
 * tables and a copy of the palette live at the end of page 1. This keeps
 * the layout the game's data and code are built around; the drawing
 * functions below are plain C++ operations on it, and the platform layer
 * turns page 0 plus the sprites into the picture (scaled, HD tiles,
 * effects).
 */
#ifndef MG_SCREEN_H
#define MG_SCREEN_H

#include <stdint.h>
#include <functional>


class Screen {
public:
	enum { kSize = 0x20000 };

						Screen();
			void		Reset();

	/* Linear access (graphics uploads). */
			void		Write(uint32_t address, uint8_t value)
							{ vram[address & (kSize - 1)] = value; }
			uint8_t		Read(uint32_t address) const
							{ return vram[address & (kSize - 1)]; }

	/* Pixel coordinates: y 0-1023 over the four pages. */
			uint8_t		Point(int x, int y) const;
			void		Pset(int x, int y, uint8_t color, int op);

	/* Rectangle operations, as the game uses them (left to right, top to
	   bottom; width clipped at the right edge; nx 0 means 512, ny 0 1024). */
	/* Fill with a byte (two pixels), x on even pixels. */
			void		FillBytes(int dx, int dy, int nx, int ny, uint8_t value);
	/* Copy bytes (two pixels at a time). */
			void		CopyBytes(int sx, int sy, int dx, int dy, int nx, int ny);
	/* Copy pixels with a logical operation (op: 0 copy, 8 copy without
	   colour 0, and so on). */
			void		CopyPixels(int sx, int sy, int dx, int dy, int nx, int ny, int op);
	/* Fill pixels with a logical operation. */
			void		FillPixels(int dx, int dy, int nx, int ny, uint8_t color, int op);
	/* Horizontal or vertical line of 'length' pixels. */
			void		Line(int x, int y, int length, bool vertical, uint8_t color);

	/* Display settings, numbered as the game sets them (BIOS WRTVDP
	   numbering: 1 display and sprites, 2 shown page, 8 sprites off,
	   23 vertical scroll, and so on). Also keeps the copies the BIOS
	   keeps in its variables, which the game reads back. */
			void		SetRegister(int reg, uint8_t value);
			uint8_t		Register(int reg) const { return reg_[reg & 63]; }
	/* Sets a setting without touching the BIOS copies (state loading). */
			void		LoadRegister(int reg, uint8_t value) { reg_[reg & 63] = value; }

	/* Palette entry as the game writes it: 0RRR0BBB, 00000GGG. */
			void		SetPalette(int index, uint8_t redBlue, uint8_t green);
			uint8_t		PaletteRB(int index) const { return palette[index & 15][0]; }
			uint8_t		PaletteG(int index) const { return palette[index & 15][1]; }

	/* Called for every byte copy, before it happens (HD tile layer). */
			std::function<void(int sx, int sy, int dx, int dy, int nx, int ny)>
						onCopyBytes;

			uint8_t		vram[kSize];
			uint8_t		palette[16][2];

private:
			uint8_t		reg_[64];
};

extern Screen g_screen;

#endif
