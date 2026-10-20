/*
 * Differential test of the whole game, tick by tick: the original runs on
 * the reference machine (one InterruptTick per step, interrupts off so the
 * two sides see the same order of events), the native game runs on its
 * own; after every tick the variables, screen memory, palette and display
 * settings must be equal. On a difference the native state is resynced
 * from the reference, so later differences are found too.
 *
 *   game_diff <rom> <ticks> [key script: lines "tick row bit down"]
 */
#include "../DiffMachine.h"
#include "../../src/game/GameMain.h"
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
	if (argc > 3) {
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
	for (long t = 0; t < ticks; t++) {
		while (next < keys.size() && keys[next].tick == t) {
			dm.machine.SetKey(keys[next].row, keys[next].bit, keys[next].down != 0);
			if (keys[next].down)
				gKeyMatrix[keys[next].row] &= (uint8_t)~(1 << keys[next].bit);
			else
				gKeyMatrix[keys[next].row] |= (uint8_t)(1 << keys[next].bit);
			next++;
		}
		uint8_t status = dm.machine.Ram()[0], sub = dm.machine.Ram()[1];
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
		if (!dm.Compare(what))
			dm.CopyToNative();
	}
	printf("%ld checks, %ld failures\n", dm.checks, dm.failures);
	return dm.failures == 0 ? 0 : 1;
}
