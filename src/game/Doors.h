/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_DOORS_H
#define MG_DOORS_H


// Doors and breakable walls: drawing, opening (cards, punches, bombs,
// events), the opening animation, walking through (logic/doors).

#include <stdint.h>


void		DrawDoors();
void		ChkDoors();
// True when the door at 'logic' (DoorsList entry + 2) opens now.
bool		ChkOpenDoor(uint16_t logic);
// True when Snake is inside the area at 'area' (Y, NY, X, NX).
bool		ChkTouchDoor2(uint16_t area);
void		EraseDoorLogic();
uint16_t	GetOpeningDoorDat();


#endif	// MG_DOORS_H
