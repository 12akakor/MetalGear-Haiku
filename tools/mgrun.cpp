/*
 * Headless run of the ROM on the reference machine (interpreter).
 *
 *   mgrun <rom> <frames> [options]
 *     -shot N,N,...   write frame N as out/frame_NNNNN.ppm
 *     -every N        write every N-th frame
 *     -wav file       write the sound as 44.1 kHz mono WAV
 *     -keys file      input script: lines "frame row bit down"
 *     -out dir        output directory (default "out")
 *     -stats          print BIOS call counts and VDP command count
 */
#include "../src/msx/machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <string>
#include <vector>


static bool
read_file(const char* path, std::vector<uint8_t>& data)
{
	FILE* fp = fopen(path, "rb");
	if (fp == NULL)
		return false;
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	data.resize((size_t)size);
	bool ok = fread(data.data(), 1, data.size(), fp) == data.size();
	fclose(fp);
	return ok;
}


static void
write_ppm(const std::string& path, const uint32_t* pixels, int w, int h)
{
	FILE* fp = fopen(path.c_str(), "wb");
	if (fp == NULL)
		return;
	fprintf(fp, "P6\n%d %d\n255\n", w, h);
	for (int i = 0; i < w * h; i++) {
		uint8_t rgb[3] = { (uint8_t)(pixels[i] >> 16), (uint8_t)(pixels[i] >> 8),
			(uint8_t)pixels[i] };
		fwrite(rgb, 1, 3, fp);
	}
	fclose(fp);
}


static void
write_wav(const char* path, const std::vector<int16_t>& samples, int rate)
{
	FILE* fp = fopen(path, "wb");
	if (fp == NULL)
		return;
	uint32_t dataSize = (uint32_t)(samples.size() * 2);
	uint32_t riff = 36 + dataSize;
	uint16_t fmt = 1, ch = 1, bits = 16, align = 2;
	uint32_t fmtSize = 16, byteRate = (uint32_t)rate * 2, r = (uint32_t)rate;
	fwrite("RIFF", 1, 4, fp); fwrite(&riff, 4, 1, fp); fwrite("WAVE", 1, 4, fp);
	fwrite("fmt ", 1, 4, fp); fwrite(&fmtSize, 4, 1, fp); fwrite(&fmt, 2, 1, fp);
	fwrite(&ch, 2, 1, fp); fwrite(&r, 4, 1, fp); fwrite(&byteRate, 4, 1, fp);
	fwrite(&align, 2, 1, fp); fwrite(&bits, 2, 1, fp);
	fwrite("data", 1, 4, fp); fwrite(&dataSize, 4, 1, fp);
	fwrite(samples.data(), 2, samples.size(), fp);
	fclose(fp);
}


int
main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: mgrun <rom> <frames> [options]\n");
		return 2;
	}
	std::vector<uint8_t> rom;
	if (!read_file(argv[1], rom)) {
		fprintf(stderr, "cannot read %s\n", argv[1]);
		return 1;
	}
	long frames = atol(argv[2]);
	std::set<long> shots;
	long every = 0;
	const char* wav = NULL;
	const char* keys = NULL;
	std::string outDir = "out";
	bool stats = false;
	long trace = 0;
	const char* stateFile = NULL;
	int dumpStatus = -1, dumpSub = -1;
	const char* dumpPrefix = NULL;
	for (int i = 3; i < argc; i++) {
		if (strcmp(argv[i], "-shot") == 0 && i + 1 < argc) {
			char* s = argv[++i];
			while (*s) {
				shots.insert(strtol(s, &s, 10));
				if (*s == ',')
					s++;
			}
		} else if (strcmp(argv[i], "-every") == 0 && i + 1 < argc)
			every = atol(argv[++i]);
		else if (strcmp(argv[i], "-wav") == 0 && i + 1 < argc)
			wav = argv[++i];
		else if (strcmp(argv[i], "-keys") == 0 && i + 1 < argc)
			keys = argv[++i];
		else if (strcmp(argv[i], "-out") == 0 && i + 1 < argc)
			outDir = argv[++i];
		else if (strcmp(argv[i], "-stats") == 0)
			stats = true;
		else if (strcmp(argv[i], "-dump") == 0 && i + 2 < argc) {
			sscanf(argv[++i], "%d,%d", &dumpStatus, &dumpSub);
			dumpPrefix = argv[++i];
		}
		else if (strcmp(argv[i], "-state") == 0 && i + 1 < argc)
			stateFile = argv[++i];
		else if (strcmp(argv[i], "-trace") == 0 && i + 1 < argc)
			trace = atol(argv[++i]);
	}

	struct KeyEvent { long frame; int row, bit, down; };
	std::vector<KeyEvent> events;
	if (keys != NULL) {
		FILE* fp = fopen(keys, "r");
		KeyEvent e;
		while (fp != NULL && fscanf(fp, "%ld %d %d %d", &e.frame, &e.row, &e.bit,
				&e.down) == 4)
			events.push_back(e);
		if (fp != NULL)
			fclose(fp);
	}

	static Machine machine;
	if (!machine.LoadRom(rom)) {
		fprintf(stderr, "ROM must be 128 KB\n");
		return 1;
	}
	machine.Reset();
	machine.traceLeft = trace;

	std::vector<int16_t> audio;
	const int rate = 44100;
	double audioOwed = 0.0;
	const double samplesPerFrame = rate / (3579545.0 / Machine::kCyclesPerFrame);
	size_t nextEvent = 0;
	FILE* state = stateFile != NULL ? fopen(stateFile, "w") : NULL;
	for (long f = 0; f < frames; f++) {
		while (nextEvent < events.size() && events[nextEvent].frame == f) {
			machine.SetKey(events[nextEvent].row, events[nextEvent].bit,
				events[nextEvent].down != 0);
			nextEvent++;
		}
		machine.RunFrame();
		if (dumpPrefix != NULL && machine.Ram()[0] == dumpStatus
			&& machine.Ram()[1] == dumpSub) {
			std::string base(dumpPrefix);
			FILE* fp = fopen((base + "_vram.bin").c_str(), "wb");
			fwrite(machine.GetVdp().Vram(), 1, 0x20000, fp);
			fclose(fp);
			fp = fopen((base + "_ram.bin").c_str(), "wb");
			fwrite(machine.Ram(), 1, 0x4000, fp);
			fclose(fp);
			fp = fopen((base + "_regs.txt").c_str(), "w");
			for (int r = 0; r < 47; r++)
				fprintf(fp, "%d %d\n", r, machine.GetVdp().Register(r));
			fclose(fp);
			printf("dump at frame %ld\n", f);
			dumpPrefix = NULL;
		}
		if (state != NULL) {
			uint8_t* ram = machine.Ram();
			fprintf(state, "%ld %d %d %d %d\n", f, ram[0], ram[1], ram[3], ram[5]);
		}
		if (wav != NULL) {
			audioOwed += samplesPerFrame;
			int n = (int)audioOwed;
			audioOwed -= n;
			size_t old = audio.size();
			audio.resize(old + (size_t)n);
			machine.GetPsg().Render(audio.data() + old, n, rate);
		}
		if (shots.count(f) || (every > 0 && f % every == 0)) {
			char name[64];
			snprintf(name, sizeof(name), "/frame_%05ld.ppm", f);
			write_ppm(outDir + name, machine.GetVdp().Frame(), Vdp::kWidth,
				Vdp::kMaxLines);
		}
	}
	if (wav != NULL)
		write_wav(wav, audio, rate);
	if (state != NULL)
		fclose(state);
	if (stats) {
		printf("frames %ld, VDP commands %u, PC %04X, banks %d %d %d\n", frames,
			machine.GetVdp().commandCount, machine.Cpu().r.pc,
			machine.Bank(1), machine.Bank(2), machine.Bank(3));
		for (int i = 0; i < 0x200; i++) {
			if (machine.biosCalls[i])
				printf("  BIOS %04X: %u\n", i, machine.biosCalls[i]);
		}
	}
	return 0;
}
