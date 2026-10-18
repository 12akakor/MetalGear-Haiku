/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_GAME_RAM_H
#define MG_GAME_RAM_H


// The game's variables. The original keeps them at fixed addresses in
// #C000-#FFFF; the port keeps the same layout in one array, so every routine
// can be checked byte for byte against the original. Named accessors are
// added per subsystem.

#include <stdint.h>


extern uint8_t gRam[0x4000];

static inline uint8_t&
Ram(uint16_t address)
{
	return gRam[static_cast<uint16_t>(address - 0xc000) & 0x3fff];
}


#endif	// MG_GAME_RAM_H
