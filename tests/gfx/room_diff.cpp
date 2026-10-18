/*
 * Differential test: loading and drawing rooms (LoadRoomTiles, SetRoomPal,
 * RenderRoom) native against the original, for every room, with and
 * without flashlight or goggles, with the tileset loaded or not.
 *
 *   room_diff <rom>
 */
#include "../diff_machine.h"
#include "../../src/game/rom.h"
#include "../../src/game/room_gfx.h"

int
main(int argc, char** argv)
{
	static DiffMachine dm;
	if (argc < 2 || !dm.Load(argv[1]))
		return 2;
	if (!dm.StartGame()) {
		fprintf(stderr, "game did not start\n");
		return 1;
	}
	uint8_t* ram = dm.machine.Ram();
	const uint8_t items[] = { 0, 3, 4 };
	char what[64];
	int skipped = 0;
	for (int room = 0; room < 251; room++) {
		/* Rooms that point at RoomUndefined are never entered; their data
		   would come from the BIOS ROM. */
		set_banks(13);
		g_ram[var::BankIn60 - 0xc000] = 13;
		bool undefined = rom16((uint16_t)(SYM(idxRooms) + room * 2)) == SYM(RoomUndefined);
		if (undefined) {
			skipped++;
			continue;
		}
		for (int k = 0; k < 3; k++) {
			ram[var::Room - 0xc000] = (uint8_t)room;
			ram[var::SelectedItem - 0xc000] = items[k];
			if (k == 1)
				ram[var::CurrentTileSet - 0xc000] = 0xff;	/* force a reload */
			ram[var::BankIn60 - 0xc000] = 1;
			ram[var::BankIn80 - 0xc000] = 2;
			ram[var::BankInA0 - 0xc000] = 3;

			dm.CopyToNative();
			dm.Call(SYM(LoadRoomTiles));
			LoadRoomTiles();
			snprintf(what, sizeof(what), "LoadRoomTiles room %d item %d", room, items[k]);
			if (!dm.Compare(what))
				dm.CopyToNative();

			dm.Call(SYM(SetRoomPal));
			SetRoomPal();
			snprintf(what, sizeof(what), "SetRoomPal room %d item %d", room, items[k]);
			if (!dm.Compare(what))
				dm.CopyToNative();

			dm.Call(SYM(RenderRoom));
			RenderRoom();
			snprintf(what, sizeof(what), "RenderRoom room %d item %d", room, items[k]);
			dm.Compare(what);
		}
	}
	printf("%ld checks, %ld failures (%d undefined rooms skipped)\n", dm.checks, dm.failures,
		skipped);
	return dm.failures == 0 ? 0 : 1;
}
