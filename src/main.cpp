/*
 * Metal Gear for Haiku and RenkuOS: the game thread. Finds the ROM, sets
 * up picture and sound, and runs the game at the original 59.92 ticks per
 * second.
 *
 * Keys: arrows move, space fire, M or N second button (weapon), F1-F5 as
 * on the MSX (pause, weapons, equipment, ...), Return. Port functions (as
 * close to the Maze of Galious port as the game allows, it uses F1-F5
 * itself): F7 next filter, F8 timing display, F9 or Print Screen screenshot, F10 music
 * classic/enhanced, F11 full screen, F12 or Alt+Q quit.
 */
#include "audio/game_audio.h"
#include "game/game_data.h"
#include "game/game_main.h"
#include "game/game_ram.h"
#include "game/input.h"
#include "game/ram_symbols.h"
#include "game/screen.h"
#include "game/screen_render.h"
#include "platform/platform.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>


namespace {

/* MSX keyboard matrix position of a key: row * 8 + bit, -1 for none. */
int
matrix_position(int key)
{
	if (key >= MOGK_0 && key <= MOGK_7)
		return 0 * 8 + (key - MOGK_0);
	if (key == MOGK_8 || key == MOGK_9)
		return 1 * 8 + (key - MOGK_8);
	if (key >= MOGK_a && key <= MOGK_z) {
		/* row 2 bit 6 = A ... */
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
		case MOGK_LALT: return 6 * 8 + 2;	/* GRAPH */
		case MOGK_CAPSLOCK: return 6 * 8 + 3;
		case MOGK_RALT: return 6 * 8 + 4;	/* CODE */
		case MOGK_F1: return 6 * 8 + 5;
		case MOGK_F2: return 6 * 8 + 6;
		case MOGK_F3: return 6 * 8 + 7;
		case MOGK_F4: return 7 * 8 + 0;
		case MOGK_F5: return 7 * 8 + 1;
		case MOGK_ESCAPE: return 7 * 8 + 2;
		case MOGK_TAB: return 7 * 8 + 3;
		case MOGK_END: return 7 * 8 + 4;	/* STOP */
		case MOGK_BACKSPACE: return 7 * 8 + 5;
		case MOGK_PAGEDOWN: return 7 * 8 + 6;	/* SELECT */
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


/* Keys pressed since the last tick count as down for one tick, so a very
   short tap between two ticks is not lost. */
bool sTapped[MOGK_LAST];

void
update_matrix()
{
	plat_pump_events();
	unsigned char* state = plat_key_state();
	for (int r = 0; r < 11; r++)
		g_keyMatrix[r] = 0xff;
	for (int key = 1; key < MOGK_LAST; key++) {
		if (state[key] == 0 && !sTapped[key])
			continue;
		sTapped[key] = false;
		int p = matrix_position(key);
		if (p >= 0)
			g_keyMatrix[p / 8] &= (uint8_t)~(1 << (p % 8));
	}
	/* joystick port 1: 0 while pressed */
	g_joystick = (uint8_t)(~plat_joystick() & 0x3f);
}


bool
file_exists(const std::string& path)
{
	FILE* fp = fopen(path.c_str(), "rb");
	if (fp == NULL)
		return false;
	fclose(fp);
	return true;
}


/* The ROM: given on the command line, or in the settings folder (any
   .rom file that is a supported version). */
bool
load_rom(int argc, char** argv, std::string* message)
{
	std::string error;
	if (argc > 1)
		return game_data_load(argv[1], message);
	std::string dir = plat_settings_dir();
	std::string mainRom = dir + "/MetalGear.rom";
	if (game_data_load(mainRom.c_str(), &error))
		return true;
	std::string mainError = error;
	DIR* d = opendir(dir.c_str());
	if (d != NULL) {
		struct dirent* e;
		while ((e = readdir(d)) != NULL) {
			std::string name = e->d_name;
			if (name.size() > 4 && (name.substr(name.size() - 4) == ".rom"
					|| name.substr(name.size() - 4) == ".ROM")) {
				if (game_data_load((dir + "/" + name).c_str(), &error)) {
					closedir(d);
					return true;
				}
			}
		}
		closedir(d);
	}
	*message = "No Metal Gear ROM found.\n\nPut the ROM (English or Japanese version, "
		"128 KB) in\n" + dir + "\nas MetalGear.rom, or give its path on the command line.";
	if (file_exists(mainRom))
		*message = mainRom + ": " + mainError;
	return false;
}


/* A SoundFont of the player's own in the settings folder comes first,
   then the one that comes with the game (data/MetalGear-FluidR3.sf2),
   then the system's. */
std::string
find_soundfont()
{
	std::string app = plat_app_dir();
	std::vector<std::string> dirs = { plat_settings_dir(), app + "/data",
		app + "/../data", app + "/../share/MetalGear",
		"/boot/home/config/non-packaged/data/synth",
		"/boot/system/non-packaged/data/synth", "/boot/system/data/synth",
		"/boot/system/data/soundfonts" };
	for (auto& dir : dirs) {
		DIR* d = opendir(dir.c_str());
		if (d == NULL)
			continue;
		struct dirent* e;
		std::string found;
		while ((e = readdir(d)) != NULL) {
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


/* Sound: the game thread renders each tick's samples into a ring buffer,
   the audio thread takes them out. */
const size_t kRing = 44100 * 2;		/* half a second, stereo */
std::vector<int16_t> sRing(kRing);
size_t sRead = 0, sWrite = 0, sFill = 0;
size_t sLargestRequest = 0;		/* samples, biggest the audio thread asked */

void
audio_callback(int16_t* out, int frames, void*)
{
	size_t need = (size_t)frames * 2;
	plat_audio_lock();
	if (need > sLargestRequest)
		sLargestRequest = need;
	size_t n = need < sFill ? need : sFill;
	for (size_t i = 0; i < n; i++) {
		out[i] = sRing[sRead];
		sRead = (sRead + 1) % kRing;
	}
	sFill -= n;
	plat_audio_unlock();
	for (size_t i = n; i < need; i++)
		out[i] = 0;
}

/* Samples queued, and how many should be queued: about 50 ms, at least
   twice what the audio thread takes at once. */
size_t
audio_fill(size_t* target)
{
	plat_audio_lock();
	size_t fill = sFill;
	size_t largest = sLargestRequest;
	plat_audio_unlock();
	*target = 44100 / 20 * 2;
	if (*target < largest * 2)
		*target = largest * 2;
	return fill;
}

void
audio_push(const std::vector<int16_t>& samples)
{
	plat_audio_lock();
	for (int16_t v : samples) {
		if (sFill == kRing)
			break;
		sRing[sWrite] = v;
		sWrite = (sWrite + 1) % kRing;
		sFill++;
	}
	plat_audio_unlock();
}

}	// namespace


int
game_main(int argc, char** argv)
{
	std::string message;
	if (!load_rom(argc, argv, &message)) {
		fprintf(stderr, "%s\n", message.c_str());
		plat_alert("Metal Gear", message.c_str());
		return 1;
	}
	const RomInfo& info = game_rom_info();
	printf("Metal Gear: %s ROM, SHA1 %s\n", game_rom_version_name(info.version), info.sha1);
	if (!info.note.empty())
		printf("  %s\n", info.note.c_str());

	if (!plat_init(kScreenWidth, kScreenHeight, false, "Metal Gear"))
		return 1;

	static GameAudio audio;
	audio.SetRate(44100);
	std::string soundfont = find_soundfont();
	if (!soundfont.empty() && audio.LoadSoundFont(soundfont.c_str())) {
		std::string config = std::string(plat_settings_dir()) + "/music.cfg";
		if (!audio.Music().LoadConfig(config.c_str()))
			audio.Music().SaveConfig(config.c_str());
		audio.SetMode(GameAudio::kEnhanced);
		printf("  music: %s\n", soundfont.c_str());
	} else {
		audio.SetMode(GameAudio::kClassic);
		printf("  music: classic (no SoundFont found)\n");
	}
	bool audioOpen = plat_audio_open(44100, audio_callback, NULL);

	Start();

	static uint8_t picture[kScreenWidth * kScreenHeight];
	std::vector<int16_t> samples;
	const double tickMs = 1000.0 / GameAudio::kTickRate;
	double next = plat_ticks_ms();
	bool quit = false;
	while (!quit) {
		PlatEvent event;
		while (plat_poll_event(&event)) {
			if (event.type == PLAT_EV_QUIT)
				quit = true;
			else if (event.type == PLAT_EV_KEYDOWN) {
				if (event.key > 0 && event.key < MOGK_LAST)
					sTapped[event.key] = true;
				if (event.key == MOGK_q && (event.modifiers & PLAT_MOD_ALT)) {
					quit = true;
					continue;
				}
				switch (event.key) {
					case MOGK_F7:
						plat_next_scaler();
						break;
					case MOGK_F8:
						plat_toggle_stats();
						break;
					case MOGK_F9:
					case MOGK_PRINT:
						plat_save_screenshot();
						break;
					case MOGK_F10:
						audio.SetMode(audio.GetMode() == GameAudio::kEnhanced
							? GameAudio::kClassic : GameAudio::kEnhanced);
						break;
					case MOGK_F11:
						plat_set_fullscreen(!plat_is_fullscreen());
						break;
					case MOGK_F12:
						quit = true;
						break;
					default:
						break;
				}
			}
		}
		update_matrix();

		InterruptTick();
		if (g_unported != NULL) {
			/* Parts not rewritten yet: back to the title. */
			printf("not rewritten yet: %s; back to the title\n", g_unported);
			g_unported = NULL;
			ram(var::GameStatus) = 0;
			ram(var::GameSubstatus) = 0;
		}
		samples.clear();
		audio.Tick(samples, false);
		audio_push(samples);

		screen_render_indexed(g_screen, picture);
		uint8_t palette[768];
		memset(palette, 0, sizeof(palette));
		for (int i = 0; i < 16; i++) {
			uint32_t c = screen_rgb(g_screen, i);
			palette[i * 3] = (uint8_t)(c >> 16);
			palette[i * 3 + 1] = (uint8_t)(c >> 8);
			palette[i * 3 + 2] = (uint8_t)c;
		}
		plat_set_palette(palette);
		plat_present(picture);

		if (audioOpen) {
			/* The sound card sets the pace: keep about 50 ms of sound
			   queued, so the delay stays short and constant and the game
			   runs at exactly the speed of the sound. */
			size_t target;
			uint32_t waited = 0;
			while (audio_fill(&target) > target && waited < 100) {
				plat_sleep_ms(2);
				waited += 2;
			}
		} else {
			next += tickMs;
			double now = plat_ticks_ms();
			if (next > now)
				plat_sleep_ms((uint32_t)(next - now));
			else if (now - next > 250)
				next = now;		/* fell behind: do not try to catch up */
		}
	}
	plat_audio_close();
	plat_shutdown();
	return 0;
}
