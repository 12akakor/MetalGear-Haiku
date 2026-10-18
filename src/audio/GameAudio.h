/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_GAME_AUDIO_H
#define MG_GAME_AUDIO_H


// Sound of the game: the driver plus the renderers.
//
// kClassic   original sound (tone generator for everything)
// kEnhanced  music on SoundFont instruments with reverb; the sound
// effects keep their original character on the tone
// generator, and the third music voice keeps playing while
// an effect sounds (the original had to mute it).

#include <stdint.h>
#include <vector>

#include "../game/SoundDriver.h"
#include "MusicRenderer.h"
#include "ToneSynth.h"


class GameAudio : public SoundListener {
public:
	enum Mode { kClassic, kEnhanced };

	// Original tick rate: 3579545 / (228 * 262) Hz.
	static constexpr double kTickRate = 3579545.0 / (228.0 * 262.0);

						GameAudio();

			void		SetRate(int rate);
	inline	int			Rate() const;
	inline	bool		LoadSoundFont(const char* path);
			void		SetMode(Mode mode);
	inline	Mode		GetMode() const;

	inline	SoundDriver&	Driver();
	inline	MusicRenderer&	Music();

	// One game tick: driver update (unless the game did it already),
	// then the samples of this tick are rendered and appended to 'out'
	// (interleaved stereo).
			void		Tick(std::vector<int16_t>& out, bool updateDriver = true);

	// SoundListener
	virtual	void		SoundRegister(int reg, uint8_t value);
	virtual	void		NoteStart(int area, int note, int octave, int instrument);

			float		fMusicGain, fEffectsGain, fClassicGain;

private:
			SoundDriver&	fDriver;
			ToneSynth	fSynth;
			MusicRenderer fMusic;
			Mode		fMode;
			int			fRate;
			double		fOwed;
			std::vector<float> fMix;
};

inline int
GameAudio::Rate() const
{
	return fRate;
}


inline bool
GameAudio::LoadSoundFont(const char* path)
{
	return fMusic.LoadSoundFont(path);
}


inline GameAudio::Mode
GameAudio::GetMode() const
{
	return fMode;
}


inline SoundDriver&
GameAudio::Driver()
{
	return fDriver;
}


inline MusicRenderer&
GameAudio::Music()
{
	return fMusic;
}


#endif	// MG_GAME_AUDIO_H
