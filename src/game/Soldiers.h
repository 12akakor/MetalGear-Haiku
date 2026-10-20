/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_SOLDIERS_H
#define MG_SOLDIERS_H


// Soldiers with their own behaviour (logic/actors/shooter.asm,
// jetpack.asm, ...).

#include <stdint.h>


void		InitShooter(uint16_t ix);
void		ShooterLogic(uint16_t ix);

void		InitJetpackTakeoff(uint16_t ix);
void		JetpacTakeoff(uint16_t ix);
void		InitJetpackSwitch(uint16_t ix);
void		JetpackSwitchLogic(uint16_t ix);
void		InitJetpack(uint16_t ix);
void		JetpackLogic(uint16_t ix);


#endif	// MG_SOLDIERS_H
