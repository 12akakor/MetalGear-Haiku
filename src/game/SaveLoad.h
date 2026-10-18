/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_SAVE_LOAD_H
#define MG_SAVE_LOAD_H


// Saving and loading the game on tape (logic/saveload.asm). The original
// writes with the BIOS tape routines; here the tape is a .CAS image
// (gTape), byte for byte what the original writes, so saves are
// interchangeable with tapes from MSX emulators.
//
// A save on tape:
//   block 1: 10 x #EA, the file name (6 bytes, ASCII, padded with 0)
//   block 2: #300 bytes from GameProgressBuffer, then TailDataByte (the
//            checksum; CalcDataChecksum always gives 0)

#include <stdint.h>

#include <string>
#include <vector>

#include "CasTape.h"


extern CasTape gTape;

void		LoadSaveLogic();
uint8_t		CalcDataChecksum();

// The saves on a tape, in order.
struct TapeSave {
	std::string	name;
	size_t		offset;		// of the name block header
	size_t		end;		// after the data block
	bool		complete;	// the data block has #301 bytes
};
std::vector<TapeSave> ListTapeSaves(const CasTape& tape);

// Port additions for what the player did with the cassette: before saving,
// a save with the same name is removed and the tape wound to its end;
// loading starts at the beginning of the tape; a verify starts at the save
// that was just written.
void		PrepareTapeForSave(CasTape& tape, const uint8_t* name);


#endif	// MG_SAVE_LOAD_H
