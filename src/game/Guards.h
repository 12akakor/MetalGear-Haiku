/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_GUARDS_H
#define MG_GUARDS_H


// Guards on patrol, what guards and cameras see, raising the alarm
// (logic/actors/guard.asm, chkdiscover.asm, guardlorry.asm, guardalert.asm,
// sentinel.asm, elevatorguardspawner.asm and the alert icon helpers of
// guardelevator.asm).

#include <stdint.h>


void		GuardLogic(uint16_t ix);
// At the destination point: on to the next one. True when the original
// drops its caller (a guard stops there).
bool		ChkReachPoint(uint16_t ix);
void		InitGuardLorry(uint16_t ix);
void		GuardLorryLogic(uint16_t ix);

void		InitSentinel(uint16_t ix);
void		SentinelLogic(uint16_t ix);
void		InitSpawnGuardElev(uint16_t ix);
void		SpawnGuardElev(uint16_t ix);

// Does the guard or camera see the player?
void		ChkActSeePlayer(uint16_t ix);
void		ChkSeePlayer(uint16_t ix, uint8_t direction, uint8_t x, uint8_t y);
void		GuardSetAlarm(uint16_t ix);
void		SetAlert(uint16_t ix);
void		SetAlertRoom();
// True (carry) when the player touches the actor or makes noise.
bool		ListenShotsChkTouch(uint16_t ix);

// Guards in an alert (GuardAlert.cpp)
void		InitGuardAlert(uint16_t ix);
void		GuardAlertLogic(uint16_t ix);
void		SetWalkSpeed(uint16_t ix);
void		SetWalkSpeedFast(uint16_t ix);
void		SetWalkSpeedFast2(uint16_t ix, uint8_t direction);
void		AnimateGuard2(uint16_t ix, uint8_t direction);
void		GuardLookDirection(uint16_t ix);
void		GuardLookDirection2(uint16_t ix, uint8_t direction);
// The larger of the X and Y distances to the player.
uint8_t		GetDistancePlayer(uint16_t ix);
uint8_t		GetRandom3(uint16_t ix);
// Rooms where the guards of an alert keep coming back.
void		SetRespawnTime();
// A shot from 16 pixels above the actor.
void		AddEnemyShot2(uint16_t ix, uint8_t id);

void		SaveAlertIconBacknd(uint8_t sx, uint8_t sy);
void		Draw16x16(uint8_t sx, uint8_t sy, uint8_t dx, uint8_t dy);


#endif	// MG_GUARDS_H
