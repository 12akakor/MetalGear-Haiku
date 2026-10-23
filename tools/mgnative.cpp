/*
 * Runs the native game without a window: frames as PPM, sound as WAV.
 *
 *   mgnative <rom> <ticks> [-shot n,n,...] [-every n] [-keys file] [-out dir]
 *            [-wav file] [-sf2 soundfont] [-scc] [-noflicker] [-hd dir]
 */
#include "../src/audio/GameAudio.h"
#include "../src/game/GameData.h"
#include "../src/game/GameMain.h"
#include "../src/game/HdTiles.h"
#include "../src/game/Input.h"
#include "../src/game/Options.h"
#include "../src/game/Screen.h"
#include "../src/game/ScreenRender.h"
#include "../src/platform/VideoScaler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <string>
#include <vector>

static void
write_ppm(const std::string& path, const uint32_t* p, int width = kScreenWidth,
	int height = kScreenHeight)
{
	FILE* fp = fopen(path.c_str(), "wb");
	if (fp == NULL)
		return;
	fprintf(fp, "P6\n%d %d\n255\n", width, height);
	for (int i = 0; i < width * height; i++) {
		uint8_t rgb[3] = { (uint8_t)(p[i] >> 16), (uint8_t)(p[i] >> 8), (uint8_t)p[i] };
		fwrite(rgb, 1, 3, fp);
	}
	fclose(fp);
}

int
main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: mgnative <rom> <ticks> [options]\n");
		return 2;
	}
	std::string error;
	if (!LoadGameRom(argv[1], &error)) {
		fprintf(stderr, "%s\n", error.c_str());
		return 1;
	}
	long ticks = atol(argv[2]);
	std::set<long> shots;
	long every = 0;
	std::string outDir = "out", wav, sf2;
	bool scc = false;
	const char* keys = NULL;
	for (int i = 3; i < argc; i++) {
		if (!strcmp(argv[i], "-shot") && i + 1 < argc) {
			char* s = argv[++i];
			while (*s) {
				shots.insert(strtol(s, &s, 10));
				if (*s == ',')
					s++;
			}
		} else if (!strcmp(argv[i], "-every") && i + 1 < argc)
			every = atol(argv[++i]);
		else if (!strcmp(argv[i], "-keys") && i + 1 < argc)
			keys = argv[++i];
		else if (!strcmp(argv[i], "-out") && i + 1 < argc)
			outDir = argv[++i];
		else if (!strcmp(argv[i], "-wav") && i + 1 < argc)
			wav = argv[++i];
		else if (!strcmp(argv[i], "-sf2") && i + 1 < argc)
			sf2 = argv[++i];
		else if (!strcmp(argv[i], "-scc"))
			scc = true;
		else if (!strcmp(argv[i], "-noflicker"))
			gOptions.noFlicker = true;
		else if (!strcmp(argv[i], "-hd") && i + 1 < argc) {
			gOptions.hdTiles = LoadHdTiles(argv[++i]) > 0;
			printf("%d HD tiles\n", HdTileCount());
		}
	}
	struct Key { long tick; int row, bit, down; };
	std::vector<Key> script;
	if (keys != NULL) {
		FILE* fp = fopen(keys, "r");
		Key k;
		while (fp != NULL && fscanf(fp, "%ld %d %d %d", &k.tick, &k.row, &k.bit, &k.down) == 4)
			script.push_back(k);
		if (fp != NULL)
			fclose(fp);
	}

	GameAudio audio;
	audio.SetRate(44100);
	if (!sf2.empty() && audio.LoadSoundFont(sf2.c_str()))
		audio.SetMode(GameAudio::kEnhanced);
	else
		audio.SetMode(GameAudio::kClassic);
	if (scc)
		audio.SetMode(GameAudio::kScc);
	std::vector<int16_t> sound;
	std::vector<uint32_t> frame(kScreenWidth * kScreenHeight);

	Start();
	size_t next = 0;
	for (long t = 0; t < ticks; t++) {
		while (next < script.size() && script[next].tick == t) {
			if (script[next].down)
				gKeyMatrix[script[next].row] &= (uint8_t)~(1 << script[next].bit);
			else
				gKeyMatrix[script[next].row] |= (uint8_t)(1 << script[next].bit);
			next++;
		}
		InterruptTick();
		audio.Tick(sound, false);
		RenderScreen(gScreen, frame.data());
		if (shots.count(t) || (every > 0 && t % every == 0)) {
			char name[64];
			snprintf(name, sizeof(name), "/native_%05ld.ppm", t);
			write_ppm(outDir + name, frame.data());
			if (gOptions.hdTiles) {
				// as the game shows it: xBR 4x, HD tiles over it
				std::vector<uint8_t> indexed(kScreenWidth * kScreenHeight);
				std::vector<uint8_t> sprites(indexed.size()), local(indexed.size());
				std::vector<const uint32_t*> tiles(indexed.size());
				RenderScreenIndexed(gScreen, indexed.data(), sprites.data());
				std::vector<uint32_t> big(frame.size() * 16);
				XbrScaler::Scale(frame.data(), kScreenWidth, kScreenHeight, 4, big.data());
				if (HdBuildOverlay(gScreen, sprites.data(), tiles.data(), local.data())) {
					HdCompose(tiles.data(), local.data(), kScreenWidth, kScreenHeight,
						big.data(), kScreenWidth * 4, 0, 0, kScreenWidth * 4,
						kScreenHeight * 4);
				}
				snprintf(name, sizeof(name), "/hd_%05ld.ppm", t);
				write_ppm(outDir + name, big.data(), kScreenWidth * 4, kScreenHeight * 4);
			}
		}
		if (gUnported != NULL) {
			printf("tick %ld: not rewritten yet: %s\n", t, gUnported);
			break;
		}
	}
	if (!wav.empty()) {
		FILE* fp = fopen(wav.c_str(), "wb");
		uint32_t data = (uint32_t)(sound.size() * 2), riff = 36 + data, fmtSize = 16, rate = 44100,
			byteRate = rate * 4;
		uint16_t fmt = 1, ch = 2, bits = 16, align = 4;
		fwrite("RIFF", 1, 4, fp); fwrite(&riff, 4, 1, fp); fwrite("WAVEfmt ", 1, 8, fp);
		fwrite(&fmtSize, 4, 1, fp); fwrite(&fmt, 2, 1, fp); fwrite(&ch, 2, 1, fp);
		fwrite(&rate, 4, 1, fp); fwrite(&byteRate, 4, 1, fp); fwrite(&align, 2, 1, fp);
		fwrite(&bits, 2, 1, fp); fwrite("data", 1, 4, fp); fwrite(&data, 4, 1, fp);
		fwrite(sound.data(), 2, sound.size(), fp);
		fclose(fp);
	}
	return 0;
}
