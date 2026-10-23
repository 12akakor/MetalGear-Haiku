/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "GameData.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "GameRam.h"
#include "RomBlockCrc.h"
#include "RomSymbols.h"


uint8_t gRam[0x4000];

static std::vector<uint8_t> sRom;
static RomVersion sVersion = kRomUnknown;


uint32_t
Crc32(const uint8_t* data, size_t size)
{
	uint32_t crc = 0xffffffff;
	for (size_t i = 0; i < size; i++) {
		crc ^= data[i];
		for (int k = 0; k < 8; k++)
			crc = (crc >> 1) ^ (0xedb88320 & (0u - (crc & 1)));
	}
	return ~crc;
}


void
Sha1(const uint8_t* data, size_t size, char out[41])
{
	uint32_t h[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
	std::vector<uint8_t> m(data, data + size);
	uint64_t bits = static_cast<uint64_t>(size) * 8;
	m.push_back(0x80);
	while (m.size() % 64 != 56)
		m.push_back(0);
	for (int i = 7; i >= 0; i--)
		m.push_back(static_cast<uint8_t>(bits >> (i * 8)));
	for (size_t chunk = 0; chunk < m.size(); chunk += 64) {
		uint32_t w[80];
		for (int i = 0; i < 16; i++)
			w[i] = static_cast<uint32_t>(m[chunk + i * 4]) << 24
				| static_cast<uint32_t>(m[chunk + i * 4 + 1]) << 16
				| static_cast<uint32_t>(m[chunk + i * 4 + 2]) << 8 | m[chunk + i * 4 + 3];
		for (int i = 16; i < 80; i++) {
			uint32_t v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
			w[i] = (v << 1) | (v >> 31);
		}
		uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
		for (int i = 0; i < 80; i++) {
			uint32_t f, k;
			if (i < 20) { f = (b & c) | (~b & d); k = 0x5a827999; }
			else if (i < 40) { f = b ^ c ^ d; k = 0x6ed9eba1; }
			else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc; }
			else { f = b ^ c ^ d; k = 0xca62c1d6; }
			uint32_t t = ((a << 5) | (a >> 27)) + f + e + k + w[i];
			e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = t;
		}
		h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
	}
	for (int i = 0; i < 5; i++)
		snprintf(out + i * 8, 9, "%08x", h[i]);
}


void
FollowRelocatedData()
{
	ResetRomSymbols();
	// The dictionary of the Japanese texts: "ld de,idxDictionary" in
	// AddDictEntry (the Remix 1.995 translations move it).
	uint16_t address = RomSymbol(sym::AddDictEntry1);
	if (sVersion == kRomJapanese && address != 0xffff && RomByte(0, address + 6) == 0x11) {
		uint16_t dictionary = static_cast<uint16_t>(RomByte(0, address + 7)
			| RomByte(0, address + 8) << 8);
		if (dictionary != RomSymbol(sym::idxDictionary))
			SetRomSymbol(sym::idxDictionary, dictionary);
	}
}


const char*
RomVersionName(RomVersion version)
{
	switch (version) {
		case kRomEnglish:
			return "English";
		case kRomJapanese:
			return "Japanese";
		default:
			return "unknown";
	}
}


namespace {

// CRC32 of every 8 KB bank of the two reference dumps (built from the
// disassembly, the same as the GoodMSX / openMSX database dumps:
// SHA1 b656bd19... English, a52021f1... Japanese). Only checksums, no game
// data.
const uint32_t kBankCrc[2][16] = {
	{ 0x219492de, 0x625a1f80, 0x8022e511, 0xf6470fee, 0xf5d585ab, 0xbe32e2f0,
	  0xbe8e48ed, 0x159a32f9, 0x4f8d7d8d, 0x19e3d6dd, 0xd5c3c2cd, 0xdc41baf3,
	  0xaf72aeba, 0x0f1f5180, 0xe641647c, 0x381d34fe },
	{ 0x9a9dc691, 0x872903d0, 0x98bba33d, 0xa60e0c42, 0xf5d585ab, 0xbe32e2f0,
	  0xbe953df8, 0x159a32f9, 0x4f8d7d8d, 0x86cb95a1, 0xd5c3c2cd, 0xdc41baf3,
	  0xbc720a97, 0x0f1f5180, 0xe641647c, 0x2b0977dd }
};

// A modified ROM is accepted when at least this share (percent) of its
// 256 byte blocks equals a reference. The two references share about 67
// percent; translations and SCC versions share 89 percent or more.
const int kMinimumSimilarity = 80;

RomInfo sInfo;


// Percentage of the (non uniform) 256 byte blocks equal to reference 'v'.
int
Similarity(const std::vector<uint8_t>& rom, int v)
{
	int equal = 0, counted = 0;
	for (int block = 0; block < 512; block++) {
		if (kBlockCrc[v][block] == 0)
			continue;
		counted++;
		if (Crc32(rom.data() + block * 256, 256) == kBlockCrc[v][block])
			equal++;
	}
	return counted == 0 ? 0 : equal * 100 / counted;
}


bool
Contains(const uint8_t* data, size_t size, const uint8_t* pattern, size_t length)
{
	return std::search(data, data + size, pattern, pattern + length) != data + size;
}

}	// namespace


RomInfo
IdentifyRom(const std::vector<uint8_t>& image)
{
	RomInfo info;
	info.originalSize = image.size();
	std::vector<uint8_t> rom = image;
	// Overdumps: 256 KB or more with the 128 KB repeated, or padding.
	while (rom.size() > 0x20000 && rom.size() % 0x20000 == 0) {
		size_t half = rom.size() / 2;
		long halfSize = static_cast<long>(half);
		if (std::equal(rom.begin(), rom.begin() + halfSize, rom.begin() + halfSize))
			rom.resize(half);
		else
			break;
	}
	if (rom.size() > 0x20000) {
		bool padding = true;
		for (size_t i = 0x20000; i < rom.size() && padding; i++)
			padding = rom[i] == 0xff || rom[i] == 0x00;
		if (padding) {
			rom.resize(0x20000);
			info.note = "extra padding after 128 KB ignored";
		}
	}
	if (!rom.empty()) {
		info.crc32 = Crc32(rom.data(), rom.size());
		Sha1(rom.data(), rom.size(), info.sha1);
	}
	if (rom.size() < 0x20000) {
		info.note = "the ROM must be at least 128 KB (131072 bytes)";
		return info;
	}

	// Candidates for the 128 KB of the game: the first 128 KB, and for the
	// SCC versions (136 KB, Konami SCC cartridge) bank 16 as bank 0: their
	// new bank 0 only starts the game and holds the SCC sound code.
	std::vector<uint8_t> candidates[2];
	candidates[0].assign(rom.begin(), rom.begin() + 0x20000);
	if (rom.size() >= 0x22000) {
		candidates[1].assign(rom.begin() + 0x20000, rom.begin() + 0x22000);
		candidates[1].insert(candidates[1].end(), rom.begin() + 0x2000, rom.begin() + 0x20000);
	}
	int best = -1, bestCandidate = 0, bestSimilarity = -1;
	for (int c = 0; c < 2; c++) {
		if (candidates[c].empty())
			continue;
		for (int v = 0; v < 2; v++) {
			int similarity = Similarity(candidates[c], v);
			if (similarity > bestSimilarity) {
				bestSimilarity = similarity;
				best = v;
				bestCandidate = c;
			}
		}
	}
	const std::vector<uint8_t>& game = candidates[bestCandidate];
	info.similarity = bestSimilarity;
	for (int b = 0; b < 16; b++) {
		if (Crc32(game.data() + b * 0x2000, 0x2000) == kBankCrc[best][b])
			info.matchingBanks++;
		else
			info.differingBanks |= 1u << b;
	}
	if (bestSimilarity < kMinimumSimilarity) {
		info.note = "not a known version of Metal Gear (MSX2)";
		return info;
	}
	info.version = best == 0 ? kRomEnglish : kRomJapanese;
	info.game = game;
	info.exact = info.matchingBanks == 16 && rom.size() == 0x20000;

	// SCC: "ld a,3Fh / ld (9000h),a" switches the SCC on.
	static const uint8_t kSccOn[] = { 0x3e, 0x3f, 0x32, 0x00, 0x90 };
	info.scc = bestCandidate == 1 && Contains(rom.data(), 0x2000, kSccOn, sizeof(kSccOn));

	if (!info.exact) {
		char buf[200];
		snprintf(buf, sizeof(buf), "modified %s ROM (%d%% equal, %d of 16 banks; differ:",
			RomVersionName(info.version), bestSimilarity, info.matchingBanks);
		std::string note = buf;
		for (int b = 0; b < 16; b++) {
			if ((info.differingBanks & (1u << b)) != 0u) {
				snprintf(buf, sizeof(buf), " %d", b);
				note += buf;
			}
		}
		note += "); texts and graphics come from this ROM";
		if (info.scc)
			note += "; SCC version: the SCC sound is on";
		else if (rom.size() > 0x20000)
			note += "; the extra banks (added code) are not used";
		if (!info.note.empty())
			note = info.note + "; " + note;
		info.note = note;
	}
	return info;
}


const RomInfo&
LoadedRomInfo()
{
	return sInfo;
}


bool
SetGameRom(const std::vector<uint8_t>& image, std::string* error)
{
	RomInfo info = IdentifyRom(image);
	if (info.version == kRomUnknown) {
		if (error != NULL) {
			char buf[300];
			snprintf(buf, sizeof(buf), "%s (CRC32 %08X, SHA1 %s); supported: English"
				" E85C5731, Japanese FAFE1303 and close variants of them",
				info.note.c_str(), info.crc32, info.sha1);
			*error = buf;
		}
		return false;
	}
	sRom = info.game;
	info.game.clear();
	sVersion = info.version;
	sInfo = info;
	FollowRelocatedData();
	return true;
}


bool
LoadGameRom(const char* romPath, std::string* error)
{
	FILE* fp = fopen(romPath, "rb");
	if (fp == NULL) {
		if (error != NULL)
			*error = std::string("cannot open ") + romPath;
		return false;
	}
	std::vector<uint8_t> rom;
	uint8_t buf[65536];
	while (rom.size() <= 0x200000) {
		size_t n = fread(buf, 1, sizeof(buf), fp);
		if (n == 0)
			break;
		rom.insert(rom.end(), buf, buf + n);
	}
	fclose(fp);
	return SetGameRom(rom, error);
}


RomVersion
LoadedRomVersion()
{
	return sVersion;
}


uint8_t
RomByte(int bank, uint16_t address)
{
	if (sRom.empty())
		return 0xff;
	if (address < 0x6000)
		bank = 0;
	return sRom[static_cast<uint32_t>(bank & 0x0f) * 0x2000 + (address & 0x1fff)];
}
