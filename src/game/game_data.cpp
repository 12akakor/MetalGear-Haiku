#include "game_data.h"
#include "game_ram.h"

#include <stdio.h>

uint8_t g_ram[0x4000];

static std::vector<uint8_t> sRom;


bool
game_data_set(const std::vector<uint8_t>& rom)
{
	if (rom.size() != 0x20000)
		return false;
	sRom = rom;
	return true;
}


bool
game_data_load(const char* romPath)
{
	FILE* fp = fopen(romPath, "rb");
	if (fp == NULL)
		return false;
	std::vector<uint8_t> rom(0x20000);
	bool ok = fread(rom.data(), 1, rom.size(), fp) == rom.size();
	fclose(fp);
	return ok && game_data_set(rom);
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
