/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Rom.h"


uint8_t
Rom(uint16_t address)
{
	if (address < 0x4000)
		return 0xff;
	if (address < 0x6000)
		return RomByte(0, address);
	if (address < 0x8000)
		return RomByte(Ram(var::BankIn60), address);
	if (address < 0xa000)
		return RomByte(Ram(var::BankIn80), address);
	if (address < 0xc000)
		return RomByte(Ram(var::BankInA0), address);
	return Ram(address);
}


uint16_t
Rom16(uint16_t address)
{
	return static_cast<uint16_t>(Rom(address) | (Rom(static_cast<uint16_t>(address + 1)) << 8));
}


void
SetBanks(uint8_t first)
{
	Ram(var::BankIn60) = first;
	Ram(var::BankIn80) = static_cast<uint8_t>(first + 1);
	Ram(var::BankInA0) = static_cast<uint8_t>(first + 2);
}
