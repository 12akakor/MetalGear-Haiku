/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Lists the Metal Gear saves on a .CAS tape image (the port's
// MetalGear.cas, or a tape from an MSX emulator).
//
//   mg-tape <file.cas>

#include <stdio.h>

#include "../src/game/SaveLoad.h"


int
main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: mg-tape <file.cas>\n");
		return 2;
	}
	CasTape tape;
	if (!tape.Load(argv[1])) {
		fprintf(stderr, "cannot read %s\n", argv[1]);
		return 1;
	}
	std::vector<TapeSave> saves = ListTapeSaves(tape);
	if (saves.empty())
		printf("no Metal Gear saves\n");
	for (const TapeSave& save : saves) {
		printf("%-6s  offset %6zu%s\n", save.name.c_str(), save.offset,
			save.complete ? "" : "  (incomplete)");
	}
	return 0;
}
