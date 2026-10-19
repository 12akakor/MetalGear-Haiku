/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


/*
 * Differential test of single routines in the running game. The reference
 * machine plays the original (title, demo, or a game started with SPACE
 * and random controls). Whenever it enters a routine from the list
 * (Routines()), a copy of the machine runs that routine to its return,
 * the rewritten routine runs from the same state, and the results are
 * compared: variables, screen memory, palette, display settings and the
 * returned registers. The game itself goes on unchanged.
 *
 *   routine_diff <rom> <frames> [-play] [-gear] [-warp n] [-seed n] [-max n]
 *                [names...]
 *
 * -play: start a game with SPACE and move around at random (otherwise the
 * title and the demo run). -warp n: every n frames of play, move to the
 * neighbour of a random room and refill the life, to see more of the game.
 * -gear: all weapons and items, and the menus are opened now and then.
 * -max: checks per routine (default 300).
 */
#include "RoutineDiff.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>

#include "../RomBanks.h"
#include "../../src/game/Common.h"
#include "../../src/game/GameData.h"
#include "../../src/game/GameRam.h"
#include "../../src/game/RamSymbols.h"
#include "../../src/game/Rom.h"
#include "../../src/game/Screen.h"
#include "../../src/msx/Machine.h"


std::vector<Routine>&
Routines()
{
	static std::vector<Routine> sRoutines;
	return sRoutines;
}


namespace {

struct Entry {
	const Routine*	routine;
	int				bank;	// expected bank in the slot of the address
	long			checks = 0;
	long			failures = 0;
};

Machine sMain;
Machine sProbe;
std::map<uint16_t, std::vector<Entry*>> sByAddress;
bool sIsEntry[0x10000];
std::vector<Entry> sEntries;
long sMaxChecks = 300;
bool sInProbe = false;
long sTotalFailures = 0;


void
CopyToNative(const Machine& m)
{
	Machine& machine = const_cast<Machine&>(m);
	memcpy(gRam, machine.Ram(), sizeof(gRam));
	memcpy(gScreen.fVram, machine.GetVdp().Vram(), Screen::kSize);
	const uint16_t* p = machine.GetVdp().Palette();
	for (int i = 0; i < 16; i++)
		gScreen.SetPalette(i, static_cast<uint8_t>(p[i] & 0x77),
			static_cast<uint8_t>((p[i] >> 8) & 7));
	for (int i = 0; i < 24; i++)
		gScreen.LoadRegister(i, machine.GetVdp().Register(i));
}


std::string
CompareState(Machine& ref, uint16_t sp)
{
	std::string report;
	char buf[128];
	const uint8_t* ram = ref.Ram();
	int diffs = 0;
	for (int a = 0xc000; a < 0x10000; a++) {
		if (a >= 0xef00 && a < 0xf0f0)
			continue;	// the game's stack
		if (a >= sp - 0x100 && a < sp + 2)
			continue;
		if (ram[a - 0xc000] != gRam[a - 0xc000] && diffs++ < 6) {
			snprintf(buf, sizeof(buf), " RAM %04X ref %02X port %02X;", a, ram[a - 0xc000],
				gRam[a - 0xc000]);
			report += buf;
		}
	}
	const uint8_t* vram = ref.GetVdp().Vram();
	int vdiffs = 0;
	for (int a = 0; a < Screen::kSize; a++) {
		if (vram[a] != gScreen.fVram[a] && vdiffs++ < 4) {
			snprintf(buf, sizeof(buf), " VRAM %05X ref %02X port %02X;", a, vram[a],
				gScreen.fVram[a]);
			report += buf;
		}
	}
	if (diffs > 6 || vdiffs > 4) {
		snprintf(buf, sizeof(buf), " (RAM %d, VRAM %d differences)", diffs, vdiffs);
		report += buf;
	}
	const uint16_t* p = ref.GetVdp().Palette();
	for (int i = 0; i < 16; i++) {
		if ((p[i] & 0x77) != gScreen.PaletteRB(i) || ((p[i] >> 8) & 7) != gScreen.PaletteG(i)) {
			snprintf(buf, sizeof(buf), " palette %d;", i);
			report += buf;
		}
	}
	for (int i = 0; i < 24; i++) {
		if (i >= 14 && i <= 17)
			continue;
		if (ref.GetVdp().Register(i) != gScreen.Register(i)) {
			snprintf(buf, sizeof(buf), " R%d ref %02X port %02X;", i, ref.GetVdp().Register(i),
				gScreen.Register(i));
			report += buf;
		}
	}
	return report;
}


std::string
CompareResults(const char* results, const Z80Regs& ref, const Z80Regs& port)
{
	std::string report;
	char buf[96];
	std::string list = results;
	size_t start = 0;
	while (start < list.size()) {
		size_t end = list.find(',', start);
		if (end == std::string::npos)
			end = list.size();
		std::string r = list.substr(start, end - start);
		start = end + 1;
		int a = -1, b = -1;
		if (r == "a") { a = ref.a; b = port.a; }
		else if (r == "b") { a = ref.b; b = port.b; }
		else if (r == "c") { a = ref.c; b = port.c; }
		else if (r == "d") { a = ref.d; b = port.d; }
		else if (r == "e") { a = ref.e; b = port.e; }
		else if (r == "h") { a = ref.h; b = port.h; }
		else if (r == "l") { a = ref.l; b = port.l; }
		else if (r == "bc") { a = ref.bc; b = port.bc; }
		else if (r == "de") { a = ref.de; b = port.de; }
		else if (r == "hl") { a = ref.hl; b = port.hl; }
		else if (r == "ix") { a = ref.ix; b = port.ix; }
		else if (r == "iy") { a = ref.iy; b = port.iy; }
		else if (r == "cf") { a = ref.f & 1; b = port.f & 1; }
		else if (r == "zf") { a = (ref.f >> 6) & 1; b = (port.f >> 6) & 1; }
		else if (r.empty())
			continue;
		else {
			report += " unknown result " + r + ";";
			continue;
		}
		if (a != b) {
			snprintf(buf, sizeof(buf), " %s ref %X port %X;", r.c_str(), a, b);
			report += buf;
		}
	}
	return report;
}


void
Check(Entry& entry)
{
	Z80Regs in = sMain.Cpu().r;
	uint16_t sp = in.sp;
	uint16_t back = static_cast<uint16_t>(sMain.Read(sp) | (sMain.Read(sp + 1) << 8));

	// The original, on a copy, interrupts off.
	sInProbe = true;
	sProbe.CopyStateFrom(sMain);
	Z80Regs& r = sProbe.Cpu().r;
	r.iff1 = r.iff2 = false;
	uint64_t time = sProbe.GetVdp().Time();
	long guard = 0;
	while (!(r.pc == back && r.sp == static_cast<uint16_t>(sp + 2)) && ++guard < 20000000) {
		time += static_cast<uint64_t>(sProbe.StepInstruction());
		sProbe.GetVdp().SetTime(time);
	}
	sInProbe = false;
	if (guard >= 20000000) {
		printf("%s: did not return\n", entry.routine->name);
		entry.failures++;
		return;
	}

	// The rewrite, from the state before.
	CopyToNative(sMain);
	Z80Regs out = in;
	entry.routine->native(in, out);

	entry.checks++;
	std::string report = CompareState(sProbe, sp)
		+ CompareResults(entry.routine->results, r, out);
	if (report.empty())
		return;
	entry.failures++;
	sTotalFailures++;
	if (entry.failures <= 5) {
		const uint8_t* ram = sMain.Ram();
		printf("FAIL %s (frame %llu, A=%02X BC=%04X DE=%04X HL=%04X IX=%04X; banks %X %X %X,"
			" variables %X %X %X; room %d):%s\n",
			entry.routine->name, static_cast<unsigned long long>(sMain.Frames()), in.a,
			in.bc, in.de, in.hl, in.ix, sMain.Bank(1), sMain.Bank(2), sMain.Bank(3),
			ram[var::BankIn60 - 0xc000], ram[var::BankIn80 - 0xc000],
			ram[var::BankInA0 - 0xc000], ram[var::Room - 0xc000], report.c_str());
	}
}


void
OnStep()
{
	if (sInProbe)
		return;
	uint16_t pc = sMain.Cpu().r.pc;
	if (!sIsEntry[pc])
		return;
	auto it = sByAddress.find(pc);
	if (it == sByAddress.end())
		return;
	const uint8_t* ram = sMain.Ram();
	if (ram[0] > 8 || (ram[0] == 5 && ram[var::Room - 0xc000] == 0xff))
		return;	// the original went astray (see main)
	int slot = pc >> 13;	// 2 = #4000, 3 = #6000, ...
	for (Entry* e : it->second) {
		if (e->bank >= 0 && slot >= 3 && slot <= 5 && sMain.Bank(slot - 2) != e->bank)
			continue;
		if (e->checks >= sMaxChecks)
			return;
		Check(*e);
		return;
	}
}

}	// namespace


int
main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: routine_diff <rom> <frames> [-play] [-seed n] [-max n] "
			"[names...]\n");
		return 2;
	}
	std::vector<uint8_t> rom(0x20000);
	FILE* fp = fopen(argv[1], "rb");
	if (fp == NULL || fread(rom.data(), 1, rom.size(), fp) != rom.size()) {
		fprintf(stderr, "cannot read %s\n", argv[1]);
		return 2;
	}
	fclose(fp);
	std::string error;
	if (!SetGameRom(rom, &error)) {
		fprintf(stderr, "%s\n", error.c_str());
		return 2;
	}
	long frames = atol(argv[2]);
	bool play = false;
	bool gear = false;
	long warp = 0;
	unsigned seed = 1;
	std::vector<std::string> only;
	for (int i = 3; i < argc; i++) {
		if (!strcmp(argv[i], "-play"))
			play = true;
		else if (!strcmp(argv[i], "-gear"))
			gear = true;
		else if (!strcmp(argv[i], "-warp") && i + 1 < argc)
			warp = atol(argv[++i]);
		else if (!strcmp(argv[i], "-seed") && i + 1 < argc)
			seed = static_cast<unsigned>(atol(argv[++i]));
		else if (!strcmp(argv[i], "-max") && i + 1 < argc)
			sMaxChecks = atol(argv[++i]);
		else
			only.push_back(argv[i]);
	}

	sEntries.reserve(Routines().size());
	for (const Routine& routine : Routines()) {
		if (!only.empty()) {
			bool found = false;
			for (const std::string& name : only)
				found = found || name == routine.name;
			if (!found)
				continue;
		}
		Entry e;
		e.routine = &routine;
		e.bank = RoutineBank(routine.id);
		sEntries.push_back(e);
	}
	for (Entry& e : sEntries)
		sByAddress[RomSymbol(e.routine->id)].push_back(&e);
	for (auto& it : sByAddress)
		sIsEntry[it.first] = true;

	sMain.LoadRom(rom);
	sMain.Reset();
	sProbe.LoadRom(rom);
	sMain.onStep = OnStep;
	printf("ROM: %s, %zu routines\n", RomVersionName(LoadedRomVersion()), sEntries.size());

	srand(seed);
	int held = 0;
	long holdUntil = 0;
	for (long f = 0; f < frames; f++) {
		if (play) {
			// SPACE on the title, then random directions and now and then
			// fire, a weapon or an item.
			if (f == 400 || f == 700)
				sMain.SetKey(8, 0, true);
			if (f == 405 || f == 705)
				sMain.SetKey(8, 0, false);
			if (f > 900 && f >= holdUntil) {
				static const uint8_t kMoves[] = { 0x01, 0x02, 0x04, 0x08, 0x05, 0x06,
					0x09, 0x0a, 0x00 };
				held = kMoves[rand() % 9];
				if (rand() % 4 == 0)
					held |= 0x10;
				holdUntil = f + 20 + rand() % 90;
			}
			sMain.SetJoystick(static_cast<uint8_t>(held));
			uint8_t* ram = sMain.Ram();
			static long lastWarp = 0;
			if (warp > 0 && f - lastWarp >= warp && ram[var::GameStatus - 0xc000] == 5
				&& ram[var::GameMode - 0xc000] == 0 && ram[var::Room - 0xc000] < 126
				&& ram[var::PlayerControlMod - 0xc000] == 0) {
				lastWarp = f;
				// a room with a neighbour on that side (rewritten
				// GetNextRoomNum on scratch variables)
				for (int tries = 0; tries < 50; tries++) {
					uint8_t room = static_cast<uint8_t>(rand() % 126);
					uint8_t side = static_cast<uint8_t>(1 + rand() % 4);
					Ram(var::Room) = room;
					Ram(var::NextRoomDirect) = side;
					uint8_t next = GetNextRoomNum();
					if (next >= 126 || next == 103 || room == 103)
						continue;
					ram[var::Room - 0xc000] = room;
					ram[var::NextRoomDirect - 0xc000] = side;
					ram[var::IdDoorEnter - 0xc000] = 0;
					ram[var::GameMode - 0xc000] = 1;
					break;
				}
			}
			if (warp > 0)
				ram[var::Life - 0xc000] = ram[var::MaxLife - 0xc000];
			if (gear) {
				static bool given = false;
				if (!given && ram[var::GameStatus - 0xc000] == 5) {
					given = true;
					for (int w = 0; w < 8; w++) {
						uint8_t* p = ram + var::Weapons - 0xc000 + w * 4;
						p[0] = static_cast<uint8_t>(w + 1);
						p[1] = 0x50;
						p[2] = 0x01;
					}
					for (int e = 0; e < 25; e++) {
						uint8_t* p = ram + var::Equipment - 0xc000 + e * 4;
						uint8_t id = static_cast<uint8_t>(e + 1);
						p[0] = id == 0x17 ? 0 : id;	// not the transmitter
						p[1] = static_cast<uint8_t>(id >= 0x0e && id <= 0x15 ? id - 0x0d : 5);
						p[2] = 0;
					}
				}
				// F2 weapons, F3 equipment, F4 radio: open, move, close
				static int menuKey = -1;
				static long menuUntil = 0;
				static long releaseAt = 0;
				uint8_t mode = ram[var::GameMode - 0xc000];
				if (f == releaseAt) {
					sMain.SetKey(6, 6, false);
					sMain.SetKey(6, 7, false);
					sMain.SetKey(7, 0, false);
				}
				if (f % 12 == 0) {
					releaseAt = f + 6;
					if (mode == 0 && menuKey < 0 && rand() % 80 == 0) {
						menuKey = rand() % 3;
						menuUntil = f + 60 + rand() % 200;
						sMain.SetKey(menuKey < 2 ? 6 : 7, menuKey == 0 ? 6 : menuKey == 1 ? 7 : 0,
							true);
					} else if (menuKey >= 0 && f >= menuUntil) {
						sMain.SetKey(menuKey < 2 ? 6 : 7, menuKey == 0 ? 6 : menuKey == 1 ? 7 : 0,
							true);
						menuKey = -1;
					}
				}
				// stuck in a menu or the radio: try the keys that leave
				static long inMode = 0;
				inMode = mode == 0 ? 0 : inMode + 1;
				if (inMode > 500 && f % 30 == 0) {
					static const int kRow[] = { 7, 6, 6 };
					static const int kBit[] = { 0, 6, 7 };
					int k = static_cast<int>((f / 30) % 3);
					sMain.SetKey(kRow[k], kBit[k], true);
					releaseAt = f + 6;
				}
				if (mode == 2 || mode == 3 || mode == 4)
					sMain.SetJoystick(static_cast<uint8_t>(f % 20 < 3 ? 1 << (rand() % 4) : 0));
			}
			// Text windows: RETURN now and then
			if (f > 900)
				sMain.SetKey(7, 7, ram[var::GameMode - 0xc000] == 10 && f % 16 < 4);
		}
		sMain.RunFrame();
		if (sMain.Ram()[0] > 8
			|| (sMain.Ram()[0] == 5 && sMain.Ram()[var::Room - 0xc000] == 0xff)) {
			// warps can leave Snake where the original never puts him
			printf("the original went astray (game status %d, room %d) at frame %ld; "
				"stopped\n", sMain.Ram()[0], sMain.Ram()[var::Room - 0xc000], f);
			break;
		}
		if (getenv("MG_TRACE") != NULL && f % 300 == 0) {
			uint8_t* ram = sMain.Ram();
			printf("frame %ld: status %d,%d mode %d room %d life %d\n", f,
				ram[var::GameStatus - 0xc000], ram[var::GameSubstatus - 0xc000],
				ram[var::GameMode - 0xc000], ram[var::Room - 0xc000],
				ram[var::Life - 0xc000]);
		}
	}

	long checks = 0;
	for (const Entry& e : sEntries) {
		checks += e.checks;
		printf("%-28s %5ld checks %5ld failures%s\n", e.routine->name, e.checks, e.failures,
			e.checks == 0 ? "  (not reached)" : "");
	}
	printf("game status %d at the end; %ld checks, %ld failures\n", sMain.Ram()[0], checks,
		sTotalFailures);
	return sTotalFailures == 0 ? 0 : 1;
}
