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

// Sprites.cpp
#include "../../src/game/Sprites.h"

ROUTINE(SetSnakeSprAtt, "") { SetSnakeSprAtt(); }
ROUTINE(SetSnakeSprCol, "") { SetSnakeSprCol(); }
ROUTINE(SetSnakeSprPatt, "") { SetSnakeSprPatt(); }
ROUTINE(UpdateSnakeSpr, "") { UpdateSnakeSpr(); }
ROUTINE(SetShotSpr, "") { SetShotSpr(in.ix); }
ROUTINE(UpdateActorSprDat, "") { UpdateActorSprDat(in.ix); }
ROUTINE(UpdateEnemySprRAM, "") { UpdateEnemySprRAM(); }
ROUTINE(HideEnemySprs, "") { HideEnemySprs(); }
ROUTINE(SetSpritesFree, "") { SetSpritesFree(in.hl, in.b); }
ROUTINE(SetActorSprCols, "") { SetActorSprCols(in.ix, in.de); }
ROUTINE(SetSpriteLayer, "") { SetSpriteLayer(in.d, in.e); }
ROUTINE(HideSprColor, "") { HideSprColor(in.ix); }
ROUTINE(SetSprColor, "") { SetSprColor(in.ix, in.a, in.c); }
ROUTINE(EraseSprAttRAM, "") { EraseSprAttRAM(); }
ROUTINE(LoadSprMissilRocket, "") { LoadSprMissilRocket(in.ix, in.de); }
ROUTINE(LoadSprTarget, "") { LoadSprTarget(); }
ROUTINE(VRAMtoRAM, "") { VRAMtoRAM(in.hl, in.de, in.bc); }
ROUTINE(WriteVRAM, "") { WriteVRAM(in.hl, in.a); }

// Actors.cpp
#include "../../src/game/Actors.h"

ROUTINE(CountEnemyType, "a") { out.a = CountEnemyType(in.a); }
ROUTINE(ChkRespawnEnemy, "") { ChkRespawnEnemy(); }
ROUTINE(ChkAlarmEnd, "") { ChkAlarmEnd(); }
ROUTINE(StopAlert, "") { StopAlert(); }
ROUTINE(GetPathPoints, "") { GetPathPoints(in.ix, in.b); }
ROUTINE(GetPathPoint, "") { GetPathPoint(in.ix, in.de); }
ROUTINE(SetDirToPoint, "") { SetDirToPoint(in.ix); }
ROUTINE(GetSentinelLookDirs, "") { GetSentinelLookDirs(in.ix); }
ROUTINE(SetActorSpeed, "") { SetActorSpeed(in.ix, in.de, in.hl); }
ROUTINE(AddActorSpeedX, "de") { out.de = AddActorSpeedX(in.ix, in.de); }
ROUTINE(Anim2FramesActor, "") { Anim2FramesActor(in.ix, in.b); }
ROUTINE(AnimZzzSign, "") { AnimZzzSign(in.ix, in.b, in.c); }
ROUTINE(ChangeGuardSprDir, "") { ChangeGuardSprDir(in.ix); }
ROUTINE(GetDirToPlayer, "a,b") { out.a = GetDirToPlayer(in.ix, &out.b); }
ROUTINE(GetSinCos, "b,c") { GetSinCos(in.a, &out.c, &out.b); }
ROUTINE(GetFlameSinCos, "b,c") { GetFlameSinCos(in.ix, &out.c, &out.b); }
ROUTINE(CalcShot, "de,hl") { out.de = CalcShot(in.ix, &out.hl); }
ROUTINE(CalcShot2, "de,hl") { out.de = CalcShot2(in.ix, in.a, &out.hl); }
ROUTINE(CalcQuadrantDegree, "") { CalcQuadrantDegree(in.d, in.e); }
ROUTINE(CalShootSpeed, "de") { out.de = CalShootSpeed(in.e); }
ROUTINE(InitLaserRoom, "") { InitLaserRoom(in.ix); }

// Text.cpp
#include "../../src/game/Text.h"

ROUTINE(SetText, "") { SetText(in.a); }
ROUTINE(SetTextUnskip2, "") { SetTextUnskip2(in.a); }
ROUTINE(SetTextUnskippable, "") { SetTextUnskippable(in.a); }
ROUTINE(TextBoxLogic, "") { TextBoxLogic(); }
ROUTINE(GetText, "a") { out.a = GetText(); }
ROUTINE(DecodeText, "a,hl,de") { out.a = DecodeText(&out.hl, &out.de); }
ROUTINE(DrawTextBoxIn, "cf") { SetCarry(out, DrawTextBoxIn()); }
ROUTINE(TextBoxExit, "") { TextBoxExit(); }
ROUTINE(GetTextBoxXYSize, "de,bc") { GetTextBoxXYSize(&out.de, &out.bc); }

// Player.cpp
#include "../../src/game/Player.h"

ROUTINE(InitPlayerVars, "") { InitPlayerVars(); }
ROUTINE(PlayerControlLogic, "") { PlayerControlLogic(); }
ROUTINE(GetPlayerDir, "") { GetPlayerDir(); }
ROUTINE(ControlPlayerV, "") { ControlPlayerV(); }
ROUTINE(ControlPlayerH, "") { ControlPlayerH(); }
ROUTINE(ChkControlPlayer, "") { ChkControlPlayer(); }
ROUTINE(chkPunch, "") { chkPunch(); }
ROUTINE(ChkPlayerColl, "") { ChkPlayerColl(); }
ROUTINE(PunchLogic2, "") { PunchLogic2(); }
ROUTINE(ChkCtrlElevator, "") { ChkCtrlElevator(); }
ROUTINE(ChkWater, "") { ChkWater(); }
ROUTINE(ChkRoofAirFlow, "") { ChkRoofAirFlow(); }
ROUTINE(ChkStartClimb, "") { ChkStartClimb(); }
ROUTINE(ChkExitLadders, "") { ChkExitLadders(); }
ROUTINE(ChkExitRoom, "") { ChkExitRoom(); }
ROUTINE(ChkLimitXElevator, "") { ChkLimitXElevator(); }
ROUTINE(ChkLadderLimits, "") { ChkLadderLimits(); }
ROUTINE(MovePlayerX, "hl") { out.hl = MovePlayerX(); }
ROUTINE(MovePlayerY, "hl") { out.hl = MovePlayerY(); }
ROUTINE(IncRescued, "") { IncRescued(); }
ROUTINE(AnimatePlayer, "") { AnimatePlayer(); }
ROUTINE(SetGrenaTargetSpr, "") { SetGrenaTargetSpr(); }
ROUTINE(ChkTileCollision, "cf") { SetCarry(out, ChkTileCollision(in.ix, in.b, in.c)); }
ROUTINE(ChkTileCollision_, "cf") { SetCarry(out, ChkTileCollision_(in.h, in.l, in.b, in.c)); }
ROUTINE(GetTilePlayer, "hl") { out.hl = GetTilePlayer(); }
ROUTINE(GetTileInXY, "hl") { out.hl = GetTileInXY(in.h, in.l); }
ROUTINE(ChkGasRooms, "") { ChkGasRooms(); }
ROUTINE(DecrementLife_C, "") { DecrementLife_C(in.c); }

// Screens.cpp
#include "../../src/game/Screens.h"

ROUTINE(RadioLogic, "") { RadioLogic(); }
ROUTINE(DrawRadioFreq, "") { DrawRadioFreq(); }
ROUTINE(DrawRadioLeds, "") { DrawRadioLeds(); }
ROUTINE(ChkRadioReply, "cf") { SetCarry(out, ChkRadioReply(in.hl)); }
ROUTINE(MenuWeapon, "") { MenuWeapon(); }
ROUTINE(GetWeaponCursor, "") { GetWeaponCursor(); }
ROUTINE(GetMenuCursor, "") { GetMenuCursor(in.hl, in.b, in.de); }
ROUTINE(EraseArrow, "") { EraseArrow(); }
ROUTINE(DrawArrow, "") { DrawArrow(); }
ROUTINE(CopyPalToRAM, "") { CopyPalToRAM(); }
ROUTINE(FadeOutLogic, "cf") { SetCarry(out, FadeOutLogic()); }
ROUTINE(FadeOutColors, "") { FadeOutColors(); }
