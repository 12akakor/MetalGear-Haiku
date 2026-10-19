/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_ITEMS_H
#define MG_ITEMS_H


// Items lying in the rooms, picking them up, inventory limits (items,
// addroomitems, drawitemsinroom, spawnitem, shrinkitems, maxammo).

#include <stdint.h>


void		AddRoomItems();
// Adds the items of a list (ID, Y, X; #FF ends) to the free entries from
// 'entry'. Returns the entry after the last one added.
uint16_t	AddRoomItems2(uint16_t list, uint16_t entry);
void		DrawRoomItems();
void		DrawRoomItems2(uint16_t ix, uint8_t count, uint8_t index);
// Drops item 'drop' (0 ration, 1 ammunition, 2 card 7, 3 card 8,
// 4 suppressor) at x, y.
void		SpawnItem(uint8_t drop, uint8_t y, uint8_t x);

void		ChkTakeItems();
void		LimitAmmo();
void		SetMaxAmmoVals();
void		CompactWeapons();
void		CompactEquipment();


#endif	// MG_ITEMS_H
