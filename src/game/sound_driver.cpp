/*
 * Metal Gear sound driver, see sound_driver.h.
 *
 * The structure follows the original routines (names in the comments) so
 * that every path can be checked against bgmdriver.asm. Data pointers are
 * the original 16 bit addresses into the sound banks.
 */
#include "sound_driver.h"

#include "game_data.h"
#include "game_ram.h"


namespace {

/* Game variables used by the driver. */
enum {
	kSoundWorkArea = 0xc010,		/* 3 music channels, 32 bytes each */
	kSoundWorkAreaB = 0xc030,
	kSoundWorkAreaC = 0xc050,
	kSoundWorkAreaSfx = 0xc070,
	kSoundWorkArea2 = 0xc090,		/* saved music while paused */
	kPsgMixerVal = 0xc0f0,
	kRestoreSoundData = 0xc0f1,
	kSoundDataSaved = 0xc0f2,
	kFlagSndDatRestored = 0xc0f3,
	kMusicToSet = 0xc0f4,
	kFadeStepCnt = 0xc0f5,
	kVolumeFadeVal = 0xc0f6,
	kMuteSoundFlag = 0xc0f8,
	kRadioFreqOffset = 0xc0f9,
	kBigBossDeadSnd = 0xc0fb,
	kRadioFreq = 0xc138,
	kAreaMusic = 0xc159
};

/* Sound data addresses (sounddata.asm, instruments.asm, bgmdriver.asm). */
enum {
	kNoteFrequency = 0x63fd,
	kInstruments = 0x652b,
	kIdxSoundData = 0x6892
};

/* Fields of a channel work area (SOUND structure). */
enum {
	NOTE_COUNTER, NOTE_LENGHT, ID, POINTER_L, POINTER_H, CONFIG, TEMPO,
	ATTACK_VOL, DECAY_VOL, OCTAVE, DECAY_CNT, LOOP_CNT, DECAY_STEPS, RELEASE,
	NOTE_MODE, FREQ_MOD, INSTRUMENT, INSTRUMENT_L, INSTRUMENT_H,
	INSTRUMENT_CNT, INSTRUMENT_LENGHT, FREQ_LOW, FREQ_HIGH, VOLUME,
	RETURN_L, RETURN_H, VIBRATO_WAIT, VIBRATO_CLK, VIBRATO_CFG
};

inline uint8_t
data(uint16_t address)
{
	return sound_data(address);
}

}	// namespace


SoundDriver::SoundDriver()
	:
	regD(0),
	regE(0),
	fListener(NULL),
	fC(1),
	fIx(kSoundWorkArea)
{
}


uint8_t&
SoundDriver::_F(int field)
{
	return ram((uint16_t)(fIx + field));
}


void
SoundDriver::_Psg(uint8_t reg, uint8_t value)
{
	if (fListener != NULL)
		fListener->SoundRegister(reg & 0x0f, value);
}


/* UpdateSound */
void
SoundDriver::Update()
{
	if (ram(kSoundDataSaved) == 0 && ram(kBigBossDeadSnd) == 0x53
		&& ram(kSoundWorkAreaB + ID) == 0) {
		/* Big Boss explosion finished: play the escape music. */
		ram(kBigBossDeadSnd) = 0;
		ram(kAreaMusic) = 0x3b;
		SetSound(0x3b);
	}

	/* UpdateSound2 */
	_SetPsgMixer(ram(kPsgMixerVal));
	ram(kFlagSndDatRestored) = 0;

	if (ram(kRestoreSoundData) != 0) {
		_CopySoundData(kSoundWorkArea2, kSoundWorkArea);
		ram(kSoundDataSaved) = 0;
		ram(kFlagSndDatRestored) = 1;
	}

	/* UpdateSound3 */
	if (ram(kSoundDataSaved) == 0 && ram(kMusicToSet) != 0)
		_MusicFadeOut();

	/* ProcessChannels */
	fC = 1;
	fIx = kSoundWorkArea;
	for (int channel = 0; channel < 4; channel++) {
		uint16_t ix = fIx;
		if (_F(ID) != 0)
			_ProcessChannelData();
		else if (fC != 7)
			_CmdEndRetLogic();
		/* UpdateSound6 */
		fC += 2;
		fIx = (uint16_t)(ix + 0x20);
	}
}


/* ProcessChannelData and everything it jumps to. */
void
SoundDriver::_ProcessChannelData()
{
	uint8_t& d = regD;
	uint8_t& e = regE;
	uint16_t hl;
	uint8_t a, b;

ProcessChannelData:
	if (_F(ID) == 0x50) {
		/* Radio noise: frequency offset from the radio frequency. */
		uint8_t radio = ram(kRadioFreq);
		b = (uint8_t)((radio >> 4) + 1);
		a = (uint8_t)(radio + 6);
		do {
			a = (uint8_t)(a - 6);
		} while (--b != 0);
		ram(kRadioFreqOffset) = a;
	}

	/* ProcessChannelData2 */
	if (_F(NOTE_MODE) != 0)
		goto NoteLogic;

	_F(INSTRUMENT) = 0;
	if (--_F(NOTE_COUNTER) != 0)
		goto UpdateFreqVol;

ProcessChannelData3:
	hl = (uint16_t)(_F(POINTER_L) | (_F(POINTER_H) << 8));
	a = data(hl);
	if (a == 0xfe)
		goto CmdLoopLogic;
	if (a == 0xff) {
		_CmdEndRetLogic();
		return;
	}

ProcessChanneldData4:
	a = data(hl);
	if (_F(NOTE_MODE) != 0)
		goto ProcessCommand;

ChkCmd_2x:
	if ((a & 0xf0) == 0x20) {
		_F(CONFIG) = data(hl);
		hl++;
		a = data(hl);
		if (_F(INSTRUMENT) == 0)
			_F(NOTE_LENGHT) = a;
		_F(INSTRUMENT_LENGHT) = a;
		hl++;
		if (_F(CONFIG) == 0x20) {
			hl--;
			b = 0;
			goto SfxLogic4;
		}
		/* SfxLogic */
		a = _F(CONFIG);
		if ((a & 0x08) != 0 && (a & 0x04) == 0) {
			e = data(hl);
			_Psg(0x0c, e);
			hl++;
			e = data(hl);
			_Psg(0x0b, e);
			hl++;
		}
	}

	/* ChkCmd_1x: noise period */
	if ((data(hl) & 0xf0) == 0x10) {
		e = (uint8_t)((data(hl) & 0x0f) * 2);
		if (!(fC == 5 && ram(kSoundWorkAreaSfx + ID) != 0))
			_Psg(6, e);
		hl++;
	}

	/* SfxLogic3 */
	b = (uint8_t)(data(hl) & 0xf0);
	d = (uint8_t)(data(hl) & 0x0f);
	e = 0;
	if ((_F(CONFIG) & 3) != 1) {
		hl++;
		e = data(hl);
	}

SfxLogic4:
	if (_F(INSTRUMENT) != 0) {
		/* SetInstrumentPointer */
		hl++;
		_F(INSTRUMENT_L) = (uint8_t)hl;
		_F(INSTRUMENT_H) = (uint8_t)(hl >> 8);
	} else
		_PointerNextCmd(hl);

	/* SetRadioFreqTone: ex de,hl */
	{
		uint16_t de = (uint16_t)((d << 8) | e);
		d = (uint8_t)(hl >> 8);
		e = (uint8_t)hl;
		hl = de;
	}
	if (ram(kSoundWorkArea + ID) == 0x50)
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - ram(kRadioFreqOffset)));

	/* SetSfxFreq */
	_F(FREQ_LOW) = (uint8_t)hl;
	_F(FREQ_HIGH) = (uint8_t)(hl >> 8);
	_UpdateChFreq();
	_F(VOLUME) = (uint8_t)((b >> 4) | (b << 4));
	if (_F(INSTRUMENT) == 0) {
		/* SetNoteLenght */
		_F(NOTE_COUNTER) = _F(NOTE_LENGHT);
	} else
		_F(INSTRUMENT_CNT) = _F(INSTRUMENT_LENGHT);
	_UpdateVolume();
	return;

NoteLogic:
	if (--_F(NOTE_COUNTER) == 0)
		goto ProcessChannelData3;
	if (ram(kMuteSoundFlag) != 0)
		return;
	if (_F(INSTRUMENT) != 0) {
		/* InstrumentTick */
		if (--_F(INSTRUMENT_CNT) != 0)
			return;
		hl = (uint16_t)(_F(INSTRUMENT_L) | (_F(INSTRUMENT_H) << 8));
		a = data(hl);
		if (a == 0xff) {
			/* MuteChannel */
			_F(CONFIG) = 0;
			_F(VOLUME) = 0;
			_UpdateVolume();
			return;
		}
		goto ChkCmd_2x;
	}
	if ((_F(FREQ_MOD) & 0x04) != 0)
		_ChkVibratoAttack();

	/* ADSR */
	_F(DECAY_CNT)--;
	a = _F(DECAY_CNT);
	if (a == _F(NOTE_COUNTER)) {
		/* Sustain */
		e = a;
		if (_F(RELEASE) >= e)
			goto Decay2;
		goto UpdateFreqVol;
	}
	/* Decay */
	_F(DECAY_CNT)--;
Decay2:
	a = (uint8_t)(_F(DECAY_VOL) - 1);
	if ((a & 0x80) != 0)
		goto UpdateFreqVol;
	_F(DECAY_VOL) = a;
	_F(VOLUME) = a;
	_UpdateVolume();
	return;

ProcessCommand:
	a = data(hl);
	if ((a & 0xf0) == 0xd0) {
		/* #Dx: tempo */
		_F(TEMPO) = (uint8_t)(a & 0x0f);
		hl++;
		a = data(hl);
	}
	/* ProcessCommand_Fx: #Fx yz = ADSR */
	if (a >= 0xf0) {
		_F(ATTACK_VOL) = (uint8_t)((a & 0x0f) + 1);
		hl++;
		a = data(hl);
		_F(DECAY_STEPS) = (uint8_t)(a >> 4);
		_F(RELEASE) = (uint8_t)(a & 0x0f);
		hl++;
		a = data(hl);
	}
	/* ProcessCommand_Ex */
	if (a >= 0xe0) {
		a &= 0x0f;
		if (a < 8) {
			/* OctaveCmd */
			_F(OCTAVE) = a;
			goto GetNote;
		}
		if (a == 8) {
			/* DetuneCmd */
			_F(FREQ_MOD) |= 0x08;
			hl++;
			goto ProcessCommand;
		}
		if (a == 0x0c) {
			/* VibratoCmd */
			_F(FREQ_MOD) |= 0x04;
			hl++;
			_F(VIBRATO_CFG) = data(hl);
			hl++;
			goto ProcessCommand;
		}
		if (a == 0x0f) {
			/* NoteOffCmd */
			_F(FREQ_MOD) = 0;
			_F(INSTRUMENT) = 0;
			hl++;
			if (ram(kMuteSoundFlag) != 0) {
				_F(CONFIG) = 0;
				_F(VOLUME) = 0;
				_UpdateVolume();
			}
			goto ProcessCommand;
		}
		/* #E9 (and the unused #EA-#EE): instrument */
		hl++;
		_F(INSTRUMENT) = data(hl);
		goto GetNote;
	}
	goto SetNote;

GetNote:
	hl++;
	a = data(hl);

SetNote:
	{
		/* Length = tempo * (low nibble + 1) */
		uint8_t count = (uint8_t)(a & 0x0f);
		uint8_t length = _F(TEMPO);
		for (int i = 0; i < count; i++)
			length = (uint8_t)(length + _F(TEMPO));
		_F(NOTE_LENGHT) = length;
	}
	a = data(hl);
	_PointerNextCmd(hl);
	b = (uint8_t)(a >> 4);		/* note 0-11, 12 = silence */

	if (_F(INSTRUMENT) != 0) {
		/* SetInstrument */
		uint8_t instrument = _F(INSTRUMENT);
		uint16_t de = (uint16_t)(kInstruments - 2 + (uint8_t)(instrument * 2));
		hl = (uint16_t)(data(de) | (data((uint16_t)(de + 1)) << 8));
		de++;
		_F(NOTE_COUNTER) = _F(NOTE_LENGHT);
		if (ram(kMuteSoundFlag) != 0) {
			d = (uint8_t)(de >> 8);
			e = (uint8_t)de;
			return;
		}
		if (fListener != NULL)
			fListener->NoteStart((fIx - kSoundWorkArea) >> 5, b, _F(OCTAVE),
				instrument);
		hl = (uint16_t)(hl + (uint8_t)(b * 2));
		e = data(hl);
		_F(INSTRUMENT_L) = e;
		hl++;
		d = data(hl);
		_F(INSTRUMENT_H) = d;
		/* ex de,hl */
		uint16_t data_pointer = (uint16_t)((d << 8) | e);
		d = (uint8_t)(hl >> 8);
		e = (uint8_t)hl;
		hl = data_pointer;
		a = data(hl);
		goto ChkCmd_2x;
	}

	if (b == 12) {
		/* MuteNote */
		_F(FREQ_LOW) = 0;
		_F(FREQ_HIGH) = 0;
		_F(VOLUME) = 0;
		_F(NOTE_COUNTER) = _F(NOTE_LENGHT);
		_UpdateChFreq();
		_UpdateVolume();
		return;
	}

	_F(DECAY_VOL) = _F(ATTACK_VOL);
	_F(VOLUME) = _F(ATTACK_VOL);
	_F(FREQ_MOD) &= (uint8_t)~0x01;
	_F(VIBRATO_WAIT) = 0;
	_F(VIBRATO_CLK) = 0;
	e = _F(NOTE_LENGHT);
	_F(NOTE_COUNTER) = e;
	if (ram(kMuteSoundFlag) != 0)
		return;
	_F(DECAY_CNT) = (uint8_t)(_F(DECAY_STEPS) + e);

	if (fListener != NULL)
		fListener->NoteStart((fIx - kSoundWorkArea) >> 5, b, _F(OCTAVE), 0);

	hl = (uint16_t)(kNoteFrequency + b);
	hl = data(hl);
	for (int i = 0; i < _F(OCTAVE); i++)
		hl = (uint16_t)(hl * 2);
	/* SetNoteFreq */
	_F(FREQ_LOW) = (uint8_t)hl;
	_F(FREQ_HIGH) = (uint8_t)(hl >> 8);
	_UpdateChFreq();
	_UpdateVolume();
	return;

UpdateFreqVol:
	if (ram(kFlagSndDatRestored) == 0)
		return;
	_UpdateChFreq();
	_UpdateVolume();
	return;

CmdLoopLogic:
	/* #FE xx address: loop, #FE #FF address: call, #FE #FE: goto,
	   #FE #00: switch note mode / sfx mode */
	hl++;
	a = data(hl);
	if (a == 0) {
		/* ChangeMode */
		if (_F(NOTE_MODE) != 0)
			_F(NOTE_MODE)--;
		else
			_F(NOTE_MODE)++;
		goto DoNextCommand;
	}
	if (a == 0xff) {
		/* CmdCallLogic */
		hl++;
		e = data(hl);
		_F(POINTER_L) = e;
		hl++;
		d = data(hl);
		_F(POINTER_H) = d;
		hl++;
		_F(RETURN_L) = (uint8_t)hl;
		_F(RETURN_H) = (uint8_t)(hl >> 8);
		uint16_t target = (uint16_t)((d << 8) | e);
		d = (uint8_t)(hl >> 8);
		e = (uint8_t)hl;
		hl = target;
		goto ProcessChanneldData4;
	}
	a = (uint8_t)(_F(LOOP_CNT) + 1);
	if (a == data(hl)) {
		/* CmdLoopEnd */
		hl = (uint16_t)(hl + 2);
		_F(LOOP_CNT) = 0;
		goto DoNextCommand;
	}
	if ((((uint8_t)(a - data(hl))) & 0x80) == 0)
		a--;			/* #FE: endless loop (goto) */
	/* CmdLoopLogic2 */
	_F(LOOP_CNT) = a;
	hl++;
	_F(POINTER_L) = data(hl);
	hl++;
	_F(POINTER_H) = data(hl);
	goto DoNextCommand2;

DoNextCommand:
	_PointerNextCmd(hl);
DoNextCommand2:
	_F(NOTE_COUNTER)++;
	goto ProcessChannelData;
}


/* CmdEndRetLogic and CmdEndLogic */
void
SoundDriver::_CmdEndRetLogic()
{
	if (_F(RETURN_H) != 0) {
		_F(POINTER_H) = _F(RETURN_H);
		_F(POINTER_L) = _F(RETURN_L);
		_F(RETURN_H) = 0;
		_F(NOTE_COUNTER) = 1;
		_ProcessChannelData();
		return;
	}

	_F(ID) = 0;
	_F(CONFIG) = 0;
	_F(LOOP_CNT) = 0;
	_F(INSTRUMENT) = 0;
	_F(FREQ_LOW) = 0;
	_F(FREQ_HIGH) = 0;
	_F(VOLUME) = 0;
	_F(VIBRATO_CLK) = 0;

	if (fC < 7) {
		_UpdateVolume();
		return;
	}
	/* A sound effect has finished: restore music channel 3. */
	fC = (uint8_t)(fC - 2);
	fIx = kSoundWorkAreaC;
	_F(VOLUME) = 5;
	_SetChnFreq();
	_UpdateChVol();
}


/* ChkVibratoAttack and VibratoLogic */
void
SoundDriver::_ChkVibratoAttack()
{
	uint8_t& d = regD;
	uint8_t& e = regE;
	uint8_t a;

	if ((_F(FREQ_MOD) & 0x01) == 0) {
		a = (uint8_t)(_F(VIBRATO_WAIT) + 1);
		if (a < 0x0a) {
			_F(VIBRATO_WAIT) = a;
			return;
		}
		_F(FREQ_MOD)++;
		_F(VIBRATO_WAIT) = 0;
		return;
	}

	e = (uint8_t)(_F(VIBRATO_CFG) >> 4);	/* speed */
	a = (uint8_t)(_F(VIBRATO_WAIT) + 1);
	if (a < e) {
		_F(VIBRATO_WAIT) = a;
		return;
	}
	e = _F(FREQ_LOW);
	d = _F(FREQ_HIGH);
	uint8_t b = (uint8_t)(_F(VIBRATO_CFG) & 0x0f);
	_F(VIBRATO_CLK) = (uint8_t)~_F(VIBRATO_CLK);
	if (_F(VIBRATO_CLK) != 0) {
		if (e < b)
			d--;
		e = (uint8_t)(e - b);
	} else {
		if ((int)e + b > 0xff)
			d++;
		e = (uint8_t)(e + b);
	}
	_F(FREQ_LOW) = e;
	_F(FREQ_HIGH) = d;
	_UpdateChFreq();
	_F(VIBRATO_WAIT) = 0;
}


/* UpdateChFreq */
void
SoundDriver::_UpdateChFreq()
{
	uint8_t& e = regE;
	e = ram(kSoundWorkAreaSfx + ID);
	if (fC < 5) {
		_SetChnFreq();
		return;
	}
	if (fC == 5) {
		if (e != 0)
			return;			/* a sound effect uses PSG channel 3 */
		_SetChnFreq();
		return;
	}
	/* UpdateSfxFreq */
	if (e == 0)
		return;
	fC = (uint8_t)(fC - 2);
	_SetChnFreq();
	fC = (uint8_t)(fC + 2);
}


/* SetChnFreq and WrtPsgChnFreq */
void
SoundDriver::_SetChnFreq()
{
	uint8_t& e = regE;
	uint16_t hl = (uint16_t)(_F(FREQ_LOW) | (_F(FREQ_HIGH) << 8));
	if ((_F(FREQ_MOD) & 0x08) != 0)
		hl++;			/* detune */
	e = (uint8_t)(hl >> 8);
	_Psg(fC, e);
	e = (uint8_t)hl;
	_Psg((uint8_t)(fC - 1), e);
	if (_F(INSTRUMENT) != 0 || _F(NOTE_MODE) == 0)
		return;
	_F(CONFIG) = 2;
}


/* UpdateVolume */
void
SoundDriver::_UpdateVolume()
{
	uint8_t& e = regE;
	e = ram(kSoundWorkAreaSfx + ID);
	if (fC < 5) {
		_UpdateChVol();
		return;
	}
	if (fC == 5) {
		if (e != 0)
			return;
		_UpdateChVol();
		return;
	}
	/* UpdateSfxVol */
	if (e == 0)
		return;
	fC = (uint8_t)(fC - 2);		/* not restored, as in the original */
	_UpdateChVol();
}


/* UpdateChVol and SetChnVolume */
void
SoundDriver::_UpdateChVol()
{
	uint8_t& d = regD;
	uint8_t& e = regE;

	_UpdateMixer();
	d = (uint8_t)(((fC >> 1) | (fC << 7)) + 0x88);	/* volume register */
	uint8_t h = _F(VOLUME);

	if (ram(kSoundDataSaved) == 0 && _F(NOTE_MODE) != 0
		&& ram(kVolumeFadeVal) != 0) {
		e = ram(kVolumeFadeVal);
		uint8_t a = (uint8_t)(h - e);
		if ((a & 0x80) != 0)
			return;
		if ((_F(CONFIG) & 0x08) != 0)
			return;
		h = a;
	}

	/* UpdateChVol2 */
	uint8_t config = _F(CONFIG);
	if ((config & 0x08) != 0) {
		if ((config & 0x04) != 0)
			return;
		e = h;
		_Psg(0x0d, e);		/* envelope shape */
		h = 0x10;
	}
	e = h;
	_Psg(d, e);
}


/* UpdateMixer */
void
SoundDriver::_UpdateMixer()
{
	uint8_t& d = regD;
	uint8_t& e = regE;

	e = ram(kPsgMixerVal);
	d = (uint8_t)(_F(CONFIG) & 3);
	uint8_t a = fC;
	if (a != 1)
		a--;				/* 1, 3, 5 -> 1, 2, 4 */
	uint8_t b = a;
	if ((d & 2) == 0) {
		a |= e;				/* tone off */
		e = a;
	} else {
		a = (uint8_t)(~a & e);	/* tone on */
		e = a;
	}
	a = (uint8_t)(b << 3);
	if ((d & 1) == 0) {
		a |= e;				/* noise off */
		e = a;
	} else {
		a = (uint8_t)(~a & e);	/* noise on */
		e = a;
	}
	_SetPsgMixer(a);
}


/* SetPsgMixer */
void
SoundDriver::_SetPsgMixer(uint8_t value)
{
	ram(kPsgMixerVal) = value;
	regE = value;
	_Psg(7, value);
}


/* MusicFadeOut */
void
SoundDriver::_MusicFadeOut()
{
	uint8_t& step = ram(kFadeStepCnt);
	step++;
	if (ram(kSoundWorkArea + ID) == 0x41) {
		if (step < 0x0e)
			return;			/* slower fade for the ending music */
	} else if (step < 3)
		return;
	step = 0;
	ram(kVolumeFadeVal)++;
	if (ram(kVolumeFadeVal) < 11)
		return;
	regE = ram(kMusicToSet);
	ram(kMusicToSet) = 0;
	ram(kFadeStepCnt) = 0;
	ram(kVolumeFadeVal) = 0;
	SetSound(regE);
}


/* CopySoundData */
void
SoundDriver::_CopySoundData(uint16_t from, uint16_t to)
{
	for (int i = 0; i < 0x60; i++)
		ram((uint16_t)(to + i)) = ram((uint16_t)(from + i));
	uint16_t de = (uint16_t)(to + 0x60);
	regD = (uint8_t)(de >> 8);
	regE = (uint8_t)de;
	ram(kRestoreSoundData) = 0;
}


/* PointerNextCmd */
void
SoundDriver::_PointerNextCmd(uint16_t& hl)
{
	hl++;
	_F(POINTER_L) = (uint8_t)hl;
	_F(POINTER_H) = (uint8_t)(hl >> 8);
}


/* SetSound */
void
SoundDriver::SetSound(uint8_t id)
{
	uint8_t& d = regD;
	uint8_t& e = regE;
	uint8_t c = id;

	if (c == 0x53)
		ram(kBigBossDeadSnd) = c;
	if (c == 0x4d || c == 0x59) {
		/* SaveSoundState: pause, keep the music to resume it later. */
		_CopySoundData(kSoundWorkArea, kSoundWorkArea2);
		ram(kSoundDataSaved) = 1;
	}

	/* SetSound3 */
	uint16_t hl = kSoundWorkArea + ID;
	int channels = 1;
	if (c < 0x29) {
		/* SetSfx */
		hl = kSoundWorkAreaSfx + ID;
		if (ram(kSoundWorkAreaC + ID) >= 0x59)
			return;			/* music paused or stopped */
		e = ram(hl);
		if (c < e)
			return;			/* the playing sfx has priority */
	} else {
		if (c >= 0x5c)
			channels++;		/* also the sfx channel */
		channels += 2;
		if (ram(kSoundDataSaved) == 0) {
			ram(kFadeStepCnt) = 0;
			ram(kVolumeFadeVal) = 0;
			ram(kMusicToSet) = 0;
		}
		if (c != 0x59)
			ram(kSoundWorkAreaSfx + ID) = 0;
	}

	/* GetSoundData */
	uint16_t de = (uint16_t)(kIdxSoundData - 2 + 2 * c);
	hl = (uint16_t)(hl - 2);
	for (int i = 0; i < channels; i++) {
		ram(hl) = 1;							/* NOTE_COUNTER */
		ram((uint16_t)(hl + ID)) = c;
		ram((uint16_t)(hl + POINTER_L)) = data(de);
		de++;
		ram((uint16_t)(hl + POINTER_H)) = data(de);
		ram((uint16_t)(hl + LOOP_CNT)) = 0;
		ram((uint16_t)(hl + NOTE_MODE)) = 1;
		ram((uint16_t)(hl + FREQ_MOD)) = 0;
		ram((uint16_t)(hl + INSTRUMENT)) = 0;
		ram((uint16_t)(hl + RETURN_H)) = 0;
		hl = (uint16_t)(hl + 0x20);
		de++;
	}
	d = (uint8_t)(de >> 8);
	e = (uint8_t)de;
}
