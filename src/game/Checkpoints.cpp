/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/checkpoints.asm.
#include "Checkpoints.h"

#include "Common.h"
#include "Rom.h"
#include "Structures.h"


using namespace var;


namespace {

// Copies the areas of GameDataAreas (address, size; #FF ends) to the
// buffer (save) or back.
void
SaveGameStatus2(bool save)
{
	Ram(TempData) = save ? 1 : 0;
	uint16_t buffer = GameProgressBuffer;
	uint16_t list = SYM(GameDataAreas);
	for (;;) {
		uint8_t low = Mem(list);
		if (low == 0xff)
			return;
		uint16_t address = static_cast<uint16_t>(low | Mem(static_cast<uint16_t>(list + 1)) << 8);
		uint8_t count = Mem(static_cast<uint16_t>(list + 2));
		list = static_cast<uint16_t>(list + 3);
		for (int i = 0; i < count; i++) {
			if (save)
				Ram(buffer) = Ram(address);
			else
				Ram(address) = Ram(buffer);
			buffer++;
			address++;
		}
	}
}

}	// namespace


void
ChkSaveGameStatus()
{
	if (Ram(MetalGear_KO) != 0)
		return;
	uint8_t room = Ram(Room);
	uint8_t previous = Ram(PreviousRoom);
	uint16_t list = SYM(SaveStatRooms);
	for (int i = 0; i < 31; i++, list = static_cast<uint16_t>(list + 2)) {
		if (Mem(list) == room && Mem(static_cast<uint16_t>(list + 1)) == previous) {
			SaveGameStatus2(true);
			return;
		}
	}
}


void
RestoreGameStat()
{
	SaveGameStatus2(false);
	Ram(DamageDelayTimer) = 0;
}
