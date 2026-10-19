/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_WEAPONS_H
#define MG_WEAPONS_H


// Snake's weapons: firing, the shots' logic and sprites, and the alert
// that noise triggers (weaponuse.asm, weapon/*, checkweaponalert.asm,
// setalert.asm). Shots are 6 structures of #40 bytes from
// PlayerShotsList (PLAYER_SHOT).

#include <stdint.h>


void		ChkWeaponShot();
void		PlayerShotsLogic();
void		SetPlayerShotSpr();
void		RemoveShot(uint16_t ix);

// 'ix' is the actor (or shot) triggering it: cameras and lasers play
// the red alert music.
void		SetAlertModeRespawn(uint16_t ix, uint8_t timer);
void		SetAlertMode(uint16_t ix);
void		ChkAlertTrigger(uint16_t ix);


#endif	// MG_WEAPONS_H
