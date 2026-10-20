/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Rewritten from Banks0123.asm (GS_DemoPlay .. GS_Ending, InitGame ..
// BinocularLogic), logic/controls.asm (UpdateControls) and
// logic/items.asm (ClearGameVars).
#include "Game.h"

#include "Actors.h"
#include "Checkpoints.h"
#include "CaptureScene.h"
#include "Common.h"
#include "Contact.h"
#include "Demo.h"
#include "Events.h"
#include "Doors.h"
#include "GameGfx.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Hud.h"
#include "Items.h"
#include "Input.h"
#include "NextRoom.h"
#include "Player.h"
#include "RoomGfx.h"
#include "Rom.h"
#include "RoomActors.h"
#include "SaveLoad.h"
#include "Screens.h"
#include "Sprites.h"
#include "Structures.h"
#include "Text.h"
#include "ElevatorRoom.h"
#include "EquipMenu.h"
#include "Unported.h"
#include "Weapons.h"


using namespace var;


static void
NextStatusT()
{
	Ram(WaitCounter) = 0x20;
	Ram(GameStatus)++;
	Ram(GameSubstatus) = 0;
}


void
SetGameStatus(uint8_t status)
{
	Ram(GameStatus) = status;
	Ram(WaitCounter) = 0x20;
	Ram(GameSubstatus) = 0;
}


void
ResetGameStat()
{
	SetGameStatus(0);
}


void
UpdateControls()
{
	if ((Ram(ControlConfig) & 0x40) == 0)
		return;
	StoreControls(FKeysHold, ReadFKeys());
	StoreControls(ControlsHold, ReadControls());
}


void
ClearGameVars()
{
	for (int i = 0; i <= 0x2b6f; i++)
		Ram(static_cast<uint16_t>(GameVars + i)) = 0;
	Ram(Pause_1_F5_2) = 0;
}


// Status 2: the demo (also the tutorial).
void
GS_DemoPlay(uint8_t substatus)
{
	if (substatus != 1) {
		// InitDemoMode
		ClearScreen();
		SetBankInA0(6);
		SetupDemoPlay();
		SetBanks1_2_3();
		Ram(WaitCounter) = 0x20;
		NextSubstatus();
		return;
	}
	SetBankInA0(6);
	DemoControler();
	GameDemoLogic();
	SetBanks1_2_3();
	if (Ram(PlayingFlag) != 0)
		return;
	ResetGameStat();
}


// Status 3: PLAY START blinks.
void
GS_PlayStart(uint8_t substatus)
{
	if (substatus == 1) {
		if (--Ram(WaitCounter) == 0) {
			NextSubstatus();
			return;
		}
		uint16_t text = SYM(txtPlayStart);
		if ((Ram(WaitCounter) & 4) == 0)
			text = PrintTextXY(text);	// (!?) then erases the text after it
		EraseTextXY(text);
		return;
	}
	if (substatus == 2) {
		ClearGameVars();
		NextStatusT();
		return;
	}
	Ram(WaitCounter) = 0x50;
	NextSubstatus();
}


// Status 4
void
GS_StartGame()
{
	ClearScreen();
	InitGame();
	Ram(PlayingFlag) = 1;
	NextStatusT();
}


void
PrintPause()
{
	VDP_Copy_Byte(0x6c, 0x58, 4, 0xd8, 0x98, 0x28, 8);	// save the background
	FillRect(0, 0x6c, 0x58, 0, 0x28, 8);
	PrintTextXY(SYM(txtPause));
}


static void
SetPauseMode()
{
	PrintPause();
	for (int i = 0; i < 0x1e; i++)
		Ram(static_cast<uint16_t>(PasswordBuffer + i)) = 0;
	Ram(RestoreSoundData) = 1;
	SetSoundEntryChk(0x4d);
	SetGameStatus(7);
}


// Status 5
void
GS_Playing()
{
	Ram(RestoreSoundData) = 0;
	GameLogic();
	if (Ram(Pause_1_F5_2) == 1) {
		SetPauseMode();
		return;
	}
	if (Ram(LeavedOuterHeaven) != 0) {
		SetGameStatus(8);
		return;
	}
	if (Ram(PlayingFlag) != 0)
		return;
	NextStatusT();
}


static void
RebootGame()
{
	Ram(ControlConfig) &= 0xbf;
	ResetGameStat();
}


static void
ChkGoBackGame()
{
	if (Ram(RestoreGameFlag) != 0)
		SetGameStatus(4);
	else
		RebootGame();
}


static void
ChkContinueKey()
{
	UpdateControls();
	if ((Ram(FKeysHold) & 0x10) == 0)
		return;
	EraseTextXY(SYM(txtContinue));
	Ram(SaveLoadMode) = 0;
	Ram(RestoreGameFlag) = 1;
}


// Status 6: GAME OVER, F5 continues from the last check point.
void
GS_GameOver(uint8_t substatus)
{
	if (substatus != 1) {
		ClearScreen();
		PrintTextXY(SYM(txtGameOver));
		Ram(RestoreGameFlag) = 0;
		NextSubstatus();
		return;
	}
	if (Ram(RestoreGameFlag) == 0)
		ChkContinueKey();
	if (Ram(static_cast<uint16_t>(SoundWorkArea + 2)) != 0)
		return;	// the game over music still plays
	ChkGoBackGame();
}


// Status 7: pause (F1 back, F4 load, F5 save, passwords).
void
GS_Pause()
{
	if (Ram(SaveLoadMode) != 0) {
		SetBankInA0(6);
		ReadKeyboard();
		SetBanks1_2_3();
		SetBankInA0(0x0f);
		LoadSaveLogic();
		SetBanks1_2_3();
		if (Ram(RestoreSavedGame) == 0)
			return;
		Ram(Pause_1_F5_2) = 0;
		Ram(DoNotAddEnemies) = 0;
		Ram(SaveLoadMode) = 0;
		Ram(RestoreSoundData) = 1;
		Ram(RestoreGameFlag) = 1;
		ChkGoBackGame();
		return;
	}
	UpdateControls();
	uint8_t keys = Ram(FKeysTrigger);
	if ((keys & 1) != 0) {
		// ExitPauseMode
		SetBankInA0(6);
		ChkPasswords();
		SetBanks1_2_3();
		Ram(Pause_1_F5_2) = 0;
		Ram(RestoreSoundData) = 1;
		VDP_Copy_Byte(0xd8, 0x98, 1, 0x6c, 0x58, 0x28, 8);	// erase PAUSE
		SetGameStatus(5);
		return;
	}
	SetBankInA0(6);
	ChkCharTyped();
	SetBanks1_2_3();
	uint8_t mode;
	if ((keys & 8) != 0)
		mode = 1;	// F4: load
	else if ((keys & 0x10) != 0)
		mode = 2;	// F5: save
	else
		return;
	Ram(SaveLoadMode) = mode;
	Ram(SaveLoadStat) = 0;
	ClearSprAttr();
}


// Status 8
void
GS_Ending(uint8_t substatus)
{
	if (substatus != 1) {
		SetBankInA0(0x0f);
		EndingSetup();
		SetBanks1_2_3();
		NextSubstatus();
		return;
	}
	SetBankInA0(0x0f);
	SwapSprBuffer();
	EndingLogic();
	UpdateSpritesShuf();
	SetBanks1_2_3();
	if (Ram(EndingStatus) != 0x0e)
		return;
	Ram(EndingStatus) = 0;
	RebootGame();
}


static void InitGame5();
static void InitRoom();
static void InitRoom2();


// A new game (or continuing from the check point).
void
InitGame()
{
	EraseSprAttRAM();
	Ram(GameMode) = 0;
	if (Ram(RestoreGameFlag) != 0) {
		// InitGame3
		SetBankInA0(6);
		RestoreGameStat();
		SetBanks1_2_3();
		StopAlert();
		InitGame4();
		return;
	}
	SetDefaultDoorLock();
	Ram(IdDoorEnter) = 0;
	Ram(Equipment) = SELECTED_CIGARETTES;
	Ram(SelectedItem) = SELECTED_CIGARETTES;
	Ram(CigarsTaken) = 1;
	Ram(CigarsUnits) = 1;
	if ((Ram(ControlConfig) & 0x40) != 0) {
		Ram(Room) = 121;	// first room
		Ram(PreviousRoom) = 121;
	}
	Ram(CurrentTileSet) = 0xff;
	Ram(RadioFreq) = FREQ_BIGBOSS;
	Ram(PrisonWall1Life) = 0x28;
	Ram(PrisonWall2Life) = 0x28;
	Ram(AreaMusic) = 0x2c;	// Theme of Tara
	SetSoundEntryChk(0x29);	// its intro
	InitPlayerVars();
	LoadGameGfx();
	LoadGfxDoors();
	SetMaxAmmoVals();
	InitGame5();
}


void
InitGame4()
{
	Ram(AreaMusic) = 0;
	SetAreaMusic2();
	InitGame5();
}


static void
InitGame5()
{
	SetRadioArea();
	UpdateRadio();
	AddDoorsData();
	InitRoom2();
}


void
NextRoomLogic()
{
	for (int i = 0; i < 0x1e; i++)
		Ram(static_cast<uint16_t>(PasswordBuffer + i)) = 0;
	SetNextRoom();
	SetRadioArea();
	UpdateRadio();
	if (Ram(GameMode) != GAME_MODE_DEAD && (Ram(ControlConfig) & 0x40) != 0)
		SetAreaMusic2();
	InitRoom();
}


static void
InitRoom()
{
	AddDoorsData();
	LocatePlayerEntry();
	InitRoom2();
}


static void
InitRoom2()
{
	SetBankInA0(6);
	ChkSaveGameStatus();
	SetBanks1_2_3();
	ChkIsolatedRoom();
	SetBankInA0(9);
	ChkLorryMov();
	ChkMadnarMsx();
	SetBanks1_2_3();
	Ram(byte_C150) = 0;
	Ram(IdDoorEnter) = 0;
	Ram(PowerSwitchOn) = 0;
	Ram(SpawnedItems) = 0;
	Ram(LorryMovTextF) = 0;
	Ram(OpenBigBossDoor) = 0;
	Ram(PlayerShotsList) = 0;
	Ldir(PlayerShotsList, PlayerShot1Stat, 0x17f);
	if (Ram(Room) < 240) {
		Ram(SprShootsAtt) = 0xe0;
		Ldir(SprShootsAtt, static_cast<uint16_t>(SprShootsAtt + 1), 0x17);
	}
	DisableScreen();
	ClearSprAttr();
	LoadRoomTiles();
	LoadSprProjectile();
	LoadRoomSpr();
	RenderRoom();
	DrawDoors();
	SetBankInA0(6);
	AddRoomItems();
	DrawRoomItems();
	SetBanks1_2_3();
	SetMenuWeaponPal();
	SetRoomPal();
	SetSprPal();
	RenderHUD();
	if (Ram(SaveLoadMode) == 0 && Ram(DoNotAddEnemies) == 0)
		SetupEnemyRoom();
	SetBankInA0(0x0f);
	DrawLaserBeams();
	SetBanks1_2_3();
	if (Ram(SaveLoadMode) != 0) {
		SetBankInA0(0x0c);
		ChkDrawMetalGear(EnemyList);	// IX of the caller (to check)
		SetBanks1_2_3();
		ChkDrawHindD();
		DrawOpenPitfalls();
		DrawCameras();
		DrawMines();
		ErasePowerSw(EnemyList + 0x800);	// IX as DrawMines leaves it (to check)
	}
	EnableScreen();
	if ((Ram(ControlConfig) & 0x40) == 0)
		Ram(BankInA0Fixed) = 6;	// demo
	if (Ram(Room) >= 0xf0) {
		SetHUDSprMask();
		SetBankInA0(9);
		SetElevatorSpr();
		SetBanks1_2_3();
		return;
	}
	if (Ram(SaveLoadMode) == 0) {
		SetBanks1_2_3();
		return;
	}
	// back from saving or loading
	UpdateSnakeSpr();
	UpdateEnemySprStr();
	UpdateEnemySprRAM();
	PrintPause();
	SetBankInA0(0x0f);
	UpdateSpritesShuf();
	SwapSprBuffer();
	UpdateSpritesShuf();
	Ram(SaveLoadMode) = 0;
	Ram(DoNotAddEnemies) = 0;
	SetBanks1_2_3();
}


static void GameModeLogic();


void
GameLogic()
{
	uint8_t mode = Ram(GameMode);
	bool swap = false;
	bool modes = false;
	if (mode == GAME_MODE_TEXT_BOX) {
		// TextBoxPatch
		uint8_t ko = Ram(MetalGear_KO);
		if (ko == 1 || ko == 2)
			swap = true;
		else
			modes = true;
	} else if (mode != 0 && mode < GAME_MODE_LORRY) {
		swap = false;	// menus, radio, next room
	} else {
		uint8_t intro = Ram(IntroSceneStatus);
		swap = intro < 5 || intro >= 8;	// 5-7: the radio in the intro
	}
	if (!modes) {
		if (swap) {
			SetBankInA0(0x0f);
			SwapSprBuffer();
			SetBanks1_2_3();
		}
		if (Ram(GameMode) != GAME_MODE_BINOCULARS) {
			DrawLife();
			DrawClass();
			mode = Ram(GameMode);
			if (mode != GAME_MODE_WEAPONS && mode != GAME_MODE_EQUIPMENT)
				DrawCallTimer();
			SetSnakeSprPatt();
			SetSnakeSprAtt();
			SetSnakeSprCol();
			SetGrenaTargetSpr();
			SetPlayerShotSpr();
		}
		if (Ram(Room) < 0xf0) {
			UpdateEnemySprStr();
			UpdateEnemySprRAM();
		}
	}
	GameModeLogic();
}


static void
RefreshSprites2()
{
	SetBankInA0(0x0f);
	UpdateSprites();
	SetBanks1_2_3();
}


// Copies sprite attributes and colours to VRAM; shuffles them (8 per line
// limit) except on the bridges; elevators do both.
void
RefreshSprites()
{
	uint8_t room = Ram(Room);
	if (room == 45 || room == 46) {
		RefreshSprites2();
		return;
	}
	if (room >= 0xf0) {
		RefreshSprites2();
		RefreshSprites2();
		return;
	}
	SetBankInA0(0x0f);
	UpdateSpritesShuf();
	SetBanks1_2_3();
}


static void
SwitchGameMode(uint8_t mode)
{
	if (Ram(Room) >= 0xf0)
		return;
	Ram(GameMode) = mode;
	if (mode == GAME_MODE_RADIO) {
		Ram(RestoreSoundData) = 1;
		SetSoundEntryChk(0x59);
	}
	Ram(EquipRadioStatus) = 0;
}


static void
PlayModeLogic()
{
	ChkAlarmEnd();
	if (Ram(DamageDelayTimer) != 0)
		Ram(DamageDelayTimer)--;
	ChkIncomingCall();
	DecNukeTimer();
	if (Ram(GameMode) == GAME_MODE_DEAD) {
		RefreshSprites();
		return;
	}
	uint8_t first = Ram(EnemyList);
	bool control = true;
	if ((first & 0x7f) == ID_METAL_GEAR
		&& (Ram(static_cast<uint16_t>(EnemyList + 0x0d)) == 0 || (first & 0x80) != 0))
		control = false;	// Metal Gear is going up
	if (control)
		PlayerControlLogic();
	ChkWeaponShot();
	PlayerShotsLogic();
	EnemiesLogic();
	CommonLogic();
	if (Ram(SelectedItem) == SELECTED_BOX)
		SetGrenaTargetSpr();
	if ((Ram(TickCounter) & 0x3f) != 0 || Ram(Poisoned) == 0) {
		RefreshSprites();
		return;
	}
	DecrementLife_4();
	RefreshSprites();
}


// F1 pause, F2 weapons, F3 equipment, F4 radio (not in the intro, on the
// ladders or the parachute).
static void
Playing()
{
	uint8_t room = Ram(Room);
	if (Ram(PlayerControlMod) != 8 && room < 224 && room != 204) {
		uint8_t keys = Ram(FKeysTrigger);
		if ((keys & 1) != 0) {
			Ram(Pause_1_F5_2) = 1;
			return;
		}
		if ((keys & 2) != 0) {
			SwitchGameMode(2);
			return;
		}
		if ((keys & 4) != 0) {
			SwitchGameMode(3);
			return;
		}
		if ((keys & 8) != 0) {
			SwitchGameMode(4);
			return;
		}
		if ((keys & 0x10) != 0) {
			Ram(Pause_1_F5_2) = 2;
			return;
		}
	}
	PlayModeLogic();
}


static void
ExitBinocularMode(uint8_t mode)
{
	Ldir(EnemyListCopy, EnemyList, 0x800);
	Ram(PowerSwitchOn) = Ram(PowerSwitchOnCopy);
	SetRam16(RadioCallFlag, Ram16(RadioCallFlagCopy));
	Ram(AlertModeCopy) = Ram(AlertModeCopy2);
	Ram(static_cast<uint16_t>(SprAttRAM + 0x0e)) = 0xe0;
	Ldir(static_cast<uint16_t>(SprAttRAM + 0x0e), static_cast<uint16_t>(SprAttRAM + 0x0f), 0x57);
	SwitchGameMode(mode);
}


static void
GameModeLogic()
{
	UpdateControls();
	switch (Ram(GameMode)) {
		case 0:
			Playing();
			break;
		case 1:
			NextRoomLogic();
			break;
		case 2:
			MenuWeapon();
			break;
		case 3:
			MenuEquip();
			break;
		case 4:
			RadioLogic();
			break;
		case 5:
			// LorryLogic
			SetBankInA0(9);
			LorryMoving();
			SetBanks1_2_3();
			RefreshSprites();
			break;
		case 6:
			// ElevatorRoom
			DecNukeTimer();
			SetBankInA0(9);
			ElevatorRoomLogic();
			SetBanks1_2_3();
			SetSnakeSprPatt();
			SetSnakeSprAtt();
			SetSnakeSprCol();
			RefreshSprites();
			break;
		case 7:
			// OpenDoorLogic
			EraseDoorLogic();
			RefreshSprites();
			break;
		case 8:
			// BinocularMode: F3 back while looking at Snake's room
			if (Ram(BinoculStatus) == 1 && (Ram(FKeysTrigger) & 4) != 0) {
				ExitBinocularMode(GAME_MODE_EQUIPMENT);
				return;
			}
			BinocularLogic();
			SetBankInA0(0x0f);
			SetBinoTargetSpr();
			SetBanks1_2_3();
			break;
		case 9:
			// DeadLogic
			if (--Ram(DeadTimer) == 0) {
				Ram(PlayingFlag) = 0;
				return;
			}
			PlayerControlLogic();
			RefreshSprites2();
			break;
		case 10:
		{
			// MessageLogic: the emergency message waits for Metal Gear
			if (Ram(TextId) != 150 || Ram(MetalGear_KO) == 3) {
				TextBoxLogic();
				return;
			}
			Ram(MetalGear_KO)++;
			RefreshSprites();
			break;
		}
		case 11:
			// CaptureLogic_
			CaptureSceneLogic();
			EnemiesLogic();
			RefreshSprites2();
			break;
		case 12:
			// EventMadnarMoved
			SetBankInA0(9);
			chkMadnarLate();
			SetBanks1_2_3();
			RefreshSprites();
			break;
		default:
			if (gUnported == NULL)
				gUnported = "GameModeLogic: bad mode";
			break;
	}
}


// Status 0 draws the room looked at; 1 = Snake's room; 2-5 look up, down,
// left, right for a while, then back.
void
BinocularLogic()
{
	uint8_t status = Ram(BinoculStatus);
	uint8_t direction;
	uint8_t c;
	if (status == 1) {
		uint8_t keys = Ram(ControlsTrigger) & 0x0f;
		if (keys == 0)
			return;
		if ((keys & 1) != 0)
			c = 2;
		else if ((keys & 2) != 0)
			c = 3;
		else if ((keys & 4) != 0)
			c = 4;
		else
			c = 5;
		Ram(TimerBinocular) = 0x80;
		direction = static_cast<uint8_t>(c - 1);
	} else if (status >= 2 && status <= 5) {
		static const uint8_t kBack[] = { 2, 1, 4, 3 };	// opposite side
		direction = kBack[status - 2];
		if (--Ram(TimerBinocular) != 0)
			return;
		c = 1;
	} else {
		// DrawBinocRoom
		DisableScreen();
		ClearPage0();
		ClearSprAttr();
		LoadRoomTiles();
		LoadRoomSpr();
		SetBankInA0(6);
		AddRoomItems();
		SetBanks1_2_3();
		SetRoomPal();
		SetSprPal();
		AddDoorsData();
		RenderRoom();
		DrawDoors();
		Ram(EnemyList) = 0;
		Ldir(EnemyList, static_cast<uint16_t>(EnemyList + 1), 0x7ff);
		Ram(EnemySprAttRAM) = 0xe0;
		Ldir(EnemySprAttRAM, static_cast<uint16_t>(EnemySprAttRAM + 1), 0x57);
		SetupEnemyRoom();
		if (Ram(BinocularDir) == 1) {
			Ram(EnemySprAttRAM) = 0xe0;
			Ldir(EnemySprAttRAM, static_cast<uint16_t>(EnemySprAttRAM + 1), 0x57);
		}
		PrintTextXY(SYM(txtTelescope));
		Ram(BinoculStatus) = Ram(BinocularDir);
		uint8_t arrow = Mem(static_cast<uint16_t>(SYM(ArrowsChars)
			+ static_cast<uint8_t>(Ram(BinocularDir) - 1)));
		DrawChar(arrow, 0xff, 0xc0, 0xc4);
		EnableScreen();
		return;
	}
	// MoveBinoculars2
	Ram(NextRoomDirect) = direction;
	uint8_t next = GetNextRoomNum();
	if (next == 0xff)
		return;
	Ram(Room) = next;
	Ram(BinocularDir) = c;
	Ram(BinoculStatus) = 0;
}
