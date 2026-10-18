/*
 * Music and sound effect driver of Metal Gear, rewritten in C++ from
 * sound/bgmdriver.asm and sound/setsound.asm of the disassembly.
 *
 * The driver keeps its state in the game variables (#C010-#C0FB) exactly
 * as the original does, and produces the same PSG register values each
 * tick. Those values describe what each of the three voices plays; the
 * renderers (classic tone generator or SoundFont instruments) turn them
 * into sound. The driver also reports note starts, so the SoundFont
 * renderer can play real instruments instead of reproducing the tones.
 */
#ifndef MG_SOUND_DRIVER_H
#define MG_SOUND_DRIVER_H

#include <stdint.h>


class SoundListener {
public:
	virtual			~SoundListener() {}
	/* reg 0-13 as on the AY-3-8910: 0-5 tone periods, 6 noise period,
	   7 mixer, 8-10 volumes, 11-13 envelope. */
	virtual	void	SoundRegister(int reg, uint8_t value) = 0;
	/* A note starts in work area 'area' (0-2 music, 3 sfx).
	   'note' 0-11 (C to B), 'octave' as in the data (0 = highest),
	   'instrument' 0 for a plain tone, else the instrument number. */
	virtual	void	NoteStart(int area, int note, int octave, int instrument)
						{ (void)area; (void)note; (void)octave; (void)instrument; }
};


class SoundDriver {
public:
						SoundDriver();

			void		SetListener(SoundListener* listener)
							{ fListener = listener; }

	/* Called once per frame (60 Hz). */
			void		Update();
	/* Starts music or a sound effect (sound IDs as in sounddata.asm). */
			void		SetSound(uint8_t id);
	/* A direct register write by the game (BIOS WRTPSG). */
			void		WritePsg(int reg, uint8_t value)
							{ if (fListener) fListener->SoundRegister(reg & 0x0f, value); }

	/* Register values the original carries from one call to the next
	   (they only matter in a few corner cases). Public for the tests. */
			uint8_t		regD, regE;

private:
			uint8_t&	_F(int field);
			void		_Psg(uint8_t reg, uint8_t value);

			void		_ProcessChannelData();
			void		_CmdEndRetLogic();
			void		_ChkVibratoAttack();
			void		_UpdateChFreq();
			void		_SetChnFreq();
			void		_UpdateVolume();
			void		_UpdateChVol();
			void		_UpdateMixer();
			void		_SetPsgMixer(uint8_t value);
			void		_MusicFadeOut();
			void		_CopySoundData(uint16_t from, uint16_t to);
			void		_PointerNextCmd(uint16_t& hl);

			SoundListener* fListener;
			uint8_t		fC;		/* PSG tone register (high) of the channel */
			uint16_t	fIx;	/* work area of the channel */
};

/* The game's sound driver. */
extern SoundDriver g_soundDriver;

#endif
