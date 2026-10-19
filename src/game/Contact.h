/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_CONTACT_H
#define MG_CONTACT_H


// What touches what each frame: Snake's shots and the actors, Snake and
// the actors (punches, damage, lasers, pitfalls, bridges), the floors,
// doors and items; the radio call and destruction timers (common.asm and
// the files it includes, incomingcall.asm, destructiontimer.asm).

#include <stdint.h>


void		CommonLogic();
void		ChkPlayerShots();
void		ChkTouchEnemies();
void		ChkOnBridge();
void		ChkElectricFloor();
void		RemoveLaserBeans();

void		ChkIncomingCall();
void		DecNukeTimer();


#endif	// MG_CONTACT_H
