/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "GameAudio.h"

#include <math.h>

#include "../game/GameRam.h"


GameAudio::GameAudio()
	:
	fMusicGain(1.0f),
	fEffectsGain(0.55f),
	fClassicGain(0.75f),
	fDriver(gSoundDriver),
	fMode(kEnhanced),
	fRate(44100),
	fOwed(0)
{
	fDriver.SetListener(this);
	SetRate(44100);
}


void
GameAudio::SetRate(int rate)
{
	fRate = rate;
	fMusic.SetRate(rate);
}


void
GameAudio::SetMode(Mode mode)
{
	fMode = mode;
	fSynth.SetScc(mode == kScc);
	fMusic.AllNotesOff();
}


void
GameAudio::SetStereoWidth(float width)
{
	fSynth.SetStereoWidth(width);
}


void
GameAudio::SoundRegister(int reg, uint8_t value)
{
	fSynth.Write(reg, value);
}


void
GameAudio::NoteStart(int area, int note, int octave, int instrument)
{
	if (fMode == kEnhanced)
		fMusic.NoteStart(area, note, octave, instrument);
}


void
GameAudio::Tick(std::vector<int16_t>& out, bool updateDriver)
{
	if (updateDriver)
		fDriver.Update();

	bool enhanced = fMode == kEnhanced && fMusic.Ready();
	if (enhanced) {
		fMusic.Tick();
		// Tone generator only for what is not music.
		int mask = 0;
		for (int a = 0; a < 3; a++) {
			uint16_t area = static_cast<uint16_t>(0xc010 + a * 0x20);
			// Not music, or music data in sound effect mode (the
			// siren of Red Alert, for example).
			if (!MusicRenderer::IsMusic(Ram(static_cast<uint16_t>(area + 2)))
				|| Ram(static_cast<uint16_t>(area + 14)) == 0)
				mask |= 1 << a;
		}
		if (Ram(0xc072) != 0)
			mask |= 4;	// sound effect on voice 3
		fSynth.SetVoiceMask(mask);
	} else
		fSynth.SetVoiceMask(7);

	fOwed += fRate / kTickRate;
	int count = static_cast<int>(fOwed);
	fOwed -= count;
	fMix.assign(static_cast<size_t>(count) * 2, 0.0f);
	if (enhanced) {
		fMusic.Render(fMix.data(), count, fMusicGain);
		fSynth.Render(fMix.data(), count, fRate, fEffectsGain);
	} else
		fSynth.Render(fMix.data(), count, fRate, fClassicGain);

	size_t old = out.size();
	out.resize(old + static_cast<size_t>(count) * 2);
	for (int i = 0; i < count * 2; i++) {
		float v = fMix[i];
		// soft limit
		if (v > 0.9f || v < -0.9f)
			v = tanh(v);
		out[old + i] = static_cast<int16_t>(lrintf(v * 32000.0f));
	}
}
