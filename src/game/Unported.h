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
void		SetupDemoPlay();
void		DemoControler();
void		GameDemoLogic();
void		ChkPasswords();
void		ChkCharTyped();
void		EndingSetup();
void		EndingLogic();
void		RestoreGameStat();
void		ChkLorryMov();
void		ChkMadnarMsx();
void		AddRoomItems();
void		SetElevatorSpr();
void		SetPlayerShotSpr();
void		ChkIncomingCall();
void		DecNukeTimer();
void		ChkWeaponShot();
void		PlayerShotsLogic();
void		CommonLogic();
void		MenuEquip();
void		LorryMoving();
void		ElevatorRoomLogic();
void		EraseDoorLogic();
void		CaptureSceneLogic();
void		chkMadnarLate();


#endif	// MG_UNPORTED_H
