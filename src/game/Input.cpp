/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Controls, rewritten from logic/controls.asm.
#include "Input.h"

#include "Rom.h"


uint8_t gKeyMatrix[11] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
uint8_t gJoystick = 0x3f;


uint8_t
StoreControls(uint16_t hold, uint8_t value)
{
	uint8_t old = Ram(hold);
	Ram(hold) = value;
	uint8_t trigger = static_cast<uint8_t>((value ^ old) & value);
	Ram(static_cast<uint16_t>(hold - 1)) = trigger;
	return trigger;
}


// Bits: 5 fire 2 (M, N), 4 fire (space), 3 right, 2 left, 1 down, 0 up.
uint8_t
ReadControls()
{
	uint8_t joystick = static_cast<uint8_t>(~gJoystick & 0x3f);
	uint8_t row4 = SNSMAT(4);
	uint8_t e = (((row4 & 0x04) == 0) || ((row4 & 0x08) == 0)) ? 0x20 : 0;	// M, N
	uint8_t row8 = static_cast<uint8_t>(~SNSMAT(8));
	uint8_t c = static_cast<uint8_t>((((row8 >> 4) & 1) << 2) | e);	// left
	c |= static_cast<uint8_t>((row8 & 1) << 4);	// space
	c |= static_cast<uint8_t>(((row8 >> 7) & 1) << 3);	// right
	c |= static_cast<uint8_t>((row8 >> 5) & 3);	// up, down
	return static_cast<uint8_t>(c | joystick);
}


// Bits: 5 return, 4 F5, 3 F4, 2 F3, 1 F2, 0 F1.
uint8_t
ReadFKeys()
{
	uint8_t row6 = static_cast<uint8_t>(~SNSMAT(6));
	uint8_t row7 = static_cast<uint8_t>(~SNSMAT(7));
	uint8_t a = static_cast<uint8_t>((row6 >> 5) & 7);
	a |= static_cast<uint8_t>((row7 & 3) << 3);
	a |= static_cast<uint8_t>((row7 >> 2) & 0x20);
	return a;
}
