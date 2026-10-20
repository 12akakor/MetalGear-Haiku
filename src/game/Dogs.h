/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_DOGS_H
#define MG_DOGS_H


// Dogs (logic/actors/dog.asm, dogbasement.asm, dogspawner.asm).

#include <stdint.h>


void		InitDog(uint16_t ix);
void		DogLogic(uint16_t ix);
void		InitDogBasement(uint16_t ix);
void		DogBasementLogic(uint16_t ix);
void		InitSpawnDog(uint16_t ix);
void		SpawnDogLogic(uint16_t ix);
// Speed by direction (DogSpeeds); Arnold uses it too.
void		SetChaseSpeed(uint16_t ix);


#endif	// MG_DOGS_H
