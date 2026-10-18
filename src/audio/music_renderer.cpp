#include "music_renderer.h"

#include "../game/game_data.h"
#include "../game/game_ram.h"
#include "../game/rom_symbols.h"

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
#define kInstruments rom_symbol(sym::Instruments)
#define kNoteFrequency rom_symbol(sym::NoteFrequency)

enum {
	ID = 2, CONFIG = 5, NOTE_MODE = 14, FREQ_MOD = 15, INSTRUMENT = 16,
	FREQ_LOW = 21, FREQ_HIGH = 22, VOLUME = 23
};

const int kDrumChannel = 9;

inline uint8_t
field(int area, int f)
{
	return ram((uint16_t)(kSoundWorkArea + area * 0x20 + f));
}

double
exact_key(int period)
{
	if (period < 1)
		period = 1;
	double frequency = 1789772.5 / (16.0 * period);
	return 69.0 + 12.0 * log2(frequency / 440.0);
}

float
amplitude(int v)
{
	return v <= 0 ? 0.0f : (float)pow(2.0, (v - 15) / 2.0);
}

/* Default instruments (GM program numbers, 0 based). */
struct Default {
	int program, lowProgram, split;
	float volume, pan;
};

const Default kDefaults[MusicRenderer::kMusicCount][MusicRenderer::kAreas] = {
	/* #29 Intro: Theme of Tara */
	{ {61, 33, 48, 0.9f, 0.5f}, {48, 33, 48, 0.75f, 0.3f}, {62, -1, 0, 0.7f, 0.7f} },
	/* #2C Theme of Tara */
	{ {61, 33, 48, 0.9f, 0.5f}, {48, 33, 48, 0.75f, 0.3f}, {62, -1, 0, 0.7f, 0.7f} },
	/* #2F Red Alert */
	{ {30, 38, 48, 0.75f, 0.45f}, {38, -1, 0, 0.9f, 0.5f}, {81, -1, 0, 0.6f, 0.65f} },
	/* #32 Alert */
	{ {30, 38, 48, 0.75f, 0.45f}, {38, -1, 0, 0.9f, 0.5f}, {81, -1, 0, 0.6f, 0.65f} },
	/* #35 Sneaking Mission */
	{ {81, -1, 0, 0.6f, 0.45f}, {38, -1, 0, 0.9f, 0.5f}, {50, 38, 48, 0.75f, 0.6f} },
	/* #38 TX-55 Metal Gear */
	{ {80, -1, 0, 0.5f, 0.4f}, {38, -1, 0, 0.9f, 0.5f}, {50, -1, 0, 0.7f, 0.6f} },
	/* #3B Beyond Big Boss */
	{ {38, -1, 0, 0.9f, 0.5f}, {30, -1, 0, 0.65f, 0.35f}, {81, 38, 48, 0.6f, 0.65f} },
	/* #3E Mercenary */
	{ {30, 38, 45, 0.7f, 0.4f}, {29, 38, 45, 0.7f, 0.6f}, {48, -1, 0, 0.7f, 0.5f} },
	/* #41 Return of Fox Hunter */
	{ {56, 33, 48, 0.85f, 0.5f}, {33, -1, 0, 0.9f, 0.45f}, {48, 33, 48, 0.75f, 0.6f} },
	/* #44 Just Another Dead Soldier */
	{ {48, 33, 48, 0.8f, 0.4f}, {48, 33, 48, 0.75f, 0.6f}, {33, 33, 48, 0.85f, 0.5f} }
};

/* Loudness per music, so all sit at about the same level. */
const float kSongGain[MusicRenderer::kMusicCount] = {
	1.15f, 1.05f, 1.1f, 1.1f, 1.8f, 0.8f, 1.1f, 0.9f, 2.6f, 2.3f
};

/* Drum kit pieces (instruments.asm) to GM drum keys. */
const int kDefaultDrums[12] = {
	42,		/* hi-hat */
	49,		/* cymbal */
	38,		/* snare drum */
	40,		/* snare drum 2 */
	0, 0, 0,
	50,		/* tom 1 (high) */
	48,		/* tom 2 */
	45,		/* tom 3 */
	41,		/* tom 4 (low) */
	36		/* bass drum */
};

}	// namespace


MusicRenderer::MusicRenderer()
	:
	slapBassProgram(36),
	bassProgram(33),
	drumVolume(0.9f),
	bassVolume(0.95f),
	reverbWet(0.22f),
	fFont(NULL),
	fRate(44100)
{
	for (int m = 0; m < kMusicCount; m++) {
		for (int a = 0; a < kAreas; a++) {
			const Default& d = kDefaults[m][a];
			setup[m][a] = { d.program, d.lowProgram, d.split, d.volume, d.pan, 0 };
		}
	}
	memcpy(songGain, kSongGain, sizeof(songGain));
	memcpy(drumKey, kDefaultDrums, sizeof(drumKey));
	memset(fLastId, 0, sizeof(fLastId));
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
	fReverb.SetParameters(0.55f, 0.35f, reverbWet);
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
	const ChannelSetup& s = setup[song][area];
	int key = (int)lround(reference) + s.transpose;
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
	/* First tone period of the instrument data for this note. */
	uint16_t family = (uint16_t)(sound_data((uint16_t)(kInstruments - 2 + instrument * 2))
		| (sound_data((uint16_t)(kInstruments - 1 + instrument * 2)) << 8));
	uint16_t p = (uint16_t)(sound_data((uint16_t)(family + note * 2))
		| (sound_data((uint16_t)(family + note * 2 + 1)) << 8));
	uint8_t config = sound_data(p);
	if ((config & 0xf0) != 0x20)
		return -1;
	p = (uint16_t)(p + 2);
	if ((config & 0x08) != 0 && (config & 0x04) == 0)
		p = (uint16_t)(p + 2);
	if ((sound_data(p) & 0xf0) == 0x10)
		p++;
	if ((config & 3) == 1)
		return -1;
	int period = ((sound_data(p) & 0x0f) << 8) | sound_data((uint16_t)(p + 1));
	return (int)lround(exact_key(period));
}


void
MusicRenderer::NoteStart(int area, int note, int octave, int instrument)
{
	if (fFont == NULL || area < 0 || area >= kAreas)
		return;
	uint8_t id = field(area, ID);
	if (!IsMusic(id))
		return;
	int song = (id - 0x29) / 3;
	_StopVoice(area);

	if (instrument == 0) {
		int period = sound_data((uint16_t)(kNoteFrequency + note)) << octave;
		double reference = exact_key(period);
		_StartTone(area, song, reference, reference);
		return;
	}
	if (instrument == 1) {
		if (note < 12 && drumKey[note] > 0)
			tsf_channel_note_on(fFont, kDrumChannel, drumKey[note],
				drumVolume * (songGain[song] > 1.2f ? 1.2f : songGain[song]));
		return;
	}
	if (instrument == 4 || instrument == 5) {
		int key = _BassKey(instrument, note);
		if (key < 0)
			return;
		int channel = 6 + area;
		int program = instrument == 4 ? slapBassProgram : bassProgram;
		if (fProgram[channel] != program) {
			tsf_channel_set_presetnumber(fFont, channel, program, 0);
			fProgram[channel] = program;
		}
		tsf_channel_set_volume(fFont, channel, bassVolume * songGain[song]);
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
		uint8_t id = field(a, ID);
		if (id != fLastId[a]) {
			/* Other music or a sound effect in this channel. */
			_StopVoice(a);
			fLastId[a] = id;
		}
		if (!IsMusic(id))
			continue;
		int song = (id - 0x29) / 3;
		Voice& v = fVoice[a];
		if (field(a, NOTE_MODE) == 0) {
			_StopVoice(a);		/* sound effect part of the music */
			continue;
		}

		int fade = ram(kSoundDataSaved) == 0 && field(a, NOTE_MODE) != 0
			? ram(kVolumeFadeVal) : 0;
		if (ram(kMuteSoundFlag) != 0) {
			_StopVoice(a);
			continue;
		}

		if (v.channel < 0 || !v.tone) {
			/* Bass instrument: ends with its data. */
			if (v.channel >= 0 && field(a, INSTRUMENT) == 0 && field(a, VOLUME) == 0)
				_StopVoice(a);
			if (v.channel >= 0) {
				float level = bassVolume * songGain[song] * amplitude(15 - fade);
				tsf_channel_set_volume(fFont, v.channel, level);
			}
			continue;
		}

		int volume = field(a, VOLUME);
		if ((field(a, CONFIG) & 0x08) != 0)
			volume = 12;		/* volume envelope */
		volume -= fade;
		if (volume <= 0) {
			_StopVoice(a);
			continue;
		}
		const ChannelSetup& s = setup[song][a];
		tsf_channel_set_volume(fFont, v.channel,
			amplitude(volume) * s.volume * songGain[song]);

		int period = field(a, FREQ_LOW) | (field(a, FREQ_HIGH) << 8);
		if ((field(a, FREQ_MOD) & 0x08) != 0)
			period++;
		if (period > 0) {
			double bend = exact_key(period) - v.reference;
			if (bend > 2.0)
				bend = 2.0;
			else if (bend < -2.0)
				bend = -2.0;
			int wheel = (int)lround(8192 + bend / 2.0 * 8191);
			tsf_channel_set_pitchwheel(fFont, v.channel, wheel);
		}
	}
}


void
MusicRenderer::Render(float* out, int count, float gain)
{
	if (fFont == NULL)
		return;
	fBuffer.assign((size_t)count * 2, 0.0f);
	tsf_render_float(fFont, fBuffer.data(), count, 0);
	fReverb.Process(fBuffer.data(), count);
	for (int i = 0; i < count * 2; i++)
		out[i] += fBuffer[i] * gain;
}


bool
MusicRenderer::LoadConfig(const char* path)
{
	FILE* fp = fopen(path, "r");
	if (fp == NULL)
		return false;
	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (line[0] == '#' || line[0] == '\n')
			continue;
		unsigned id;
		int area, program, lowProgram, split, transpose = 0, value;
		float volume, pan;
		char word[32];
		if (sscanf(line, "music %x %d %d %d %d %f %f %d", &id, &area, &program,
				&lowProgram, &split, &volume, &pan, &transpose) >= 7) {
			if (!IsMusic((uint8_t)id) || area < 0 || area >= kAreas)
				continue;
			setup[(id - 0x29) / 3][area]
				= { program, lowProgram, split, volume, pan, transpose };
		} else if (sscanf(line, "gain %x %f", &id, &volume) == 2) {
			if (IsMusic((uint8_t)id))
				songGain[(id - 0x29) / 3] = volume;
		} else if (sscanf(line, "drum %d %d", &area, &value) == 2) {
			if (area >= 0 && area < 12)
				drumKey[area] = value;
		} else if (sscanf(line, "%31s %d", word, &value) == 2) {
			if (strcmp(word, "slapbass") == 0)
				slapBassProgram = value;
			else if (strcmp(word, "bass") == 0)
				bassProgram = value;
		}
		if (sscanf(line, "reverb %f", &volume) == 1)
			reverbWet = volume;
	}
	fclose(fp);
	fReverb.SetParameters(0.55f, 0.35f, reverbWet);
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
		"# music <sound id> <channel 0-2> <program> <low program or -1> <split key>"
		" <volume> <pan 0-1> [transpose]\n");
	for (int m = 0; m < kMusicCount; m++) {
		fprintf(fp, "\n# %s\n", kNames[m]);
		for (int a = 0; a < kAreas; a++) {
			const ChannelSetup& s = setup[m][a];
			fprintf(fp, "music %02X %d %d %d %d %.2f %.2f %d\n", 0x29 + m * 3, a,
				s.program, s.lowProgram, s.split, s.volume, s.pan, s.transpose);
		}
		fprintf(fp, "gain %02X %.2f\n", 0x29 + m * 3, songGain[m]);
	}
	fprintf(fp, "\n# bass instruments of the original\nslapbass %d\nbass %d\n",
		slapBassProgram, bassProgram);
	fprintf(fp, "\n# drum kit: drum <piece 0-11> <GM drum key>\n");
	for (int i = 0; i < 12; i++)
		fprintf(fp, "drum %d %d\n", i, drumKey[i]);
	fprintf(fp, "\nreverb %.2f\n", reverbWet);
	fclose(fp);
	return true;
}
