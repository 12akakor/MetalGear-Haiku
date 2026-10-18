/*
 * Differential test of the C++ sound driver against the original Z80 code.
 *
 * Part 1: every sound ID is started on its own (and some combinations),
 *   then the driver runs for many ticks. After every call the PSG writes
 *   and the driver variables (#C000-#C1FF) must be identical.
 * Part 2: the real game runs (title, demo, input script); at every entry
 *   of UpdateSound and SetSound the state is copied to the C++ driver,
 *   the call is repeated there and the results are compared.
 *
 *   sound_diff <rom> [frames] [keys]
 */
#include "../../src/game/GameData.h"
#include "../../src/game/GameRam.h"
#include "../../src/game/SoundDriver.h"
#include "../../src/game/RomSymbols.h"
#include "../../src/msx/Machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>


namespace {

#define kUpdateSound RomSymbol(sym::UpdateSound)
#define kSetSound RomSymbol(sym::SetSound)
const uint16_t kReturn = 0xe000;

typedef std::vector<std::pair<int, int> > Writes;

class Recorder : public SoundListener {
public:
	void SoundRegister(int reg, uint8_t value) { writes.push_back(std::make_pair(reg, value)); }
	Writes writes;
};

Machine sMachine;
SoundDriver sDriver;
Recorder sRecorder;
long sChecks = 0, sFailures = 0;


/* Calls a routine of the original with banks 4 and 5 mapped. */
void
call_original(uint16_t address, uint8_t a, Writes& writes)
{
	Z80Regs& r = sMachine.Cpu().r;
	sMachine.Write(0x6000, 4);
	sMachine.Write(0x8000, 5);
	sMachine.Ram()[kReturn - 0xc000] = 0x76;
	r.sp = 0xf000;
	r.sp -= 2;
	sMachine.Write(r.sp, kReturn & 0xff);
	sMachine.Write((uint16_t)(r.sp + 1), kReturn >> 8);
	r.pc = address;
	r.a = a;
	r.iff1 = r.iff2 = false;
	writes.clear();
	sMachine.onPsgWrite = [&](int reg, uint8_t v) { writes.push_back(std::make_pair(reg, v)); };
	long guard = 0;
	while (r.pc != kReturn && ++guard < 10000000)
		sMachine.StepInstruction();
	sMachine.onPsgWrite = nullptr;
	if (guard >= 10000000)
		fprintf(stderr, "original did not return from %04X\n", address);
}


bool
compare(const char* what, int id, long tick, const Writes& original)
{
	sChecks++;
	bool ok = original == sRecorder.writes;
	int firstRam = -1;
	for (int i = 0; i < 0x200 && firstRam < 0; i++) {
		if (sMachine.Ram()[i] != gRam[i])
			firstRam = i;
	}
	if (ok && firstRam < 0)
		return true;
	sFailures++;
	if (sFailures <= 10) {
		printf("FAIL %s id %02X tick %ld:", what, id, tick);
		if (firstRam >= 0)
			printf(" RAM %04X original %02X port %02X;", 0xc000 + firstRam,
				sMachine.Ram()[firstRam], gRam[firstRam]);
		if (!ok) {
			printf(" PSG original");
			for (auto& w : original)
				printf(" %d=%02X", w.first, w.second);
			printf(" | port");
			for (auto& w : sRecorder.writes)
				printf(" %d=%02X", w.first, w.second);
		}
		printf("\n");
	}
	return false;
}


/* Part 1 */
void
run_sound(const std::vector<std::pair<long, int> >& script, long ticks, int id)
{
	memset(sMachine.Ram(), 0, 0x4000);
	memset(gRam, 0, sizeof(gRam));
	sDriver.fRegD = sDriver.fRegE = 0;
	sMachine.Cpu().r.d = sMachine.Cpu().r.e = 0;
	Writes original;
	size_t next = 0;
	for (long t = 0; t < ticks; t++) {
		while (next < script.size() && script[next].first == t) {
			int sound = script[next++].second;
			sMachine.Cpu().r.d = sDriver.fRegD;
			sMachine.Cpu().r.e = sDriver.fRegE;
			call_original(kSetSound, (uint8_t)sound, original);
			sRecorder.writes.clear();
			sDriver.SetSound((uint8_t)sound);
			if (!compare("SetSound", sound, t, original))
				return;
		}
		/* The original's DE at entry is whatever the game left; use the
		   same value on both sides. */
		uint8_t d = (uint8_t)(t * 7), e = (uint8_t)(t * 13);
		sMachine.Cpu().r.d = d;
		sMachine.Cpu().r.e = e;
		sDriver.fRegD = d;
		sDriver.fRegE = e;
		call_original(kUpdateSound, 0, original);
		sRecorder.writes.clear();
		sDriver.Update();
		if (!compare("UpdateSound", id, t, original))
			return;
	}
}

}	// namespace


int
main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: sound_diff <rom> [frames] [keys]\n");
		return 2;
	}
	std::vector<uint8_t> rom(0x20000);
	FILE* fp = fopen(argv[1], "rb");
	if (fp == NULL || fread(rom.data(), 1, rom.size(), fp) != rom.size()) {
		fprintf(stderr, "cannot read %s\n", argv[1]);
		return 1;
	}
	fclose(fp);
	std::string error;
	if (!SetGameRom(rom, &error)) {
		fprintf(stderr, "%s\n", error.c_str());
		return 1;
	}
	printf("ROM: %s\n", RomVersionName(LoadedRomVersion()));
	sMachine.LoadRom(rom);
	sMachine.Reset();
	sDriver.SetListener(&sRecorder);

	/* Part 1: every sound on its own, 3000 ticks (50 s) each. */
	for (int id = 1; id <= 0x5c; id++) {
		if (id > 0x28 && (id - 0x29) % 3 != 0)
			continue;
		std::vector<std::pair<long, int> > script;
		script.push_back(std::make_pair(0L, id));
		run_sound(script, 3000, id);
	}
	/* Music with sound effects on top, pause and resume, fades. */
	for (int music = 0x29; music <= 0x44; music += 3) {
		std::vector<std::pair<long, int> > script;
		script.push_back(std::make_pair(0L, music));
		for (int k = 0; k < 40; k++)
			script.push_back(std::make_pair(30L + k * 50, 1 + (k * 7 + music) % 0x28));
		script.push_back(std::make_pair(700L, 0x4d));		/* pause */
		script.push_back(std::make_pair(900L, 0x59));
		script.push_back(std::make_pair(1500L, 0x50));		/* radio */
		run_sound(script, 2500, music);
		/* fade to another music: MusicToSet */
		script.clear();
		script.push_back(std::make_pair(0L, music));
		run_sound(script, 200, music);
	}
	printf("part 1: %ld checks, %ld failures\n", sChecks, sFailures);

	/* Part 2: the game itself. */
	long frames = argc > 2 ? atol(argv[2]) : 3000;
	sMachine.Reset();
	long part2Checks = sChecks, part2Failures = sFailures;
	int inUpdate = 0;
	sMachine.onStep = [&]() {
		Z80Regs& r = sMachine.Cpu().r;
		if (sMachine.Bank(1) != 4)
			return;
		if (r.pc != kUpdateSound && r.pc != kSetSound)
			return;
		/* Copy state, run the C++ version, then let the original run
		   to the return address and compare. */
		memcpy(gRam, sMachine.Ram(), sizeof(gRam));
		sDriver.fRegD = r.d;
		sDriver.fRegE = r.e;
		uint16_t ret = (uint16_t)(sMachine.Read(r.sp) | (sMachine.Read((uint16_t)(r.sp + 1)) << 8));
		uint16_t sp = (uint16_t)(r.sp + 2);
		bool update = r.pc == kUpdateSound;
		uint8_t a = r.a;
		sRecorder.writes.clear();
		if (update)
			sDriver.Update();
		else
			sDriver.SetSound(a);
		Writes original;
		auto saved = sMachine.onStep;
		sMachine.onStep = nullptr;
		sMachine.onPsgWrite = [&](int reg, uint8_t v) { original.push_back(std::make_pair(reg, v)); };
		/* Interrupts stay as the game has them; for SetSound an interrupt
		   could run UpdateSound in between, then the check is skipped. */
		bool interrupted = false;
		while (!(r.pc == ret && r.sp == sp)) {
			if (!update && r.iff1 && sMachine.GetVdp().InterruptPending()) {
				interrupted = true;
			}
			sMachine.StepInstruction();
		}
		sMachine.onPsgWrite = nullptr;
		sMachine.onStep = saved;
		if (!interrupted)
			compare(update ? "game UpdateSound" : "game SetSound", a, inUpdate++, original);
	};
	for (long f = 0; f < frames; f++)
		sMachine.RunFrame();
	printf("part 2: %ld checks, %ld failures\n", sChecks - part2Checks, sFailures - part2Failures);
	return sFailures == 0 ? 0 : 1;
}
