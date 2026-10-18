#include "tone_synth.h"

#include <math.h>
#include <string.h>


namespace {

const double kClock = 1789772.5;

/* 3 dB per step, 0 is silent. */
struct AmplitudeTable {
	float level[16];
	AmplitudeTable()
	{
		level[0] = 0.0f;
		for (int v = 1; v < 16; v++)
			level[v] = (float)pow(2.0, (v - 15) / 2.0);
	}
};
const AmplitudeTable kAmplitude;

inline float
volume_amplitude(int v)
{
	return kAmplitude.level[v & 15];
}

inline double
poly_blep(double t, double dt)
{
	if (t < dt) {
		t /= dt;
		return t + t - t * t - 1.0;
	}
	if (t > 1.0 - dt) {
		t = (t - 1.0) / dt;
		return t * t + t + t + 1.0;
	}
	return 0.0;
}

}	// namespace


ToneSynth::ToneSynth()
{
	Reset();
}


void
ToneSynth::Reset()
{
	memset(fReg, 0, sizeof(fReg));
	fReg[7] = 0xff;
	fMask = 7;
	fPan[0] = 0.35f;
	fPan[1] = 0.65f;
	fPan[2] = 0.5f;
	for (int i = 0; i < 3; i++) {
		fPhase[i] = 0;
		fLevel[i] = 0;
	}
	fNoisePhase = 0;
	fLfsr = 1;
	fNoise = 1;
	fEnvPhase = 0;
	fEnvStep = 0;
	fEnvHold = true;
	fEnvHeld = 0;
	fEnvAttack = false;
	fEnvAlternate = fEnvHoldMode = fEnvContinue = false;
	fDcLeft = fDcRight = fDcInLeft = fDcInRight = 0;
}


void
ToneSynth::Write(int reg, uint8_t value)
{
	reg &= 15;
	fReg[reg] = value;
	if (reg == 13) {
		fEnvContinue = (value & 8) != 0;
		fEnvAttack = (value & 4) != 0;
		fEnvAlternate = (value & 2) != 0;
		fEnvHoldMode = (value & 1) != 0;
		fEnvStep = 0;
		fEnvPhase = 0;
		fEnvHold = false;
	}
}


float
ToneSynth::_Envelope() const
{
	int level = fEnvAttack ? fEnvStep : 15 - fEnvStep;
	return volume_amplitude(level);
}


void
ToneSynth::Render(float* out, int count, int rate, float gain)
{
	double toneDt[3];
	for (int c = 0; c < 3; c++) {
		int period = fReg[c * 2] | ((fReg[c * 2 + 1] & 0x0f) << 8);
		if (period < 1)
			period = 1;
		toneDt[c] = kClock / (16.0 * period) / rate;
	}
	int noisePeriod = fReg[6] & 0x1f;
	if (noisePeriod < 1)
		noisePeriod = 1;
	double noiseDt = kClock / (16.0 * noisePeriod) / rate;
	int envPeriod = fReg[11] | (fReg[12] << 8);
	if (envPeriod < 1)
		envPeriod = 1;
	double envDt = kClock / (16.0 * envPeriod) / rate;

	float target[3];
	bool useEnv[3];
	for (int c = 0; c < 3; c++) {
		useEnv[c] = (fReg[8 + c] & 0x10) != 0;
		target[c] = (fMask & (1 << c)) ? volume_amplitude(fReg[8 + c] & 0x0f) : 0.0f;
	}
	const float smooth = 1.0f - expf(-1.0f / (0.002f * rate));	/* 2 ms */

	for (int i = 0; i < count; i++) {
		/* noise: 17 bit LFSR */
		fNoisePhase += noiseDt;
		while (fNoisePhase >= 1.0) {
			fNoisePhase -= 1.0;
			fLfsr = (fLfsr >> 1) | (((fLfsr ^ (fLfsr >> 3)) & 1) << 16);
			fNoise = (int)(fLfsr & 1);
		}
		/* envelope */
		if (!fEnvHold) {
			fEnvPhase += envDt;
			while (fEnvPhase >= 1.0 && !fEnvHold) {
				fEnvPhase -= 1.0;
				if (++fEnvStep > 15) {
					fEnvStep = 0;
					if (!fEnvContinue) {
						fEnvHeld = 0;
						fEnvHold = true;
					} else if (fEnvHoldMode) {
						fEnvHeld = (fEnvAttack != fEnvAlternate) ? 15 : 0;
						fEnvHold = true;
					} else if (fEnvAlternate)
						fEnvAttack = !fEnvAttack;
				}
			}
		}
		float env = fEnvHold ? volume_amplitude(fEnvHeld) : _Envelope();

		float left = 0, right = 0;
		for (int c = 0; c < 3; c++) {
			float level = (fMask & (1 << c)) ? (useEnv[c] ? env : target[c]) : 0.0f;
			fLevel[c] += (level - fLevel[c]) * smooth;
			if (fLevel[c] < 1e-5f && level == 0)
				continue;
			bool toneOn = (fReg[7] & (1 << c)) == 0;
			bool noiseOn = (fReg[7] & (8 << c)) == 0;
			double square = 1.0;
			if (toneOn) {
				double dt = toneDt[c];
				fPhase[c] += dt;
				if (fPhase[c] >= 1.0)
					fPhase[c] -= floor(fPhase[c]);
				if (dt < 0.5) {
					double t = fPhase[c];
					double s = t < 0.5 ? 1.0 : -1.0;
					s += poly_blep(t, dt);
					double t2 = t + 0.5;
					if (t2 >= 1.0)
						t2 -= 1.0;
					s -= poly_blep(t2, dt);
					square = 0.5 * (s + 1.0);
				} else
					square = 0.5;	/* above the audio range */
			}
			double value = square * (noiseOn ? fNoise : 1);
			float v = (float)value * fLevel[c];
			left += v * (1.0f - fPan[c]);
			right += v * fPan[c];
		}
		/* DC blocker */
		float l = left - fDcInLeft + 0.9995f * fDcLeft;
		fDcInLeft = left;
		fDcLeft = l;
		float r = right - fDcInRight + 0.9995f * fDcRight;
		fDcInRight = right;
		fDcRight = r;
		out[i * 2] += l * gain;
		out[i * 2 + 1] += r * gain;
	}
}
