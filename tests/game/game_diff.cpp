/*
 * Differential test of the whole game, tick by tick: the original runs on
 * the reference machine (one InterruptTick per step, interrupts off so the
 * two sides see the same order of events), the native game runs on its
 * own; after every tick the variables, screen memory, palette and display
 * settings must be equal. On a difference the native state is resynced
 * from the reference, so later differences are found too.
 *
 *   game_diff <rom> <ticks> [key script: lines "tick row bit down"]
 *   game_diff <rom> <ticks> -play seed [-warp n]
 *
 * -play: SPACE on the title, then random joystick moves and fire (as
 * routine_diff -play). -warp n: every n ticks of play, go to the
 * neighbour of a random room and refill the life.
 */
#include "../DiffMachine.h"
#include "../../src/game/GameMain.h"
#include "../../src/game/Common.h"
#include "../../src/game/GameRam.h"
#include "../../src/game/Input.h"
#include "../../src/game/Random.h"
#include "../../src/game/Rom.h"

#include <stdlib.h>

int
main(int argc, char** argv)
{
	static DiffMachine dm;
	if (argc < 3 || !dm.Load(argv[1]))
		return 2;
	long ticks = atol(argv[2]);
	struct Key { long tick; int row, bit, down; };
	std::vector<Key> keys;
	bool play = false;
	long warp = 0;
	if (argc > 4 && !strcmp(argv[3], "-play")) {
		play = true;
		srand((unsigned)atol(argv[4]));
		if (argc > 6 && !strcmp(argv[5], "-warp"))
			warp = atol(argv[6]);
		keys.push_back({400, 8, 0, 1});
		keys.push_back({405, 8, 0, 0});
		keys.push_back({700, 8, 0, 1});
		keys.push_back({705, 8, 0, 0});
	} else if (argc > 3) {
		FILE* fp = fopen(argv[3], "r");
		Key k;
		while (fp != NULL && fscanf(fp, "%ld %d %d %d", &k.tick, &k.row, &k.bit, &k.down) == 4)
			keys.push_back(k);
		if (fp != NULL)
			fclose(fp);
	}

	/* Start: the reference runs from reset to the idle loop. */
	memcpy(gRam, dm.machine.Ram(), sizeof(gRam));
	Z80Regs& r = dm.machine.Cpu().r;
	uint16_t idle = SYM(DummyLoop);
	long guard = 0;
	while (r.pc != idle && ++guard < 50000000) {
		dm.time += (uint64_t)dm.machine.StepInstruction();
		dm.machine.GetVdp().SetTime(dm.time);
	}
	Start();
	if (!dm.Compare("Start"))
		dm.CopyToNative();

	size_t next = 0;
	char what[80];
	int held = 0;
	long holdUntil = 0, lastWarp = 0;
	for (long t = 0; t < ticks; t++) {
		if (play && t > 900 && t >= holdUntil) {
			static const uint8_t kMoves[] = { 0x01, 0x02, 0x04, 0x08, 0x05, 0x06,
				0x09, 0x0a, 0x00 };
			held = kMoves[rand() % 9];
			if (rand() % 4 == 0)
				held |= 0x10;
			if (rand() % 6 == 0)
				held |= 0x20;	/* fire 2: also goes on in text windows */
			holdUntil = t + 20 + rand() % 90;
		}
		if (play) {
			dm.machine.SetJoystick((uint8_t)held);
			gJoystick = (uint8_t)(~dm.machine.Joystick() & 0x3f);
		}
		uint8_t* ram = dm.machine.Ram();
		if (warp > 0 && ram[0] == 5 && ram[var::Room - 0xc000] == 0xff) {
			printf("tick %ld: the original went astray (room 255 after a warp)\n", t);
			break;
		}
		if (warp > 0 && t - lastWarp >= warp && ram[var::GameStatus - 0xc000] == 5
			&& ram[var::GameMode - 0xc000] == 0 && ram[var::Room - 0xc000] < 126
			&& ram[var::PlayerControlMod - 0xc000] == 0) {
			lastWarp = t;
			for (int tries = 0; tries < 50; tries++) {
				uint8_t room = (uint8_t)(rand() % 126);
				uint8_t side = (uint8_t)(1 + rand() % 4);
				uint8_t saveRoom = Ram(var::Room), saveSide = Ram(var::NextRoomDirect);
				Ram(var::Room) = room;
				Ram(var::NextRoomDirect) = side;
				uint8_t nextRoom = GetNextRoomNum();
				Ram(var::Room) = saveRoom;
				Ram(var::NextRoomDirect) = saveSide;
				if (nextRoom >= 126 || nextRoom == 103 || room == 103)
					continue;
				ram[var::Room - 0xc000] = room;
				ram[var::NextRoomDirect - 0xc000] = side;
				ram[var::IdDoorEnter - 0xc000] = 0;
				ram[var::GameMode - 0xc000] = 1;
				break;
			}
			ram[var::Life - 0xc000] = ram[var::MaxLife - 0xc000];
			dm.CopyToNative();
		}
		while (next < keys.size() && keys[next].tick == t) {
			dm.machine.SetKey(keys[next].row, keys[next].bit, keys[next].down != 0);
			if (keys[next].down)
				gKeyMatrix[keys[next].row] &= (uint8_t)~(1 << keys[next].bit);
			else
				gKeyMatrix[keys[next].row] |= (uint8_t)(1 << keys[next].bit);
			next++;
		}
		uint8_t status = dm.machine.Ram()[0], sub = dm.machine.Ram()[1];
		if (getenv("MG_TRACE") != NULL && (t % 250 == 0 || t >= atol(getenv("MG_TRACE"))))
			printf("tick %ld: status %d,%d room %d mode %d control %d intro %d hold %02X\n", t, status, sub,
				ram[var::Room - 0xc000], ram[var::GameMode - 0xc000],
				ram[var::PlayerControlMod - 0xc000], ram[var::IntroSceneStatus - 0xc000], ram[var::ControlsHold - 0xc000]);
		dm.Call(SYM(InterruptTick));
		size_t used = 0;
		gRefreshHook = [&]() -> uint8_t {
			return used < dm.refresh.size() ? dm.refresh[used++] : (used++, 0);
		};
		InterruptTick();
		gRefreshHook = nullptr;
		if (used != dm.refresh.size() && gUnported == NULL)
			printf("tick %ld: LD A,R original %zu, port %zu\n", t, dm.refresh.size(), used);
		if (gUnported != NULL) {
			printf("tick %ld: stopped, not rewritten yet: %s\n", t, gUnported);
			break;
		}
		snprintf(what, sizeof(what), "tick %ld (status %d,%d)", t, status, sub);
		if (!dm.Compare(what)) {
			const char* dump = getenv("MG_DUMP");	/* "addr len" (hex, decimal) */
			unsigned address = 0;
			int length = 0;
			if (dump != NULL && sscanf(dump, "%x %d", &address, &length) == 2) {
				printf("  ref: ");
				for (int i = 0; i < length; i++)
					printf(" %02X", dm.machine.Ram()[address + i - 0xc000]);
				printf("\n  port:");
				for (int i = 0; i < length; i++)
					printf(" %02X", gRam[address + i - 0xc000]);
				printf("\n");
			}
			dm.CopyToNative();
		}
	}
	printf("game status %d, room %d at the end\n", dm.machine.Ram()[0],
		dm.machine.Ram()[var::Room - 0xc000]);
	printf("%ld checks, %ld failures\n", dm.checks, dm.failures);
	return dm.failures == 0 ? 0 : 1;
}
