/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_GAME_DATA_H
#define MG_GAME_DATA_H


// Read-only game data (music, graphics, maps, texts) in its original
// format, taken from the ROM the player provides. Data is addressed as the
// original code sees it: a 16 bit address plus the ROM bank that holds it.
// Addresses of named data come from rom_symbols.h, per ROM version.

#include <stdint.h>
#include <string>
#include <vector>


enum RomVersion {
	kRomUnknown,
	kRomEnglish,	// CRC32 E85C5731
	kRomJapanese	// CRC32 FAFE1303
};

// How the loaded ROM was recognised.
struct RomInfo {
	RomVersion	version = kRomUnknown;
	bool		exact = false;	// one of the two reference dumps
	uint32_t	crc32 = 0;
	char		sha1[41] = "";
	size_t		originalSize = 0;
	int			matchingBanks = 0;	// of 16, with the chosen version
	uint32_t	differingBanks = 0;	// bit mask
	std::string	note;	// remark for the player
};

// Identifies a ROM image (any size); does not load it.
RomInfo		IdentifyRom(const std::vector<uint8_t>& image);
const RomInfo&	LoadedRomInfo();

// Loads and checks the ROM; on failure 'error' says why.
bool		LoadGameRom(const char* romPath, std::string* error = NULL);
bool		SetGameRom(const std::vector<uint8_t>& rom, std::string* error = NULL);
RomVersion	LoadedRomVersion();
const char*	RomVersionName(RomVersion version);
uint32_t	Crc32(const uint8_t* data, size_t size);
void		Sha1(const uint8_t* data, size_t size, char out[41]);

// Byte at 'address' (#4000-#BFFF) when 'bank' is mapped at that address.
uint8_t		RomByte(int bank, uint16_t address);

// Sound data: banks 4 and 5 at #6000-#9FFF.
static inline uint8_t
SoundData(uint16_t address)
{
	return RomByte(address < 0x8000 ? 4 : 5, address);
}


#endif	// MG_GAME_DATA_H
