/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Routines that are not rewritten yet: they flag gUnported, so the game
// goes back to the title and the tests show what is missing. Each one moves
// to its module when it is rewritten.
#include "Actors.h"
#include "GameMain.h"

#include <stddef.h>


#define UNPORTED(name) \
	void name(uint16_t) { if (gUnported == NULL) gUnported = #name; }


#include "Unported.h"

#define UNPORTED0(name) \
	void name() { if (gUnported == NULL) gUnported = #name; }

UNPORTED0(EndingSetup)
UNPORTED0(EndingLogic)
