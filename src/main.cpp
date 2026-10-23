/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Metal Gear for Haiku and RenkuOS: the game thread. Finds the ROM, sets
// up picture and sound, and runs the game at the original 59.92 ticks per
// second.
//
// Keys: arrows move, space fire, M or N second button (weapon), F1-F5 as
// on the MSX (pause, weapons, equipment, ...), Return. Port functions (as
// close to the Maze of Galious port as the game allows, it uses F1-F5
// itself): F7 next filter, F8 timing display, F9 or Print Screen screenshot, F10 music (enhanced, original, original with SCC)
// classic/enhanced, F11 full screen, F12 or Alt+Q quit.
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "audio/GameAudio.h"
#include "game/GameData.h"
#include "game/GameMain.h"
#include "game/GameRam.h"
#include "game/Input.h"
#include "game/RamSymbols.h"
#include "game/SaveLoad.h"
#include "game/Screen.h"
#include "game/ScreenRender.h"
#include "platform/Platform.h"


namespace {

// MSX keyboard matrix position of a key: row * 8 + bit, -1 for none.
int
MatrixPosition(int key)
{
	if (key >= MOGK_0 && key <= MOGK_7)
		return 0 * 8 + (key - MOGK_0);
	if (key == MOGK_8 || key == MOGK_9)
		return 1 * 8 + (key - MOGK_8);
	if (key >= MOGK_a && key <= MOGK_z) {
		// row 2 bit 6 = A ...
		int n = key - MOGK_a + 22;
		return (n / 8) * 8 + n % 8;
	}
	switch (key) {
		case MOGK_MINUS: return 1 * 8 + 2;
		case MOGK_EQUALS: return 1 * 8 + 3;
		case MOGK_BACKSLASH: return 1 * 8 + 4;
		case MOGK_LEFTBRACKET: return 1 * 8 + 5;
		case MOGK_RIGHTBRACKET: return 1 * 8 + 6;
		case MOGK_SEMICOLON: return 1 * 8 + 7;
		case MOGK_QUOTE: return 2 * 8 + 0;
		case MOGK_BACKQUOTE: return 2 * 8 + 1;
		case MOGK_COMMA: return 2 * 8 + 2;
		case MOGK_PERIOD: return 2 * 8 + 3;
		case MOGK_SLASH: return 2 * 8 + 4;
		case MOGK_LSHIFT: case MOGK_RSHIFT: return 6 * 8 + 0;
		case MOGK_LCTRL: case MOGK_RCTRL: return 6 * 8 + 1;
		case MOGK_LALT: return 6 * 8 + 2;	// GRAPH
		case MOGK_CAPSLOCK: return 6 * 8 + 3;
		case MOGK_RALT: return 6 * 8 + 4;	// CODE
		case MOGK_F1: return 6 * 8 + 5;
		case MOGK_F2: return 6 * 8 + 6;
		case MOGK_F3: return 6 * 8 + 7;
		case MOGK_F4: return 7 * 8 + 0;
		case MOGK_F5: return 7 * 8 + 1;
		case MOGK_ESCAPE: return 7 * 8 + 2;
		case MOGK_TAB: return 7 * 8 + 3;
		case MOGK_END: return 7 * 8 + 4;	// STOP
		case MOGK_BACKSPACE: return 7 * 8 + 5;
		case MOGK_PAGEDOWN: return 7 * 8 + 6;	// SELECT
		case MOGK_RETURN: case MOGK_KP_ENTER: return 7 * 8 + 7;
		case MOGK_SPACE: return 8 * 8 + 0;
		case MOGK_HOME: return 8 * 8 + 1;
		case MOGK_INSERT: return 8 * 8 + 2;
		case MOGK_DELETE: return 8 * 8 + 3;
		case MOGK_LEFT: return 8 * 8 + 4;
		case MOGK_UP: return 8 * 8 + 5;
		case MOGK_DOWN: return 8 * 8 + 6;
		case MOGK_RIGHT: return 8 * 8 + 7;
	}
	return -1;
}


// Keys pressed since the last tick count as down for one tick, so a very
// short tap between two ticks is not lost.
bool sTapped[MOGK_LAST];

void
UpdateMatrix()
{
	PlatPumpEvents();
	unsigned char* state = PlatKeyState();
	for (int r = 0; r < 11; r++)
		gKeyMatrix[r] = 0xff;
	for (int key = 1; key < MOGK_LAST; key++) {
		if (state[key] == 0 && !sTapped[key])
			continue;
		sTapped[key] = false;
		int p = MatrixPosition(key);
		if (p >= 0)
			gKeyMatrix[p / 8] &= static_cast<uint8_t>(~(1 << (p % 8)));
	}
	// joystick port 1: 0 while pressed
	gJoystick = static_cast<uint8_t>(~PlatJoystick() & 0x3f);
}


bool
FileExists(const std::string& path)
{
	FILE* fp = fopen(path.c_str(), "rb");
	if (fp == NULL)
		return false;
	fclose(fp);
	return true;
}


// The ROM: given on the command line, or in the settings folder (any
// .rom file that is a supported version).
bool
LoadRom(int argc, char** argv, std::string* message)
{
	std::string error;
	if (argc > 1)
		return LoadGameRom(argv[1], message);
	std::string dir = PlatSettingsDir();
	std::string mainRom = dir + "/MetalGear.rom";
	if (LoadGameRom(mainRom.c_str(), &error))
		return true;
	std::string mainError = error;
	DIR* d = opendir(dir.c_str());
	if (d != NULL) {
		for (;;) {
			struct dirent* e = readdir(d);
			if (e == NULL)
				break;
			std::string name = e->d_name;
			if (name.size() > 4 && (name.substr(name.size() - 4) == ".rom"
					|| name.substr(name.size() - 4) == ".ROM")) {
				if (LoadGameRom((dir + "/" + name).c_str(), &error)) {
					closedir(d);
					return true;
				}
			}
		}
		closedir(d);
	}
	*message = "No Metal Gear ROM found.\n\nPut the ROM (English or Japanese version, "
		"128 KB) in\n" + dir + "\nas MetalGear.rom, or give its path on the command line.";
	if (FileExists(mainRom))
		*message = mainRom + ": " + mainError;
	return false;
}


// A SoundFont of the player's own in the settings folder comes first,
// then the one that comes with the game (data/MetalGear-FluidR3.sf2),
// then the system's.
std::string
FindSoundfont()
{
	std::string app = PlatAppDir();
	std::vector<std::string> dirs = { PlatSettingsDir(), app + "/data",
		app + "/../data", app + "/../share/MetalGear",
		"/boot/home/config/non-packaged/data/synth",
		"/boot/system/non-packaged/data/synth", "/boot/system/data/synth",
		"/boot/system/data/soundfonts" };
	for (auto& dir : dirs) {
		DIR* d = opendir(dir.c_str());
		if (d == NULL)
			continue;
		std::string found;
		for (;;) {
			struct dirent* e = readdir(d);
			if (e == NULL)
				break;
			std::string name = e->d_name;
			if (name.size() > 4 && (name.substr(name.size() - 4) == ".sf2"
					|| name.substr(name.size() - 4) == ".SF2")) {
				found = dir + "/" + name;
				break;
			}
		}
		closedir(d);
		if (!found.empty())
			return found;
	}
	return "";
}


// Sound: the game thread renders each tick's samples into a ring buffer,
// the audio thread takes them out.
const size_t kRing = 44100 * 2;	// half a second, stereo
std::vector<int16_t> sRing(kRing);
size_t sRead = 0, sWrite = 0, sFill = 0;
size_t sLargestRequest = 0;	// samples, biggest the audio thread asked

void
AudioCallback(int16_t* out, int frames, void*)
{
	size_t need = static_cast<size_t>(frames) * 2;
	PlatAudioLock();
	if (need > sLargestRequest)
		sLargestRequest = need;
	size_t n = need < sFill ? need : sFill;
	for (size_t i = 0; i < n; i++) {
		out[i] = sRing[sRead];
		sRead = (sRead + 1) % kRing;
	}
	sFill -= n;
	PlatAudioUnlock();
	for (size_t i = n; i < need; i++)
		out[i] = 0;
}

// Samples queued, and how many should be queued: about 50 ms, at least
// twice what the audio thread takes at once.
size_t
AudioFill(size_t* target)
{
	PlatAudioLock();
	size_t fill = sFill;
	size_t largest = sLargestRequest;
	PlatAudioUnlock();
	*target = 44100 / 20 * 2;
	if (*target < largest * 2)
		*target = largest * 2;
	return fill;
}

void
AudioPush(const std::vector<int16_t>& samples)
{
	PlatAudioLock();
	for (int16_t v : samples) {
		if (sFill == kRing)
			break;
		sRing[sWrite] = v;
		sWrite = (sWrite + 1) % kRing;
		sFill++;
	}
	PlatAudioUnlock();
}

}	// namespace


int
GameMain(int argc, char** argv)
{
	std::string message;
	if (!LoadRom(argc, argv, &message)) {
		fprintf(stderr, "%s\n", message.c_str());
		PlatAlert("Metal Gear", message.c_str());
		return 1;
	}
	const RomInfo& info = LoadedRomInfo();
	printf("Metal Gear: %s ROM, SHA1 %s\n", RomVersionName(info.version), info.sha1);
	if (!info.note.empty())
		printf("  %s\n", info.note.c_str());

	if (!PlatInit(kScreenWidth, kScreenHeight, false, "Metal Gear"))
		return 1;

	static GameAudio audio;
	audio.SetRate(44100);
	std::string soundfont = FindSoundfont();
	if (!soundfont.empty() && audio.LoadSoundFont(soundfont.c_str())) {
		std::string config = std::string(PlatSettingsDir()) + "/music.cfg";
		if (!audio.Music().LoadConfig(config.c_str()))
			audio.Music().SaveConfig(config.c_str());
		audio.SetMode(GameAudio::kEnhanced);
		printf("  music: %s\n", soundfont.c_str());
	} else {
		audio.SetMode(GameAudio::kClassic);
		printf("  music: classic (no SoundFont found)\n");
	}
	// The SCC versions: their own sound first (F10 switches).
	if (info.scc)
		audio.SetMode(GameAudio::kScc);
	bool audioOpen = PlatAudioOpen(44100, AudioCallback, NULL);

	// The cassette for saving and loading: a .CAS image, also usable in
	// MSX emulators (and theirs here).
	const std::string tapePath = std::string(PlatSettingsDir()) + "/MetalGear.cas";
	if (gTape.Load(tapePath.c_str()))
		printf("  tape: %s, %zu saves\n", tapePath.c_str(), ListTapeSaves(gTape).size());

	Start();

	static uint8_t picture[kScreenWidth * kScreenHeight];
	std::vector<int16_t> samples;
	const double tickMs = 1000.0 / GameAudio::kTickRate;
	double next = PlatTicksMs();
	bool quit = false;
	while (!quit) {
		PlatEvent event;
		while (PlatPollEvent(&event)) {
			if (event.type == PLAT_EV_QUIT)
				quit = true;
			else if (event.type == PLAT_EV_KEYDOWN) {
				if (event.key > 0 && event.key < MOGK_LAST)
					sTapped[event.key] = true;
				if (event.key == MOGK_BACKSPACE || event.key == MOGK_RETURN
					|| event.key == MOGK_ESCAPE
					|| (event.key >= MOGK_SPACE && event.key < 127))
					QueueTypedChar(static_cast<uint8_t>(event.key));
				if (event.key == MOGK_q && ((event.modifiers & PLAT_MOD_ALT) != 0)) {
					quit = true;
					continue;
				}
				switch (event.key) {
					case MOGK_F7:
						PlatNextScaler();
						break;
					case MOGK_F8:
						PlatToggleStats();
						break;
					case MOGK_F9:
					case MOGK_PRINT:
						PlatSaveScreenshot();
						break;
					case MOGK_F10:
					{
						// enhanced (with a SoundFont), original, original + SCC
						GameAudio::Mode mode = audio.GetMode();
						if (mode == GameAudio::kEnhanced)
							mode = GameAudio::kClassic;
						else if (mode == GameAudio::kClassic)
							mode = GameAudio::kScc;
						else
							mode = audio.Music().Ready() ? GameAudio::kEnhanced : GameAudio::kClassic;
						audio.SetMode(mode);
					}
						break;
					case MOGK_F11:
						PlatSetFullscreen(!PlatIsFullscreen());
						break;
					case MOGK_F12:
						quit = true;
						break;
					default:
						break;
				}
			}
		}
		UpdateMatrix();

		InterruptTick();
		if (gUnported != NULL) {
			// Parts not rewritten yet: back to the title.
			printf("not rewritten yet: %s; back to the title\n", gUnported);
			gUnported = NULL;
			Ram(var::GameStatus) = 0;
			Ram(var::GameSubstatus) = 0;
		}
		// The name block is written, the data block follows next tick.
		bool halfSaved = Ram(var::SaveLoadMode) == 2 && Ram(var::SaveLoadStat) == 3;
		if (gTape.IsModified() && !halfSaved) {
			if (gTape.Save(tapePath.c_str()))
				gTape.SetModified(false);
			else
				fprintf(stderr, "cannot write %s\n", tapePath.c_str());
		}
		samples.clear();
		audio.Tick(samples, false);
		AudioPush(samples);

		RenderScreenIndexed(gScreen, picture);
		uint8_t palette[768];
		memset(palette, 0, sizeof(palette));
		for (int i = 0; i < 16; i++) {
			uint32_t c = ScreenColor(gScreen, i);
			palette[i * 3] = static_cast<uint8_t>(c >> 16);
			palette[i * 3 + 1] = static_cast<uint8_t>(c >> 8);
			palette[i * 3 + 2] = static_cast<uint8_t>(c);
		}
		PlatSetPalette(palette);
		PlatPresent(picture);

		if (audioOpen) {
			// The sound card sets the pace: keep about 50 ms of sound
			// queued, so the delay stays short and constant and the game
			// runs at exactly the speed of the sound.
			size_t target;
			uint32_t waited = 0;
			while (AudioFill(&target) > target && waited < 100) {
				PlatSleepMs(2);
				waited += 2;
			}
		} else {
			next += tickMs;
			double now = PlatTicksMs();
			if (next > now)
				PlatSleepMs(static_cast<uint32_t>(next - now));
			else if (now - next > 250)
				next = now;	// fell behind: do not try to catch up
		}
	}
	PlatAudioClose();
	PlatShutdown();
	return 0;
}
