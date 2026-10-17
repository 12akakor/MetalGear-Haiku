/* Which routines of the original talk to the VDP, and how.
   vdp_profile <rom> <sym> <frames> [keys] */
#include "../src/msx/machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

struct Sym { uint32_t addr; std::string name; };
static std::vector<Sym> syms;
static std::string lookup(uint16_t pc, int bank) {
	/* symbols are 16 bit addresses; banks overlap, so use the name with the
	   highest address <= pc (approximate for banked code) */
	std::string best = "?"; uint32_t ba = 0;
	for (auto& s : syms) if (s.addr <= pc && s.addr >= ba && pc - s.addr < 0x400) { ba = s.addr; best = s.name; }
	char buf[32]; snprintf(buf, sizeof buf, " [%04X b%d]", pc, bank);
	return best + buf;
}
int main(int argc, char** argv) {
	std::vector<uint8_t> rom(0x20000); FILE* fp = fopen(argv[1], "rb"); fread(rom.data(), 1, rom.size(), fp); fclose(fp);
	fp = fopen(argv[2], "r"); char line[256];
	while (fgets(line, sizeof line, fp)) { char n[128]; unsigned a; if (sscanf(line, "%127[^:]: equ %xh", n, &a) == 2 && a >= 0x4000 && a < 0xc000) syms.push_back({a, n}); }
	fclose(fp);
	static Machine m; m.LoadRom(rom); m.Reset();
	std::map<std::string, std::map<std::string, long>> stats;
	int reg = -1; uint8_t latch = 0; bool full = false;
	/* a cache per pc for speed */
	std::map<uint32_t, std::string> cache;
	auto who = [&]() -> std::string& {
		uint16_t pc = m.Cpu().r.pc; int bank = pc >= 0xa000 ? m.Bank(3) : pc >= 0x8000 ? m.Bank(2) : pc >= 0x6000 ? m.Bank(1) : 0;
		uint32_t key = pc | (bank << 16);
		auto it = cache.find(key); if (it != cache.end()) return it->second;
		return cache[key] = lookup(pc, bank);
	};
	m.onPortOut = [&](int port, uint8_t v) {
		if (port < 0x98 || port > 0x9b) return;
		std::string& w = who();
		if (port == 0x98) stats[w]["vram write"]++;
		else if (port == 0x9a) stats[w]["palette"]++;
		else if (port == 0x99) {
			if (!full) { latch = v; full = true; }
			else { full = false; if (v & 0x80) { char b[32]; snprintf(b, sizeof b, "R%d", v & 0x3f); stats[w][b]++; if ((v & 0x3f) == 46) { snprintf(b, sizeof b, "cmd %02X", latch); stats[w][b]++; } } else stats[w]["set address"]++; }
		} else if (port == 0x9b) {
			char b[32]; snprintf(b, sizeof b, "indirect"); stats[w][b]++;
		}
		(void)reg;
	};
	std::vector<std::pair<long, std::vector<int>>> keys;
	if (argc > 4) { FILE* k = fopen(argv[4], "r"); long f; int r, b, d; while (fscanf(k, "%ld %d %d %d", &f, &r, &b, &d) == 4) keys.push_back({f, {r, b, d}}); fclose(k); }
	size_t ki = 0; long frames = atol(argv[3]);
	for (long f = 0; f < frames; f++) {
		while (ki < keys.size() && keys[ki].first == f) { m.SetKey(keys[ki].second[0], keys[ki].second[1], keys[ki].second[2]); ki++; }
		m.RunFrame();
	}
	for (auto& s : stats) {
		printf("%s:", s.first.c_str());
		for (auto& x : s.second) printf("  %s=%ld", x.first.c_str(), x.second);
		printf("\n");
	}
}
