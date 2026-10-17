/*
 * Renders every music (and a sound effect demo) to WAV files, classic and
 * enhanced, for listening to the instrument choices.
 *
 *   music_preview <rom> <soundfont.sf2> <outdir> [seconds] [music.cfg]
 */
#include "../src/audio/game_audio.h"
#include "../src/game/game_data.h"
#include "../src/game/game_ram.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

static void
write_wav(const std::string& path, const std::vector<int16_t>& s, int rate)
{
	FILE* fp = fopen(path.c_str(), "wb");
	if (fp == NULL)
		return;
	uint32_t data = (uint32_t)(s.size() * 2), riff = 36 + data, fmtSize = 16;
	uint16_t fmt = 1, ch = 2, bits = 16, align = 4;
	uint32_t r = (uint32_t)rate, byteRate = r * 4;
	fwrite("RIFF", 1, 4, fp); fwrite(&riff, 4, 1, fp); fwrite("WAVEfmt ", 1, 8, fp);
	fwrite(&fmtSize, 4, 1, fp); fwrite(&fmt, 2, 1, fp); fwrite(&ch, 2, 1, fp);
	fwrite(&r, 4, 1, fp); fwrite(&byteRate, 4, 1, fp); fwrite(&align, 2, 1, fp);
	fwrite(&bits, 2, 1, fp); fwrite("data", 1, 4, fp); fwrite(&data, 4, 1, fp);
	fwrite(s.data(), 2, s.size(), fp);
	fclose(fp);
}

int
main(int argc, char** argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: music_preview <rom> <sf2> <outdir> [seconds] [music.cfg]\n");
		return 2;
	}
	if (!game_data_load(argv[1])) {
		fprintf(stderr, "cannot read ROM\n");
		return 1;
	}
	double seconds = argc > 4 ? atof(argv[4]) : 60;
	static const char* names[] = { "29_intro_theme_of_tara", "2C_theme_of_tara",
		"2F_red_alert", "32_alert", "35_sneaking_mission", "38_tx55_metal_gear",
		"3B_beyond_big_boss", "3E_mercenary", "41_return_of_fox_hunter",
		"44_just_another_dead_soldier" };
	for (int mode = 0; mode < 2; mode++) {
		for (int m = 0; m < 10; m++) {
			memset(g_ram, 0, sizeof(g_ram));
			static GameAudio* audio = NULL;
			delete audio;
			audio = new GameAudio();
			audio->SetRate(44100);
			if (mode == 1) {
				if (!audio->LoadSoundFont(argv[2])) {
					fprintf(stderr, "cannot load %s\n", argv[2]);
					return 1;
				}
				if (argc > 5)
					audio->Music().LoadConfig(argv[5]);
			}
			audio->SetMode(mode == 1 ? GameAudio::kEnhanced : GameAudio::kClassic);
			audio->Driver().SetSound((uint8_t)(0x29 + 3 * m));
			std::vector<int16_t> out;
			long ticks = (long)(seconds * GameAudio::kTickRate);
			for (long t = 0; t < ticks; t++) {
				/* Some sound effects over the music after 20 s */
				if (t > 1200 && t % 150 == 0)
					audio->Driver().SetSound((uint8_t)(t / 150 % 2 ? 0x0c : 0x24));
				audio->Tick(out);
				bool playing = ram(0xc012) | ram(0xc032) | ram(0xc052) | ram(0xc072);
				if (!playing && t > 60)
					break;
			}
			std::string path = std::string(argv[3]) + "/" + names[m]
				+ (mode ? "_enhanced.wav" : "_classic.wav");
			write_wav(path, out, 44100);
			printf("%s %.1f s\n", path.c_str(), out.size() / 2 / 44100.0);
		}
	}
	return 0;
}
