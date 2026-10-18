/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_ROM_H
#define MG_ROM_H


// Access to the game data in the ROM the way the original code sees it:
// #4000-#5FFF is bank 0, the other three 8 KB windows hold the banks the
// game selected last (BankIn60, BankIn80, BankInA0 in the game variables).

#include <stdint.h>

#include "GameData.h"
#include "GameRam.h"
#include "RamSymbols.h"
#include "RomSymbols.h"


uint8_t		Rom(uint16_t address);
uint16_t	Rom16(uint16_t address);

// SetBanks: banks n, n+1, n+2 at #6000, #8000, #A000.
void		SetBanks(uint8_t first);

static inline uint16_t
Ram16(uint16_t address)
{
	return static_cast<uint16_t>(Ram(address) | (Ram(static_cast<uint16_t>(address + 1)) << 8));
}

static inline void
SetRam16(uint16_t address, uint16_t value)
{
	Ram(address) = static_cast<uint8_t>(value);
	Ram(static_cast<uint16_t>(address + 1)) = static_cast<uint8_t>(value >> 8);
}

// Symbol address in the loaded ROM version.
#define SYM(name) RomSymbol(sym::name)


#endif	// MG_ROM_H
