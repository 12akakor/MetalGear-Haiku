/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_EVENTS_H
#define MG_EVENTS_H


// Room events: lorries that drive off with Snake inside, Madnar moved,
// Big Boss's order to switch the MSX off (lorry.asm,
// madnarbigbossevent.asm).

void		ChkLorryMov();
void		LorryMoving();
void		ChkMadnarMsx();
void		chkMadnarLate();


#endif	// MG_EVENTS_H
