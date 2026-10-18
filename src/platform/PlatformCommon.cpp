/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Platform code shared by all back ends: key state, event queue, palette.
#include <mutex>
#include <string.h>

#include "PlatformInternal.h"
#include "VideoScaler.h"


namespace {

std::mutex sLock;
unsigned char sKeys[MOGK_LAST];
bool sOverride = false;

const int kQueueSize = 64;
PlatEvent sQueue[kQueueSize];
int sQueueHead = 0;
int sQueueCount = 0;

uint8_t sPalette[768];
Scaler* sScaler = NULL;
int sSourceFactor = 1;
uint32_t sLut[256];

}	// namespace


void
PlatInternalSetKey(MOGKey key, bool down)
{
	if (key <= 0 || key >= MOGK_LAST || sOverride)
		return;
	sKeys[key] = down ? 1 : 0;
}


void
PlatInternalClearKeys(void)
{
	if (!sOverride)
		memset(sKeys, 0, sizeof(sKeys));
}


void
PlatInternalPushEvent(int type, MOGKey key, int modifiers)
{
	std::lock_guard<std::mutex> guard(sLock);
	if (sQueueCount == kQueueSize)
		return;
	PlatEvent& event = sQueue[(sQueueHead + sQueueCount) % kQueueSize];
	event.type = type;
	event.key = key;
	event.modifiers = modifiers;
	sQueueCount++;
}


bool
PlatInternalPopEvent(PlatEvent* event)
{
	std::lock_guard<std::mutex> guard(sLock);
	if (sQueueCount == 0)
		return false;
	*event = sQueue[sQueueHead];
	sQueueHead = (sQueueHead + 1) % kQueueSize;
	sQueueCount--;
	return true;
}


bool
PlatInternalInputOverride(void)
{
	return sOverride;
}


void
PlatSetInputOverride(bool enabled)
{
	sOverride = enabled;
}


unsigned char*
PlatKeyState(void)
{
	return sKeys;
}


void
PlatSetPalette(const uint8_t* rgb)
{
	memcpy(sPalette, rgb, sizeof(sPalette));
	for (int i = 0; i < 256; i++) {
		sLut[i] = 0xff000000u | (static_cast<uint32_t>(rgb[i * 3]) << 16)
			| (static_cast<uint32_t>(rgb[i * 3 + 1]) << 8) | rgb[i * 3 + 2];
	}
}


void
PlatGetPalette(uint8_t* rgb)
{
	memcpy(rgb, sPalette, sizeof(sPalette));
}


void
PlatInternalConvert(const uint8_t* src, uint32_t* dst, int count)
{
	for (int i = 0; i < count; i++)
		dst[i] = sLut[src[i]];
}


int
PlatModifiers(void)
{
	int modifiers = 0;
	if ((sKeys[MOGK_LSHIFT] != 0u) || (sKeys[MOGK_RSHIFT] != 0u))
		modifiers |= PLAT_MOD_SHIFT;
	if ((sKeys[MOGK_LCTRL] != 0u) || (sKeys[MOGK_RCTRL] != 0u))
		modifiers |= PLAT_MOD_CTRL;
	if ((sKeys[MOGK_LALT] != 0u) || (sKeys[MOGK_RALT] != 0u))
		modifiers |= PLAT_MOD_ALT;
	if ((sKeys[MOGK_LMETA] != 0u) || (sKeys[MOGK_RMETA] != 0u) || (sKeys[MOGK_LSUPER] != 0u)
		|| (sKeys[MOGK_RSUPER] != 0u))
		modifiers |= PLAT_MOD_META;
	return modifiers;
}


void
PlatSetScaler(const char* name)
{
	sScaler = ScalerForName(name);
	PlatBackendScalerChanged();
}


void
PlatNextScaler(void)
{
	sScaler = ScalerNext(PlatInternalScaler());
	PlatBackendScalerChanged();
}


const char*
PlatScalerName(void)
{
	return PlatInternalScaler()->Name();
}


Scaler*
PlatInternalScaler(void)
{
	if (sScaler == NULL)
		sScaler = ScalerForName("xbr");
	return sScaler;
}


void
PlatSetSourceFactor(int factor)
{
	sSourceFactor = factor == 2 ? 2 : 1;
}


int
PlatInternalSourceFactor(void)
{
	return sSourceFactor;
}
