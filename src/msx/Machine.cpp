/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// MSX2 machine for Metal Gear, see machine.h.
#include "Machine.h"

#include <stdio.h>
#include <string.h>


namespace {

// System variables used by the game or the interrupt handler.
const uint16_t kRG0SAV = 0xf3df;
const uint16_t kRG8SAV = 0xffe7;
const uint16_t kSTATFL = 0xf3e7;
const uint16_t kJIFFY = 0xfc9e;
const uint16_t kSCRMOD = 0xfcaf;
const uint16_t kHOOKS = 0xfd9a;
const uint16_t kHOOKS_END = 0xffca;

// BIOS entry points (see constants/bios.asm of the disassembly).
enum {
	kENASLT = 0x0024,
	kWRTVDP = 0x0047,
	kCHGMOD = 0x005f,
	kWRTPSG = 0x0093,
	kRDPSG = 0x0096,
	kCHGET = 0x009f,
	kTAPION = 0x00e1,
	kTAPIN = 0x00e4,
	kTAPIOF = 0x00e7,
	kTAPOON = 0x00ea,
	kTAPOUT = 0x00ed,
	kTAPOOF = 0x00f0,
	kRSLREG = 0x0138,
	kRDVDP = 0x013e,
	kSNSMAT = 0x0141
};

// Interrupt handler (at #1000, entered via #0038), as Z80 code (like the real BIOS: save all
// registers, read S#0, call H.TIMI, count JIFFY, restore, EI, RET).
const uint8_t kIntHandler[] = {
	0xf5, 0xc5, 0xd5, 0xe5,	// push af, bc, de, hl
	0x08, 0xd9,	// ex af,af'; exx
	0xf5, 0xc5, 0xd5, 0xe5,	// push af, bc, de, hl
	0xfd, 0xe5, 0xdd, 0xe5,	// push iy; push ix
	0xdb, 0x99,	// in a,(#99)
	0x32, 0xe7, 0xf3,	// ld (STATFL),a
	0xcd, 0x9f, 0xfd,	// call H.TIMI
	0x2a, 0x9e, 0xfc,	// ld hl,(JIFFY)
	0x23,	// inc hl
	0x22, 0x9e, 0xfc,	// ld (JIFFY),hl
	0xdd, 0xe1, 0xfd, 0xe1,	// pop ix; pop iy
	0xe1, 0xd1, 0xc1, 0xf1,	// pop hl, de, bc, af
	0x08, 0xd9,	// ex af,af'; exx
	0xe1, 0xd1, 0xc1, 0xf1,	// pop hl, de, bc, af
	0xfb, 0xc9	// ei; ret
};

}	// namespace


Machine::Machine()
	:
	fCpu(this)
{
	memset(biosCalls, 0, sizeof(biosCalls));
	Reset();
}


void
Machine::CopyStateFrom(const Machine& other)
{
	fCpu.r = other.fCpu.r;
	fCpu.eiPending = other.fCpu.eiPending;
	fCpu.m1Wait = other.fCpu.m1Wait;
	fVdp = other.fVdp;
	fPsg = other.fPsg;
	if (fRom.size() != other.fRom.size())
		fRom = other.fRom;
	memcpy(fBios, other.fBios, sizeof(fBios));
	memcpy(fRam, other.fRam, sizeof(fRam));
	memcpy(fBank, other.fBank, sizeof(fBank));
	memcpy(fKeys, other.fKeys, sizeof(fKeys));
	fJoystick = other.fJoystick;
	fPsgLatch = other.fPsgLatch;
	fFrames = other.fFrames;
	fCycleDebt = other.fCycleDebt;
	fTime = other.fTime;
	tape = other.tape;
	typed = other.typed;
}


bool
Machine::LoadRom(const std::vector<uint8_t>& rom)
{
	if (rom.size() != 0x20000)
		return false;
	fRom = rom;
	return true;
}


void
Machine::Reset()
{
	bool japanese = false;
	if (fRom.size() == 0x20000) {
		uint32_t crc = 0xffffffff;
		for (uint8_t b : fRom) {
			crc ^= b;
			for (int k = 0; k < 8; k++)
				crc = (crc >> 1) ^ (0xedb88320 & (0u - (crc & 1)));
		}
		japanese = ~crc == 0xfafe1303;
	}
	memset(fBios, 0xc9, sizeof(fBios));	// RET everywhere
	fBios[0x0006] = 0x98;	// VDP read port
	fBios[0x0007] = 0x98;	// VDP write port
	fBios[0x002b] = 0x11;	// international, 60 Hz
	fBios[0x002c] = 0x00;
	fBios[0x002d] = 0x01;	// MSX2
	if (japanese) {
		// Japanese MSX: character set, date format and keyboard
		// (the Japanese version checks this)
		fBios[0x002b] = 0x00;
		fBios[0x002c] = 0x00;
	}
	// #0038: JP to the handler (the entry points from #0047 on follow).
	fBios[0x38] = 0xc3;
	fBios[0x39] = 0x00;
	fBios[0x3a] = 0x10;
	memcpy(fBios + 0x1000, kIntHandler, sizeof(kIntHandler));

	memset(fRam, 0, sizeof(fRam));
	for (uint32_t a = kHOOKS; a < kHOOKS_END; a++)
		fRam[a - 0xc000] = 0xc9;

	for (int i = 0; i < 4; i++)
		fBank[i] = static_cast<uint8_t>(i);
	memset(fKeys, 0xff, sizeof(fKeys));
	fJoystick = 0;
	fPsgLatch = 0;
	fFrames = 0;
	fCycleDebt = 0;
	fTime = 0;

	fVdp.Reset();
	fPsg.Reset();
	fCpu.Reset();
	fCpu.m1Wait = 1;
	fCpu.r.sp = 0xf380;
	fCpu.r.im = 1;
	if (fRom.size() == 0x20000)
		fCpu.r.pc = static_cast<uint16_t>(fRom[2] | (fRom[3] << 8));	// INIT
}


uint8_t
Machine::Read(uint16_t address)
{
	if (address < 0x4000)
		return fBios[address];
	if (address < 0xc000) {
		int slot = (address - 0x4000) >> 13;
		uint32_t offset = static_cast<uint32_t>((fBank[slot] & 0x0f) * 0x2000
			+ (address & 0x1fff));
		return fRom.empty() ? 0xff : fRom[offset];
	}
	return fRam[address - 0xc000];
}


void
Machine::Write(uint16_t address, uint8_t value)
{
	if (address >= 0xc000) {
		fRam[address - 0xc000] = value;
		return;
	}
	if (address >= 0x6000 && address < 0xc000)
		fBank[(address - 0x4000) >> 13] = value;
}


uint8_t
Machine::In(uint16_t port)
{
	port &= 0xff;
	if (port >= 0x98 && port <= 0x9b)
		return fVdp.ReadPort(port);
	if (port == 0xa2) {
		if (fPsgLatch == 14)
			return static_cast<uint8_t>((0x3f & ~fJoystick) | 0x40);
		return fPsg.Register(fPsgLatch);
	}
	return 0xff;
}


void
Machine::Out(uint16_t port, uint8_t value)
{
	port &= 0xff;
	if (onPortOut)
		onPortOut(port, value);
	if (port >= 0x98 && port <= 0x9b)
		fVdp.WritePort(port, value);
	else if (port == 0xa0)
		fPsgLatch = value & 0x0f;
	else if (port == 0xa1) {
		if (onPsgWrite)
			onPsgWrite(fPsgLatch, value);
		fPsg.Write(fPsgLatch, value);
	}
}


void
Machine::SetKey(int row, int bit, bool down)
{
	if (row < 0 || row > 10 || bit < 0 || bit > 7)
		return;
	if (down)
		fKeys[row] &= static_cast<uint8_t>(~(1 << bit));
	else
		fKeys[row] |= static_cast<uint8_t>(1 << bit);
}


// BIOS -------------------------------------------------------------------

void
Machine::_Bios_WRTVDP(int reg, uint8_t value)
{
	reg &= 0x3f;
	fVdp.WriteRegister(reg, value);
	if (reg < 8)
		fRam[kRG0SAV + reg - 0xc000] = value;
	else if (reg < 24)
		fRam[kRG8SAV + reg - 8 - 0xc000] = value;
}


void
Machine::_Bios_CHGMOD(Z80Regs& r)
{
	if (r.a == 5) {
		// GRAPHIC 4, 212 lines, sprites 16x16 off by default.
		static const uint8_t kRegs[][2] = {
			{0, 0x06}, {1, 0x60}, {2, 0x1f}, {5, 0xef}, {6, 0x0f},
			{11, 0x00}, {8, 0x08}, {9, 0x80}
		};
		for (size_t i = 0; i < sizeof(kRegs) / sizeof(kRegs[0]); i++)
			_Bios_WRTVDP(kRegs[i][0], kRegs[i][1]);
	}
	fRam[kSCRMOD - 0xc000] = r.a;
}


bool
Machine::HandleBios(Z80Regs& r, int* cycles)
{
	uint16_t pc = r.pc;
	if (pc >= 0x0200)
		return false;
	switch (pc) {
		case kENASLT:
			r.iff1 = r.iff2 = false;
			break;
		case kWRTVDP:
			_Bios_WRTVDP(r.c, r.b);
			r.iff1 = r.iff2 = true;
			break;
		case kCHGMOD:
			_Bios_CHGMOD(r);
			r.iff1 = r.iff2 = true;
			break;
		case kWRTPSG:
			if (onPsgWrite)
				onPsgWrite(r.a & 0x0f, r.e);
			fPsg.Write(r.a, r.e);
			fPsgLatch = r.a & 0x0f;
			r.iff1 = r.iff2 = true;
			break;
		case kRDPSG:
			fPsgLatch = r.a & 0x0f;
			r.a = In(0xa2);
			break;
		case kTAPION:
		case kTAPIN:
		case kTAPOON:
		case kTAPOUT:
		{
			// Without a tape: error (carry), as the old stub did.
			bool ok = false;
			if (tape != NULL) {
				if (pc == kTAPION)
					ok = tape->InOn();
				else if (pc == kTAPIN)
					ok = tape->In(&r.a);
				else if (pc == kTAPOON)
					ok = tape->OutOn();
				else
					ok = tape->Out(r.a);
			}
			if (ok)
				r.f &= static_cast<uint8_t>(~Z80_CF);
			else
				r.f |= Z80_CF;
			r.iff1 = r.iff2 = false;	// the BIOS reads and writes the tape with DI
			break;
		}
		case kTAPIOF:
			if (tape != NULL)
				tape->InOff();
			r.iff1 = r.iff2 = true;
			break;
		case kTAPOOF:
			if (tape != NULL)
				tape->OutOff();
			r.iff1 = r.iff2 = true;
			break;
		case kCHGET:
			if (!typed.empty()) {
				r.a = typed.front();
				typed.erase(typed.begin());
			} else
				r.a = 0x0d;
			break;
		case kRSLREG:
			r.a = 0x14;	// page 1 and 2: slot 1
			break;
		case kRDVDP:
			r.a = fVdp.ReadPort(1);
			break;
		case kSNSMAT:
			r.a = (r.a < 11) ? fKeys[r.a] : 0xff;
			r.iff1 = r.iff2 = true;
			break;
		default:
			return false;
	}
	biosCalls[pc]++;
	// RET
	r.pc = static_cast<uint16_t>(Read(r.sp) | (Read(static_cast<uint16_t>(r.sp + 1)) << 8));
	r.sp += 2;
	*cycles = 40;
	return true;
}


int
Machine::StepInstruction()
{
	int c = 0;
	if (fCpu.r.pc < 0x0200 && HandleBios(fCpu.r, &c)) {
		fCpu.eiPending = false;
		return c;
	}
	return fCpu.Step();
}


void
Machine::RunFrame()
{
	int displayLines = fVdp.DisplayLines();
	for (int line = 0; line < Vdp::kLinesPerFrame; line++) {
		fVdp.SetBeam(line, 0);
		fVdp.StartLine(line);
		if (line == displayLines)
			fVdp.StartVBlank();
		int cycles = fCycleDebt;
		uint64_t lineStart = fTime;
		while (cycles < kCyclesPerLine) {
			fVdp.SetTime(lineStart + static_cast<uint64_t>(cycles));
			if (fVdp.InterruptPending() && fCpu.r.iff1 && !fCpu.eiPending) {
				int c = fCpu.Interrupt();
				if (c > 0) {
					cycles += c;
					continue;
				}
			}
			int c = 0;
			if (onStep)
				onStep();
			if (traceLeft > 0) {
				traceLeft--;
				fprintf(stderr, "%04X A=%02X F=%02X BC=%04X DE=%04X HL=%04X SP=%04X\n",
					fCpu.r.pc, fCpu.r.a, fCpu.r.f, fCpu.r.bc, fCpu.r.de, fCpu.r.hl,
					fCpu.r.sp);
			}
			if (fCpu.r.pc < 0x0200 && HandleBios(fCpu.r, &c)) {
				cycles += c;
				fCpu.eiPending = false;
			} else
				cycles += fCpu.Step();
			fVdp.SetBeam(line, cycles < kCyclesPerLine ? cycles : kCyclesPerLine - 1);
		}
		fCycleDebt = cycles - kCyclesPerLine;
		fTime = lineStart + kCyclesPerLine;
		if (line < Vdp::kMaxLines)
			fVdp.RenderLine(line);
	}
	fFrames++;
}
