/*
 * Sound of the game: the driver plus the renderers.
 *
 *   kClassic   original sound (tone generator for everything)
 *   kEnhanced  music on SoundFont instruments with reverb; the sound
 *              effects keep their original character on the tone
 *              generator, and the third music voice keeps playing while
 *              an effect sounds (the original had to mute it).
 */
#ifndef MG_GAME_AUDIO_H
#define MG_GAME_AUDIO_H

#include "../game/sound_driver.h"
#include "music_renderer.h"
#include "tone_synth.h"

#include <stdint.h>
#include <vector>


class GameAudio : public SoundListener {
public:
	enum Mode { kClassic, kEnhanced };

	/* Original tick rate: 3579545 / (228 * 262) Hz. */
	static constexpr double kTickRate = 3579545.0 / (228.0 * 262.0);

						GameAudio();

			void		SetRate(int rate);
			int			Rate() const { return fRate; }
			bool		LoadSoundFont(const char* path)
							{ return fMusic.LoadSoundFont(path); }
			void		SetMode(Mode mode);
			Mode		GetMode() const { return fMode; }

			SoundDriver& Driver() { return fDriver; }
			MusicRenderer& Music() { return fMusic; }

	/* One game tick: driver update (unless the game did it already),
	   then the samples of this tick are rendered and appended to 'out'
	   (interleaved stereo). */
			void		Tick(std::vector<int16_t>& out, bool updateDriver = true);

	/* SoundListener */
	virtual	void		SoundRegister(int reg, uint8_t value);
	virtual	void		NoteStart(int area, int note, int octave, int instrument);

			float		musicGain, effectsGain, classicGain;

private:
			SoundDriver&	fDriver;
			ToneSynth	fSynth;
			MusicRenderer fMusic;
			Mode		fMode;
			int			fRate;
			double		fOwed;
			std::vector<float> fMix;
};

#endif
