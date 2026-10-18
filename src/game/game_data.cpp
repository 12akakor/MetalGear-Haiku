#include "game_data.h"
#include "game_ram.h"

#include <stdio.h>

uint8_t g_ram[0x4000];

static std::vector<uint8_t> sRom;
static RomVersion sVersion = kRomUnknown;


uint32_t
game_crc32(const uint8_t* data, size_t size)
{
	uint32_t crc = 0xffffffff;
	for (size_t i = 0; i < size; i++) {
		crc ^= data[i];
		for (int k = 0; k < 8; k++)
			crc = (crc >> 1) ^ (0xedb88320 & (0u - (crc & 1)));
	}
	return ~crc;
}


const char*
game_rom_version_name(RomVersion version)
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


bool
game_data_set(const std::vector<uint8_t>& rom, std::string* error)
{
	if (rom.size() != 0x20000) {
		if (error != NULL)
			*error = "the ROM must be 131072 bytes (128 KB)";
		return false;
	}
	uint32_t crc = game_crc32(rom.data(), rom.size());
	RomVersion version = kRomUnknown;
	if (crc == 0xe85c5731)
		version = kRomEnglish;
	else if (crc == 0xfafe1303)
		version = kRomJapanese;
	if (version == kRomUnknown) {
		if (error != NULL) {
			char buf[160];
			snprintf(buf, sizeof(buf), "unknown ROM version (CRC32 %08X); supported:"
				" English E85C5731, Japanese FAFE1303", crc);
			*error = buf;
		}
		return false;
	}
	sRom = rom;
	sVersion = version;
	return true;
}


bool
game_data_load(const char* romPath, std::string* error)
{
	FILE* fp = fopen(romPath, "rb");
	if (fp == NULL) {
		if (error != NULL)
			*error = std::string("cannot open ") + romPath;
		return false;
	}
	std::vector<uint8_t> rom;
	uint8_t buf[65536];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), fp)) > 0 && rom.size() <= 0x20000)
		rom.insert(rom.end(), buf, buf + n);
	fclose(fp);
	return game_data_set(rom, error);
}


RomVersion
game_rom_version()
{
	return sVersion;
}


uint8_t
game_data_byte(int bank, uint16_t address)
{
	if (sRom.empty())
		return 0xff;
	if (address < 0x6000)
		bank = 0;
	return sRom[(uint32_t)(bank & 0x0f) * 0x2000 + (address & 0x1fff)];
}
