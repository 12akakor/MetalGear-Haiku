/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from logic/saveload.asm. The status of the save and load
// screens is SaveLoadStat; the original dispatches it with a djnz chain.
#include "SaveLoad.h"

#include <string.h>

#include "GameMain.h"
#include "GfxBasic.h"
#include "Input.h"
#include "RamSymbols.h"
#include "Rom.h"


CasTape gTape;

static size_t sSaveStart = 0;

static const int kDataSize = 0x300;
static const int kNameSize = 6;
static const int kSyncBytes = 10;


// Not rewritten yet (Banks0123.asm: InitGame4).
static void
InitGameArea()
{
	gUnported = "InitGameArea";
}


// The _ variants of the original restore the banks afterwards.
static void
PrintText_(uint16_t text)
{
	PrintTextXY(text);
	SetBanks_1_2_X();
}


static void
EraseText_(uint16_t text)
{
	EraseTextXY(text);
	SetBanks_1_2_X();
}


static void
NextSaveLoadStat()
{
	Ram(var::SaveLoadStat)++;
}


static void
ClearBuffer(uint16_t address, int count)
{
	for (int i = 0; i < count; i++)
		Ram(static_cast<uint16_t>(address + i)) = 0;
}


static void
ResetFilename()
{
	ClearBuffer(var::FilenameSize, 0x0e);
}


static void
InitSaveLoad()
{
	Ram(var::RestoreSavedGame) = 0;
	Ram(var::DoNotAddEnemies) = 0;
	FillRect(0, 0x08, 0x08, 0, 0xa0, 0xa0);
	SetBanks_1_2_X();
	ClearBuffer(var::KEYBUF, 0x28);
	ClearTypedChars();	// port: keys typed while playing are not a name
	ClearBuffer(var::Filename, kNameSize);
}


// The original adds the bytes but overwrites the sum with the loop counter
// (ld a,c / or b), so the result is always 0. Kept as it is: the tail byte
// on every original tape is 0 and loading checks for it.
uint8_t
CalcDataChecksum()
{
	uint16_t address = var::GameProgressBuffer;
	uint16_t count = kDataSize - 1;
	uint8_t a = Ram(address++);
	do {
		a = static_cast<uint8_t>(a + Ram(address++));
		count--;
		a = static_cast<uint8_t>((count & 0xff) | (count >> 8));
	} while (count != 0);
	return a;
}


static void
PrintFilename2(uint16_t name, uint8_t x, uint8_t y)
{
	for (int i = 0; i < kNameSize; i++) {
		DrawChar(Ram(static_cast<uint16_t>(name + i)), 1, x, y);
		SetBanks_1_2_X();
		x = static_cast<uint8_t>(x + 8);
	}
}


static void
PrintFilename()
{
	PrintFilename2(var::Filename, 0x30, 0x50);
}


enum {
	kKeyNone,		// port: nothing typed yet (the original waits in CHGET)
	kKeyEscape,		// carry
	kKeyReturn,
	kKeyOther
};


static int
GetKeyTyped()
{
	Ram(var::KANAST) = 0;
	uint8_t c;
	if (!CHGET(&c))
		return kKeyNone;
	uint16_t size = var::FilenameSize;
	uint16_t at = static_cast<uint16_t>(var::Filename + Ram(size));
	if (c == 0x1b)
		return kKeyEscape;
	if (c == 0x0d) {
		PrintFilename();
		return kKeyReturn;
	}
	if (c == 0x08) {
		// EraseCharacter
		uint8_t erased = Ram(at);
		Ram(at) = 0;
		uint8_t length = static_cast<uint8_t>(Ram(size) - 1);
		if ((length & 0x80) == 0) {
			Ram(size) = length;
			if (erased == 0)
				Ram(static_cast<uint16_t>(at - 1)) = 0;
			else
				Ram(size)++;
		}
		PrintFilename();
		return kKeyOther;
	}
	if (static_cast<uint8_t>(c - '0') >= 10 && static_cast<uint8_t>(c - 'A') >= 26) {
		if (static_cast<uint8_t>(c - 'a') >= 26)
			return kKeyOther;
		c = static_cast<uint8_t>(c - 0x20);
	}
	Ram(at) = c;
	uint8_t length = static_cast<uint8_t>(Ram(size) + 1);
	if (length < kNameSize)
		Ram(size) = length;
	PrintFilename();
	return kKeyOther;
}


static void
ExitSaveLoad()
{
	ClearBuffer(var::SoundWorkArea, 0xe0);
	Ram(var::RestoreSavedGame) = 0;
	if (Ram(var::SaveLoadMode) != 1 || Ram(var::DoNotAddEnemies) != 0) {
		InitGameArea();
		return;
	}
	Ram(var::RestoreSavedGame)++;
}


static void
PrintError(uint16_t text, uint8_t status)
{
	Ram(var::SaveLoadStat) = status;
	PrintText_(text);
	PrintText_(SYM(txtRetry));
	PrintText_(SYM(txtYesNo));
	ResetFilename();
}


static void
TapeError()
{
	gTape.InOff();
	Ram(var::DoNotAddEnemies) = 1;
	if (Ram(var::SaveLoadMode) == 1)
		PrintError(SYM(txtLoadError), 3);
	else
		PrintError(SYM(txtVerifyError), 6);
}


static void
SaveError2()
{
	ResetFilename();
	PrintError(SYM(txtSaveError), 6);
}


static void
SaveError()
{
	gTape.OutOff();
	EraseText_(SYM(txtSaving));
	SaveError2();
}


static void
InitSave()
{
	Ram(var::TailDataByte) = CalcDataChecksum();
	PrintText_(SYM(txtSaving));
	NextSaveLoadStat();
}


enum {
	kSearchFound,
	kSearchSkipped,	// the original drops the caller's return address
	kSearchError	// the original returns into the caller (reading then fails)
};


static int
SearchFile()
{
	for (;;) {
		if (!gTape.InOn()) {
			TapeError();
			return kSearchError;
		}
		bool sync = true;
		for (int i = 0; i < kSyncBytes; i++) {
			uint8_t value;
			if (!gTape.In(&value)) {
				TapeError();
				return kSearchError;
			}
			if (value != 0xea) {
				sync = false;
				break;
			}
		}
		if (sync)
			break;
	}
	for (int i = 0; i < kNameSize; i++) {
		uint8_t value = 0;
		gTape.In(&value);	// the original ignores errors here: A is
							// whatever the BIOS left
		Ram(static_cast<uint16_t>(var::FilenameFound + i)) = value;
	}
	if (memcmp(&Ram(var::FilenameFound), &Ram(var::Filename), kNameSize) != 0) {
		// PrintSkipName: skip the data block; the next tick goes on.
		gTape.InOn();
		PrintText_(SYM(txtSkip));
		PrintFilename2(var::FilenameFound, 0x58, 0x60);
		return kSearchSkipped;
	}
	PrintText_(SYM(txtFound));
	PrintFilename2(var::FilenameFound, 0x58, 0x60);
	if (!gTape.InOn()) {
		TapeError();
		return kSearchError;
	}
	return kSearchFound;
}


// Status 4, Y: read the save back and compare.
static void
SaveVerify()
{
	EraseText_(SYM(txtYesNo));
	PrintText_(SYM(txtIng));
	EraseText_(SYM(txtYesNo));
	PrintText_(SYM(txtIng));
	if (SearchFile() == kSearchSkipped)
		return;
	for (int i = 0; i < kDataSize; i++) {
		uint8_t value;
		if (!gTape.In(&value)
			|| value != Ram(static_cast<uint16_t>(var::GameProgressBuffer + i))) {
			TapeError();
			return;
		}
	}
	uint8_t tail;
	if (!gTape.In(&tail) || tail != Ram(var::TailDataByte)) {
		TapeError();
		return;
	}
	gTape.InOff();
	if (CalcDataChecksum() != Ram(var::TailDataByte)) {
		TapeError();
		return;
	}
	PrintText_(SYM(txtVerifyOk));
	PrintText_(SYM(txtYesNo));
	NextSaveLoadStat();
}


static bool
KeyY()
{
	return (Ram(var::KeyboardRow5) & 0x40) != 0;
}


static bool
KeyN()
{
	return (Ram(var::KeyboardRow4) & 0x08) != 0;
}


static void
SaveMode(uint8_t status)
{
	switch (status) {
		case 0:
			InitSaveLoad();
			PrintText_(SYM(txtSaveMode));
			ResetFilename();
			NextSaveLoadStat();
			return;

		case 1:
		{
			// EnterSaveName
			int key = GetKeyTyped();
			if (key == kKeyEscape) {
				ExitSaveLoad();
				return;
			}
			if (key != kKeyReturn)
				return;
			EraseText_(SYM(txtSaveError));
			InitSave();
			return;
		}

		case 2:
			// SaveFilename. The original ignores a TAPOON error here, and
			// on a TAPOUT error in the name it jumps to SaveError with two
			// words still on the stack; writing a .CAS image cannot fail.
			PrepareTapeForSave(gTape, &Ram(var::Filename));
			gTape.OutOn();
			sSaveStart = gTape.Position() - sizeof(CasTape::kHeader);
			for (int i = 0; i < kSyncBytes; i++) {
				if (!gTape.Out(0xea)) {
					SaveError();
					return;
				}
			}
			for (int i = 0; i < kNameSize; i++) {
				if (!gTape.Out(Ram(static_cast<uint16_t>(var::Filename + i)))) {
					SaveError();
					return;
				}
			}
			NextSaveLoadStat();
			return;

		case 3:
			// SaveGameData ("#220 should be enough", but #300 is written)
			if (!gTape.OutOn()) {
				SaveError();
				return;
			}
			for (int i = 0; i < kDataSize; i++) {
				if (!gTape.Out(Ram(static_cast<uint16_t>(var::GameProgressBuffer + i)))) {
					SaveError();
					return;
				}
			}
			if (!gTape.Out(Ram(var::TailDataByte))) {
				SaveError();
				return;
			}
			gTape.OutOff();
			// Port: the player rewinds to the start of the save to verify.
			gTape.SetPosition(sSaveStart);
			PrintText_(SYM(txtVerify));
			NextSaveLoadStat();
			return;

		case 4:
			// SaveChkVerify
			if (KeyY()) {
				SaveVerify();
				return;
			}
			if (!KeyN())
				return;
			EraseText_(SYM(txtVerify));
			PrintText_(SYM(txtRetry));
			PrintText_(SYM(txtYesNo));
			Ram(var::SaveLoadStat) = 5;
			return;

		case 5:
			// SaveNotVerify ("this part has no sense": it asks to retry)
			if (KeyY()) {
				EraseText_(SYM(txtVerifyOk));
				EraseText_(SYM(txtYesNo));
				SaveError2();
				return;
			}
			if (KeyN())
				ExitSaveLoad();
			return;

		default:
			// SaveRetry
			if (KeyY()) {
				EraseText_(SYM(txtRetry));
				EraseText_(SYM(txtYesNo));
				EraseText_(SYM(txtVerifyError));
				Ram(var::SaveLoadStat) = 1;
				InitSave();
				return;
			}
			if (KeyN()) {
				Ram(var::DoNotAddEnemies) = 1;
				ExitSaveLoad();
			}
			return;
	}
}


static void
LoadMode(uint8_t status)
{
	switch (status) {
		case 0:
			InitSaveLoad();
			PrintText_(SYM(txtLoadMode));
			ResetFilename();
			NextSaveLoadStat();
			return;

		case 1:
		{
			// EnterLoadName
			int key = GetKeyTyped();
			if (key == kKeyNone)
				return;
			if (key == kKeyEscape) {
				Ram(var::DoNotAddEnemies) = 1;
				ExitSaveLoad();
				return;
			}
			EraseText_(SYM(txtLoadError));
			if (key != kKeyReturn)
				return;
			// Port: the player rewinds the tape.
			gTape.Rewind();
			NextSaveLoadStat();
			return;
		}

		case 2:
		{
			// LoadData: into EnemyListCopy first, then GameProgressBuffer
			if (SearchFile() == kSearchSkipped)
				return;
			for (int i = 0; i < kDataSize; i++) {
				uint8_t value;
				if (!gTape.In(&value)) {
					TapeError();
					return;
				}
				Ram(static_cast<uint16_t>(var::EnemyListCopy + i)) = value;
			}
			uint8_t tail;
			if (!gTape.In(&tail)) {
				TapeError();
				return;
			}
			Ram(var::TailDataByte) = tail;
			gTape.InOff();
			if (CalcDataChecksum() != Ram(var::TailDataByte)) {
				TapeError();
				return;
			}
			memmove(&Ram(var::GameProgressBuffer), &Ram(var::EnemyListCopy), kDataSize);
			Ram(var::DoNotAddEnemies) = 0;
			ExitSaveLoad();
			return;
		}

		default:
			// ChkLoadRetry
			if (KeyY()) {
				Ram(var::SaveLoadStat) = 2;
				// Port: the player rewinds the tape.
				gTape.Rewind();
				EraseText_(SYM(txtVerifyError));
				EraseText_(SYM(txtRetry));
				EraseText_(SYM(txtVerify));
				return;
			}
			if (KeyN())
				ExitSaveLoad();
			return;
	}
}


void
LoadSaveLogic()
{
	uint8_t status = Ram(var::SaveLoadStat);
	if (Ram(var::SaveLoadMode) == 1)
		LoadMode(status);
	else
		SaveMode(status);
}


std::vector<TapeSave>
ListTapeSaves(const CasTape& tape)
{
	std::vector<TapeSave> saves;
	const std::vector<uint8_t>& data = tape.Data();
	size_t header = tape.FindHeader(0);
	while (header < data.size()) {
		size_t start = header + sizeof(CasTape::kHeader);
		size_t next = tape.FindHeader(start);
		bool isName = next - start >= kSyncBytes + kNameSize;
		for (int i = 0; isName && i < kSyncBytes; i++)
			isName = data[start + i] == 0xea;
		if (!isName) {
			header = next;
			continue;
		}
		TapeSave save;
		save.offset = header;
		for (int i = 0; i < kNameSize; i++) {
			uint8_t c = data[start + kSyncBytes + i];
			if (c != 0)
				save.name += static_cast<char>(c);
		}
		size_t dataEnd = next < data.size() ? tape.FindHeader(next + sizeof(CasTape::kHeader))
			: next;
		save.complete = next < data.size()
			&& dataEnd - next - sizeof(CasTape::kHeader) >= kDataSize + 1;
		save.end = save.complete ? dataEnd : next;
		saves.push_back(save);
		header = save.end;
	}
	return saves;
}


void
PrepareTapeForSave(CasTape& tape, const uint8_t* name)
{
	std::vector<uint8_t> data = tape.Data();
	std::vector<TapeSave> saves = ListTapeSaves(tape);
	for (size_t i = saves.size(); i-- > 0;) {
		const uint8_t* stored = data.data() + saves[i].offset + sizeof(CasTape::kHeader)
			+ kSyncBytes;
		if (memcmp(stored, name, kNameSize) == 0)
			data.erase(data.begin() + saves[i].offset, data.begin() + saves[i].end);
	}
	bool modified = tape.IsModified() || data.size() != tape.Data().size();
	tape.SetData(data);
	tape.SetModified(modified);
	tape.SetPosition(data.size());
}
