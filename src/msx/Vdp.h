/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_VDP_H
#define MG_VDP_H


// Yamaha V9938 (MSX2 VDP), the parts Metal Gear needs, plus the rest of the
// command engine for safety:
// - ports #98-#9B (VRAM, registers, palette, indirect register access)
// - 128 KB VRAM, 16 colour palette (9 bit RGB)
// - GRAPHIC 4 (SCREEN 5, 256 x 212, 4 bit per pixel), vertical scroll
// - sprite mode 2 (colour per line, CC, IC, EC, 8 per line)
// - all VDP commands, executed at once
// - status registers S#0 to S#9, vertical blank and line interrupts
// Other screen modes are not rendered (black).

#include <functional>
#include <stdint.h>


class Vdp {
public:
	enum {
		kWidth = 256,
		kMaxLines = 212,
		kLinesPerFrame = 262
	};

							Vdp();
			void			Reset();

			uint8_t			ReadPort(int port);
			void			WritePort(int port, uint8_t value);

	// Direct register write as by the BIOS WRTVDP.
			void			WriteRegister(int reg, uint8_t value);
			uint8_t			Register(int reg) const { return fReg[reg]; }

	// Beam position, kept up to date by the machine: line 0 is the first
	// displayed line, 'cycle' the CPU cycle within the line (0..227).
	// 'time' is the absolute CPU cycle count (for command durations).
			void			SetBeam(int line, int cycle);
			void			SetTime(uint64_t time) { fTime = time; }
			uint64_t		Time() const { return fTime; }

	// Renders one display line into the frame buffer.
			void			RenderLine(int line);
	// Number of displayed lines (192 or 212).
			int				DisplayLines() const;
	// Called at the start of vertical blanking.
			void			StartVBlank();
	// Called at the start of every line, for the line interrupt.
			void			StartLine(int line);

			bool			InterruptPending() const;

	// 0xRRGGBB, kWidth x kMaxLines. Lines past DisplayLines() are black.
			const uint32_t*	Frame() const { return fFrame; }

			const uint8_t*	Vram() const { return fVram; }
			uint8_t*		VramWritable() { return fVram; }
			const uint16_t*	Palette() const { return fPalette; }

	// Statistics for tests.
			unsigned		commandCount;
	// Test hook, called before a command runs.
	struct Command { int cmd, op, sx, sy, dx, dy, nx, ny, arg, clr; };
			std::function<void(const Command&)> onCommand;

private:
			void			_StartCommand();
			void			_CommandTransfer(uint8_t value);
			uint8_t			_Point(int x, int y) const;
			void			_Pset(int x, int y, uint8_t color, int op);
			uint8_t			_LogicOp(uint8_t dst, uint8_t src, int op) const;
			void			_FinishCommand();
			uint32_t		_Rgb(int index) const;
			void			_RenderSprites(int line, uint8_t* lineColors,
								bool* lineSet);

			uint8_t			fVram[0x20000];
			uint8_t			fReg[64];
			uint8_t			fStatus[10];
			uint16_t		fPalette[16];	// 0RRR0BBB 00000GGG
			uint32_t		fFrame[kWidth * kMaxLines];

			uint32_t		fAddress;
			uint8_t			fReadAhead;
			uint8_t			fLatch;
			bool			fLatchFull;
			uint8_t			fPaletteLatch;
			bool			fPaletteLatchFull;

			int				fLine;
			int				fCycle;

	// Command in progress (only HMMC, LMMC and LMCM wait for the CPU).
			int				fCmd;
			int				fCmdOp;
			int				fCx, fCy, fCnx, fCny;
			int				fCdx, fCdy;
			int				fCx0, fCnx0;

	// Command duration model: CE stays set until this CPU cycle.
			uint64_t		fTime;
			uint64_t		fBusyUntil;
			void			_AddDuration(int cmd, long steps, long lines);
};


#endif	// MG_VDP_H
