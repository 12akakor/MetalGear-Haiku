/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_ACTORS_H
#define MG_ACTORS_H


// The actors (guards, cameras, bosses, shots...): 16 structures of #80
// bytes from EnemyList (ACTOR in Structures.h). Routines take the actor's
// address (the original's IX). Banks0123.asm and logic/actors.

#include <stdint.h>


// Setting up the actors of a room
void		SetupEnemyRoom();
// Adds an actor (ID, Y, X); returns its structure (as the original's HL).
uint16_t	AddEnemy(uint8_t id, uint8_t y, uint8_t x);
uint8_t		CountEnemyType(uint8_t id);
void		ChkRespawnEnemy();
void		ChkAlarmEnd();
void		StopAlert();
void		TransformAlertGuard(uint16_t ix);
void		TransformAlertGuard2(uint16_t ix, uint8_t id);

// Movement along paths
void		InitGuardFast(uint16_t ix);
void		InitGuardMedium(uint16_t ix);
void		InitGuardSlow(uint16_t ix);
void		InitGuard(uint16_t ix, uint8_t speed);
void		InitGuardPath(uint16_t ix);
void		InitGuardPath2(uint16_t ix, uint8_t index);
void		GetPathPoints(uint16_t ix, uint8_t index);
void		GetPathPoint(uint16_t ix, uint16_t point);
void		GetPathPoint_(uint16_t ix, uint16_t point);
void		SetDirToPoint(uint16_t ix);
// True when the original drops its caller (the actor waits at the point).
bool		UpdateActorPath(uint16_t ix);
void		GetSentinelLookDirs(uint16_t ix);
void		SetSentinelLookDir(uint16_t ix, uint16_t pointer);
void		SetSentinelLookDir_(uint16_t ix, uint16_t pointer);
uint8_t		HideGuardRoom39(uint16_t ix);

// Speeds (8.8 fixed point) and animation
void		SetActorSpeedY(uint16_t ix, uint16_t speed);
void		SetActorSpeedX(uint16_t ix, uint16_t speed);
uint16_t	AddActorSpeedX(uint16_t ix, uint16_t speed);
void		SetActorSpeed(uint16_t ix, uint16_t speedX, uint16_t speedY);
void		ResetActorSpeed(uint16_t ix);
void		Anim2FramesActor(uint16_t ix, uint8_t mask);
void		AnimZzzSign(uint16_t ix, uint8_t mask, uint8_t firstSprite);
void		NextActorStatus(uint16_t ix);
void		ResetActorStatus(uint16_t ix);
void		SetActorStatus(uint16_t ix, uint8_t status);
void		ChangeGuardSprDir(uint16_t ix);

// Directions: returns the main direction to the player, *other the second.
uint8_t		GetDirToPlayer(uint16_t ix, uint8_t* other);
uint8_t		GetOppositeDir(uint8_t direction);

// Shots aimed at the player
void		GetSinCos(uint8_t angle, uint8_t* sine, uint8_t* cosine);
void		GetFlameSinCos(uint16_t ix, uint8_t* sine, uint8_t* cosine);
uint16_t	ChangeSignHL(uint16_t value);
// Speeds towards the player: returns speed X, *speedY.
uint16_t	CalcShot(uint16_t ix, uint16_t* speedY);
uint16_t	CalcShot2(uint16_t ix, uint8_t speed, uint16_t* speedY);
void		CalcQuadrantDegree(uint8_t x, uint8_t y);
uint16_t	CalShootSpeed(uint8_t factor);
uint16_t	Multiply8(uint8_t a, uint8_t b);

void		InitDummy(uint16_t ix);
// Actor logic (Enemies.cpp)
void		EnemiesLogic();
// The actor EnemiesLogic goes on with after the logic of the current one
// (the original's IX, which some logic leaves on another actor).
extern uint16_t gEnemyIX;
// Set by logic that drops the return address of RunEnemyLogic: the
// original then returns to an address in the BIOS (a RET there) and the
// other actors are skipped this tick.
extern bool gEnemiesLogicAbort;
void		RunEnemyLogic(uint16_t ix);
void		ChkEraseAlertSign();
void		StunnedBounce(uint16_t ix);
void		MoveActor(uint16_t ix);
void		ChkActorExitRoom(uint16_t ix);
void		DismissActor0(uint16_t ix);
void		DismissActor(uint16_t ix);
void		RemoveActor_(uint16_t ix);
void		SpawnItem_(uint8_t item, uint8_t y, uint8_t x);
void		RestoreBackMine(uint16_t ix);
void		UpdateEnemySprStr();
void		KillActor(uint16_t ix);
void		BulletLogic(uint16_t ix);
void		EraseBitmapActor(uint16_t ix);
void		ErasePowerSw(uint16_t ix);
void		ChkDrawDestroyPS(uint16_t ix);
void		RestLaserBack_();
void		DrawMovingLasers(uint16_t ix);
void		InitMetalGear(uint16_t ix);
void		InitLaserRoom(uint16_t ix);

// Not rewritten yet (Unported.cpp)
void		InitEllenVoice(uint16_t ix);
void		InitBigBoss(uint16_t ix);
void		InitCowardDuck(uint16_t ix);
void		InitCaptureScene(uint16_t ix);
void		InitPrisoner(uint16_t ix);
void		InitGuardSilencer(uint16_t ix);
void		InitBoomerang(uint16_t ix);
void		ChkSayHelpMe(uint16_t ix);
void		BigBossLogic(uint16_t ix);
void		CowardDuckLogic(uint16_t ix);
void		CaptureGuardsLogic(uint16_t ix);
void		PrisonerLogic(uint16_t ix);
void		FakeMadnadLogic(uint16_t ix);
void		GuardSilencerLogic(uint16_t ix);
void		BoomerangLogic(uint16_t ix);


#endif	// MG_ACTORS_H
