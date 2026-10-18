/*
 * Controls as the game reads them: the MSX keyboard matrix (11 rows, a bit
 * is 0 while the key is down) and joystick port 1 (bits 0-5 up, down,
 * left, right, trigger A, trigger B; 0 while pressed). The platform layer
 * fills these from the real keyboard and joystick.
 */
#ifndef MG_INPUT_H
#define MG_INPUT_H

#include <stdint.h>

extern uint8_t g_keyMatrix[11];
extern uint8_t g_joystick;

/* BIOS SNSMAT */
static inline uint8_t SNSMAT(int row) { return row >= 0 && row < 11 ? g_keyMatrix[row] : 0xff; }

uint8_t		ReadControls();
uint8_t		ReadFKeys();
/* StoreControls: hold at 'hold', newly pressed at hold-1; returns those. */
uint8_t		StoreControls(uint16_t hold, uint8_t value);

#endif
