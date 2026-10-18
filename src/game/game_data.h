/*
 * Read-only game data (music, graphics, maps, texts) in its original
 * format, taken from the ROM the player provides. Data is addressed as the
 * original code sees it: a 16 bit address plus the ROM bank that holds it.
 * Addresses of named data come from rom_symbols.h, per ROM version.
 */
#ifndef MG_GAME_DATA_H
#define MG_GAME_DATA_H

#include <stdint.h>
#include <string>
#include <vector>

enum RomVersion {
	kRomUnknown,
	kRomEnglish,		/* CRC32 E85C5731 */
	kRomJapanese		/* CRC32 FAFE1303 */
};

/* Loads and checks the ROM; on failure 'error' says why. */
bool		game_data_load(const char* romPath, std::string* error = NULL);
bool		game_data_set(const std::vector<uint8_t>& rom, std::string* error = NULL);
RomVersion	game_rom_version();
const char*	game_rom_version_name(RomVersion version);
uint32_t	game_crc32(const uint8_t* data, size_t size);

/* Byte at 'address' (#4000-#BFFF) when 'bank' is mapped at that address. */
uint8_t		game_data_byte(int bank, uint16_t address);

/* Sound data: banks 4 and 5 at #6000-#9FFF. */
static inline uint8_t sound_data(uint16_t address)
{
	return game_data_byte(address < 0x8000 ? 4 : 5, address);
}

#endif
