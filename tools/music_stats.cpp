/* Statistics of the note events per music and channel (for choosing
   instruments). */
#include "../src/game/GameData.h"
#include "../src/game/GameRam.h"
#include "../src/game/SoundDriver.h"
#include <stdio.h>
#include <string.h>
#include <map>

struct Stats { int notes = 0, minN = 999, maxN = -1; long sum = 0; std::map<int,int> instr; long len = 0; };

class L : public SoundListener {
public:
	void SoundRegister(int, uint8_t) {}
	void NoteStart(int area, int note, int octave, int instrument) {
		Stats& s = stats[area];
		s.instr[instrument]++;
		if (instrument == 0) {
			int m = 84 + note - 12 * octave;
			s.notes++; s.sum += m; if (m < s.minN) s.minN = m; if (m > s.maxN) s.maxN = m;
			s.len += Ram(0xc010 + area * 32 + 1);
		}
	}
	Stats stats[4];
};

int main(int argc, char** argv) {
	LoadGameRom(argv[1]);
	const char* names[] = {"IntroTara","ThemeTara","RedAlert","Alert","Basement","MetalGear","Escape","Boss","Ending","GameOver"};
	for (int m = 0; m < 10; m++) {
		memset(gRam, 0, sizeof(gRam));
		SoundDriver d; L l; d.SetListener(&l);
		d.SetSound((uint8_t)(0x29 + 3 * m));
		long t; for (t = 0; t < 7200 && Ram(0xc012) | Ram(0xc032) | Ram(0xc052); t++) d.Update();
		printf("%02X %-10s ticks %ld\n", 0x29 + 3 * m, names[m], t);
		for (int a = 0; a < 3; a++) {
			Stats& s = l.stats[a];
			printf("   ch%d tones %4d range %3d-%3d avg %5.1f avglen %4.1f instr:", a, s.notes, s.minN, s.maxN,
				s.notes ? (double)s.sum / s.notes : 0.0, s.notes ? (double)s.len / s.notes : 0.0);
			for (auto& i : s.instr) printf(" %d:%d", i.first, i.second);
			printf("\n");
		}
	}
}
