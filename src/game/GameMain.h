/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_GAME_MAIN_H
#define MG_GAME_MAIN_H


// The game's main program: start up and the 60 Hz tick (the original runs
// everything from the vertical blank interrupt: InterruptTick).

#include <stdint.h>


// Start: clear the variables, set up the screen.
void		Start();
// InterruptTick: sound, then one step of the game logic.
void		InterruptTick();
void		GameStatusLogic();

void		SetSoundEntry(uint8_t id);
void		SetSoundEntryChk(uint8_t id);
void		NextSubstatus();

// Set when the game reaches a part that is not rewritten yet.
extern const char* gUnported;


#endif	// MG_GAME_MAIN_H
