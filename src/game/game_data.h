/*
 * Read-only game data (music, graphics, maps, texts) in its original
 * format, taken from the ROM built from the disassembly. Data is
 * addressed as the original code sees it: a 16 bit address plus the ROM
 * bank that holds it.
 */
#ifndef MG_GAME_DATA_H
#define MG_GAME_DATA_H

#include <stdint.h>
#include <vector>

bool	game_data_load(const char* romPath);
bool	game_data_set(const std::vector<uint8_t>& rom);

/* Byte at 'address' (#4000-#BFFF) when 'bank' is mapped at that address. */
uint8_t	game_data_byte(int bank, uint16_t address);

/* Sound data: banks 4 and 5 at #6000-#9FFF. */
static inline uint8_t sound_data(uint16_t address)
{
	return game_data_byte(address < 0x8000 ? 4 : 5, address);
}

#endif
