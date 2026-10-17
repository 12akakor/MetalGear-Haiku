/*
 * The MSX2 as Metal Gear sees it:
 *   page 0 (#0000-#3FFF)  minimal BIOS: interrupt handler at #0038 (Z80
 *                         code) and entry points handled in C++ (HLE)
 *   page 1-2 (#4000-#BFFF) the cartridge, Konami mapper (no SCC):
 *                         #4000 bank 0 fixed, #6000/#8000/#A000 switchable
 *   page 3 (#C000-#FFFF)  RAM with the system variables
 * plus the VDP, the PSG and the keyboard matrix.
 *
 * The CPU is any object with the interface of CpuCore: the interpreter
 * (reference) or, later, the code generated from the disassembly.
 */
#ifndef MG_MACHINE_H
#define MG_MACHINE_H

#include "../z80/z80.h"
#include "psg.h"
#include "vdp.h"

#include <stdint.h>
#include <functional>
#include <vector>


class Machine : public Z80Bus {
public:
	enum {
		kCyclesPerLine = 228,
		kCyclesPerFrame = kCyclesPerLine * Vdp::kLinesPerFrame
	};

							Machine();

	/* rom: the 128 KB MegaROM. Returns false if the size is wrong. */
			bool			LoadRom(const std::vector<uint8_t>& rom);
			void			Reset();

	/* Runs one video frame (262 lines) with the interpreter. */
			void			RunFrame();

	/* Keyboard matrix: row 0..10, bit 0..7; true = pressed. */
			void			SetKey(int row, int bit, bool down);
	/* Joystick port 1: bits 0..5 = up, down, left, right, A, B. */
			void			SetJoystick(uint8_t bits) { fJoystick = bits; }

			Vdp&			GetVdp() { return fVdp; }
			Psg&			GetPsg() { return fPsg; }
			Z80&			Cpu() { return fCpu; }
	/* One instruction (or one emulated BIOS call), without interrupts. */
			int				StepInstruction();
			uint8_t*		Ram() { return fRam; }
			uint64_t		Frames() const { return fFrames; }
			uint8_t			Bank(int slot) const { return fBank[slot]; }

	/* Z80Bus */
	virtual	uint8_t			Read(uint16_t address);
	virtual	void			Write(uint16_t address, uint8_t value);
	virtual	uint8_t			In(uint16_t port);
	virtual	void			Out(uint16_t port, uint8_t value);

	/* BIOS entry points handled in C++. Returns true if 'pc' is one; then
	   the call has been carried out including the RET. */
			bool			HandleBios(Z80Regs& r, int* cycles);

	/* Statistics */
			unsigned		biosCalls[0x200];
	/* Debug: print this many executed PCs. */
			long			traceLeft = 0;
	/* Test hooks: called before every instruction, and for every PSG
	   register write (BIOS WRTPSG or port #A1). */
			std::function<void()>	onStep;
			std::function<void(int, uint8_t)>	onPsgWrite;
			std::function<void(int, uint8_t)>	onPortOut;

private:
			void			_Bios_CHGMOD(Z80Regs& r);
			void			_Bios_WRTVDP(int reg, uint8_t value);

			Z80				fCpu;
			Vdp				fVdp;
			Psg				fPsg;
			std::vector<uint8_t> fRom;
			uint8_t			fBios[0x4000];
			uint8_t			fRam[0x4000];		/* #C000-#FFFF */
			uint8_t			fBank[4];			/* 8 KB banks at #4000.. */
			uint8_t			fKeys[11];			/* 0 bits = pressed */
			uint8_t			fJoystick;
			uint8_t			fPsgLatch;
			uint64_t		fFrames;
			int				fCycleDebt;
			uint64_t		fTime;
};

#endif
