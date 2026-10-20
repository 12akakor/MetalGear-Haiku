/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_BOSSES_H
#define MG_BOSSES_H


// Bosses and other big enemies (logic/actors/tank.asm, arnold.asm, ...).

#include <stdint.h>


void		InitTank(uint16_t ix);
void		TankLogic(uint16_t ix);
void		InitArnold(uint16_t ix);
void		ArnoldLogic(uint16_t ix);


#endif	// MG_BOSSES_H
