/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_MUSIC_RENDERER_H
#define MG_MUSIC_RENDERER_H


// Plays the music of the sound driver with SoundFont instruments.
//
// The driver reports every note start; the renderer plays it on a
// General MIDI instrument chosen per music and channel (music.cfg), and
// follows the driver's volume, vibrato and fades every tick, so phrasing
// and timing stay those of the original. Drum kit notes go to the GM drum
// kit, the two bass instruments to bass presets.

#include <stdint.h>
#include <string>
#include <vector>

#include "Reverb.h"


struct tsf;


class MusicRenderer {
public:
	enum {
		kMusicCount = 10,	// sound IDs #29, #2C ... #44
		kAreas = 3
	};

	struct ChannelSetup {
		int		program;	// GM program 0-127
		int		lowProgram;	// below 'split', -1 = same
		int		split;	// MIDI key
		float	volume;
		float	pan;	// 0 left, 1 right
		int		transpose;	// semitones
	};

						MusicRenderer();
						~MusicRenderer();

			bool		LoadSoundFont(const char* path);
	inline	bool		Ready() const;
			void		SetRate(int rate);
	// Reads instrument choices; missing entries keep the defaults.
			bool		LoadConfig(const char* path);
			bool		SaveConfig(const char* path) const;

			void		NoteStart(int area, int note, int octave, int instrument);
	// After every driver update.
			void		Tick();
	// Adds 'count' stereo frames to 'out'.
			void		Render(float* out, int count, float gain);

			void		AllNotesOff();

	static	bool		IsMusic(uint8_t id);

			ChannelSetup fSetup[kMusicCount][kAreas];
			int			fSlapBassProgram, fBassProgram;
			float		fSongGain[kMusicCount];
			int			fDrumKey[12];
			float		fDrumVolume, fBassVolume;
			float		fReverbWet;

private:
	struct Voice {
		int		channel = -1;	// MIDI channel, -1 = none
		int		key = 0;
		double	reference = 0;	// exact key of the note without modulation
		bool	tone = false;	// plain tone: follows volume and pitch
	};

			void		_StopVoice(int area);
			void		_StartTone(int area, int song, double exactKey,
							double reference);
			int			_BassKey(int instrument, int note) const;

			tsf*		fFont;
			int			fRate;
			Voice		fVoice[kAreas];
			uint8_t		fLastId[kAreas];
			bool		fResume[kAreas];
			int			fProgram[16];
			std::vector<float> fBuffer;
			Reverb		fReverb;
};

inline bool
MusicRenderer::Ready() const
{
	return fFont != NULL;
}


#endif	// MG_MUSIC_RENDERER_H
