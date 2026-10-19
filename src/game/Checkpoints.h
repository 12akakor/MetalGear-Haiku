/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_CHECKPOINTS_H
#define MG_CHECKPOINTS_H


// Checkpoints: going from some rooms to others saves the progress, the
// continue after a game over restores it (checkpoints.asm).

void		ChkSaveGameStatus();
void		RestoreGameStat();


#endif	// MG_CHECKPOINTS_H
