/*
 * The game's variables. The original keeps them at fixed addresses in
 * #C000-#FFFF; the port keeps the same layout in one array, so every routine
 * can be checked byte for byte against the original. Named accessors are
 * added per subsystem.
 */
#ifndef MG_GAME_RAM_H
#define MG_GAME_RAM_H

#include <stdint.h>

extern uint8_t g_ram[0x4000];

static inline uint8_t& ram(uint16_t address)
{
	return g_ram[(uint16_t)(address - 0xc000) & 0x3fff];
}

#endif
