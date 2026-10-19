/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_ROUTINE_DIFF_H
#define MG_ROUTINE_DIFF_H


// Routines checked by routine_diff: the original routine (by symbol) and
// the rewritten one, called with the registers the original was entered
// with. 'out' starts as a copy of the entry registers; the native side
// sets the registers the routine returns, listed in 'results' (a, f-carry
// "cf", f-zero "zf", b, c, d, e, h, l, bc, de, hl, ix, iy).

#include <functional>
#include <vector>

#include "../../src/game/RomSymbols.h"
#include "../../src/z80/Z80.h"


struct Routine {
	sym::Id		id;
	const char*	name;
	const char*	results;
	std::function<void(const Z80Regs& in, Z80Regs& out)> native;
};

std::vector<Routine>& Routines();

struct RoutineAdder {
	RoutineAdder(sym::Id id, const char* name, const char* results,
		std::function<void(const Z80Regs&, Z80Regs&)> native)
	{
		Routines().push_back({id, name, results, native});
	}
};

#define ROUTINE_CAT2(a, b) a##b
#define ROUTINE_CAT(a, b) ROUTINE_CAT2(a, b)
// ROUTINE(Name, "results") { body using in, out }
#define ROUTINE(name, results) \
	static void ROUTINE_CAT(Native_, name)(const Z80Regs& in, Z80Regs& out); \
	static RoutineAdder ROUTINE_CAT(sAdd_, name)(sym::name, #name, results, \
		ROUTINE_CAT(Native_, name)); \
	static void ROUTINE_CAT(Native_, name)(const Z80Regs& in, Z80Regs& out)


#endif	// MG_ROUTINE_DIFF_H
