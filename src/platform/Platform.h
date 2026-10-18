/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MOG_PLATFORM_H
#define MOG_PLATFORM_H


// Platform layer (taken from the Maze of Galious port).
//
// The game code only talks to this interface. Back end:
// platform_haiku.cpp  native Haiku / RenkuOS build (Be API)
//
// The back end owns main(). It initialises itself and then calls GameMain()
// on the thread that runs the game.

#include <stdint.h>

#include "Keys.h"


// Implemented by the game (main.cpp).
int GameMain(int argc, char** argv);


// Lifecycle -------------------------------------------------------------

// Creates the window (width x height logical pixels, 8-bit indexed).
bool PlatInit(int width, int height, bool fullscreen, const char* title);
void PlatShutdown(void);
const char* PlatName(void);


// Video -----------------------------------------------------------------

// 256 entries, 3 bytes each (R, G, B).
void PlatSetPalette(const uint8_t* rgb);
void PlatGetPalette(uint8_t* rgb);

// Shows an 8-bit indexed frame of the size given to PlatInit().
void PlatPresent(const uint8_t* pixels);

bool PlatSetFullscreen(bool fullscreen);
bool PlatIsFullscreen(void);

// Scaling filter: "xbr" (default), "sharp" or "nearest".
void PlatSetScaler(const char* name);
void PlatNextScaler(void);
const char* PlatScalerName(void);

// Timing display (F4): filter and drawing time per frame, frames per
// second.
void PlatToggleStats(void);

// Saves the picture as shown (filter applied, without black borders) as a
// PNG file in the home folder, in the background.
void PlatSaveScreenshot(void);

// Wait for the screen refresh before drawing (less tearing). Only works
// with graphics drivers that report the refresh; ignored otherwise.
void PlatSetVsync(bool enabled);
bool PlatVsync(void);

// Windowed placement, remembered between runs. PlatSetSavedWindow() is
// called before PlatInit(); PlatGetWindow() returns false when there is
// no window.
void PlatSetSavedWindow(int x, int y, int width, int height);
bool PlatGetWindow(int* x, int* y, int* width, int* height);

// 2 when the current graphics set is 320x200 art stored with doubled pixels
// (the "original" set), 1 otherwise. Used by the scaler.
void PlatSetSourceFactor(int factor);


// Shows a message box and waits until it is closed.
void PlatAlert(const char* title, const char* text);


// Time ------------------------------------------------------------------

uint32_t PlatTicksMs(void);
void PlatSleepMs(uint32_t ms);


// Input -----------------------------------------------------------------

enum {
	PLAT_EV_NONE = 0,
	PLAT_EV_KEYDOWN,
	PLAT_EV_QUIT
};

enum {
	PLAT_MOD_SHIFT = 1,
	PLAT_MOD_CTRL = 2,
	PLAT_MOD_ALT = 4,
	PLAT_MOD_META = 8
};

struct PlatEvent {
	int		type;
	MOGKey	key;
	int		modifiers;
};

// Returns false when there are no more events.
bool PlatPollEvent(PlatEvent* event);

// Refreshes the key state array returned by PlatKeyState().
void PlatPumpEvents(void);

// MOGK_LAST entries, nonzero when the key is held.
unsigned char* PlatKeyState(void);
int PlatModifiers(void);

// While enabled, the back end leaves the key state array alone (used by the
// deterministic test driver, which writes the key state itself).
void PlatSetInputOverride(bool enabled);

// First joystick or game pad: bits 0-5 up, down, left, right, button 1,
// button 2 (1 while pressed). 0 when there is none.
uint8_t PlatJoystick(void);

// Same key names as the original game used (SDL 1.2 names).
const char* PlatKeyName(MOGKey key);


// Files -----------------------------------------------------------------

// Directory that contains graphics/, sound/ and rooms/. The back end makes it
// the current directory before GameMain() runs.
const char* PlatDataDir(void);

// Directory of the program itself (bundled data lives in its "data"
// subdirectory).
const char* PlatAppDir(void);

// Writable directory for MoG.cfg and other user files (created if needed).
// Both can be overridden with the environment variables MOG_DATA and
// MOG_SETTINGS (used by the tests).
const char* PlatSettingsDir(void);


// Audio -----------------------------------------------------------------

// Called from the audio thread: fill 'frames' stereo frames (interleaved
// int16, 44100 Hz).
typedef void (*PlatAudioCallback)(int16_t* out, int frames, void* cookie);

bool PlatAudioOpen(int rate, PlatAudioCallback callback, void* cookie);
void PlatAudioClose(void);

// Protects data shared with the audio callback.
void PlatAudioLock(void);
void PlatAudioUnlock(void);


#endif	// MOG_PLATFORM_H
