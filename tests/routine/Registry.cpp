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

// Common.cpp
#include "../../src/game/Common.h"

static void
SetCarry(Z80Regs& r, bool carry)
{
	r.f = static_cast<uint8_t>(carry ? (r.f | 1) : (r.f & ~1));
}

ROUTINE(GetNextRoomNum, "a") { out.a = GetNextRoomNum(); }
ROUTINE(GetElevatorRoomDat, "hl") { out.hl = GetElevatorRoomDat(); }
ROUTINE(GetElevatorPosY, "b,c") { GetElevatorPosY(&out.c, &out.b); }
ROUTINE(SetDefaultDoorLock, "") { SetDefaultDoorLock(); }
ROUTINE(ChkIsolatedRoom, "") { ChkIsolatedRoom(); }
ROUTINE(SetRadioArea, "") { SetRadioArea(); }
ROUTINE(GetWeaponDamages, "a") { out.a = GetWeaponDamages(in.a); }
ROUTINE(GetTempPHL_A, "a") { out.a = GetTempPHL_A(in.a); }
ROUTINE(GET_HL_A_DEC, "a") { out.a = GET_HL_A_DEC(in.hl, in.a); }
ROUTINE(GetItemAmount, "a") { out.a = GetItemAmount(in.a); }
ROUTINE(GetShapeInfo, "hl") { out.hl = GetShapeInfo(in.a); }
ROUTINE(CalcCursorXYWeapon, "") { CalcCursorXYWeapon(); }
ROUTINE(CalcCursorXYEquip, "") { CalcCursorXYEquip(); }
ROUTINE(GetItemInvAdd, "hl,c,cf")
{
	SetCarry(out, !GetItemInvAdd(in.a, &out.hl, &out.c));
}
ROUTINE(GetWeaponInvAdd, "hl,c,cf")
{
	SetCarry(out, !GetWeaponInvAdd(in.a, &out.hl, &out.c));
}
ROUTINE(AddDoorsData, "") { AddDoorsData(); }
ROUTINE(CoordToBuffTile, "hl") { out.hl = CoordToBuffTile(in.h, in.l); }
ROUTINE(DrawTileTIMP, "") { DrawTileTIMP(in.a, in.d, in.e); }
ROUTINE(DrawTilesBlock, "") { DrawTilesBlock(in.hl, in.d, in.e); }
ROUTINE(DrawTileBlkTimp, "") { DrawTileBlkTimp(in.hl, in.d, in.e); }
ROUTINE(DrawMetalGear, "") { DrawMetalGear(in.d, in.e); }
ROUTINE(EraseMetalGear, "") { EraseMetalGear(in.d, in.e); }
ROUTINE(SaveBackgTiles, "") { SaveBackgTiles(in.de, in.h, in.l); }
ROUTINE(SetAreaMusic, "") { SetAreaMusic(); }
ROUTINE(SetAreaMusic2, "") { SetAreaMusic2(); }
ROUTINE(RemoveItem, "") { RemoveItem(in.hl); }
ROUTINE(SetActorSprColors, "") { SetActorSprColors(in.ix); }
ROUTINE(SetActorSprColors2, "") { SetActorSprColors2(in.ix, in.a); }

// Hud.cpp
#include "../../src/game/Hud.h"

ROUTINE(DrawLineHoriz, "") { DrawLineHoriz(in.h, in.l, in.b, in.c); }
ROUTINE(DrawLineVert, "") { DrawLineVert(in.h, in.l, in.b, in.c); }
ROUTINE(DrawRect, "") { DrawRect(in.h, in.l, in.d, in.e, in.c); }
ROUTINE(Render3Numbers, "") { Render3Numbers(in.hl, in.d, in.e); }
ROUTINE(PrintDigit, "d") { PrintDigit(in.a, in.c, &out.d, in.e); }
ROUTINE(RenderHUD, "") { RenderHUD(); }
ROUTINE(DrawLife, "") { DrawLife(); }
ROUTINE(DrawClass, "") { DrawClass(); }
ROUTINE(DrawCallTimer, "") { DrawCallTimer(); }
ROUTINE(DrawDestrucTimer, "") { DrawDestrucTimer(); }
ROUTINE(DrawWeaponHUD, "") { DrawWeaponHUD(); }
ROUTINE(EraseWeaponBox, "") { EraseWeaponBox(); }
ROUTINE(RenderAmmoHUD, "") { RenderAmmoHUD(in.hl); }
ROUTINE(DrawItemHUD, "") { DrawItemHUD(); }
ROUTINE(EraseItemBox, "") { EraseItemBox(); }
ROUTINE(DrawItemAmountInv, "") { DrawItemAmountInv(in.hl, in.c); }
ROUTINE(DecrementLife_4, "") { DecrementLife_4(); }
ROUTINE(DecrementLife_B, "") { DecrementLife_B(in.b); }
ROUTINE(SetDead, "") { SetDead(); }
ROUTINE(DecItemUnits, "") { DecItemUnits(in.hl, in.c); }
ROUTINE(DrawWeaponMenu, "") { DrawWeaponMenu(); }
ROUTINE(DrawEquipMenu, "") { DrawEquipMenu(); }

// GameGfx.cpp
#include "../../src/game/GameGfx.h"

ROUTINE(UnpackGfx1, "") { UnpackGfx1(in.hl, in.de); }
ROUTINE(UnpackGfx, "") { UnpackGfx(in.de); }
ROUTINE(LoadGfxDoors, "") { LoadGfxDoors(); }
ROUTINE(LoadTilesGfxBlk, "hl") { out.hl = LoadTilesGfxBlk(in.hl, in.de, in.b, in.c); }
ROUTINE(LoadRoomSpr, "") { LoadRoomSpr(); }
ROUTINE(LoadSprProjectile, "") { LoadSprProjectile(); }
ROUTINE(LoadGameGfx, "") { LoadGameGfx(); }
ROUTINE(PrintTransceiv, "") { PrintTransceiv(); }
ROUTINE(SetupPitfall, "") { SetupPitfall(in.hl, in.de, in.b); }
ROUTINE(DrawPitfall, "") { DrawPitfall(); }
ROUTINE(DrawPitfall_, "") { DrawPitfall_(); }
ROUTINE(DrawPitfallTiles, "") { DrawPitfallTiles(in.hl, in.d, in.e); }
ROUTINE(DrawPitfallTile, "") { DrawPitfallTile(in.a, in.d, in.e); }
ROUTINE(SetMenuWeaponPal, "") { SetMenuWeaponPal(); }
ROUTINE(SetSnakePal, "") { SetSnakePal(); }
ROUTINE(SetRadioPal, "") { SetRadioPal(); }
ROUTINE(SetSprPal, "") { SetSprPal(); }
ROUTINE(UpdateRadio, "") { UpdateRadio(); }
