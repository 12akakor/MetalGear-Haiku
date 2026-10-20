/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// Running the actors: their logic, movement, leaving the room, being
// punched, killed, exploding (Banks0123.asm EnemiesLogic ..
// RestLaserBack_, DrawMovingLasers; logic/collisions.asm BulletLogic).
#include "Actors.h"

#include "Common.h"
#include "Bosses.h"
#include "Cameras.h"
#include "Dogs.h"
#include "Guards.h"
#include "Items.h"
#include "GameMain.h"
#include "GfxBasic.h"
#include "Player.h"
#include "Random.h"
#include "RoomGfx.h"
#include "Rom.h"
#include "Shots.h"
#include "Soldiers.h"
#include "RoomActors.h"
#include "Sprites.h"
#include "Structures.h"
#include "Text.h"
#include "Unported.h"


using namespace var;


namespace {

inline uint8_t&
A(uint16_t ix, int field)
{
	return Ram(static_cast<uint16_t>(ix + field));
}


void
EnemyLogicEnd(uint16_t)
{
}


typedef void (*ActorLogic)(uint16_t ix);

const ActorLogic kActorLogic[] = {
	BridgeLogic, EnemyLogicEnd, ChkSayHelpMe, GuardLogic, GuardLogic, CameraLogic,
	EnemyLogicEnd, GasLogic,	// 8
	TankLogic, GuardAlertLogic, GuardAlertLogic, ThankShellLogic, ShooterLogic,
	GuardElevator, RollingBarrelLogic, PitfallLogic,	// #10
	EnemyLogicEnd, BulldozerLogic, GuardLorryLogic, JetpacTakeoff, JetpackSwitchLogic,
	JetpackLogic, DummyLogic2, GuardSwitchLogic,	// #18
	DogLogic, ArnoldLogic, DogBasementLogic, LorryShooterLogic, SpawnDogLogic, GuardLogic,
	ScorpionLogic, BigBossLogic,	// #20
	ShotGunnerLogic, MachGunKidLogic, DrawMovingLasers, FireTrooperLoogic, FlamesLogic,
	HindDLogic, SpawnTankShell, SpawnGuardElev,	// #28
	CowardDuckLogic, EnemyLogicEnd, ShotGunnerShot, PowerSwitchLogic, CaptureGuardsLogic,
	DesertSecurityLogic, BulletLogic, SentinelLogic,	// #30
	PrisonerLogic, PrisonerLogic, PrisonerLogic, PrisonerLogic, LaserCameraLogic,
	LaserShotLogic, FakeMadnadLogic, EnemyLogicEnd,	// #38
	GuardSilencerLogic, DummyLogic2, BulletLogic, MGunKidShotLogic, DummyLogic2,
	TankShotLogic, BoomerangLogic, SnoringSymbolLogic, BigExplosionLogic
};


void
ChkDropItem(uint16_t ix)
{
	// (!?) only ID_GUARD_SLOW gets here
	if (A(ix, ACTOR::ID) == ID_GUARD_SLOW) {
		uint8_t y = static_cast<uint8_t>(A(ix, ACTOR::Y) - 4);
		uint8_t x = static_cast<uint8_t>(A(ix, ACTOR::X) - 8);
		uint8_t item = static_cast<uint8_t>((RefreshR() >> 2) & 3);
		if (item < 2)
			SpawnItem_(item, y, x);	// 0 = ration, 1 = ammunition
	}
	KillActor(ix);
}


// Three punches: knocked out.
void
ChkKillPunching(uint16_t ix)
{
	A(ix, ACTOR::PunchesCnt)++;
	if (A(ix, ACTOR::PunchesCnt) == 3) {
		ChkDropItem(ix);
		return;
	}
	A(ix, ACTOR::StunnedCnt) = 0x40;
}


void RemoveActorMusic(uint16_t ix);


void
RemoveActorDrop(uint16_t ix, uint8_t item, uint8_t y, uint8_t x)
{
	SpawnItem_(item, y, x);
	RemoveActorMusic(ix);
}


void
RemoveActorMusic(uint16_t ix)
{
	SetAreaMusic();
	RemoveActor_(ix);
}


void
SetExplSprColors(uint16_t colors, uint16_t de, uint8_t count)
{
	int n = count == 0 ? 256 : count;
	for (int i = 0; i < n; i++) {
		Ram(de) = Mem(colors++);
		de = static_cast<uint16_t>(de + 5);
	}
}


void
ExplosionAnim3(uint16_t ix, uint16_t colors, uint8_t sprite, uint8_t count)
{
	A(ix, ACTOR::SpriteId) = sprite;
	SetExplSprColors(colors, static_cast<uint16_t>(ix + ACTOR::Spr1Color), count);
}


// Explosion frames by ANIM_CNT: < 5, < #0A, then the third until #10.
bool
ExplosionFrame(uint16_t ix, uint8_t first, uint8_t* sprite)
{
	uint8_t t = A(ix, ACTOR::ANIM_CNT);
	if (t < 5) {
		*sprite = first;
		return true;
	}
	if (t < 0x0a) {
		*sprite = static_cast<uint8_t>(first + 1);
		return true;
	}
	if (t == 0x10) {
		DismissActor(ix);
		return false;
	}
	*sprite = static_cast<uint8_t>(first + 2);
	return true;
}


void
KillJetpack(uint16_t ix)
{
	uint8_t sprite;
	if (ExplosionFrame(ix, 0x7a, &sprite))
		ExplosionAnim3(ix, SYM(ExplosionSprColx1), sprite, 6);
}


void
ExplosionAnim(uint16_t ix)
{
	uint8_t sprite;
	if (ExplosionFrame(ix, 0x6a, &sprite))
		ExplosionAnim3(ix, SYM(ExplosionSprColx3), sprite, 2);
}


void
BossDefeatedLogic3(uint16_t ix, uint16_t colors, uint8_t sprite)
{
	A(ix, ACTOR::SpriteId) = sprite;
	uint8_t id = A(ix, ACTOR::ID) & 0x7f;
	uint8_t count = (id == ID_TANK || id == ID_BULLDOZER) ? 18 : 8;
	SetExplSprColors(colors, static_cast<uint16_t>(ix + ACTOR::Spr1Color), count);
}


// Metal Gear: the screen flashes, explosions, then the emergency message.
void
DestroyMetalG(uint16_t ix)
{
	if ((A(ix, ACTOR::ANIM_CNT) & 2) != 0)
		SetPaletteColor(0x0f, 0, 0);
	else
		SetPaletteColor(0x0f, 0x77, 0x07);
	uint8_t t = A(ix, ACTOR::ANIM_CNT);
	if (t < 5) {
		BossDefeatedLogic3(ix, SYM(ExplosionSprColx2), 0x89);
	} else if (t < 0x0a) {
		BossDefeatedLogic3(ix, SYM(ExplosionSprColx3), 0x8a);
	} else if (t < 0x0f) {
		BossDefeatedLogic3(ix, SYM(ExplosionSprColx4), 0x8b);
	} else if (t < 0x14) {
		BossDefeatedLogic3(ix, SYM(ExplosionSprColx3), 0x8c);
	} else if (t < 0x19) {
		BossDefeatedLogic3(ix, SYM(ExplosSprColWht), 0x8d);
	} else if (t < 0x21) {
		BossDefeatedLogic3(ix, SYM(ExplosSprColWht2), 0x8e);
	} else if (t == 0x21) {
		// ClearMetalGear
		EraseBitmapActor(ix);
		BossDefeatedLogic3(ix, SYM(EmptySprColx6), 0x8e);
	} else if (t == 0x22) {
		Ram(OpenBigBossDoor) = 1;
	} else {
		// ShowEmergencyText
		DismissActor(ix);
		SetTextUnskippable(150);
	}
}


// Metal Gear, tanks, Hind D, bulldozer: a flag marks them destroyed; Metal
// Gear starts the self destruction countdown.
void
BossDefeatedLogic(uint16_t ix)
{
	uint16_t flag = static_cast<uint16_t>(A(ix, ACTOR::BASE_SPR_ID)
		| (A(ix, ACTOR::KO_POINTER_H) << 8));
	if (Mem(flag) == 0) {
		Ram(flag) = 1;
		SetAreaMusic();
		if ((A(ix, ACTOR::ID) & 0x7f) == ID_METAL_GEAR) {
			Ram(DestructionTimerOn) = 1;
			SetRam16(DestructTimer, 0x3000);
			Ram(static_cast<uint16_t>(DoorOpenArray + 0x62)) = 0;	// door to Big Boss
			SetSoundEntry(0x53);
		}
	}
	if ((A(ix, ACTOR::ID) & 0x7f) == ID_METAL_GEAR) {
		DestroyMetalG(ix);
		return;
	}
	uint8_t t = A(ix, ACTOR::ANIM_CNT);
	if (t < 5) {
		BossDefeatedLogic3(ix, SYM(ExplosionSprColx3), 0x7f);
		return;
	}
	if (t == 0x10) {
		EraseBitmapActor(ix);
		return;
	}
	if (t < 0x0a) {
		BossDefeatedLogic3(ix, SYM(ExplosionSprColx3), 0x80);
		return;
	}
	if (t == 0x0a)
		Ram(OpenBigBossDoor) = 1;
	BossDefeatedLogic3(ix, SYM(ExpSprColWhtRed), 0x81);
}


void
KillEnemy(uint16_t ix)
{
	if ((A(ix, ACTOR::ID) & 0x7f) == ID_BIG_BOSS)
		Ram(static_cast<uint16_t>(DoorOpenArray + 0x6a)) = 0;	// ladders door
	DismissActor(ix);
}


// Killing a prisoner costs the rank.
void
KillPrisoner2(uint16_t ix)
{
	if (A(ix, ACTOR::ANIM_CNT) == 0)
		DowngradeRank();
	KillEnemy(ix);
}


void
KillPrisoner(uint16_t ix)
{
	if (Ram(Room) == 193 && A(ix, ACTOR::IDX_SAME_ID) == 3) {
		// Jennifer's brother
		Ram(JennifBrotherDead) = 1;
		Ram(static_cast<uint16_t>(RescuedArray + 0x0d)) = 1;
	}
	KillPrisoner2(ix);
}


void
KillActor3(uint16_t ix)
{
	uint8_t id = static_cast<uint8_t>((A(ix, ACTOR::ID) & 0x7f) - 1);
	switch (GetNibbleHL_A2(SYM(IdsKillLogic), id)) {
		case 0:
			DismissActor0(ix);
			break;
		case 1:
			KillEnemy(ix);
			break;
		case 2:
			KillPrisoner2(ix);
			break;
		case 3:
			KillPrisoner(ix);
			break;
		case 4:
			KillJetpack(ix);
			break;
		case 5:
			ExplosionAnim(ix);
			break;
		case 6:
			BossDefeatedLogic(ix);
			break;
		case 7:
			EraseBitmapActor(ix);
			break;
		default:
			if (gUnported == NULL)
				gUnported = "KillActor: bad kill logic";
			break;
	}
}


void
DrawDestroyPowSw(uint16_t hl, uint8_t x, uint8_t y)
{
	VDP_Copy_Byte(Mem(hl), 0x10, 1, x, y, 8, 8);
	VDP_Copy_Byte(Mem(static_cast<uint16_t>(hl + 1)), 0x10, 1, x, static_cast<uint8_t>(y + 8),
		8, 8);
}

}	// namespace


uint16_t gEnemyIX = EnemyList;
bool gEnemiesLogicAbort = false;


void
EnemiesLogic()
{
	ChkEraseAlertSign();
	ChkRespawnEnemy();
	uint16_t ix = EnemyList;
	for (int i = 0; i < 16; i++, ix = static_cast<uint16_t>(ix + 0x80)) {
		if (A(ix, ACTOR::ID) == 0)
			continue;
		if (A(ix, ACTOR::StunnedCnt) != 0) {
			A(ix, ACTOR::StunnedCnt)--;
			StunnedBounce(ix);
			continue;
		}
		gEnemyIX = ix;
		gEnemiesLogicAbort = false;
		RunEnemyLogic(ix);
		if (gEnemiesLogicAbort) {
			gEnemiesLogicAbort = false;
			return;
		}
		ix = gEnemyIX;	// some logic leaves IX on another actor
		MoveActor(ix);
		ChkActorExitRoom(ix);
	}
}


void
RunEnemyLogic(uint16_t ix)
{
	A(ix, ACTOR::ANIM_CNT)++;
	if (A(ix, ACTOR::LIFE) == 0) {
		KillActor(ix);
		return;
	}
	uint8_t id = A(ix, ACTOR::ID);
	if ((id & 0x80) != 0) {
		KillActor3(ix);
		return;
	}
	if ((A(ix, ACTOR::TOUCH_INFO) & 0x40) != 0) {
		ChkKillPunching(ix);
		return;
	}
	uint8_t index = static_cast<uint8_t>(id - 1);
	if (index < sizeof(kActorLogic) / sizeof(kActorLogic[0]))
		kActorLogic[index](ix);
	else if (gUnported == NULL)
		gUnported = "RunEnemyLogic: bad actor";
}


// The alert sign over a guard is erased after a while.
void
ChkEraseAlertSign()
{
	if (Ram(AlertIconTimer) == 0 || --Ram(AlertIconTimer) != 0)
		return;
	if (Ram(RoomAlertTrigged) != Ram(Room))
		return;
	uint8_t notOnScreen = Ram(AlertSignNotOnScreen);
	Ram(AlertSignNotOnScreen) = 0;
	if (notOnScreen != 0)
		return;
	VDP_Copy_Byte(0x60, 0x80, 1, Ram(static_cast<uint16_t>(XY_AlertIcon + 1)), Ram(XY_AlertIcon),
		0x10, 0x10);
}


void
StunnedBounce(uint16_t ix)
{
	uint8_t c;
	switch (A(ix, ACTOR::StunnedCnt)) {
		case 0x3f:
		case 0x3d:
			c = static_cast<uint8_t>(-4);
			break;
		case 0x3b:
		case 0x39:
			c = 4;
			break;
		default:
			return;
	}
	A(ix, ACTOR::Y) = static_cast<uint8_t>(A(ix, ACTOR::Y) + c);
}


// 8.8 positions plus speeds.
void
MoveActor(uint16_t ix)
{
	if ((A(ix, ACTOR::ID) & 0x80) != 0 || A(ix, ACTOR::Moving) == 0)
		return;
	for (int i = 0; i < 2; i++) {
		uint16_t base = static_cast<uint16_t>(ix + i * 2);
		uint16_t speed = static_cast<uint16_t>(A(base, ACTOR::speedYdec) | (A(base, ACTOR::SpeedY) << 8));
		uint16_t pos = static_cast<uint16_t>(A(base, ACTOR::Ydec) | (A(base, ACTOR::Y) << 8));
		pos = static_cast<uint16_t>(pos + speed);
		A(base, ACTOR::Ydec) = static_cast<uint8_t>(pos);
		A(base, ACTOR::Y) = static_cast<uint8_t>(pos >> 8);
	}
}


void
ChkActorExitRoom(uint16_t ix)
{
	if (A(ix, ACTOR::Y) < 193 && A(ix, ACTOR::X) != 255 && A(ix, ACTOR::X) >= 7)
		return;
	DismissActor0(ix);
}


void
DismissActor0(uint16_t ix)
{
	if ((A(ix, ACTOR::ID) & 0x7f) == ID_FLAME) {
		FlameOutScreen(ix);
		return;
	}
	DismissActor(ix);
}


// Removing some actors has consequences: doors open, cards are dropped,
// flags are set, the area music comes back.
void
DismissActor(uint16_t ix)
{
	uint8_t id = A(ix, ACTOR::ID);
	if (id == (ID_BIG_BOSS | 0x80)) {
		Ram(OpenBigBossDoor) = 1;
		Ram(BigBossStat) |= 1;
		RemoveActorMusic(ix);
		return;
	}
	if (id == (ID_ARNOLD | 0x80) && --Ram(ArnoldsCnt) == 0) {
		RemoveActorDrop(ix, SPAWN_CARD7, 0x30, 0x30);
		return;
	}
	if (A(ix, ACTOR::ID) == (ID_COWARD_DUCK | 0x80)) {
		RemoveActorDrop(ix, SPAWN_CARD8, 0x70, 0x38);
		return;
	}
	if (A(ix, ACTOR::ID) == (ID_FIRE_TROOPER | 0x80)) {
		Ram(FireTrooper_KO) = 1;
		RemoveActorMusic(ix);
		return;
	}
	if (A(ix, ACTOR::ID) == (ID_SHOT_GUNNER | 0x80)) {
		Ram(ShotGunnerStat) |= 1;
		RemoveActorMusic(ix);
		return;
	}
	if (A(ix, ACTOR::ID) == (ID_MACH_GUN_KID | 0x80)) {
		Ram(MachGunStatus) |= 1;
		RemoveActorMusic(ix);
		return;
	}
	if (A(ix, ACTOR::ID) == (ID_GUARD_SILENCER | 0x80) && --Ram(GuardSilencerCnt) == 0) {
		RemoveActorDrop(ix, SPAWN_SUPRESSOR, 0x24, 0x62);
		return;
	}
	if ((A(ix, ACTOR::ID) & 0x7f) == ID_BULLDOZER)
		SetBulldozerKO();
	id = A(ix, ACTOR::ID) & 0x7f;
	if (id == ID_POWER_SWITCH) {
		A(ix, ACTOR::ID) = id;
		return;
	}
	RemoveActor_(ix);
}


// Frees the structure and hides the actor's sprites.
void
RemoveActor_(uint16_t ix)
{
	A(ix, ACTOR::ID) = 0;
	A(ix, ACTOR::COLLISION_CFG) = 0;
	uint16_t hl = static_cast<uint16_t>(ix | 0x20);
	uint8_t count = Ram(hl);
	if (count == 0)
		return;
	uint16_t de = static_cast<uint16_t>((hl & 0xff00) | ((hl + 1) & 0xff));
	for (; count > 0; count--) {
		uint16_t attr = static_cast<uint16_t>((EnemySprAttRAM & 0xff00)
			| ((EnemySprAttRAM + Ram(de) * 4) & 0xff));
		Ram(attr) = 0xe0;
		de = static_cast<uint16_t>((de & 0xff00) | ((de + 1) & 0xff));
		Ram(de) = 0xe0;
		de = static_cast<uint16_t>((de & 0xff00) | ((de + 4) & 0xff));
	}
}


void
SpawnItem_(uint8_t item, uint8_t y, uint8_t x)
{
	SetBankInA0(6);
	SpawnItem(item, y, x);
	SetBanks1_2_3();
}


void
RestoreBackMine(uint16_t ix)
{
	uint8_t sx;
	uint16_t de = GetMineBackXY(ix, A(ix, ACTOR::X), A(ix, ACTOR::Y), &sx);
	VDP_Copy_Dot(sx, 0xa0, 0x40, static_cast<uint8_t>(de >> 8), static_cast<uint8_t>(de), 0x10,
		0x10);
}


void
UpdateEnemySprStr()
{
	uint16_t ix = EnemyList;
	for (int i = 0; i < 16; i++, ix = static_cast<uint16_t>(ix + 0x80)) {
		if (A(ix, ACTOR::ID) != 0)
			UpdateActorSprDat(ix);
	}
}


void
KillActor(uint16_t ix)
{
	uint8_t id = A(ix, ACTOR::ID);
	if (id == ID_GUARD_ALERT || id == ID_GUARD_REDALERT || id == ID_SHOOTER
		|| id == ID_JETPACK_TAKEOFF || id == ID_JETPACK_SWITCH || id == ID_JETPACK) {
		if ((--Ram(NumRespawnGuards) & 0x80) != 0)
			Ram(NumRespawnGuards) = 0;
	}
	if (id == ID_LAND_MINE)
		RestoreBackMine(ix);
	if (id != ID_SLEEPING_SIGN) {
		A(ix, ACTOR::ID) |= 0x80;
		A(ix, ACTOR::COLLISION_CFG) = 0;
		A(ix, ACTOR::ANIM_CNT) = 0;
		A(ix, ACTOR::LIFE) = 1;
		SetSoundEntry(0x16);
	}
	KillActor3(ix);
}


// Bullets stop at solid tiles (two tiles apart), but fly over railings.
void
BulletLogic(uint16_t ix)
{
	uint16_t hl = CoordToBuffTile(A(ix, ACTOR::X), A(ix, ACTOR::Y));
	for (int i = 0; i < 2; i++) {
		uint8_t tile = Ram(hl);
		if (tile == 0x6b || tile == 0x6e)
			return;
		if ((Ram(static_cast<uint16_t>(CollisionTiles + tile)) & 1) == 0)
			return;
		hl = static_cast<uint16_t>(hl + 0x40);
	}
	DismissActor0(ix);
}


void
EraseBitmapActor(uint16_t ix)
{
	uint8_t id = A(ix, ACTOR::ID) & 0x7f;
	if (id == ID_TANK || id == ID_BULLDOZER) {
		DismissActor(ix);	// made of sprites
		return;
	}
	if (id == ID_HIND_D) {
		RemoveHindD(ix);
		return;
	}
	if (id == ID_METAL_GEAR) {
		// RemoveMetalGear_
		SetBankInA0(0x0c);
		RemoveMetalGear(ix);
		SetBanks1_2_3();
		return;
	}
	if (id != ID_POWER_SWITCH)
		return;
	Ram(PowerSwitchOn) = 0;
	A(ix, ACTOR::ID) = 0;
	Ram(PowerSwitchY) = static_cast<uint8_t>(A(ix, ACTOR::Y) - 8);
	Ram(static_cast<uint16_t>(PowerSwitchY + 1)) = static_cast<uint8_t>(A(ix, ACTOR::X) - 4);
	SetRoomPal();
	DismissActor(ix);
	ErasePowerSw(ix);
}


// The destroyed power switch (two tiles); room 40 has other tiles.
void
ErasePowerSw(uint16_t ix)
{
	uint8_t y = Ram(PowerSwitchY);
	uint8_t x = Ram(static_cast<uint16_t>(PowerSwitchY + 1));
	if ((y | x) == 0)
		return;
	uint16_t hl = SYM(PowSwOffGfxX);
	if (Ram(Room) != 40)
		hl = static_cast<uint16_t>(hl - 2);
	A(ix, ACTOR::COLLISION_CFG) = 0;
	DrawDestroyPowSw(hl, x, y);
}


// ix: what the caller left in IX (the original clears that actor's
// collision flags).
void
ChkDrawDestroyPS(uint16_t ix)
{
	if (Ram(PowerSwitchY) == 0 || Ram(PowerSwitchOn) != 0)
		return;
	ErasePowerSw(ix);
}


void
RestLaserBack_()
{
	SetBankInA0(0x0f);
	RestLasersBack();
	SetBanks1_2_3();
}


// Room 72 with the goggles: every #C0 ticks the beams change (5 patterns).
void
DrawMovingLasers(uint16_t)
{
	if (Ram(Room) != 72 || Ram(SelectedItem) != SELECTED_GOGGLES)
		return;
	if (++Ram(LaserRoomTimer) != 0xc0)
		return;
	Ram(LaserRoomTimer) = 0;
	uint16_t counter = static_cast<uint16_t>(LaserRoomTimer + 1);
	Ram(counter)++;
	if (Ram(counter) == 5)
		Ram(counter) = 0;
	SetBankInA0(0x0f);
	RestLasersBack();
	SetBanks1_2_3();
	SetBanks_A_B_C();
	uint16_t de = GetPointerDE2A(SYM(idxLaserOnOff), Ram(LaserRoomCnt));
	uint16_t hl = static_cast<uint16_t>(EnemyList + 1);
	int count = Ram(NumEnemies);
	if (count == 0)
		count = 256;
	for (int i = 0; i < count; i++) {
		uint8_t on = Mem(de++);
		Ram(hl) = on;
		hl = static_cast<uint16_t>(hl + 0x0d);
		Ram(hl) = on;
		hl = static_cast<uint16_t>(hl + 0x73);
	}
	SetBanks1_2_3();
	SetBankInA0(0x0f);
	DrawLaserBeams();
	SetBanks1_2_3();
}
