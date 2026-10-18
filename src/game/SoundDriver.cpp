/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Metal Gear sound driver, see sound_driver.h.
//
// The structure follows the original routines (names in the comments) so
// that every path can be checked against bgmdriver.asm. Data pointers are
// the original 16 bit addresses into the sound banks.
#include "SoundDriver.h"

#include "GameData.h"
#include "GameRam.h"
#include "RomSymbols.h"


namespace {

// Game variables used by the driver.
enum {
	kSoundWorkArea = 0xc010,	// 3 music channels, 32 bytes each
	kSoundWorkAreaB = 0xc030,
	kSoundWorkAreaC = 0xc050,
	kSoundWorkAreaSfx = 0xc070,
	kSoundWorkArea2 = 0xc090,	// saved music while paused
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

// Sound data addresses (sounddata.asm, instruments.asm, bgmdriver.asm).
#define kNoteFrequency RomSymbol(sym::NoteFrequency)
#define kInstruments RomSymbol(sym::Instruments)
#define kIdxSoundData RomSymbol(sym::idxSoundData)

// Fields of a channel work area (SOUND structure).
enum {
	NOTE_COUNTER, NOTE_LENGHT, ID, POINTER_L, POINTER_H, CONFIG, TEMPO,
	ATTACK_VOL, DECAY_VOL, OCTAVE, DECAY_CNT, LOOP_CNT, DECAY_STEPS, RELEASE,
	NOTE_MODE, FREQ_MOD, INSTRUMENT, INSTRUMENT_L, INSTRUMENT_H,
	INSTRUMENT_CNT, INSTRUMENT_LENGHT, FREQ_LOW, FREQ_HIGH, VOLUME,
	RETURN_L, RETURN_H, VIBRATO_WAIT, VIBRATO_CLK, VIBRATO_CFG
};

inline uint8_t
Data(uint16_t address)
{
	return SoundData(address);
}

}	// namespace


SoundDriver gSoundDriver;


SoundDriver::SoundDriver()
	:
	fRegD(0),
	fRegE(0),
	fListener(NULL),
	fC(1),
	fIx(kSoundWorkArea)
{
}


uint8_t&
SoundDriver::_F(int field)
{
	return Ram(static_cast<uint16_t>(fIx + field));
}


void
SoundDriver::_Psg(uint8_t reg, uint8_t value)
{
	if (fListener != NULL)
		fListener->SoundRegister(reg & 0x0f, value);
}


// UpdateSound
void
SoundDriver::Update()
{
	if (Ram(kSoundDataSaved) == 0 && Ram(kBigBossDeadSnd) == 0x53
		&& Ram(kSoundWorkAreaB + ID) == 0) {
		// Big Boss explosion finished: play the escape music.
		Ram(kBigBossDeadSnd) = 0;
		Ram(kAreaMusic) = 0x3b;
		SetSound(0x3b);
	}

	// UpdateSound2
	_SetPsgMixer(Ram(kPsgMixerVal));
	Ram(kFlagSndDatRestored) = 0;

	if (Ram(kRestoreSoundData) != 0) {
		_CopySoundData(kSoundWorkArea2, kSoundWorkArea);
		Ram(kSoundDataSaved) = 0;
		Ram(kFlagSndDatRestored) = 1;
	}

	// UpdateSound3
	if (Ram(kSoundDataSaved) == 0 && Ram(kMusicToSet) != 0)
		_MusicFadeOut();

	// ProcessChannels
	fC = 1;
	fIx = kSoundWorkArea;
	for (int channel = 0; channel < 4; channel++) {
		uint16_t ix = fIx;
		if (_F(ID) != 0)
			_ProcessChannelData();
		else if (fC != 7)
			_CmdEndRetLogic();
		// UpdateSound6
		fC += 2;
		fIx = static_cast<uint16_t>(ix + 0x20);
	}
}


// ProcessChannelData and everything it jumps to.
void
SoundDriver::_ProcessChannelData()
{
	uint8_t& d = fRegD;
	uint8_t& e = fRegE;
	uint16_t hl;
	uint8_t a, b;

ProcessChannelData:
	if (_F(ID) == 0x50) {
		// Radio noise: frequency offset from the radio frequency.
		uint8_t radio = Ram(kRadioFreq);
		b = static_cast<uint8_t>((radio >> 4) + 1);
		a = static_cast<uint8_t>(radio + 6);
		do {
			a = static_cast<uint8_t>(a - 6);
		} while (--b != 0);
		Ram(kRadioFreqOffset) = a;
	}

	// ProcessChannelData2
	if (_F(NOTE_MODE) != 0)
		goto NoteLogic;

	_F(INSTRUMENT) = 0;
	if (--_F(NOTE_COUNTER) != 0)
		goto UpdateFreqVol;

ProcessChannelData3:
	hl = static_cast<uint16_t>(_F(POINTER_L) | (_F(POINTER_H) << 8));
	a = Data(hl);
	if (a == 0xfe)
		goto CmdLoopLogic;
	if (a == 0xff) {
		_CmdEndRetLogic();
		return;
	}

ProcessChanneldData4:
	a = Data(hl);
	if (_F(NOTE_MODE) != 0)
		goto ProcessCommand;

ChkCmd_2x:
	if ((a & 0xf0) == 0x20) {
		_F(CONFIG) = Data(hl);
		hl++;
		a = Data(hl);
		if (_F(INSTRUMENT) == 0)
			_F(NOTE_LENGHT) = a;
		_F(INSTRUMENT_LENGHT) = a;
		hl++;
		if (_F(CONFIG) == 0x20) {
			hl--;
			b = 0;
			goto SfxLogic4;
		}
		// SfxLogic
		a = _F(CONFIG);
		if ((a & 0x08) != 0 && (a & 0x04) == 0) {
			e = Data(hl);
			_Psg(0x0c, e);
			hl++;
			e = Data(hl);
			_Psg(0x0b, e);
			hl++;
		}
	}

	// ChkCmd_1x: noise period
	if ((Data(hl) & 0xf0) == 0x10) {
		e = static_cast<uint8_t>((Data(hl) & 0x0f) * 2);
		if (!(fC == 5 && Ram(kSoundWorkAreaSfx + ID) != 0))
			_Psg(6, e);
		hl++;
	}

	// SfxLogic3
	b = static_cast<uint8_t>(Data(hl) & 0xf0);
	d = static_cast<uint8_t>(Data(hl) & 0x0f);
	e = 0;
	if ((_F(CONFIG) & 3) != 1) {
		hl++;
		e = Data(hl);
	}

SfxLogic4:
	if (_F(INSTRUMENT) != 0) {
		// SetInstrumentPointer
		hl++;
		_F(INSTRUMENT_L) = static_cast<uint8_t>(hl);
		_F(INSTRUMENT_H) = static_cast<uint8_t>(hl >> 8);
	} else
		_PointerNextCmd(hl);

	// SetRadioFreqTone: ex de,hl
	{
		uint16_t de = static_cast<uint16_t>((d << 8) | e);
		d = static_cast<uint8_t>(hl >> 8);
		e = static_cast<uint8_t>(hl);
		hl = de;
	}
	if (Ram(kSoundWorkArea + ID) == 0x50)
		hl = static_cast<uint16_t>((hl & 0xff00)
			| static_cast<uint8_t>(hl - Ram(kRadioFreqOffset)));

	// SetSfxFreq
	_F(FREQ_LOW) = static_cast<uint8_t>(hl);
	_F(FREQ_HIGH) = static_cast<uint8_t>(hl >> 8);
	_UpdateChFreq();
	_F(VOLUME) = static_cast<uint8_t>((b >> 4) | (b << 4));
	if (_F(INSTRUMENT) == 0) {
		// SetNoteLenght
		_F(NOTE_COUNTER) = _F(NOTE_LENGHT);
	} else
		_F(INSTRUMENT_CNT) = _F(INSTRUMENT_LENGHT);
	_UpdateVolume();
	return;

NoteLogic:
	if (--_F(NOTE_COUNTER) == 0)
		goto ProcessChannelData3;
	if (Ram(kMuteSoundFlag) != 0)
		return;
	if (_F(INSTRUMENT) != 0) {
		// InstrumentTick
		if (--_F(INSTRUMENT_CNT) != 0)
			return;
		hl = static_cast<uint16_t>(_F(INSTRUMENT_L) | (_F(INSTRUMENT_H) << 8));
		a = Data(hl);
		if (a == 0xff) {
			// MuteChannel
			_F(CONFIG) = 0;
			_F(VOLUME) = 0;
			_UpdateVolume();
			return;
		}
		goto ChkCmd_2x;
	}
	if ((_F(FREQ_MOD) & 0x04) != 0)
		_ChkVibratoAttack();

	// ADSR
	_F(DECAY_CNT)--;
	a = _F(DECAY_CNT);
	if (a == _F(NOTE_COUNTER)) {
		// Sustain
		e = a;
		if (_F(RELEASE) >= e)
			goto Decay2;
		goto UpdateFreqVol;
	}
	// Decay
	_F(DECAY_CNT)--;
Decay2:
	a = static_cast<uint8_t>(_F(DECAY_VOL) - 1);
	if ((a & 0x80) != 0)
		goto UpdateFreqVol;
	_F(DECAY_VOL) = a;
	_F(VOLUME) = a;
	_UpdateVolume();
	return;

ProcessCommand:
	a = Data(hl);
	if ((a & 0xf0) == 0xd0) {
		// #Dx: tempo
		_F(TEMPO) = static_cast<uint8_t>(a & 0x0f);
		hl++;
		a = Data(hl);
	}
	// ProcessCommand_Fx: #Fx yz = ADSR
	if (a >= 0xf0) {
		_F(ATTACK_VOL) = static_cast<uint8_t>((a & 0x0f) + 1);
		hl++;
		a = Data(hl);
		_F(DECAY_STEPS) = static_cast<uint8_t>(a >> 4);
		_F(RELEASE) = static_cast<uint8_t>(a & 0x0f);
		hl++;
		a = Data(hl);
	}
	// ProcessCommand_Ex
	if (a >= 0xe0) {
		a &= 0x0f;
		if (a < 8) {
			// OctaveCmd
			_F(OCTAVE) = a;
			goto GetNote;
		}
		if (a == 8) {
			// DetuneCmd
			_F(FREQ_MOD) |= 0x08;
			hl++;
			goto ProcessCommand;
		}
		if (a == 0x0c) {
			// VibratoCmd
			_F(FREQ_MOD) |= 0x04;
			hl++;
			_F(VIBRATO_CFG) = Data(hl);
			hl++;
			goto ProcessCommand;
		}
		if (a == 0x0f) {
			// NoteOffCmd
			_F(FREQ_MOD) = 0;
			_F(INSTRUMENT) = 0;
			hl++;
			if (Ram(kMuteSoundFlag) != 0) {
				_F(CONFIG) = 0;
				_F(VOLUME) = 0;
				_UpdateVolume();
			}
			goto ProcessCommand;
		}
		// #E9 (and the unused #EA-#EE): instrument
		hl++;
		_F(INSTRUMENT) = Data(hl);
		goto GetNote;
	}
	goto SetNote;

GetNote:
	hl++;
	a = Data(hl);

SetNote:
	{
		// Length = tempo * (low nibble + 1)
		uint8_t count = static_cast<uint8_t>(a & 0x0f);
		uint8_t length = _F(TEMPO);
		for (int i = 0; i < count; i++)
			length = static_cast<uint8_t>(length + _F(TEMPO));
		_F(NOTE_LENGHT) = length;
	}
	a = Data(hl);
	_PointerNextCmd(hl);
	b = static_cast<uint8_t>(a >> 4);	// note 0-11, 12 = silence

	if (_F(INSTRUMENT) != 0) {
		// SetInstrument
		uint8_t instrument = _F(INSTRUMENT);
		uint16_t de = static_cast<uint16_t>(kInstruments - 2
			+ static_cast<uint8_t>(instrument * 2));
		hl = static_cast<uint16_t>(Data(de) | (Data(static_cast<uint16_t>(de + 1)) << 8));
		de++;
		_F(NOTE_COUNTER) = _F(NOTE_LENGHT);
		if (Ram(kMuteSoundFlag) != 0) {
			d = static_cast<uint8_t>(de >> 8);
			e = static_cast<uint8_t>(de);
			return;
		}
		if (fListener != NULL)
			fListener->NoteStart((fIx - kSoundWorkArea) >> 5, b, _F(OCTAVE),
				instrument);
		hl = static_cast<uint16_t>(hl + static_cast<uint8_t>(b * 2));
		e = Data(hl);
		_F(INSTRUMENT_L) = e;
		hl++;
		d = Data(hl);
		_F(INSTRUMENT_H) = d;
		// ex de,hl
		uint16_t data_pointer = static_cast<uint16_t>((d << 8) | e);
		d = static_cast<uint8_t>(hl >> 8);
		e = static_cast<uint8_t>(hl);
		hl = data_pointer;
		a = Data(hl);
		goto ChkCmd_2x;
	}

	if (b == 12) {
		// MuteNote
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
	_F(FREQ_MOD) &= static_cast<uint8_t>(~0x01);
	_F(VIBRATO_WAIT) = 0;
	_F(VIBRATO_CLK) = 0;
	e = _F(NOTE_LENGHT);
	_F(NOTE_COUNTER) = e;
	if (Ram(kMuteSoundFlag) != 0)
		return;
	_F(DECAY_CNT) = static_cast<uint8_t>(_F(DECAY_STEPS) + e);

	if (fListener != NULL)
		fListener->NoteStart((fIx - kSoundWorkArea) >> 5, b, _F(OCTAVE), 0);

	hl = static_cast<uint16_t>(kNoteFrequency + b);
	hl = Data(hl);
	for (int i = 0; i < _F(OCTAVE); i++)
		hl = static_cast<uint16_t>(hl * 2);
	// SetNoteFreq
	_F(FREQ_LOW) = static_cast<uint8_t>(hl);
	_F(FREQ_HIGH) = static_cast<uint8_t>(hl >> 8);
	_UpdateChFreq();
	_UpdateVolume();
	return;

UpdateFreqVol:
	if (Ram(kFlagSndDatRestored) == 0)
		return;
	_UpdateChFreq();
	_UpdateVolume();
	return;

CmdLoopLogic:
	// #FE xx address: loop, #FE #FF address: call, #FE #FE: goto,
	// #FE #00: switch note mode / sfx mode
	hl++;
	a = Data(hl);
	if (a == 0) {
		// ChangeMode
		if (_F(NOTE_MODE) != 0)
			_F(NOTE_MODE)--;
		else
			_F(NOTE_MODE)++;
		goto DoNextCommand;
	}
	if (a == 0xff) {
		// CmdCallLogic
		hl++;
		e = Data(hl);
		_F(POINTER_L) = e;
		hl++;
		d = Data(hl);
		_F(POINTER_H) = d;
		hl++;
		_F(RETURN_L) = static_cast<uint8_t>(hl);
		_F(RETURN_H) = static_cast<uint8_t>(hl >> 8);
		uint16_t target = static_cast<uint16_t>((d << 8) | e);
		d = static_cast<uint8_t>(hl >> 8);
		e = static_cast<uint8_t>(hl);
		hl = target;
		goto ProcessChanneldData4;
	}
	a = static_cast<uint8_t>(_F(LOOP_CNT) + 1);
	if (a == Data(hl)) {
		// CmdLoopEnd
		hl = static_cast<uint16_t>(hl + 2);
		_F(LOOP_CNT) = 0;
		goto DoNextCommand;
	}
	if (((static_cast<uint8_t>(a - Data(hl))) & 0x80) == 0)
		a--;	// #FE: endless loop (goto)
	// CmdLoopLogic2
	_F(LOOP_CNT) = a;
	hl++;
	_F(POINTER_L) = Data(hl);
	hl++;
	_F(POINTER_H) = Data(hl);
	goto DoNextCommand2;

DoNextCommand:
	_PointerNextCmd(hl);
DoNextCommand2:
	_F(NOTE_COUNTER)++;
	goto ProcessChannelData;
}


// CmdEndRetLogic and CmdEndLogic
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
	// A sound effect has finished: restore music channel 3.
	fC = static_cast<uint8_t>(fC - 2);
	fIx = kSoundWorkAreaC;
	_F(VOLUME) = 5;
	_SetChnFreq();
	_UpdateChVol();
}


// ChkVibratoAttack and VibratoLogic
void
SoundDriver::_ChkVibratoAttack()
{
	uint8_t& d = fRegD;
	uint8_t& e = fRegE;
	uint8_t a;

	if ((_F(FREQ_MOD) & 0x01) == 0) {
		a = static_cast<uint8_t>(_F(VIBRATO_WAIT) + 1);
		if (a < 0x0a) {
			_F(VIBRATO_WAIT) = a;
			return;
		}
		_F(FREQ_MOD)++;
		_F(VIBRATO_WAIT) = 0;
		return;
	}

	e = static_cast<uint8_t>(_F(VIBRATO_CFG) >> 4);	// speed
	a = static_cast<uint8_t>(_F(VIBRATO_WAIT) + 1);
	if (a < e) {
		_F(VIBRATO_WAIT) = a;
		return;
	}
	e = _F(FREQ_LOW);
	d = _F(FREQ_HIGH);
	uint8_t b = static_cast<uint8_t>(_F(VIBRATO_CFG) & 0x0f);
	_F(VIBRATO_CLK) = static_cast<uint8_t>(~_F(VIBRATO_CLK));
	if (_F(VIBRATO_CLK) != 0) {
		if (e < b)
			d--;
		e = static_cast<uint8_t>(e - b);
	} else {
		if (static_cast<int>(e) + b > 0xff)
			d++;
		e = static_cast<uint8_t>(e + b);
	}
	_F(FREQ_LOW) = e;
	_F(FREQ_HIGH) = d;
	_UpdateChFreq();
	_F(VIBRATO_WAIT) = 0;
}


// UpdateChFreq
void
SoundDriver::_UpdateChFreq()
{
	uint8_t& e = fRegE;
	e = Ram(kSoundWorkAreaSfx + ID);
	if (fC < 5) {
		_SetChnFreq();
		return;
	}
	if (fC == 5) {
		if (e != 0)
			return;	// a sound effect uses PSG channel 3
		_SetChnFreq();
		return;
	}
	// UpdateSfxFreq
	if (e == 0)
		return;
	fC = static_cast<uint8_t>(fC - 2);
	_SetChnFreq();
	fC = static_cast<uint8_t>(fC + 2);
}


// SetChnFreq and WrtPsgChnFreq
void
SoundDriver::_SetChnFreq()
{
	uint8_t& e = fRegE;
	uint16_t hl = static_cast<uint16_t>(_F(FREQ_LOW) | (_F(FREQ_HIGH) << 8));
	if ((_F(FREQ_MOD) & 0x08) != 0)
		hl++;	// detune
	e = static_cast<uint8_t>(hl >> 8);
	_Psg(fC, e);
	e = static_cast<uint8_t>(hl);
	_Psg(static_cast<uint8_t>(fC - 1), e);
	if (_F(INSTRUMENT) != 0 || _F(NOTE_MODE) == 0)
		return;
	_F(CONFIG) = 2;
}


// UpdateVolume
void
SoundDriver::_UpdateVolume()
{
	uint8_t& e = fRegE;
	e = Ram(kSoundWorkAreaSfx + ID);
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
	// UpdateSfxVol
	if (e == 0)
		return;
	fC = static_cast<uint8_t>(fC - 2);	// not restored, as in the original
	_UpdateChVol();
}


// UpdateChVol and SetChnVolume
void
SoundDriver::_UpdateChVol()
{
	uint8_t& d = fRegD;
	uint8_t& e = fRegE;

	_UpdateMixer();
	d = static_cast<uint8_t>(((fC >> 1) | (fC << 7)) + 0x88);	// volume register
	uint8_t h = _F(VOLUME);

	if (Ram(kSoundDataSaved) == 0 && _F(NOTE_MODE) != 0
		&& Ram(kVolumeFadeVal) != 0) {
		e = Ram(kVolumeFadeVal);
		uint8_t a = static_cast<uint8_t>(h - e);
		if ((a & 0x80) != 0)
			return;
		if ((_F(CONFIG) & 0x08) != 0)
			return;
		h = a;
	}

	// UpdateChVol2
	uint8_t config = _F(CONFIG);
	if ((config & 0x08) != 0) {
		if ((config & 0x04) != 0)
			return;
		e = h;
		_Psg(0x0d, e);	// envelope shape
		h = 0x10;
	}
	e = h;
	_Psg(d, e);
}


// UpdateMixer
void
SoundDriver::_UpdateMixer()
{
	uint8_t& d = fRegD;
	uint8_t& e = fRegE;

	e = Ram(kPsgMixerVal);
	d = static_cast<uint8_t>(_F(CONFIG) & 3);
	uint8_t a = fC;
	if (a != 1)
		a--;	// 1, 3, 5 -> 1, 2, 4
	uint8_t b = a;
	if ((d & 2) == 0) {
		a |= e;	// tone off
		e = a;
	} else {
		a = static_cast<uint8_t>(~a & e);	// tone on
		e = a;
	}
	a = static_cast<uint8_t>(b << 3);
	if ((d & 1) == 0) {
		a |= e;	// noise off
		e = a;
	} else {
		a = static_cast<uint8_t>(~a & e);	// noise on
		e = a;
	}
	_SetPsgMixer(a);
}


// SetPsgMixer
void
SoundDriver::_SetPsgMixer(uint8_t value)
{
	Ram(kPsgMixerVal) = value;
	fRegE = value;
	_Psg(7, value);
}


// MusicFadeOut
void
SoundDriver::_MusicFadeOut()
{
	uint8_t& step = Ram(kFadeStepCnt);
	step++;
	if (Ram(kSoundWorkArea + ID) == 0x41) {
		if (step < 0x0e)
			return;	// slower fade for the ending music
	} else if (step < 3)
		return;
	step = 0;
	Ram(kVolumeFadeVal)++;
	if (Ram(kVolumeFadeVal) < 11)
		return;
	fRegE = Ram(kMusicToSet);
	Ram(kMusicToSet) = 0;
	Ram(kFadeStepCnt) = 0;
	Ram(kVolumeFadeVal) = 0;
	SetSound(fRegE);
}


// CopySoundData
void
SoundDriver::_CopySoundData(uint16_t from, uint16_t to)
{
	for (int i = 0; i < 0x60; i++)
		Ram(static_cast<uint16_t>(to + i)) = Ram(static_cast<uint16_t>(from + i));
	uint16_t de = static_cast<uint16_t>(to + 0x60);
	fRegD = static_cast<uint8_t>(de >> 8);
	fRegE = static_cast<uint8_t>(de);
	Ram(kRestoreSoundData) = 0;
}


// PointerNextCmd
void
SoundDriver::_PointerNextCmd(uint16_t& hl)
{
	hl++;
	_F(POINTER_L) = static_cast<uint8_t>(hl);
	_F(POINTER_H) = static_cast<uint8_t>(hl >> 8);
}


// SetSound
void
SoundDriver::SetSound(uint8_t id)
{
	uint8_t& d = fRegD;
	uint8_t& e = fRegE;
	uint8_t c = id;

	if (c == 0x53)
		Ram(kBigBossDeadSnd) = c;
	if (c == 0x4d || c == 0x59) {
		// SaveSoundState: pause, keep the music to resume it later.
		_CopySoundData(kSoundWorkArea, kSoundWorkArea2);
		Ram(kSoundDataSaved) = 1;
	}

	// SetSound3
	uint16_t hl = kSoundWorkArea + ID;
	int channels = 1;
	if (c < 0x29) {
		// SetSfx
		hl = kSoundWorkAreaSfx + ID;
		if (Ram(kSoundWorkAreaC + ID) >= 0x59)
			return;	// music paused or stopped
		e = Ram(hl);
		if (c < e)
			return;	// the playing sfx has priority
	} else {
		if (c >= 0x5c)
			channels++;	// also the sfx channel
		channels += 2;
		if (Ram(kSoundDataSaved) == 0) {
			Ram(kFadeStepCnt) = 0;
			Ram(kVolumeFadeVal) = 0;
			Ram(kMusicToSet) = 0;
		}
		if (c != 0x59)
			Ram(kSoundWorkAreaSfx + ID) = 0;
	}

	// GetSoundData
	uint16_t de = static_cast<uint16_t>(kIdxSoundData - 2 + 2 * c);
	hl = static_cast<uint16_t>(hl - 2);
	for (int i = 0; i < channels; i++) {
		Ram(hl) = 1;	// NOTE_COUNTER
		Ram(static_cast<uint16_t>(hl + ID)) = c;
		Ram(static_cast<uint16_t>(hl + POINTER_L)) = Data(de);
		de++;
		Ram(static_cast<uint16_t>(hl + POINTER_H)) = Data(de);
		Ram(static_cast<uint16_t>(hl + LOOP_CNT)) = 0;
		Ram(static_cast<uint16_t>(hl + NOTE_MODE)) = 1;
		Ram(static_cast<uint16_t>(hl + FREQ_MOD)) = 0;
		Ram(static_cast<uint16_t>(hl + INSTRUMENT)) = 0;
		Ram(static_cast<uint16_t>(hl + RETURN_H)) = 0;
		hl = static_cast<uint16_t>(hl + 0x20);
		de++;
	}
	d = static_cast<uint8_t>(de >> 8);
	e = static_cast<uint8_t>(de);
}
