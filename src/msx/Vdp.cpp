/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// V9938 subset, see vdp.h. Behaviour follows the V9938 Technical Data
// Book (Yamaha) and the MSX2 Technical Handbook (ASCII).
#include "Vdp.h"

#include <string.h>


namespace {

enum {
	kCmdStop = 0x0, kCmdPoint = 0x4, kCmdPset = 0x5, kCmdSrch = 0x6,
	kCmdLine = 0x7, kCmdLmmv = 0x8, kCmdLmmm = 0x9, kCmdLmcm = 0xa,
	kCmdLmmc = 0xb, kCmdHmmv = 0xc, kCmdHmmm = 0xd, kCmdYmmm = 0xe,
	kCmdHmmc = 0xf
};

}	// namespace


Vdp::Vdp()
{
	Reset();
}


void
Vdp::Reset()
{
	memset(fVram, 0, sizeof(fVram));
	memset(fReg, 0, sizeof(fReg));
	memset(fStatus, 0, sizeof(fStatus));
	memset(fFrame, 0, sizeof(fFrame));
	// MSX2 power-on palette (R, G, B, 0..7), as set by the BIOS.
	static const uint8_t kDefault[16][3] = {
		{0,0,0}, {0,0,0}, {1,6,1}, {3,7,3}, {1,1,7}, {2,3,7}, {5,1,1}, {2,6,7},
		{7,1,1}, {7,3,3}, {6,6,1}, {6,6,4}, {1,4,1}, {6,2,5}, {5,5,5}, {7,7,7}
	};
	for (int i = 0; i < 16; i++) {
		fPalette[i] = static_cast<uint16_t>((kDefault[i][0] << 4) | kDefault[i][2]
			| (kDefault[i][1] << 8));
	}
	fAddress = 0;
	fReadAhead = 0;
	fLatch = 0;
	fLatchFull = false;
	fPaletteLatch = 0;
	fPaletteLatchFull = false;
	fLine = 0;
	fCycle = 0;
	fCmd = 0;
	commandCount = 0;
	fTime = 0;
	fBusyUntil = 0;
	fStatus[1] = 0x04;	// V9938 ID
	fStatus[2] = 0x0c;
}


int
Vdp::DisplayLines() const
{
	return ((fReg[9] & 0x80) != 0) ? 212 : 192;
}


void
Vdp::SetBeam(int line, int cycle)
{
	fLine = line;
	fCycle = cycle;
}


void
Vdp::StartVBlank()
{
	fStatus[0] |= 0x80;
}


void
Vdp::StartLine(int line)
{
	if (line < DisplayLines()
		&& ((line + fReg[23]) & 0xff) == fReg[19])
		fStatus[1] |= 0x01;
}


bool
Vdp::InterruptPending() const
{
	return (((fStatus[0] & 0x80) != 0) && ((fReg[1] & 0x20) != 0))
		|| (((fStatus[1] & 0x01) != 0) && ((fReg[0] & 0x10) != 0));
}


uint8_t
Vdp::ReadPort(int port)
{
	switch (port & 3) {
		case 0: {
			uint8_t v = fReadAhead;
			fReadAhead = fVram[fAddress & 0x1ffff];
			fAddress = (fAddress + 1) & 0x1ffff;
			fLatchFull = false;
			return v;
		}
		case 1: {
			fLatchFull = false;
			int s = fReg[15] & 0x0f;
			uint8_t v = 0xff;
			switch (s) {
				case 0:
					v = fStatus[0];
					fStatus[0] &= 0x1f;
					break;
				case 1:
					v = fStatus[1];
					fStatus[1] &= 0xfe;
					break;
				case 2: {
					v = static_cast<uint8_t>(fStatus[2] & 0x9f);
					v |= 0x0c;
					if (fTime < fBusyUntil)
						v |= 0x01;
					bool vblank = fLine >= DisplayLines();
					if (vblank)
						v |= 0x40;
					if (fCycle >= 172)
						v |= 0x20;
					break;
				}
				case 7:
					v = fStatus[7];
					if (fCmd == kCmdLmcm)
						_CommandTransfer(0);
					break;
				default:
					if (s < 10)
						v = fStatus[s];
					break;
			}
			return v;
		}
		default:
			return 0xff;
	}
}


void
Vdp::WritePort(int port, uint8_t value)
{
	switch (port & 3) {
		case 0:
			fVram[fAddress & 0x1ffff] = value;
			fReadAhead = value;
			fAddress = (fAddress + 1) & 0x1ffff;
			fLatchFull = false;
			break;
		case 1:
			if (!fLatchFull) {
				fLatch = value;
				fLatchFull = true;
				break;
			}
			fLatchFull = false;
			if ((value & 0x80) != 0) {
				WriteRegister(value & 0x3f, fLatch);
			} else {
				fAddress = static_cast<uint32_t>(((fReg[14] & 7) << 14)
					| ((value & 0x3f) << 8) | fLatch);
				if ((value & 0x40) == 0) {
					fReadAhead = fVram[fAddress];
					fAddress = (fAddress + 1) & 0x1ffff;
				}
			}
			break;
		case 2:
			if (!fPaletteLatchFull) {
				fPaletteLatch = value;
				fPaletteLatchFull = true;
				break;
			}
			fPaletteLatchFull = false;
			fPalette[fReg[16] & 15] = static_cast<uint16_t>((fPaletteLatch & 0x77)
				| ((value & 7) << 8));
			fReg[16] = static_cast<uint8_t>((fReg[16] + 1) & 15);
			break;
		default: {
			int reg = fReg[17] & 0x3f;
			if (reg != 17)
				WriteRegister(reg, value);
			if ((fReg[17] & 0x80) == 0)
				fReg[17] = static_cast<uint8_t>((fReg[17] & 0xc0) | ((reg + 1) & 0x3f));
			break;
		}
	}
}


void
Vdp::WriteRegister(int reg, uint8_t value)
{
	if (reg >= 47)
		return;
	if (reg == 44 && (fCmd == kCmdHmmc || fCmd == kCmdLmmc)) {
		fReg[44] = value;
		_CommandTransfer(value);
		return;
	}
	fReg[reg] = value;
	switch (reg) {
		case 14:
			fAddress = (((value & 7) << 14) | (fAddress & 0x3fff));
			break;
		case 16:
			fPaletteLatchFull = false;
			break;
		case 46:
			_StartCommand();
			break;
	}
}


// Commands ----------------------------------------------------------------

uint8_t
Vdp::_Point(int x, int y) const
{
	uint32_t a = static_cast<uint32_t>(((y & 1023) << 7) | ((x & 255) >> 1));
	uint8_t b = fVram[a & 0x1ffff];
	return ((x & 1) != 0) ? (b & 0x0f) : static_cast<uint8_t>(b >> 4);
}


uint8_t
Vdp::_LogicOp(uint8_t dst, uint8_t src, int op) const
{
	if (((op & 8) != 0) && src == 0)
		return dst;
	switch (op & 7) {
		case 0: return src;
		case 1: return dst & src;
		case 2: return dst | src;
		case 3: return dst ^ src;
		case 4: return static_cast<uint8_t>(~src & 0x0f);
		default: return dst;
	}
}


void
Vdp::_Pset(int x, int y, uint8_t color, int op)
{
	uint32_t a = static_cast<uint32_t>(((y & 1023) << 7) | ((x & 255) >> 1)) & 0x1ffff;
	uint8_t b = fVram[a];
	if ((x & 1) != 0) {
		uint8_t c = _LogicOp(b & 0x0f, color & 0x0f, op);
		fVram[a] = static_cast<uint8_t>((b & 0xf0) | c);
	} else {
		uint8_t c = _LogicOp(static_cast<uint8_t>(b >> 4), color & 0x0f, op);
		fVram[a] = static_cast<uint8_t>((b & 0x0f) | (c << 4));
	}
}


void
Vdp::_FinishCommand()
{
	fCmd = 0;
	fStatus[2] &= static_cast<uint8_t>(~0x81);	// CE = 0, TR = 0
}


// Durations in VDP cycles (21.48 MHz; 6 per CPU cycle) per step and per
// line, for: sprites on, sprites off, display off. Values from the timing
// tables of openMSX (VDPCmdEngine); see docs/01-voortgang.md for the
// comparison with openMSX.
void
Vdp::_AddDuration(int cmd, long steps, long lines)
{
	static const int kTiming[16][6] = {
		{ 0, 0, 0, 0, 0, 0 },	// STOP
		{ 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 },
		{ 92, 0, 65, 0, 65, 0 },	// POINT
		{ 129, 0, 92, 0, 92, 0 },	// PSET
		{ 92, 125, 65, 101, 65, 101 },	// SRCH
		{ 120, 147, 84, 120, 84, 120 },	// LINE
		{ 98, 137, 72, 124, 72, 124 },	// LMMV
		{ 129, 197, 92, 145, 92, 145 },	// LMMM
		{ 0, 0, 0, 0, 0, 0 },	// LMCM (CPU driven)
		{ 0, 0, 0, 0, 0, 0 },	// LMMC (CPU driven)
		{ 49, 65, 45, 54, 45, 54 },	// HMMV
		{ 92, 136, 65, 136, 65, 136 },	// HMMM
		{ 65, 125, 45, 101, 45, 101 },	// YMMM
		{ 0, 0, 0, 0, 0, 0 }	// HMMC (CPU driven)
	};
	int mode;
	if ((fReg[1] & 0x40) == 0)
		mode = 2;
	else if ((fReg[8] & 0x02) != 0)
		mode = 1;
	else
		mode = 0;
	uint64_t vdpCycles = static_cast<uint64_t>(steps) * kTiming[cmd][mode * 2]
		+ static_cast<uint64_t>(lines) * kTiming[cmd][mode * 2 + 1];
	uint64_t start = fTime > fBusyUntil ? fTime : fBusyUntil;
	fBusyUntil = start + (vdpCycles + 5) / 6;
}


void
Vdp::_StartCommand()
{
	commandCount++;
	int cmd = fReg[46] >> 4;
	int op = fReg[46] & 0x0f;
	int sx = (fReg[32] | ((fReg[33] & 1) << 8)) & 0x1ff;
	int sy = (fReg[34] | ((fReg[35] & 3) << 8)) & 0x3ff;
	int dx = (fReg[36] | ((fReg[37] & 1) << 8)) & 0x1ff;
	int dy = (fReg[38] | ((fReg[39] & 3) << 8)) & 0x3ff;
	int nx = (fReg[40] | ((fReg[41] & 1) << 8)) & 0x1ff;
	int ny = (fReg[42] | ((fReg[43] & 3) << 8)) & 0x3ff;
	int arg = fReg[45];
	int tx = ((arg & 0x04) != 0) ? -1 : 1;
	int ty = ((arg & 0x08) != 0) ? -1 : 1;
	uint8_t clr = fReg[44];
	if (onCommand) {
		Command c = { cmd, op, sx, sy, dx, dy, nx, ny, arg, clr };
		onCommand(c);
	}
	if (nx == 0)
		nx = 512;
	if (ny == 0)
		ny = 1024;

	fCmd = 0;
	fStatus[2] &= static_cast<uint8_t>(~0x81);

	switch (cmd) {
		case kCmdStop:
			return;

		case kCmdPoint:
			fStatus[7] = _Point(sx, sy);
			_AddDuration(cmd, 1, 0);
			return;

		case kCmdPset:
			_Pset(dx, dy, clr, op);
			_AddDuration(cmd, 1, 0);
			return;

		case kCmdSrch: {
			bool eq = (arg & 0x02) != 0;
			int x = sx;
			fStatus[2] &= static_cast<uint8_t>(~0x10);
			for (;;) {
				uint8_t c = _Point(x, sy);
				if ((c == (clr & 0x0f)) != eq) {
					fStatus[2] |= 0x10;
					fStatus[8] = static_cast<uint8_t>(x);
					fStatus[9] = static_cast<uint8_t>(0xfe | ((x >> 8) & 1));
					break;
				}
				x += tx;
				if (x < 0 || x > 255)
					break;
			}
			_AddDuration(cmd, (x - sx) * tx + 1, 1);
			return;
		}

		case kCmdLine: {
			int maj = arg & 0x01;
			int x = dx, y = dy;
			int err = (nx - 1) >> 1;
			if (fReg[40] == 0 && ((fReg[41] & 1) == 0))
				nx = 0;
			for (int i = 0; i <= nx; i++) {
				_Pset(x, y, clr, op);
				if (maj == 0) {
					x += tx;
					if ((err -= ny == 1024 ? 0 : ny) < 0) {
						err += nx;
						y += ty;
					}
				} else {
					y += ty;
					if ((err -= ny == 1024 ? 0 : ny) < 0) {
						err += nx;
						x += tx;
					}
				}
				if (x < 0 || x > 255)
					break;
			}
			fReg[38] = static_cast<uint8_t>(y);
			fReg[39] = static_cast<uint8_t>((y >> 8) & 3);
			_AddDuration(cmd, nx + 1, ny == 1024 ? 0 : ny);
			return;
		}

		case kCmdLmmv:
		case kCmdLmmm:
		case kCmdHmmv:
		case kCmdHmmm:
		case kCmdYmmm: {
			bool bytes = cmd >= kCmdHmmv;
			int width;
			if (cmd == kCmdYmmm) {
				width = tx > 0 ? 256 - dx : dx + 1;
			} else {
				int edge = tx > 0 ? 256 - dx : dx + 1;
				width = nx < edge ? nx : edge;
				if (cmd == kCmdLmmm || cmd == kCmdHmmm) {
					int sedge = tx > 0 ? 256 - sx : sx + 1;
					if (sedge < width)
						width = sedge;
				}
			}
			int srcY = sy, dstY = dy;
			int srcX0 = cmd == kCmdYmmm ? dx : sx;
			for (int line = 0; line < ny; line++) {
				if (bytes) {
					int step = tx * 2;
					int sxp = srcX0 & ~1, dxp = dx & ~1;
					for (int i = 0; i < width; i += 2) {
						uint32_t da = static_cast<uint32_t>(((dstY & 1023) << 7)
							| ((dxp & 255) >> 1));
						if (cmd == kCmdHmmv) {
							fVram[da & 0x1ffff] = clr;
						} else {
							uint32_t sa = static_cast<uint32_t>(((srcY & 1023) << 7)
								| ((sxp & 255) >> 1));
							fVram[da & 0x1ffff] = fVram[sa & 0x1ffff];
						}
						sxp += step;
						dxp += step;
					}
				} else {
					int sxp = sx, dxp = dx;
					for (int i = 0; i < width; i++) {
						uint8_t c = cmd == kCmdLmmv ? static_cast<uint8_t>(clr & 0x0f)
							: _Point(sxp, srcY);
						_Pset(dxp, dstY, c, op);
						sxp += tx;
						dxp += tx;
					}
				}
				srcY = (srcY + ty) & 1023;
				dstY = (dstY + ty) & 1023;
			}
			if (cmd == kCmdLmmm || cmd == kCmdHmmm || cmd == kCmdYmmm) {
				fReg[34] = static_cast<uint8_t>(srcY);
				fReg[35] = static_cast<uint8_t>((srcY >> 8) & 3);
			}
			fReg[38] = static_cast<uint8_t>(dstY);
			fReg[39] = static_cast<uint8_t>((dstY >> 8) & 3);
			fReg[42] = 0;
			fReg[43] = 0;
			_AddDuration(cmd, static_cast<long>(bytes ? (width + 1) / 2 : width) * ny, ny);
			return;
		}

		case kCmdLmmc:
		case kCmdHmmc:
		case kCmdLmcm: {
			fCmd = cmd;
			fCmdOp = op;
			fCdx = tx;
			fCdy = ty;
			int edge = tx > 0 ? 256 - (cmd == kCmdLmcm ? sx : dx)
				: (cmd == kCmdLmcm ? sx : dx) + 1;
			fCnx0 = nx < edge ? nx : edge;
			fCnx = fCnx0;
			fCny = ny;
			fCx0 = cmd == kCmdLmcm ? sx : dx;
			fCx = fCx0;
			fCy = cmd == kCmdLmcm ? sy : dy;
			if (cmd == kCmdHmmc) {
				fCx0 &= ~1;
				fCx = fCx0;
			}
			fStatus[2] |= 0x81;	// CE, TR
			if (cmd == kCmdLmcm) {
				fStatus[7] = _Point(fCx, fCy);
			} else {
				// The first byte is the CLR value written before the
				// command.
				_CommandTransfer(clr);
			}
			return;
		}

		default:
			return;
	}
}


void
Vdp::_CommandTransfer(uint8_t value)
{
	if (fCmd == 0)
		return;
	if (fCmd == kCmdHmmc) {
		uint32_t a = static_cast<uint32_t>(((fCy & 1023) << 7) | ((fCx & 255) >> 1));
		fVram[a & 0x1ffff] = value;
		fCx += 2 * fCdx;
		fCnx -= 2;
	} else if (fCmd == kCmdLmmc) {
		_Pset(fCx, fCy, value, fCmdOp);
		fCx += fCdx;
		fCnx--;
	} else {
		// LMCM: the value was read from S#7; advance and fetch the next.
		fCx += fCdx;
		fCnx--;
	}
	if (fCnx <= 0) {
		fCx = fCx0;
		fCnx = fCnx0;
		fCy = (fCy + fCdy) & 1023;
		if (--fCny <= 0) {
			fReg[38] = static_cast<uint8_t>(fCy);
			fReg[39] = static_cast<uint8_t>((fCy >> 8) & 3);
			_FinishCommand();
			return;
		}
	}
	if (fCmd == kCmdLmcm)
		fStatus[7] = _Point(fCx, fCy);
}


// Rendering ---------------------------------------------------------------

uint32_t
Vdp::_Rgb(int index) const
{
	uint16_t p = fPalette[index & 15];
	int r = (p >> 4) & 7, b = p & 7, g = (p >> 8) & 7;
	// 3 bit to 8 bit: 0, 36, 73, 109, 146, 182, 219, 255
	return static_cast<uint32_t>(((r * 255 / 7) << 16) | ((g * 255 / 7) << 8)
		| (b * 255 / 7));
}


void
Vdp::_RenderSprites(int line, uint8_t* lineColors, bool* lineSet)
{
	if ((fReg[8] & 0x02) != 0)
		return;	// sprites disabled
	uint32_t colorTable = static_cast<uint32_t>(((fReg[11] & 3) << 15)
		| ((fReg[5] & 0xf8) << 7));
	uint32_t attrTable = colorTable + 0x200;
	uint32_t patternTable = static_cast<uint32_t>((fReg[6] & 0x3f) << 11);
	bool big = (fReg[1] & 0x02) != 0;
	bool mag = (fReg[1] & 0x01) != 0;
	int size = (big ? 16 : 8) * (mag ? 2 : 1);
	int vline = (line + fReg[23]) & 0xff;

	// Collect up to 8 sprites on this line.
	int visible[8];
	int count = 0;
	for (int n = 0; n < 32; n++) {
		uint8_t y = fVram[(attrTable + n * 4) & 0x1ffff];
		if (y == 216)
			break;
		int row = (vline - y - 1) & 0xff;
		if (row >= size)
			continue;
		if (count == 8) {
			if ((fStatus[0] & 0x40) == 0)
				fStatus[0] = static_cast<uint8_t>((fStatus[0] & 0xa0) | 0x40 | n);
			break;
		}
		visible[count++] = n;
	}

	int8_t owner[256];
	memset(owner, -1, sizeof(owner));
	bool groupActive = false;
	bool tp = (fReg[8] & 0x20) != 0;
	for (int k = 0; k < count; k++) {
		int n = visible[k];
		uint32_t at = attrTable + n * 4;
		uint8_t y = fVram[at & 0x1ffff];
		int x = fVram[(at + 1) & 0x1ffff];
		int pattern = fVram[(at + 2) & 0x1ffff];
		int row = (vline - y - 1) & 0xff;
		if (mag)
			row >>= 1;
		uint8_t colorByte = fVram[(colorTable + n * 16 + row) & 0x1ffff];
		bool cc = (colorByte & 0x40) != 0;
		if ((colorByte & 0x80) != 0)
			x -= 32;
		uint8_t color = colorByte & 0x0f;
		if (big)
			pattern &= 0xfc;
		uint32_t pa = patternTable + pattern * 8 + row;
		uint16_t bits = static_cast<uint16_t>(fVram[pa & 0x1ffff] << 8);
		if (big)
			bits |= fVram[(pa + 16) & 0x1ffff];
		int width = big ? 16 : 8;

		if (!cc)
			groupActive = true;
		else if (!groupActive)
			continue;	// CC sprite without a preceding normal sprite

		for (int i = 0; i < width; i++) {
			if ((bits & (0x8000 >> i)) == 0)
				continue;
			for (int m = 0; m < (mag ? 2 : 1); m++) {
				int px = x + i * (mag ? 2 : 1) + m;
				if (px < 0 || px > 255)
					continue;
				if (cc) {
					if (owner[px] >= 0 && lineSet[px]) {
						lineColors[px] |= color;
					} else if (!lineSet[px] && (color != 0 || tp)) {
						lineColors[px] = color;
						lineSet[px] = true;
						owner[px] = static_cast<int8_t>(n);
					}
				} else if (!lineSet[px]) {
					if (color == 0 && !tp)
						continue;
					lineColors[px] = color;
					lineSet[px] = true;
					owner[px] = static_cast<int8_t>(n);
				} else if (((colorByte & 0x20) == 0) && ((fStatus[0] & 0x20) == 0)) {
					fStatus[0] |= 0x20;	// collision
				}
			}
		}
	}
}


void
Vdp::RenderLine(int line)
{
	if (line < 0 || line >= kMaxLines)
		return;
	uint32_t* out = fFrame + line * kWidth;
	uint32_t backdrop = _Rgb(fReg[7] & 0x0f);
	if (line >= DisplayLines() || ((fReg[1] & 0x40) == 0)) {
		for (int x = 0; x < kWidth; x++)
			out[x] = backdrop;
		return;
	}
	bool g4 = (fReg[0] & 0x0e) == 0x06 && (fReg[1] & 0x18) == 0;
	if (!g4) {
		for (int x = 0; x < kWidth; x++)
			out[x] = backdrop;
		return;
	}

	uint32_t page = static_cast<uint32_t>((fReg[2] & 0x60) << 10);
	int vline = (line + fReg[23]) & 0xff;
	const uint8_t* src = fVram + page + vline * 128;
	bool tp = (fReg[8] & 0x20) != 0;

	uint8_t spriteColors[256];
	bool spriteSet[256];
	memset(spriteSet, 0, sizeof(spriteSet));
	_RenderSprites(line, spriteColors, spriteSet);

	uint32_t palette[16];
	for (int i = 0; i < 16; i++)
		palette[i] = _Rgb(i);
	for (int x = 0; x < kWidth; x++) {
		uint8_t c;
		if (spriteSet[x])
			c = spriteColors[x];
		else {
			uint8_t b = src[x >> 1];
			c = ((x & 1) != 0) ? (b & 0x0f) : static_cast<uint8_t>(b >> 4);
		}
		out[x] = (c == 0 && !tp) ? backdrop : palette[c];
	}
}
