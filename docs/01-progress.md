# Progress of the Metal Gear port

## Phase 0: analysis (done)
- The disassembly (Manuel Pazos) builds with Sjasm 0.39j into a ROM with
  CRC32 E85C5731.
- All game logic runs in the H.TIMI hook (InterruptTick #41AC).

## Phase 1: reference machine (done)
- Z80 interpreter: zexdoc and zexall pass, M1 wait state (+1 T).
- V9938 subset: G4, sprite mode 2, all commands with a timing model.
- AY-3-8910 PSG, Konami mapper, BIOS calls in C++.
- Headless runner tools/mgrun (screenshots, WAV, key script, state, dump).

### Comparison with openMSX 19.1 + C-BIOS (start of the demo, GameStatus 2/1)
- VRAM: 131071 of 131072 bytes equal.
- RAM C000-EFFF: 31 bytes differ, all counters and sound state (another
  phase of the music, because the BIOS start up is shorter than in C-BIOS).
- Length of the phases in frames equal within about 1 to 5 percent.

## Next steps (at that time)
- Phase 2: static recompiler (listing to C++) with a differential test
  against the interpreter (RAM, VRAM, PSG per frame).
- Phase 3: Haiku platform layer (reused from the Maze of Galious port).
- Phase 4: clean up.

## 17 October 2026: course changed to fully native

- Sound driver rewritten in C++, bit for bit equal to the original (all 64
  sounds alone, combinations with pause and radio, and the real demo).
- Tone generator, SoundFont music with reverb, test recordings of all
  music.
- The drawing routines of the game mapped (a small set of primitives).
- HD kit: export of 251 rooms and 1221 tiles, `mg-hdtool`, manual.

## 18 October 2026

- Public repository MetalGear-Haiku with only our own code (MIT); the ROM,
  graphics and music come from the player.
- English (E85C5731) and Japanese (FAFE1303) ROM supported: CRC check and
  address tables per version, generated from the disassembly. The RAM
  layout is the same in both versions. Other versions get a clear message.
- Native screen layer (pages, palette, settings, rectangle and copy
  operations) and loading and drawing the rooms: differential test 2115
  comparisons, 0 failures, in both versions (16 unused rooms,
  RoomUndefined, skipped).
- Automatic HD tiles: army green tanks, olive green trucks, wooden crates,
  brightness equal to the original.

Next step (at that time): start up (InitHardware), the main loop
(InterruptTick, GameStatusLogic) and the Konami logo and title screen
native, with a frame by frame comparison.

## 18 October 2026 (saving and loading)

- `logic/saveload.asm` native (SaveLoad.cpp): entering the name, saving,
  verifying, loading while skipping other names, error messages.
  Differential test `save_diff`: 97 comparisons, 0 failures, both versions.
- The cassette is a .CAS file (`MetalGear.cas` in the settings folder),
  byte for byte what the original puts on tape: block 1 is 10 x EA plus the
  name, block 2 is #300 bytes of progress plus a final byte. Exchangeable
  with openMSX, blueMSX and fMSX.
- Notable in the original: the checksum is always 0 (`ld a,c` overwrites
  the sum), and #300 bytes are saved while the buffer is #220.
- Additions of the port: an older save with the same name is replaced,
  loading starts at the beginning, verify starts at the save just made.

## 19 and 20 October 2026: the whole game native

- All routines of the game are rewritten in C++, routine by routine after
  the disassembly: main loop and game states, Snake, text windows, radio,
  weapon and equipment screens, doors, items, weapons, demo, all enemies
  and bosses, prisoners, capture, passwords and the ending.
  `Unported.cpp` is gone.
- Quirks of the original are imitated where the game relies on them, such
  as registers left over after a routine, `LD A,R` as a random number
  (replayed in the tests) and returns that skip the calling level.
- Tests:
  - `game_diff` compares RAM, VRAM and palette with the original every
    tick. With random input and jumps to other rooms: 0 failures in 40000
    ticks per seed (English and Japanese).
  - `MG_ENDING=tick` jumps to the ending and plays it until the restart,
    also with 0 failures.
  - `routine_diff` checks single routines inside the running original.
  - `room_diff` and `save_diff` also give 0 failures.
- The game runs at 60 frames per second, as the MSX2 at 60 Hz.

## 23 October 2026: ROM variants and SCC sound

- Nine ROMs tested (translations, Remix 1.995, SCC versions, Nekura Hoka).
  All French, the Russian and the Remix versions are made from the
  Japanese ROM, the TFH SCC version from the English one. They change
  almost only texts, font and item names.
- Recognition per 256 byte block (only checksums in the source): the base
  ROM 89 percent or more equal, the other version about 67 percent,
  threshold 80 percent. SCC versions (136 KB): bank 16 is the original
  bank 0. Nekura Hoka (160 KB): the extra banks (speed up) are not used.
- Remix 1.995 moves the dictionary of the Japanese texts; its address is
  now read from the code (`ld de,idxDictionary` in AddDictEntry).
- Differential test with every ROM (3 seeds, ending, passwords): 0
  failures. For the SCC and Nekura versions with the original code plus
  their data.
- SCC versions (Fony, 2018): every PSG write also goes to the SCC (pitch,
  volume without envelope, channel on or off), all three channels with the
  same sawtooth. The same notes with a fuller sound; no new music.
  Imitated in the tone generator; F10 switches between enhanced, original
  and original with SCC. SCC ROMs start with the SCC sound.
- Test platform: Haiku R1/beta6 x86_64, hrev59866+92 (VirtualBox). Builds
  and plays there.

## 24 October 2026: GeneralUser GS as the SoundFont

- Compared (a page with clips per music): FluidR3, GeneralUser GS 2.0.3,
  MuseScore General, ChoriumRevA, Weeds GM3 and SGM V2.01. Choice:
  GeneralUser GS. Its licence allows use in software; Weeds is for
  personal use only, Chorium and SGM state no licence.
- Subset with the 13 instruments and 9 drum sounds of the game: 6.9 MB
  (FluidR3 was 20.5 MB), sounds the same as the complete file. Stays sf2;
  sf3 is not needed at this size.
- Instrument choice unchanged; the volume of every instrument measured in
  both SoundFonts so the balance stays the same, loudness per music the
  same as before. `music.cfg` now has a `tuning` line; an old file becomes
  `music.cfg.old`. Drum and bass volume are in `music.cfg` too.

## 24 October 2026: improvements of picture, sound and handling

All optional, the way the game plays does not change; the tests run with
the original settings (0 failures).

- **No flicker:** all sprites on every line in a fixed order. The HUD rows
  keep the limit of 8, because the elevators hide sprites with it
  (SetHUDSprMask).
- **HD tiles in the game:** the key of every 8x8 cell (hash plus palette)
  is looked up in `hd/tiles`; painted over the filtered picture, sprites
  stay on top. Pixels in a colour that is changed at the moment (a pulsing
  lamp, a flash, a fade) keep the original. With a cache: 0.6 ms per
  frame, about 12 ms for a new room. Checked over all rooms reached
  through a door: HD in 119 rooms, mostly in the 2 power switch rooms,
  none only in the dark rooms without flashlight. `tools/hd/make_hd.sh`
  makes the automatic set from the player's ROM; `xbr_png` added, a bug in
  `gfx_export` fixed.
- **Smooth room changes:** a short cross-fade (220 ms) of the picture.
- **Stereo:** mono, a little (as before) or wide.
- **Settings window (F6):** picture, music, stereo, flicker, HD, fades,
  full screen, waiting for the screen refresh; kept in
  `MetalGear.settings` together with the window position.
- **Choosing the ROM:** without a ROM a file panel; a ROM dropped on the
  window or the program starts at once. The chosen ROM is kept as
  `MetalGear.rom`.

## 24 October 2026: package for Haiku

- `make package` builds `metalgear-1.0.0-1-<arch>.hpkg`: the game, the
  SoundFont and the HD tools in `/boot/system/apps/MetalGear`, an entry in
  the Deskbar menu and the licence of GeneralUser GS. Version 1.0.0 (also
  in the resources of the program).
- `make_hd.sh` also runs from the installed package, without source code
  or compiler.
- Saving and loading checked the way a player does it: F1, then F5 (save)
  or F4 (load). F5 while playing does nothing, as in the original.
- README rewritten for players, with two HD screenshots; the technical
  notes moved to `docs/DEVELOPMENT.md`. All documentation in English.
