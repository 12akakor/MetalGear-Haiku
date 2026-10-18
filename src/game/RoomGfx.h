/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_ROOM_GFX_H
#define MG_ROOM_GFX_H


// Loading and drawing rooms (Banks0123.asm: LoadRoomTiles, SetRoomPal,
// RenderRoom and helpers).

#include <stdint.h>


void	LoadRoomTiles();
void	LoadPowSwitTiles();
void	LoadGfxCrates();
void	SetRoomPal();
void	SetRoomPal2(uint8_t palette);
void	UnpackMetatiles(uint16_t buffer, uint16_t map);
void	RenderRoom();


#endif	// MG_ROOM_GFX_H
