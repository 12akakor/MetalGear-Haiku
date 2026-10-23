/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_TONE_SYNTH_H
#define MG_TONE_SYNTH_H


// Tone generator for the classic sound and for the sound effects: three
// square wave voices, a noise source and a volume envelope, controlled by
// the register values the sound driver produces (AY-3-8910 layout).
// Band limited (PolyBLEP) square waves, so the high notes do not alias at
// modern output rates. Stereo: each voice has its own pan position.
//
// SCC layer (optional): what the SCC versions of the game do (the patch by
// Fony, 2018): every write to the PSG also goes to the SCC sound chip of
// a Konami SCC cartridge. Tone period (registers 0-5), volume (8-10, the
// envelope bit is ignored there) and tone on or off (7) of each voice
// drive an SCC voice playing a sawtooth: the same notes, with a fuller
// sound. The SCC runs at 3.58 MHz: f = clock / (32 * (period + 1)), so
// its notes are the PSG notes, high notes a little lower.

#include <stdint.h>


class ToneSynth {
public:
						ToneSynth();
			void		Reset();

			void		Write(int reg, uint8_t value);
	inline	uint8_t		Register(int reg) const;

	// Voices that are heard (bit 0 = A, 1 = B, 2 = C).
	inline	void		SetVoiceMask(int mask);
	inline	void		SetPan(int voice, float pan);
	inline	void		SetScc(bool on, float gain = 0.4f);
	inline	bool		Scc() const;

	// Adds 'count' stereo frames to 'out' (interleaved), scaled by gain.
			void		Render(float* out, int count, int rate, float gain);

private:
			float		_Envelope() const;

			uint8_t		fReg[16];
			int			fMask;
			float		fPan[3];

			double		fPhase[3];
			double		fNoisePhase;
			uint32_t	fLfsr;
			int			fNoise;
			double		fEnvPhase;
			int			fEnvStep;
			bool		fEnvHold;
			int			fEnvHeld;
			bool		fEnvAttack;
			bool		fEnvAlternate;
			bool		fEnvHoldMode;
			bool		fEnvContinue;
			float		fLevel[3];	// smoothed volume (no clicks)
			float		fDcLeft, fDcRight, fDcInLeft, fDcInRight;

			bool		fScc;
			float		fSccGain;
			double		fSccPhase[3];
			float		fSccLevel[3];
};

inline uint8_t
ToneSynth::Register(int reg) const
{
	return fReg[reg & 15];
}


inline void
ToneSynth::SetVoiceMask(int mask)
{
	fMask = mask;
}


inline void
ToneSynth::SetPan(int voice, float pan)
{
	fPan[voice] = pan;
}


inline void
ToneSynth::SetScc(bool on, float gain)
{
	fScc = on;
	fSccGain = gain;
}


inline bool
ToneSynth::Scc() const
{
	return fScc;
}


#endif	// MG_TONE_SYNTH_H
