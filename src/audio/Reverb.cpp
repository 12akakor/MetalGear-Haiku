/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Reverb.h"


namespace {

// Freeverb tunings at 44.1 kHz; the right side is spread by 23 samples.
const int kCombTuning[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
const int kAllpassTuning[4] = { 556, 441, 341, 225 };
const int kSpread = 23;

}	// namespace


Reverb::Reverb()
	:
	fFeedback(0.84f),
	fDamp(0.2f),
	fWet(0.18f)
{
	SetRate(44100);
}


void
Reverb::SetRate(int rate)
{
	double scale = rate / 44100.0;
	for (int side = 0; side < 2; side++) {
		for (int i = 0; i < 8; i++) {
			fComb[side][i].buffer.assign(
				static_cast<size_t>((kCombTuning[i] + side * kSpread) * scale) + 1, 0.0f);
			fComb[side][i].index = 0;
			fComb[side][i].store = 0;
		}
		for (int i = 0; i < 4; i++) {
			fAllpass[side][i].buffer.assign(
				static_cast<size_t>((kAllpassTuning[i] + side * kSpread) * scale) + 1, 0.0f);
			fAllpass[side][i].index = 0;
		}
	}
}


void
Reverb::SetParameters(float room, float damp, float wet)
{
	fFeedback = 0.7f + room * 0.28f;
	fDamp = damp * 0.4f;
	fWet = wet;
}


void
Reverb::Process(float* buffer, int count)
{
	for (int n = 0; n < count; n++) {
		float input = (buffer[n * 2] + buffer[n * 2 + 1]) * 0.015f;
		for (int side = 0; side < 2; side++) {
			float out = 0;
			for (int i = 0; i < 8; i++) {
				Comb& c = fComb[side][i];
				float y = c.buffer[c.index];
				c.store = y * (1.0f - fDamp) + c.store * fDamp;
				c.buffer[c.index] = input + c.store * fFeedback;
				if (++c.index >= c.buffer.size())
					c.index = 0;
				out += y;
			}
			for (int i = 0; i < 4; i++) {
				Allpass& a = fAllpass[side][i];
				float b = a.buffer[a.index];
				a.buffer[a.index] = out + b * 0.5f;
				if (++a.index >= a.buffer.size())
					a.index = 0;
				out = b - out;
			}
			buffer[n * 2 + side] += out * fWet;
		}
	}
}
