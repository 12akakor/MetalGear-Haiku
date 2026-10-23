/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "MusicRenderer.h"

#include "../game/GameData.h"
#include "../game/GameRam.h"
#include "../game/RomSymbols.h"

#define TSF_IMPLEMENTATION
#include "../../third_party/tsf/tsf.h"

#include <math.h>
#include <stdio.h>
#include <string.h>


namespace {

const uint16_t kSoundWorkArea = 0xc010;
const uint16_t kSoundWorkAreaSfx = 0xc070;
const uint16_t kVolumeFadeVal = 0xc0f6;
const uint16_t kSoundDataSaved = 0xc0f2;
const uint16_t kMuteSoundFlag = 0xc0f8;
#define kInstruments RomSymbol(sym::Instruments)
#define kNoteFrequency RomSymbol(sym::NoteFrequency)

enum {
	ID = 2, CONFIG = 5, NOTE_MODE = 14, FREQ_MOD = 15, INSTRUMENT = 16,
	FREQ_LOW = 21, FREQ_HIGH = 22, VOLUME = 23
};

const int kDrumChannel = 9;

inline uint8_t
Field(int area, int f)
{
	return Ram(static_cast<uint16_t>(kSoundWorkArea + area * 0x20 + f));
}

double
ExactKey(int period)
{
	if (period < 1)
		period = 1;
	double frequency = 1789772.5 / (16.0 * period);
	return 69.0 + 12.0 * log2(frequency / 440.0);
}

float
Amplitude(int v)
{
	return v <= 0 ? 0.0f : static_cast<float>(pow(2.0, (v - 15) / 2.0));
}

// Default instruments (GM program numbers, 0 based) and levels, set for
// GeneralUser GS 2.0.3 (kTuning): the balance of the voices is the one
// first chosen by ear with FluidR3, measured per instrument.
struct Default {
	int program, lowProgram, split;
	float volume, pan;
};

const Default kDefaults[MusicRenderer::kMusicCount][MusicRenderer::kAreas] = {
	// #29 Intro: Theme of Tara
	{ {61, 33, 48, 1.05f, 0.5f}, {48, 33, 48, 0.67f, 0.3f}, {62, -1, 0, 0.75f, 0.7f} },
	// #2C Theme of Tara
	{ {61, 33, 48, 1.05f, 0.5f}, {48, 33, 48, 0.67f, 0.3f}, {62, -1, 0, 0.75f, 0.7f} },
	// #2F Red Alert
	{ {30, 38, 48, 0.80f, 0.45f}, {38, -1, 0, 0.85f, 0.5f}, {81, -1, 0, 0.70f, 0.65f} },
	// #32 Alert
	{ {30, 38, 48, 0.80f, 0.45f}, {38, -1, 0, 0.85f, 0.5f}, {81, -1, 0, 0.70f, 0.65f} },
	// #35 Sneaking Mission
	{ {81, -1, 0, 0.70f, 0.45f}, {38, -1, 0, 0.85f, 0.5f}, {50, 38, 48, 1.29f, 0.6f} },
	// #38 TX-55 Metal Gear
	{ {80, -1, 0, 0.71f, 0.4f}, {38, -1, 0, 0.85f, 0.5f}, {50, -1, 0, 2.18f, 0.6f} },
	// #3B Beyond Big Boss
	{ {38, -1, 0, 0.85f, 0.5f}, {30, -1, 0, 0.78f, 0.35f}, {81, 38, 48, 0.63f, 0.65f} },
	// #3E Mercenary
	{ {30, 38, 45, 0.74f, 0.4f}, {29, 38, 45, 0.70f, 0.6f}, {48, -1, 0, 0.54f, 0.5f} },
	// #41 Return of Fox Hunter
	{ {56, 33, 48, 0.91f, 0.5f}, {33, -1, 0, 0.93f, 0.45f}, {48, 33, 48, 0.67f, 0.6f} },
	// #44 Just Another Dead Soldier
	{ {48, 33, 48, 0.72f, 0.4f}, {48, 33, 48, 0.67f, 0.6f}, {33, 33, 48, 0.88f, 0.5f} }
};

// Loudness per music, so all sit at about the same level.
const float kSongGain[MusicRenderer::kMusicCount] = {
	0.86f, 0.82f, 1.26f, 1.24f, 1.81f, 0.74f, 1.15f, 1.01f, 1.36f, 1.81f
};

// Drum kit pieces (instruments.asm) to GM drum keys.
const int kDefaultDrums[12] = {
	42,	// hi-hat
	49,	// cymbal
	38,	// snare drum
	40,	// snare drum 2
	0, 0, 0,
	50,	// tom 1 (high)
	48,	// tom 2
	45,	// tom 3
	41,	// tom 4 (low)
	36	// bass drum
};

}	// namespace


MusicRenderer::MusicRenderer()
	:
	fSlapBassProgram(36),
	fBassProgram(33),
	fDrumVolume(0.72f),
	fBassVolume(1.05f),
	fReverbWet(0.22f),
	fFont(NULL),
	fRate(44100)
{
	for (int m = 0; m < kMusicCount; m++) {
		for (int a = 0; a < kAreas; a++) {
			const Default& d = kDefaults[m][a];
			fSetup[m][a] = { d.program, d.lowProgram, d.split, d.volume, d.pan, 0 };
		}
	}
	memcpy(fSongGain, kSongGain, sizeof(fSongGain));
	memcpy(fDrumKey, kDefaultDrums, sizeof(fDrumKey));
	memset(fLastId, 0, sizeof(fLastId));
	memset(fResume, 0, sizeof(fResume));
	for (int i = 0; i < 16; i++)
		fProgram[i] = -1;
}


MusicRenderer::~MusicRenderer()
{
	if (fFont != NULL)
		tsf_close(fFont);
}


bool
MusicRenderer::LoadSoundFont(const char* path)
{
	if (fFont != NULL)
		tsf_close(fFont);
	fFont = tsf_load_filename(path);
	if (fFont == NULL)
		return false;
	SetRate(fRate);
	tsf_channel_set_bank_preset(fFont, kDrumChannel, 128, 0);
	return true;
}


void
MusicRenderer::SetRate(int rate)
{
	fRate = rate;
	fReverb.SetRate(rate);
	fReverb.SetParameters(0.55f, 0.35f, fReverbWet);
	if (fFont != NULL)
		tsf_set_output(fFont, TSF_STEREO_INTERLEAVED, rate, -6.0f);
}


bool
MusicRenderer::IsMusic(uint8_t id)
{
	return id >= 0x29 && id <= 0x46;
}


void
MusicRenderer::_StopVoice(int area)
{
	Voice& v = fVoice[area];
	if (v.channel >= 0 && fFont != NULL)
		tsf_channel_note_off(fFont, v.channel, v.key);
	v.channel = -1;
}


void
MusicRenderer::AllNotesOff()
{
	for (int a = 0; a < kAreas; a++)
		_StopVoice(a);
	if (fFont != NULL)
		tsf_note_off_all(fFont);
}


void
MusicRenderer::_StartTone(int area, int song, double exactKey, double reference)
{
	const ChannelSetup& s = fSetup[song][area];
	int key = static_cast<int>(lround(reference)) + s.transpose;
	int channel = area;
	int program = s.program;
	if (s.lowProgram >= 0 && key < s.split) {
		channel = 3 + area;
		program = s.lowProgram;
	}
	if (fProgram[channel] != program) {
		tsf_channel_set_presetnumber(fFont, channel, program, 0);
		tsf_channel_set_pitchrange(fFont, channel, 2.0f);
		fProgram[channel] = program;
	}
	tsf_channel_set_pan(fFont, channel, s.pan);
	Voice& v = fVoice[area];
	v.channel = channel;
	v.key = key;
	v.reference = reference;
	v.tone = true;
	(void)exactKey;
	tsf_channel_set_pitchwheel(fFont, channel, 8192);
	tsf_channel_note_on(fFont, channel, key, 0.85f);
}


int
MusicRenderer::_BassKey(int instrument, int note) const
{
	// First tone period of the instrument data for this note.
	uint16_t family = static_cast<uint16_t>(
		SoundData(static_cast<uint16_t>(kInstruments - 2 + instrument * 2))
		| (SoundData(static_cast<uint16_t>(kInstruments - 1 + instrument * 2)) << 8));
	uint16_t p = static_cast<uint16_t>(SoundData(static_cast<uint16_t>(family + note * 2))
		| (SoundData(static_cast<uint16_t>(family + note * 2 + 1)) << 8));
	uint8_t config = SoundData(p);
	if ((config & 0xf0) != 0x20)
		return -1;
	p = static_cast<uint16_t>(p + 2);
	if ((config & 0x08) != 0 && (config & 0x04) == 0)
		p = static_cast<uint16_t>(p + 2);
	if ((SoundData(p) & 0xf0) == 0x10)
		p++;
	if ((config & 3) == 1)
		return -1;
	int period = ((SoundData(p) & 0x0f) << 8) | SoundData(static_cast<uint16_t>(p + 1));
	return static_cast<int>(lround(ExactKey(period)));
}


void
MusicRenderer::NoteStart(int area, int note, int octave, int instrument)
{
	if (fFont == NULL || area < 0 || area >= kAreas)
		return;
	uint8_t id = Field(area, ID);
	if (id != fLastId[area])
		fLastId[area] = id;	// new music: Tick() must not stop this note
	if (!IsMusic(id))
		return;
	int song = (id - 0x29) / 3;
	_StopVoice(area);
	fResume[area] = false;

	if (instrument == 0) {
		int period = SoundData(static_cast<uint16_t>(kNoteFrequency + note)) << octave;
		double reference = ExactKey(period);
		_StartTone(area, song, reference, reference);
		return;
	}
	if (instrument == 1) {
		if (note < 12 && fDrumKey[note] > 0)
			tsf_channel_note_on(fFont, kDrumChannel, fDrumKey[note],
				fDrumVolume * (fSongGain[song] > 1.2f ? 1.2f : fSongGain[song]));
		return;
	}
	if (instrument == 4 || instrument == 5) {
		int key = _BassKey(instrument, note);
		if (key < 0)
			return;
		int channel = 6 + area;
		int program = instrument == 4 ? fSlapBassProgram : fBassProgram;
		if (fProgram[channel] != program) {
			tsf_channel_set_presetnumber(fFont, channel, program, 0);
			fProgram[channel] = program;
		}
		tsf_channel_set_volume(fFont, channel, fBassVolume * fSongGain[song]);
		Voice& v = fVoice[area];
		v.channel = channel;
		v.key = key;
		v.reference = key;
		v.tone = false;
		tsf_channel_note_on(fFont, channel, key, 0.9f);
	}
}


void
MusicRenderer::Tick()
{
	if (fFont == NULL)
		return;
	for (int a = 0; a < kAreas; a++) {
		uint8_t id = Field(a, ID);
		if (id != fLastId[a]) {
			// Other music or a sound effect in this channel; when the music
			// comes back (after a pause or the radio), the note that was
			// playing is started again.
			_StopVoice(a);
			fLastId[a] = id;
			fResume[a] = true;
		}
		if (!IsMusic(id))
			continue;
		int song = (id - 0x29) / 3;
		Voice& v = fVoice[a];
		if (Field(a, NOTE_MODE) == 0) {
			_StopVoice(a);	// sound effect part of the music
			continue;
		}

		int fade = Ram(kSoundDataSaved) == 0 && Field(a, NOTE_MODE) != 0
			? Ram(kVolumeFadeVal) : 0;
		if (Ram(kMuteSoundFlag) != 0) {
			_StopVoice(a);
			fResume[a] = true;
			continue;
		}

		if (v.channel < 0 && fResume[a]) {
			fResume[a] = false;
			int period = Field(a, FREQ_LOW) | (Field(a, FREQ_HIGH) << 8);
			if (Field(a, INSTRUMENT) == 0 && Field(a, VOLUME) != 0 && period != 0) {
				double key = ExactKey(period);
				_StartTone(a, song, key, key);
			}
		}

		if (v.channel < 0 || !v.tone) {
			// Bass instrument: ends with its data (MuteChannel clears the
			// configuration and the volume, not the instrument).
			if (v.channel >= 0 && Field(a, CONFIG) == 0 && Field(a, VOLUME) == 0)
				_StopVoice(a);
			if (v.channel >= 0) {
				float level = fBassVolume * fSongGain[song] * Amplitude(15 - fade);
				tsf_channel_set_volume(fFont, v.channel, level);
			}
			continue;
		}

		int volume = Field(a, VOLUME);
		if ((Field(a, CONFIG) & 0x08) != 0)
			volume = 12;	// volume envelope
		volume -= fade;
		if (volume <= 0) {
			_StopVoice(a);
			continue;
		}
		const ChannelSetup& s = fSetup[song][a];
		tsf_channel_set_volume(fFont, v.channel,
			Amplitude(volume) * s.volume * fSongGain[song]);

		int period = Field(a, FREQ_LOW) | (Field(a, FREQ_HIGH) << 8);
		if ((Field(a, FREQ_MOD) & 0x08) != 0)
			period++;
		if (period > 0) {
			double bend = ExactKey(period) - v.reference;
			if (bend > 2.0)
				bend = 2.0;
			else if (bend < -2.0)
				bend = -2.0;
			int wheel = static_cast<int>(lround(8192 + bend / 2.0 * 8191));
			tsf_channel_set_pitchwheel(fFont, v.channel, wheel);
		}
	}
}


void
MusicRenderer::Render(float* out, int count, float gain)
{
	if (fFont == NULL)
		return;
	fBuffer.assign(static_cast<size_t>(count) * 2, 0.0f);
	tsf_render_float(fFont, fBuffer.data(), count, 0);
	fReverb.Process(fBuffer.data(), count);
	for (int i = 0; i < count * 2; i++)
		out[i] += fBuffer[i] * gain;
}


// The SoundFont the default instruments and levels are set for. A
// music.cfg written for another one is not used (the caller keeps it as
// music.cfg.old and writes a new one).
const char* const MusicRenderer::kTuning = "GeneralUser-GS-2.0.3";


bool
MusicRenderer::LoadConfig(const char* path)
{
	FILE* fp = fopen(path, "r");
	if (fp == NULL)
		return false;
	char line[256];
	bool tuned = false;
	while (fgets(line, sizeof(line), fp) != NULL) {
		char name[64];
		if (sscanf(line, "tuning %63s", name) == 1 && strcmp(name, kTuning) == 0)
			tuned = true;
	}
	if (!tuned) {
		fclose(fp);
		return false;
	}
	rewind(fp);
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (line[0] == '#' || line[0] == '\n')
			continue;
		unsigned id;
		int area, program, lowProgram, split, transpose = 0, value;
		float volume, pan;
		char word[32];
		if (sscanf(line, "music %x %d %d %d %d %f %f %d", &id, &area, &program,
				&lowProgram, &split, &volume, &pan, &transpose) >= 7) {
			if (!IsMusic(static_cast<uint8_t>(id)) || area < 0 || area >= kAreas)
				continue;
			fSetup[(id - 0x29) / 3][area]
				= { program, lowProgram, split, volume, pan, transpose };
		} else if (sscanf(line, "gain %x %f", &id, &volume) == 2) {
			if (IsMusic(static_cast<uint8_t>(id)))
				fSongGain[(id - 0x29) / 3] = volume;
		} else if (sscanf(line, "drum %d %d", &area, &value) == 2) {
			if (area >= 0 && area < 12)
				fDrumKey[area] = value;
		} else if (sscanf(line, "drumvolume %f", &volume) == 1) {
			fDrumVolume = volume;
		} else if (sscanf(line, "bassvolume %f", &volume) == 1) {
			fBassVolume = volume;
		} else if (sscanf(line, "%31s %d", word, &value) == 2) {
			if (strcmp(word, "slapbass") == 0)
				fSlapBassProgram = value;
			else if (strcmp(word, "bass") == 0)
				fBassProgram = value;
		}
		if (sscanf(line, "reverb %f", &volume) == 1)
			fReverbWet = volume;
	}
	fclose(fp);
	fReverb.SetParameters(0.55f, 0.35f, fReverbWet);
	return true;
}


bool
MusicRenderer::SaveConfig(const char* path) const
{
	FILE* fp = fopen(path, "w");
	if (fp == NULL)
		return false;
	static const char* kNames[kMusicCount] = {
		"Intro, Theme of Tara", "Theme of Tara", "Red Alert", "Alert",
		"Sneaking Mission", "TX-55 Metal Gear", "Beyond Big Boss",
		"Mercenary", "Return of Fox Hunter", "Just Another Dead Soldier"
	};
	fprintf(fp, "# Metal Gear music instruments (General MIDI program numbers 0-127)\n"
		"# Set for this SoundFont (a file for another one is replaced):\ntuning %s\n\n"
		"# music <sound id> <channel 0-2> <program> <low program or -1> <split key>"
		" <volume> <pan 0-1> [transpose]\n", kTuning);
	for (int m = 0; m < kMusicCount; m++) {
		fprintf(fp, "\n# %s\n", kNames[m]);
		for (int a = 0; a < kAreas; a++) {
			const ChannelSetup& s = fSetup[m][a];
			fprintf(fp, "music %02X %d %d %d %d %.2f %.2f %d\n", 0x29 + m * 3, a,
				s.program, s.lowProgram, s.split, s.volume, s.pan, s.transpose);
		}
		fprintf(fp, "gain %02X %.2f\n", 0x29 + m * 3, fSongGain[m]);
	}
	fprintf(fp, "\n# bass instruments of the original\nslapbass %d\nbass %d\n"
		"bassvolume %.2f\n", fSlapBassProgram, fBassProgram, fBassVolume);
	fprintf(fp, "\n# drum kit: drum <piece 0-11> <GM drum key>\ndrumvolume %.2f\n", fDrumVolume);
	for (int i = 0; i < 12; i++)
		fprintf(fp, "drum %d %d\n", i, fDrumKey[i]);
	fprintf(fp, "\nreverb %.2f\n", fReverbWet);
	fclose(fp);
	return true;
}
