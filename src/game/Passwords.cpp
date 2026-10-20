/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/passwords.asm. The keys typed during the pause go
// into a buffer of 12; F1 checks the last letters for the secret words.
#include "Passwords.h"

#include "Common.h"
#include "GfxBasic.h"
#include "Input.h"
#include "Player.h"
#include "Rom.h"
#include "Structures.h"


using namespace var;


namespace {

// The last letters typed match this word (stored ending with #FF).
bool
ChkPassword(uint16_t word)
{
	uint16_t hl = static_cast<uint16_t>(PasswordBuffer + 0x0b);
	for (;;) {
		uint8_t c = Mem(word++);
		if (c == 0xff)
			return true;
		if (c != Ram(hl))
			return false;
		hl--;
	}
}


void
IncClassLv_()
{
	IncClassLv();
	SetBanks_1_2_X();
}


void
AddCardToEquip(uint8_t card)
{
	uint16_t hl = Equipment;
	while (Ram(hl) != 0) {
		hl = static_cast<uint16_t>(hl + 4);
		SetBanks_1_2_X();	// ADD_HL_A_
	}
	Ram(hl) = card;
	Ram(static_cast<uint16_t>(hl + 1)) = static_cast<uint8_t>(card + 0x23);	// "1".."8"
	SetBanks_1_2_X();
	Ram(static_cast<uint16_t>(ItemsTaken + card - 1)) = 1;
}

}	// namespace


void
ChkCharTyped()
{
	ReadKeyboard();
	uint16_t hl = PassKeysTrigger;
	for (int b = 9; b > 0; b--, hl++) {
		uint8_t keys = Ram(hl);
		if (keys == 0)
			continue;
		uint8_t row = static_cast<uint8_t>(b - 1);
		uint8_t column = 0;
		while ((keys & 0x80) == 0) {
			keys = static_cast<uint8_t>(keys << 1);
			column++;
		}
		for (int i = 0; i < 0x0b; i++)
			Ram(static_cast<uint16_t>(PasswordBuffer + i))
				= Ram(static_cast<uint16_t>(PasswordBuffer + i + 1));
		Ram(static_cast<uint16_t>(PasswordBuffer + 0x0b)) = static_cast<uint8_t>(row * 8 + column);
		return;
	}
}


void
ChkPasswords()
{
	static const sym::Id kWords[] = {
		sym::PassDS_4, sym::PassANTA_WA, sym::PassINTRUDER, sym::PassISOLATION,
		sym::PassHIRAKE
	};
	int word = 0;
	while (!ChkPassword(RomSymbol(kWords[word]))) {
		if (++word == 5)
			return;
	}
	switch (word) {
		case 1:	// ANTA WA ERAI: three classes up
			IncClassLv_();
			IncClassLv_();
			IncClassLv_();
			return;
		case 2:	// INTRUDER: the most ammunition
			Ram(MaxAmmoRatioF) |= 1;
			for (int i = 0; i < 8; i++)
				SetRam16(static_cast<uint16_t>(MaxAmmoGun + i * 2), 0x0999);
			return;
		case 3:	// ISOLATION: the most rations
			Ram(MaxAmmoRatioF) |= 2;
			SetRam16(MaxRations, 0x0999);
			return;
		case 4:	// HIRAKE GOMA: all cards
		{
			for (uint8_t card = SELECTED_CARD1; card < SELECTED_CARD1 + 8; card++) {
				uint16_t address;
				uint8_t index;
				bool found = GetItemInvAdd(card, &address, &index);
				SetBanks_1_2_X();
				if (!found)
					AddCardToEquip(card);
			}
			Ram(static_cast<uint16_t>(DoorOpenArray + 0x0b)) = 0;	// Grey Fox's cell
			return;
		}
		default:	// DS 4: (!?) DJNZ from 0 ends in one class up
			IncClassLv_();
			return;
	}
}
