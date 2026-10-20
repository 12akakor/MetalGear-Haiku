/*
 * Helpers for differential tests: the reference machine runs original
 * routines; the native state (game variables, screen) is copied from it and
 * compared with it.
 */
#ifndef MG_DIFF_MACHINE_H
#define MG_DIFF_MACHINE_H

#include "../src/game/GameData.h"
#include "../src/game/GameRam.h"
#include "../src/game/RamSymbols.h"
#include "../src/game/Screen.h"
#include "../src/msx/Machine.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>


class DiffMachine {
public:
	Machine		machine;
	uint64_t	time = 0;
	long		checks = 0, failures = 0;
	std::vector<uint8_t> refresh;	/* values of LD A,R in the last Call */

	bool Load(const char* path)
	{
		std::vector<uint8_t> rom(0x20000);
		FILE* fp = fopen(path, "rb");
		if (fp == NULL || fread(rom.data(), 1, rom.size(), fp) != rom.size()) {
			fprintf(stderr, "cannot read %s\n", path);
			return false;
		}
		fclose(fp);
		std::string error;
		if (!SetGameRom(rom, &error)) {
			fprintf(stderr, "%s\n", error.c_str());
			return false;
		}
		machine.LoadRom(rom);
		machine.Reset();
		printf("ROM: %s\n", RomVersionName(LoadedRomVersion()));
		return true;
	}

	/* Runs the game until gameplay (SPACE on the title screen). */
	bool StartGame()
	{
		for (long f = 0; f < 2000; f++) {
			if (f == 400 || f == 700 || f == 1000 || f == 1300)
				machine.SetKey(8, 0, true);
			if (f == 405 || f == 705 || f == 1005 || f == 1305)
				machine.SetKey(8, 0, false);
			machine.RunFrame();
		}
		return machine.Ram()[0] == 5;
	}

	/* Calls an original routine (interrupts off) with the banks the game
	   has selected in its variables. */
	/* False when the routine did not return. */
	bool Call(uint16_t address, uint8_t a = 0)
	{
		Z80Regs& r = machine.Cpu().r;
		set_mapper_from_variables();
		const uint16_t kReturn = 0x3ff0;
		r.sp = 0xf300;
		r.sp -= 2;
		machine.Write(r.sp, kReturn & 0xff);
		machine.Write((uint16_t)(r.sp + 1), kReturn >> 8);
		r.pc = address;
		r.a = a;
		r.iff1 = r.iff2 = false;
		long guard = 0;
		refresh.clear();
		while (r.pc != kReturn && ++guard < 100000000) {
			bool ldAR = machine.Read(r.pc) == 0xed
				&& machine.Read((uint16_t)(r.pc + 1)) == 0x5f;
			time += (uint64_t)machine.StepInstruction();
			machine.GetVdp().SetTime(time);
			if (ldAR)
				refresh.push_back(r.a);
		}
		if (guard >= 100000000) {
			fprintf(stderr, "routine %04X did not return\n", address);
			return false;
		}
		return true;
	}

	/* Copies the reference state to the native side. */
	void CopyToNative()
	{
		memcpy(gRam, machine.Ram(), sizeof(gRam));
		memcpy(gScreen.fVram, machine.GetVdp().Vram(), Screen::kSize);
		const uint16_t* p = machine.GetVdp().Palette();
		for (int i = 0; i < 16; i++)
			gScreen.SetPalette(i, (uint8_t)(p[i] & 0x77), (uint8_t)((p[i] >> 8) & 7));
		for (int i = 0; i < 24; i++)
			gScreen.LoadRegister(i, machine.GetVdp().Register(i));
	}

	/* Compares; prints the first differences. */
	bool Compare(const char* what)
	{
		checks++;
		std::string report;
		char buf[160];
		const uint8_t* ram = machine.Ram();
		int ramDiffs = 0;
		for (int a = 0xc000; a < 0x10000; a++) {
			if (a >= 0xf200 && a < 0xf380)
				continue;		/* stack used by the test */
			if (a >= 0xef00 && a < 0xf0f0)
				continue;		/* the game's stack (no variables there) */
			if (a == 0xfc9e || a == 0xfc9f)
				continue;		/* JIFFY: the BIOS counts interrupts that come
						   in during a long tick of the reference */
			if (ram[a - 0xc000] != gRam[a - 0xc000]) {
				if (ramDiffs++ < 4) {
					snprintf(buf, sizeof(buf), " RAM %04X ref %02X port %02X;", a,
						ram[a - 0xc000], gRam[a - 0xc000]);
					report += buf;
				}
			}
		}
		const uint8_t* vram = machine.GetVdp().Vram();
		int vramDiffs = 0;
		for (int a = 0; a < Screen::kSize; a++) {
			if (vram[a] != gScreen.fVram[a] && vramDiffs++ < 4) {
				snprintf(buf, sizeof(buf), " VRAM %05X ref %02X port %02X;", a, vram[a],
					gScreen.fVram[a]);
				report += buf;
			}
		}
		const uint16_t* p = machine.GetVdp().Palette();
		for (int i = 0; i < 16; i++) {
			if ((p[i] & 0x77) != gScreen.PaletteRB(i) || ((p[i] >> 8) & 7) != gScreen.PaletteG(i)) {
				snprintf(buf, sizeof(buf), " palette %d;", i);
				report += buf;
			}
		}
		for (int i = 0; i < 24; i++) {
			if (i >= 14 && i <= 17)
				continue;	/* VRAM address and port pointers: access protocol only */
			if (machine.GetVdp().Register(i) != gScreen.Register(i)) {
				snprintf(buf, sizeof(buf), " R%d ref %02X port %02X;", i,
					machine.GetVdp().Register(i), gScreen.Register(i));
				report += buf;
			}
		}
		if (report.empty())
			return true;
		if (failures++ < 200)
			printf("FAIL %s:%s (RAM %d, VRAM %d)\n", what, report.c_str(), ramDiffs, vramDiffs);
		return false;
	}

private:
	void set_mapper_from_variables()
	{
		/* BankIn60/80/A0 decide what the original sees */
		uint8_t* ram = machine.Ram();
		machine.Write(0x6000, ram[var::BankIn60 - 0xc000]);
		machine.Write(0x8000, ram[var::BankIn80 - 0xc000]);
		machine.Write(0xa000, ram[var::BankInA0 - 0xc000]);
	}
};

#endif
