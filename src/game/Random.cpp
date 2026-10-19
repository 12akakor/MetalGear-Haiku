/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Random.h"

#include "GameRam.h"
#include "RamSymbols.h"


std::function<uint8_t()> gRefreshHook;

static uint32_t sState = 0x2545f491;


uint8_t
RefreshR()
{
	if (gRefreshHook)
		return gRefreshHook();
	// xorshift, stirred with the tick counter
	sState ^= sState << 13;
	sState ^= sState >> 17;
	sState ^= sState << 5;
	sState += Ram(var::TickCounter);
	return static_cast<uint8_t>((sState >> 9) & 0x7f);
}
