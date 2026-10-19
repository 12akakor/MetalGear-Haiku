/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_UNPORTED_H
#define MG_UNPORTED_H

#include <stdint.h>


// Routines without arguments that are not rewritten yet (Unported.cpp).

void		DrawOpenPitfalls();
void		IntroSceneLogic();
void		ChkSaveGameStatus();
void		SetMaxAmmoVals();
void		LimitAmmo();
void		CompactWeapons();
void		DrawDoors();
void		DrawRoomItems();
void		DrawLaserBeams();
void		ChkDrawMetalGear();
void		DrawMines();
void		ChkDrawHindD();
void		DrawCameras();
void		SetBulldozerKO();
void		RestLasersBack();
// Mine background: returns DE (destination), *sx.
uint16_t	GetMineBackXY(uint8_t x, uint8_t y, uint8_t* sx);


#endif	// MG_UNPORTED_H
