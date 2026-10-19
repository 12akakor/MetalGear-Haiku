/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// The rewritten routines that routine_diff checks, with their register
// conventions.
#include "RoutineDiff.h"

#include "../../src/game/GameMain.h"
#include "../../src/game/GfxBasic.h"
#include "../../src/game/RoomGfx.h"
#include "../../src/game/Title.h"


ROUTINE(RenderRoom, "") { RenderRoom(); }
ROUTINE(SetRoomPal, "") { SetRoomPal(); }
ROUTINE(LoadRoomTiles, "") { LoadRoomTiles(); }
ROUTINE(ClearScreen, "") { ClearScreen(); }
ROUTINE(PrintTextXY, "") { PrintTextXY(in.hl); }
ROUTINE(EraseTextXY, "") { EraseTextXY(in.hl); }
ROUTINE(MenuLogoLogic, "") { MenuLogoLogic(); }
ROUTINE(DrawKonamiLogo, "") { DrawKonamiLogo(); }
