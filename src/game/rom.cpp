#include "rom.h"


uint8_t
rom(uint16_t address)
{
	if (address < 0x4000)
		return 0xff;
	if (address < 0x6000)
		return game_data_byte(0, address);
	if (address < 0x8000)
		return game_data_byte(ram(var::BankIn60), address);
	if (address < 0xa000)
		return game_data_byte(ram(var::BankIn80), address);
	if (address < 0xc000)
		return game_data_byte(ram(var::BankInA0), address);
	return ram(address);
}


uint16_t
rom16(uint16_t address)
{
	return (uint16_t)(rom(address) | (rom((uint16_t)(address + 1)) << 8));
}


void
set_banks(uint8_t first)
{
	ram(var::BankIn60) = first;
	ram(var::BankIn80) = (uint8_t)(first + 1);
	ram(var::BankInA0) = (uint8_t)(first + 2);
}
