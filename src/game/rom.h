/*
 * Access to the game data in the ROM the way the original code sees it:
 * #4000-#5FFF is bank 0, the other three 8 KB windows hold the banks the
 * game selected last (BankIn60, BankIn80, BankInA0 in the game variables).
 */
#ifndef MG_ROM_H
#define MG_ROM_H

#include <stdint.h>

#include "game_data.h"
#include "game_ram.h"
#include "ram_symbols.h"
#include "rom_symbols.h"

uint8_t		rom(uint16_t address);
uint16_t	rom16(uint16_t address);

/* SetBanks: banks n, n+1, n+2 at #6000, #8000, #A000. */
void		set_banks(uint8_t first);

static inline uint16_t
ram16(uint16_t address)
{
	return (uint16_t)(ram(address) | (ram((uint16_t)(address + 1)) << 8));
}

static inline void
set_ram16(uint16_t address, uint16_t value)
{
	ram(address) = (uint8_t)value;
	ram((uint16_t)(address + 1)) = (uint8_t)(value >> 8);
}

/* Symbol address in the loaded ROM version. */
#define SYM(name) rom_symbol(sym::name)

#endif
