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

// Enemies.cpp
ROUTINE(EnemiesLogic, "") { EnemiesLogic(); }
ROUTINE(RunEnemyLogic, "") { RunEnemyLogic(in.ix); }
ROUTINE(ChkEraseAlertSign, "") { ChkEraseAlertSign(); }
ROUTINE(StunnedBounce, "") { StunnedBounce(in.ix); }
ROUTINE(MoveActor, "") { MoveActor(in.ix); }
ROUTINE(ChkActorExitRoom, "") { ChkActorExitRoom(in.ix); }
ROUTINE(DismissActor, "") { DismissActor(in.ix); }
ROUTINE(RemoveActor_, "") { RemoveActor_(in.ix); }
ROUTINE(UpdateEnemySprStr, "") { UpdateEnemySprStr(); }
ROUTINE(KillActor, "") { KillActor(in.ix); }
ROUTINE(BulletLogic, "") { BulletLogic(in.ix); }
ROUTINE(EraseBitmapActor, "") { EraseBitmapActor(in.ix); }
ROUTINE(ChkDrawDestroyPS, "") { ChkDrawDestroyPS(in.ix); }

// Game.cpp
#include "../../src/game/Game.h"

ROUTINE(GS_PlayStart, "") { GS_PlayStart(in.b); }
ROUTINE(GS_StartGame, "") { GS_StartGame(); }
ROUTINE(GS_Playing, "") { GS_Playing(); }
ROUTINE(GS_GameOver, "") { GS_GameOver(in.b); }
ROUTINE(GS_DemoPlay, "") { GS_DemoPlay(in.b); }
ROUTINE(PrintPause, "") { PrintPause(); }
ROUTINE(UpdateControls, "") { UpdateControls(); }
ROUTINE(ClearGameVars, "") { ClearGameVars(); }
ROUTINE(InitGame, "") { InitGame(); }
ROUTINE(NextRoomLogic, "") { NextRoomLogic(); }
ROUTINE(GameLogic, "") { GameLogic(); }
ROUTINE(BinocularLogic, "") { BinocularLogic(); }
ROUTINE(SwapSprBuffer, "") { SwapSprBuffer(); }
ROUTINE(UpdateSpritesShuf, "") { UpdateSpritesShuf(); }
ROUTINE(UpdateSprites, "") { UpdateSprites(); }
ROUTINE(SetBinoTargetSpr, "") { SetBinoTargetSpr(); }
ROUTINE(SetHUDSprMask, "") { SetHUDSprMask(); }

#include "../../src/game/NextRoom.h"
ROUTINE(SetNextRoom, "") { SetNextRoom(); }
ROUTINE(LocatePlayerEntry, "") { LocatePlayerEntry(); }

#include "../../src/game/Doors.h"
ROUTINE(DrawDoors, "") { DrawDoors(); }
ROUTINE(ChkDoors, "") { ChkDoors(); }
ROUTINE(ChkOpenDoor, "cf") { SetCarry(out, ChkOpenDoor(in.hl)); }
ROUTINE(EraseDoorLogic, "") { EraseDoorLogic(); }

#include "../../src/game/Items.h"
ROUTINE(AddRoomItems, "") { AddRoomItems(); }
ROUTINE(DrawRoomItems, "") { DrawRoomItems(); }
ROUTINE(SpawnItem, "") { SpawnItem(in.a, in.e, in.d); }
ROUTINE(ChkTakeItems, "") { ChkTakeItems(); }
ROUTINE(LimitAmmo, "") { LimitAmmo(); }
ROUTINE(SetMaxAmmoVals, "") { SetMaxAmmoVals(); }
ROUTINE(CompactWeapons, "") { CompactWeapons(); }
ROUTINE(CompactEquipment, "") { CompactEquipment(); }

#include "../../src/game/Weapons.h"
#include "../../src/game/Contact.h"
ROUTINE(ChkWeaponShot, "") { ChkWeaponShot(); }
ROUTINE(PlayerShotsLogic, "") { PlayerShotsLogic(); }
ROUTINE(SetPlayerShotSpr, "") { SetPlayerShotSpr(); }
ROUTINE(ChkAlertTrigger, "") { ChkAlertTrigger(in.ix); }
ROUTINE(SetAlertModeRespawn, "") { SetAlertModeRespawn(in.ix, in.a); }
ROUTINE(CommonLogic, "") { CommonLogic(); }
ROUTINE(ChkPlayerShots, "") { ChkPlayerShots(); }
ROUTINE(ChkTouchEnemies, "") { ChkTouchEnemies(); }
ROUTINE(ChkOnBridge, "") { ChkOnBridge(); }
ROUTINE(ChkElectricFloor, "") { ChkElectricFloor(); }
ROUTINE(ChkIncomingCall, "") { ChkIncomingCall(); }
ROUTINE(DecNukeTimer, "") { DecNukeTimer(); }

#include "../../src/game/Demo.h"
ROUTINE(SetupDemoPlay, "") { SetupDemoPlay(); }
ROUTINE(GameDemoLogic, "") { GameDemoLogic(); }
ROUTINE(DemoControler, "") { DemoControler(); }

#include "../../src/game/Checkpoints.h"
ROUTINE(ChkSaveGameStatus, "") { ChkSaveGameStatus(); }
ROUTINE(RestoreGameStat, "") { RestoreGameStat(); }

#include "../../src/game/Events.h"
ROUTINE(ChkLorryMov, "") { ChkLorryMov(); }
ROUTINE(LorryMoving, "") { LorryMoving(); }
ROUTINE(ChkMadnarMsx, "") { ChkMadnarMsx(); }
ROUTINE(chkMadnarLate, "") { chkMadnarLate(); }

#include "../../src/game/Guards.h"
ROUTINE(GuardLogic, "") { GuardLogic(in.ix); }
ROUTINE(InitGuardLorry, "") { InitGuardLorry(in.ix); }
ROUTINE(GuardLorryLogic, "") { GuardLorryLogic(in.ix); }
ROUTINE(ChkActSeePlayer, "") { ChkActSeePlayer(in.ix); }
ROUTINE(GuardSetAlarm, "") { GuardSetAlarm(in.ix); }
ROUTINE(ListenShotsChkTouch, "cf") { SetCarry(out, ListenShotsChkTouch(in.ix)); }

#include "../../src/game/RoomActors.h"
ROUTINE(DrawLaserBeams, "") { DrawLaserBeams(); }
ROUTINE(RestLasersBack, "") { RestLasersBack(); }
ROUTINE(RenderCamera, "") { RenderCamera(in.ix); }
ROUTINE(DrawCameras, "") { DrawCameras(); }
ROUTINE(InitMines, "") { InitMines(in.ix); }
ROUTINE(DrawMines, "") { DrawMines(); }
ROUTINE(InitPitfall, "") { InitPitfall(in.ix); }
ROUTINE(PitfallLogic, "") { PitfallLogic(in.ix); }
ROUTINE(DrawOpenPitfalls, "") { DrawOpenPitfalls(); }
ROUTINE(InitHindD, "") { InitHindD(in.ix); }
ROUTINE(ChkDrawHindD, "") { ChkDrawHindD(); }
ROUTINE(HindDLogic, "") { HindDLogic(in.ix); }
ROUTINE(InitMetalGear_, "") { InitMetalGear_(in.ix); }
ROUTINE(ChkDrawMetalGear, "") { ChkDrawMetalGear(in.ix); }
ROUTINE(InitGuardAlert, "") { InitGuardAlert(in.ix); }
ROUTINE(GuardAlertLogic, "") { GuardAlertLogic(in.ix); }

#include "../../src/game/Cameras.h"
ROUTINE(InitCamera, "") { InitCamera(in.ix); }
ROUTINE(InitCameraLaser, "") { InitCameraLaser(in.ix); }
ROUTINE(CameraLogic, "") { CameraLogic(in.ix); }
ROUTINE(LaserCameraLogic, "") { LaserCameraLogic(in.ix); }

#include "../../src/game/Shots.h"
ROUTINE(InitGuardShot, "") { InitGuardShot(in.ix); }
ROUTINE(InitShotToPlayer, "") { InitShotToPlayer(in.ix); }
ROUTINE(InitBulletHor, "") { InitBulletHor(in.ix); }
ROUTINE(InitBulletVert, "") { InitBulletVert(in.ix); }
ROUTINE(InitSnoringSymbol, "") { InitSnoringSymbol(in.ix); }
ROUTINE(SnoringSymbolLogic, "") { SnoringSymbolLogic(in.ix); }
ROUTINE(BigExplosionLogic, "") { BigExplosionLogic(in.ix); }
ROUTINE(InitTankShellBoss, "") { InitTankShellBoss(in.ix); }
ROUTINE(InitTankShot, "") { InitTankShot(in.ix); }
ROUTINE(TankShotLogic, "") { TankShotLogic(in.ix); }
ROUTINE(InitTankShell, "") { InitTankShell(in.ix); }
ROUTINE(ThankShellLogic, "") { ThankShellLogic(in.ix); }
ROUTINE(InitSpawnTankShell, "") { InitSpawnTankShell(in.ix); }
ROUTINE(SpawnTankShell, "") { SpawnTankShell(in.ix); }
ROUTINE(InitGas, "") { InitGas(in.ix); }
ROUTINE(GasLogic, "") { GasLogic(in.ix); }
ROUTINE(InitLaserShot, "") { InitLaserShot(in.ix); }
ROUTINE(LaserShotLogic, "") { LaserShotLogic(in.ix); }
ROUTINE(InitFlame, "") { InitFlame(in.ix); }
ROUTINE(FlamesLogic, "") { FlamesLogic(in.ix); }
ROUTINE(FlameOutScreen, "") { FlameOutScreen(in.ix); }

#include "../../src/game/IntroScene.h"
ROUTINE(IntroSceneLogic, "") { IntroSceneLogic(); }

#include "../../src/game/Dogs.h"
ROUTINE(InitSentinel, "") { InitSentinel(in.ix); }
ROUTINE(SentinelLogic, "") { SentinelLogic(in.ix); }
ROUTINE(InitSpawnGuardElev, "") { InitSpawnGuardElev(in.ix); }
ROUTINE(SpawnGuardElev, "") { SpawnGuardElev(in.ix); }
ROUTINE(InitDog, "") { InitDog(in.ix); }
ROUTINE(DogLogic, "") { DogLogic(in.ix); }
ROUTINE(InitDogBasement, "") { InitDogBasement(in.ix); }
ROUTINE(DogBasementLogic, "") { DogBasementLogic(in.ix); }
ROUTINE(InitSpawnDog, "") { InitSpawnDog(in.ix); }
ROUTINE(SpawnDogLogic, "") { SpawnDogLogic(in.ix); }

#include "../../src/game/Bosses.h"
#include "../../src/game/Soldiers.h"
ROUTINE(InitTank, "") { InitTank(in.ix); }
ROUTINE(TankLogic, "") { TankLogic(in.ix); }
ROUTINE(InitArnold, "") { InitArnold(in.ix); }
ROUTINE(ArnoldLogic, "") { ArnoldLogic(in.ix); }
ROUTINE(InitShooter, "") { InitShooter(in.ix); }
ROUTINE(ShooterLogic, "") { ShooterLogic(in.ix); }
ROUTINE(InitJetpackTakeoff, "") { InitJetpackTakeoff(in.ix); }
ROUTINE(JetpacTakeoff, "") { JetpacTakeoff(in.ix); }
ROUTINE(InitJetpackSwitch, "") { InitJetpackSwitch(in.ix); }
ROUTINE(JetpackSwitchLogic, "") { JetpackSwitchLogic(in.ix); }
ROUTINE(InitJetpack, "") { InitJetpack(in.ix); }
ROUTINE(JetpackLogic, "") { JetpackLogic(in.ix); }
ROUTINE(InitGuardSwitch, "") { InitGuardSwitch(in.ix); }
ROUTINE(GuardSwitchLogic, "") { GuardSwitchLogic(in.ix); }
ROUTINE(InitPowerSwitch, "") { InitPowerSwitch(in.ix); }
ROUTINE(PowerSwitchLogic, "") { PowerSwitchLogic(in.ix); }
ROUTINE(InitGuardElevat, "") { InitGuardElevat(in.ix); }
ROUTINE(GuardElevator, "") { GuardElevator(in.ix); }

#include "../../src/game/ElevatorRoom.h"
ROUTINE(ElevatorRoomLogic, "") { ElevatorRoomLogic(); }
ROUTINE(SetElevatorSpr, "") { SetElevatorSpr(); }

#include "../../src/game/Hazards.h"
ROUTINE(InitBridge, "") { InitBridge(in.ix); }
ROUTINE(BridgeLogic, "") { BridgeLogic(in.ix); }
ROUTINE(InitRollingBarrel, "") { InitRollingBarrel(in.ix); }
ROUTINE(RollingBarrelLogic, "") { RollingBarrelLogic(in.ix); }
ROUTINE(InitBulldozer, "") { InitBulldozer(in.ix); }
ROUTINE(BulldozerLogic, "") { BulldozerLogic(in.ix); }
ROUTINE(InitDesertSecurity, "") { InitDesertSecurity(in.ix); }
ROUTINE(DesertSecurityLogic, "") { DesertSecurityLogic(in.ix); }
ROUTINE(InitLorryShooter, "") { InitLorryShooter(in.ix); }
ROUTINE(LorryShooterLogic, "") { LorryShooterLogic(in.ix); }
