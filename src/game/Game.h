/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_GAME_H
#define MG_GAME_H


// The game itself: statuses 2-8 (demo, play start, start, playing, game
// over, pause, ending), starting a game, entering a room, the game modes
// of every tick, binoculars (Banks0123.asm).

#include <stdint.h>


void		GS_DemoPlay(uint8_t substatus);
void		GS_PlayStart(uint8_t substatus);
void		GS_StartGame();
void		GS_Playing();
void		GS_GameOver(uint8_t substatus);
void		GS_Pause();
void		GS_Ending(uint8_t substatus);
void		SetGameStatus(uint8_t status);
void		ResetGameStat();
void		PrintPause();
void		UpdateControls();
void		ClearGameVars();

void		InitGame();
void		InitGame4();
void		NextRoomLogic();
void		GameLogic();
void		RefreshSprites();
void		BinocularLogic();


#endif	// MG_GAME_H
