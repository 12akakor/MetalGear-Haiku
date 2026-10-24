# Metal Gear (MSX2) to C++ for x86_64, Haiku and RenkuOS: analysis and plan

Date: 2026-10-17. This is the plan the port started from; what came of it is
in [01-progress.md](01-progress.md).

## 1. Source

- Repository: https://github.com/GuillianSeed/MetalGear (last commit `30d1b94`, January 2022).
- Contents: the fully annotated disassembly of Metal Gear (Konami, MSX2, 1987, code RC750) by Manuel Pazos (2017). 225 `.asm` files, about 56,600 lines, plus 251 PNGs (room pictures for documentation).
- Assembles with Sjasm 0.39 (`sjasm MetalGear.asm MetalGear.rom`). **Checked:** Sjasm 0.39j built here, the output is 131072 bytes with CRC32 `E85C5731`, exactly the English ROM according to the README. The Japanese version with `JAPANESE equ 1`.
- Licence: none. README: "for education purposes only", and the repository is removed if Konami asks. The code and all data (graphics, music, maps) belong to Konami.

## 2. What the code does (analysis)

- **Hardware:** MSX2, Z80 at 3.58 MHz, 128 KB MegaROM with the Konami mapper (16 banks of 8 KB; bank 0 fixed at #4000, banks switched at #6000, #8000 and #A000 by writing to those addresses).
- **Picture:** SCREEN 5 (256x212, 16 colours out of a palette of 512), sprite mode 2 (16x16), VDP commands (HMMM/LMMM/HMMV and the like) to draw the rooms and the HUD, two VRAM pages. Direct VDP access with `out (c)` (about 95 times) and `otir`.
- **Sound:** its own music and effects driver (`sound/bgmdriver.asm`) that drives the PSG (AY-3-8910) through the BIOS routine WRTPSG. No SCC.
- **Input:** keyboard through BIOS SNSMAT (rows 4, 6 and 8), joystick through the PSG registers.
- **Saving:** to cassette (BIOS TAPION/TAPOUT and so on), and passwords.
- **Program flow:** after start up the code sets an interrupt hook (H.TIMI, #FD9F) and then waits in `jr $`. **All game logic runs in the 60 Hz VBlank interrupt** (`InterruptTick` → `UpdateSound` + `GameStatusLogic`). A game tick that takes too long is skipped at the next interrupt (only the sound runs).
- **BIOS routines used (few):** CHGMOD, WRTVDP, RDVDP, WRTPSG, RDPSG, SNSMAT, CHGET, ENASLT, RSLREG and the cassette routines. Otherwise only system variables (EXPTBL, H.TIMI, CLIKSW, FNKSTR, BASVER).
- **Jump tables:** `call JumpIndex` followed by a table of addresses; the routine takes the return address from the stack as the table address. Stack tricks like this make a direct translation into plain C++ functions difficult.

## 3. Approach (updated 17 October 2026)

**Choice: fully native C++, like the Maze of Galious port.** No emulation in
the game: no Z80, no imitated VDP or PSG, no BIOS. Every routine of the
original is rewritten by hand in C++ and then compared automatically with the
original (differential test).

The Z80 interpreter and the MSX machine of phase 1 remain **test tools**
only: they run the original code, so every rewritten routine can be checked
bit for bit (same variables, same output). They are not part of the game or
the package.

Size: about 17,000 Z80 instructions of code (the rest is data: graphics,
maps, music, texts). The data stays in its original format and is read from
the ROM the player provides.

### Wishes (17 October 2026)

- **Music:** the notes of the original music on better instruments
  (SoundFont, General MIDI) with reverb, recognisable and with the same
  timing. The original sound as an option. Sound effects keep their
  original character.
- **Graphics:** refined (xBR) at full screen resolution by default.
- **HD drawing:** tools and instructions to draw HD tiles; the game loads
  them automatically, with xBR as the fallback.
- **Modern effects:** soft transitions between screens, light and shadow,
  night vision and dark rooms as real effects, HD HUD, optional CRT.

## 4. Phases

**Phases 0 and 1: source and reference machine (done)**

**Phase 2: sound (done)**
- Sound driver in C++ (`src/game/SoundDriver.cpp`), bit for bit equal to
  the original: 204,475 comparisons, 0 failures.
- Tone generator (PolyBLEP) for the original sound and the effects.
- SoundFont playback (TinySoundFont, MIT) with GeneralUser GS 2.0.3
  (FluidR3_GM before), instruments per music and channel in `music.cfg`,
  reverb.

**Phase 3: HD kit (first version done)**
- Export of all 251 rooms and 1221 tiles, a key per tile, the tool
  `mg-hdtool` (slicing, coverage). Sprites, HUD and menus to follow.

**Phase 4: game logic native**
- The drawing routines turn out to be a small set: copying 8x8 tiles
  (`VDP_Copy_Byte`), filling rectangles, lines, uploading sprites, palette.
  These become native drawing functions on bitmaps of our own.
- Then the game logic part by part (rooms, Snake, enemies, weapons, menus,
  radio), each time with a differential test against the original.

**Phase 5: Haiku platform and effects**
- Window, scaling, keys, settings and screenshots from the Maze of Galious
  port.
- Loading HD tiles, xBR as the fallback, modern effects.

**Phase 6: clean up and package**

## 5. Legal

Metal Gear belongs to Konami and is still sold (Metal Gear Solid: Master
Collection Vol. 1, 2023). Therefore (decision of 18 October 2026):

- The repository is public but contains **only our own code**: no ROM, no
  graphics, no music recordings, no HD tiles.
- The player provides the ROM (built from the disassembly by Manuel Pazos,
  or a dump of a cartridge they own).
- The enhanced music and the HD tiles are made from that ROM on the
  player's computer. Own drawings stay local in `hd/tiles`.
- Licence of our own code: MIT.

## 6. Way of working and checks

- Small commits, every step tested.
- Every rewritten routine gets a differential test against the original.
- The game is done when a complete play session (from a fixed key script)
  gives the same game state as the original, frame by frame.
