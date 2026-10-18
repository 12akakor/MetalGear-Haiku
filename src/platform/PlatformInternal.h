/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MOG_PLATFORM_INTERNAL_H
#define MOG_PLATFORM_INTERNAL_H


// Shared helpers for the platform back ends (not used by the game).

#include "Platform.h"


// Key state and events. Thread safe.
void PlatInternalSetKey(MOGKey key, bool down);
void PlatInternalClearKeys(void);
void PlatInternalPushEvent(int type, MOGKey key, int modifiers);
bool PlatInternalPopEvent(PlatEvent* event);
bool PlatInternalInputOverride(void);

// Current scaler and source factor (see PlatSetScaler()).
class Scaler;
Scaler* PlatInternalScaler(void);
int PlatInternalSourceFactor(void);

// Implemented by the back end: called after the scaler changed (e.g. to show
// its name in the window title).
void PlatBackendScalerChanged(void);

// Palette -> 32 bit pixels (0xffRRGGBB, matches B_RGB32 on little endian).
void PlatInternalConvert(const uint8_t* src, uint32_t* dst, int count);


#endif	// MOG_PLATFORM_INTERNAL_H
