/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_PRISONERS_H
#define MG_PRISONERS_H


// Prisoners, Ellen's voice and the fake Madnar
// (logic/actors/prisoner.asm, fakemadnar.asm).

#include <stdint.h>


void		InitPrisoner(uint16_t ix);
void		PrisonerLogic(uint16_t ix);
void		InitEllenVoice(uint16_t ix);
void		ChkSayHelpMe(uint16_t ix);
void		FakeMadnadLogic(uint16_t ix);


#endif	// MG_PRISONERS_H
