/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_ROOM_ACTORS_H
#define MG_ROOM_ACTORS_H


// Actors drawn into the room: laser beams, cameras in an alert, mines,
// pitfalls, the Hind D and Metal Gear (logic/drawlaserbeams.asm,
// actors/rendercameras.asm, mine.asm, pitfall.asm, hindd.asm, BanksABC.asm,
// SetBossMusic of tank.asm).

#include <stdint.h>


void		DrawLaserBeams();
void		RestLasersBack();

void		RenderCamera(uint16_t ix);
void		DrawCamera(uint16_t ix);
void		DrawCameras();

void		InitMines(uint16_t ix);
void		DrawMine(uint16_t ix);
void		DrawMines();
// Mine background: returns the mine's top left corner (D = X, E = Y as
// the original's HL), *sx the X of its copy in page 1.
uint16_t	GetMineBackXY(uint16_t ix, uint8_t x, uint8_t y, uint8_t* sx);

void		InitPitfall(uint16_t ix);
void		PitfallLogic(uint16_t ix);
void		DrawOpenPitfalls();
void		RenderPitfallP0(uint16_t ix);

void		SetBossMusic();
void		InitHindD(uint16_t ix);
void		ChkDrawHindD();
void		HindDLogic(uint16_t ix);
void		RemoveHindD(uint16_t ix);
void		DummyLogic2(uint16_t ix);

void		InitMetalGear_(uint16_t ix);
void		ChkDrawMetalGear(uint16_t ix);
void		RemoveMetalGear(uint16_t ix);


#endif	// MG_ROOM_ACTORS_H
