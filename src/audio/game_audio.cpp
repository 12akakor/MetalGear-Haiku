#include "game_audio.h"

#include "../game/game_ram.h"

#include <math.h>


GameAudio::GameAudio()
	:
	musicGain(1.0f),
	effectsGain(0.55f),
	classicGain(0.75f),
	fDriver(g_soundDriver),
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
	fMusic.AllNotesOff();
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
		/* Tone generator only for what is not music. */
		int mask = 0;
		for (int a = 0; a < 3; a++) {
			uint16_t area = (uint16_t)(0xc010 + a * 0x20);
			/* Not music, or music data in sound effect mode (the
			   siren of Red Alert, for example). */
			if (!MusicRenderer::IsMusic(ram((uint16_t)(area + 2)))
				|| ram((uint16_t)(area + 14)) == 0)
				mask |= 1 << a;
		}
		if (ram(0xc072) != 0)
			mask |= 4;			/* sound effect on voice 3 */
		fSynth.SetVoiceMask(mask);
	} else
		fSynth.SetVoiceMask(7);

	fOwed += fRate / kTickRate;
	int count = (int)fOwed;
	fOwed -= count;
	fMix.assign((size_t)count * 2, 0.0f);
	if (enhanced) {
		fMusic.Render(fMix.data(), count, musicGain);
		fSynth.Render(fMix.data(), count, fRate, effectsGain);
	} else
		fSynth.Render(fMix.data(), count, fRate, classicGain);

	size_t old = out.size();
	out.resize(old + (size_t)count * 2);
	for (int i = 0; i < count * 2; i++) {
		float v = fMix[i];
		/* soft limit */
		if (v > 0.9f || v < -0.9f)
			v = (float)tanh(v);
		out[old + i] = (int16_t)lrintf(v * 32000.0f);
	}
}
