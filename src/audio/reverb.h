/*
 * Small stereo reverb (Freeverb structure: 8 comb filters and 4 allpass
 * filters per side), for the SoundFont music.
 */
#ifndef MG_REVERB_H
#define MG_REVERB_H

#include <vector>


class Reverb {
public:
						Reverb();
			void		SetRate(int rate);
	/* room 0..1 (size), damp 0..1, wet 0..1 */
			void		SetParameters(float room, float damp, float wet);
	/* In place on interleaved stereo. */
			void		Process(float* buffer, int count);

private:
	struct Comb {
		std::vector<float> buffer;
		size_t index = 0;
		float store = 0;
	};
	struct Allpass {
		std::vector<float> buffer;
		size_t index = 0;
	};

			Comb		fComb[2][8];
			Allpass		fAllpass[2][4];
			float		fFeedback, fDamp, fWet;
};

#endif
