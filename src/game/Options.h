/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_OPTIONS_H
#define MG_OPTIONS_H


// Improvements of the port that change only how the game looks or sounds,
// never how it plays. The defaults here are the original; the program sets
// the player's choices (settings window, MetalGear.settings).

#include <stdint.h>


struct Options {
	// All sprites on every line, in a fixed order: no flicker. The original
	// shows at most 8 sprites per line (MSX limit) and rotates their order
	// every tick, so that each one only disappears now and then.
	bool	noFlicker = false;
	// Room tiles from hd/tiles where there is one (HdTiles.cpp).
	bool	hdTiles = false;
};

extern Options gOptions;


#endif	// MG_OPTIONS_H
