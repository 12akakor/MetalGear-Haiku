/*
 * Differential test of saving and loading (logic/saveload.asm): the
 * original runs on the reference machine with its BIOS tape routines
 * writing a .CAS image, the rewritten code writes gTape. After every tick
 * the variables, screen and the two tapes must be equal.
 *
 *   save_diff <rom> [out.cas]
 *
 * Scenarios: save "SNAKE3" (typing with backspace and too many letters,
 * verify), save "OTHER", load "SNAKE3" (skips "OTHER"), load a name that
 * is not on the tape (error, retry, give up).
 */
#include "../DiffMachine.h"
#include "../../src/game/GameMain.h"
#include "../../src/game/GfxBasic.h"
#include "../../src/game/Input.h"
#include "../../src/game/Rom.h"
#include "../../src/game/SaveLoad.h"

#include <functional>


static DiffMachine sDm;
static CasTape sRefTape;
static long sTick = 0;
// What the player does with the reference cassette in the same tick (the
// rewritten code does it itself).
static std::function<void()> sPlayer;


static void
CompareTapes(const char* what)
{
	sDm.checks++;
	if (sRefTape.Data() != gTape.Data() || sRefTape.Position() != gTape.Position()) {
		sDm.failures++;
		printf("FAIL %s: tape ref %zu bytes at %zu, port %zu bytes at %zu\n", what,
			sRefTape.Data().size(), sRefTape.Position(), gTape.Data().size(),
			gTape.Position());
	}
}


static void
SetBankVariables(uint8_t a0)
{
	SetBanks(1);
	SetBankInA0(a0);
	const uint16_t vars[] = { var::BankIn60, var::BankIn80, var::BankInA0,
		var::BankInA0Fixed };
	for (uint16_t v : vars)
		sDm.machine.Ram()[v - 0xc000] = Ram(v);
}


// One game tick of GS_Pause in save/load mode. typed: the character CHGET
// returns (0: none); key: row*8+bit held during this tick (-1: none).
static bool
Tick(uint8_t typed, int key = -1)
{
	if (typed != 0) {
		sDm.machine.typed.push_back(typed);
		QueueTypedChar(typed);
	}
	if (key >= 0) {
		sDm.machine.SetKey(key / 8, key % 8, true);
		gKeyMatrix[key / 8] &= static_cast<uint8_t>(~(1 << (key % 8)));
	}
	uint8_t status = Ram(var::SaveLoadStat);

	SetBankVariables(6);
	sDm.Call(SYM(ReadKeyboard));
	ReadKeyboard();
	SetBankVariables(0x0f);
	sDm.machine.Ram()[var::BankInA0 - 0xc000] = 0x0f;
	sDm.Call(SYM(LoadSaveLogic));
	LoadSaveLogic();
	if (sPlayer) {
		sPlayer();
		sPlayer = nullptr;
	}

	if (key >= 0) {
		sDm.machine.SetKey(key / 8, key % 8, false);
		gKeyMatrix[key / 8] |= static_cast<uint8_t>(1 << (key % 8));
	}
	char what[80];
	snprintf(what, sizeof(what), "tick %ld (mode %d status %d)", sTick++,
		Ram(var::SaveLoadMode), status);
	if (gUnported != NULL) {
		// InitGameArea: the end of save/load, not rewritten yet
		printf("%s: reached %s\n", what, gUnported);
		gUnported = NULL;
		CompareTapes(what);
		sDm.CopyToNative();
		return false;
	}
	if (!sDm.Compare(what))
		sDm.CopyToNative();
	CompareTapes(what);
	return true;
}


static void
Begin(uint8_t mode)
{
	sDm.machine.Ram()[var::SaveLoadMode - 0xc000] = mode;
	sDm.machine.Ram()[var::SaveLoadStat - 0xc000] = 0;
	sDm.CopyToNative();
}


// Status 2 and 3: the name block and the data block. Before, the port
// removes an older save with the same name and winds to the end; after,
// the player rewinds to the start of the save.
static void
SaveTwoBlocks()
{
	PrepareTapeForSave(sRefTape, &sDm.machine.Ram()[var::Filename - 0xc000]);
	static size_t start;
	sPlayer = [] { start = sRefTape.Data().size() - 24; };
	Tick(0);
	sPlayer = [] { sRefTape.SetPosition(start); };
	Tick(0);
}


static const int kKeyY = 5 * 8 + 6;
static const int kKeyN = 4 * 8 + 3;


int
main(int argc, char** argv)
{
	if (argc < 2 || !sDm.Load(argv[1]))
		return 2;
	if (!sDm.StartGame()) {
		printf("game did not start\n");
		return 2;
	}
	sDm.machine.tape = &sRefTape;
	uint8_t* ref = sDm.machine.Ram();
	for (int i = 0; i < 0x300; i++)
		ref[var::GameProgressBuffer - 0xc000 + i] = static_cast<uint8_t>(i * 7 + 3);
	sDm.CopyToNative();

	// Save SNAKE3: "snake" + "12" (one too many: the last letter is
	// replaced), backspace, "3".
	Begin(2);
	Tick(0);
	const char* typed = "snake12\b3";
	for (const char* c = typed; *c != 0; c++)
		Tick(static_cast<uint8_t>(*c));
	Tick('\r');
	SaveTwoBlocks();
	Tick(0);
	Tick(0, kKeyY);	// verify
	Tick(0);
	if (Ram(var::SaveLoadStat) != 5)
		printf("FAIL verify: status %d\n", Ram(var::SaveLoadStat)), sDm.failures++;
	Tick(0, kKeyN);	// leaves (InitGameArea)

	// Save OTHER, no verify.
	for (int i = 0; i < 0x300; i++)
		ref[var::GameProgressBuffer - 0xc000 + i] = static_cast<uint8_t>(0x55 ^ i);
	sDm.CopyToNative();
	Begin(2);
	Tick(0);
	for (const char* c = "other\r"; *c != 0; c++)
		Tick(static_cast<uint8_t>(*c));
	SaveTwoBlocks();
	// Saving SNAKE3 again would replace the first one: move OTHER first.
	{
		std::vector<TapeSave> saves = ListTapeSaves(gTape);
		if (saves.size() != 2 || saves[0].name != "SNAKE3" || saves[1].name != "OTHER"
			|| !saves[0].complete || !saves[1].complete) {
			printf("FAIL tape listing\n");
			sDm.failures++;
		}
	}

	// Load OTHER: skips SNAKE3.
	memset(&ref[var::GameProgressBuffer - 0xc000], 0, 0x300);
	sDm.CopyToNative();
	Begin(1);
	Tick(0);
	for (const char* c = "OTHER"; *c != 0; c++)
		Tick(static_cast<uint8_t>(*c));
	sPlayer = [] { sRefTape.Rewind(); };
	Tick('\r');
	Tick(0);	// SKIP SNAKE3
	Tick(0);	// FOUND OTHER, loaded
	if (Ram(var::RestoreSavedGame) != 1 || Ram(var::GameProgressBuffer + 1) != (0x55 ^ 1)) {
		printf("FAIL load\n");
		sDm.failures++;
	}

	// Load a name that is not there: error, retry, then give up.
	Begin(1);
	Tick(0);
	for (const char* c = "NONE"; *c != 0; c++)
		Tick(static_cast<uint8_t>(*c));
	sPlayer = [] { sRefTape.Rewind(); };
	Tick('\r');
	for (int i = 0; i < 4 && Ram(var::SaveLoadStat) == 2; i++)
		Tick(0);
	if (Ram(var::SaveLoadStat) != 3)
		printf("FAIL load error: status %d\n", Ram(var::SaveLoadStat)), sDm.failures++;
	sPlayer = [] { sRefTape.Rewind(); };
	Tick(0, kKeyY);
	Tick(0);
	Tick(0);
	Tick(0);
	Tick(0, kKeyN);

	// Saving SNAKE3 again removes the old one and goes to the end.
	{
		CasTape copy;
		copy.SetData(gTape.Data());
		const uint8_t name[6] = { 'S', 'N', 'A', 'K', 'E', '3' };
		PrepareTapeForSave(copy, name);
		std::vector<TapeSave> saves = ListTapeSaves(copy);
		sDm.checks++;
		if (saves.size() != 1 || saves[0].name != "OTHER" || saves[0].offset != 0
			|| copy.Position() != copy.Data().size()) {
			printf("FAIL replacing a save\n");
			sDm.failures++;
		}
	}

	if (argc > 2 && !gTape.Save(argv[2]))
		printf("cannot write %s\n", argv[2]);
	printf("%ld checks, %ld failures\n", sDm.checks, sDm.failures);
	return sDm.failures == 0 ? 0 : 1;
}
