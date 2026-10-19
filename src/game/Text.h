/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_TEXT_H
#define MG_TEXT_H


// Text windows (messages, radio conversations, the staff roll): the
// window grows, the text is typed character by character, pages wait for
// a key or a time (Banks0123.asm, logic/textboxappear.asm).

#include <stdint.h>


// Starts a text: skippable, not skippable (key for the next page), or not
// skippable (pages follow by themselves).
void		SetText(uint8_t id);
void		SetTextUnskip2(uint8_t id);
void		SetTextUnskippable(uint8_t id);
void		TextBoxLogic();

uint8_t		GetText();
// Decodes one page into TextBuffer: returns 1 if more pages follow;
// *source and *dest as the original leaves HL and DE.
uint8_t		DecodeText(uint16_t* source, uint16_t* dest);
// True when the window is complete.
bool		DrawTextBoxIn();
void		DrawSnakeFrame();
void		DrawSnakeFrame1();
void		TextBoxExit();
// Window position and size: *xy (Y, X), *size (NY, NX).
void		GetTextBoxXYSize(uint16_t* xy, uint16_t* size);


#endif	// MG_TEXT_H
