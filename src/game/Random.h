/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_RANDOM_H
#define MG_RANDOM_H


// The original takes its random numbers from the Z80 refresh register
// (LD A,R), which counts instruction fetches. RefreshR gives a value in
// the same range (0-127). Tests set gRefreshHook to the values the
// original read, so both sides make the same choices.

#include <stdint.h>

#include <functional>


extern std::function<uint8_t()> gRefreshHook;

uint8_t		RefreshR();


#endif	// MG_RANDOM_H
