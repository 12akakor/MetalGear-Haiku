/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_HAZARDS_H
#define MG_HAZARDS_H


// Moving bridges, rolling barrels and the bulldozer
// (logic/actors/bridge.asm, rollingbarrels.asm, bulldozer.asm).

#include <stdint.h>


void		InitBridge(uint16_t ix);
void		BridgeLogic(uint16_t ix);
void		InitRollingBarrel(uint16_t ix);
void		RollingBarrelLogic(uint16_t ix);
void		InitBulldozer(uint16_t ix);
void		BulldozerLogic(uint16_t ix);
void		SetBulldozerKO();


#endif	// MG_HAZARDS_H
