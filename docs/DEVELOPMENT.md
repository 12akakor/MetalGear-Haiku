# Metal Gear for Haiku: technical notes

For players the [README](../README.md) is enough. This page is about how the
port is made, the supported ROM versions in detail, and the tools.

## How it is made

The original program is rewritten routine by routine in C++ from the
annotated disassembly by Manuel Pazos
([GuillianSeed/MetalGear](https://github.com/GuillianSeed/MetalGear)). Every
rewritten routine is checked against the original code with differential
tests: a Z80 reference machine (`src/msx`, `src/z80`, used only for testing,
not part of the game) runs the original on the same state, and the variables,
video memory and palette must match bit for bit.

| Test | What it compares |
|---|---|
| `game_diff` | the whole game, tick by tick (random play, warps between rooms, the ending, typed passwords) |
| `routine_diff` | single routines inside the running original |
| `sound_diff` | the sound driver, every sound and combination |
| `room_diff` | every room as drawn |
| `save_diff` | saving and loading |

The improvements of the port (no flicker, HD tiles, fades, stereo) are
options that are off in the tests, so the tests always compare with the
original behaviour.

## ROM versions

Supported: the English version (CRC32 `E85C5731`, SHA1 `b656bd19...`) and the
Japanese version (CRC32 `FAFE1303`, SHA1 `a52021f1...`), the same dumps as in
the openMSX software database. Overdumps (128 KB repeated) are accepted, and
modified versions are accepted with a note when at least 80 percent of their
256 byte blocks equal one of the two (unrelated ROMs score near 0, the two
versions 67 percent with each other). Texts, fonts and graphics then come
from that ROM; code changes of a hack are not followed. Tested with the
differential test, 0 differences each:

| ROM | Base | Remark |
|---|---|---|
| Official English Translation (= English) | English | |
| French translation by Django, French 6393, French 6874 | Japanese | |
| Russian translation by Andrew Shtein | Japanese | |
| Remix 1.995 | Japanese | its moved dictionary is found in the code |
| Remix 1.995 SCC Version, TFH SCC version (136 KB) | Japanese / English | SCC sound on, see below |
| 1.995 Remix + Nekura Hoka + Improvements v1.0 (160 KB) | Japanese | the speed improvements in the extra banks are not needed and not used |

The SCC versions (patch by Fony, 2018) need a Konami SCC cartridge: bank 0
is a new start bank with the SCC code and the original bank 0 is moved to
the end; every write to the PSG also goes to the SCC, which plays the same
notes as a sawtooth. The port recognises them, uses the original bank 0 and
plays that SCC layer itself (it can be switched on for any ROM).

Check a ROM with `mg-romcheck`: it prints only checksums, so its output can
be shared.

    make mg-romcheck
    ./mg-romcheck MetalGear.rom

Build the ROM from the disassembly:

    git clone https://github.com/GuillianSeed/MetalGear
    cd MetalGear
    sjasm MetalGear.asm MetalGear.rom      # Sjasm 0.39

## Music

The original sound driver is rewritten in C++ (bit exact). For the enhanced
music the notes go to General MIDI instruments of a SoundFont with reverb.
The game comes with the instruments and drums it uses from GeneralUser GS
2.0.3 by S. Christian Collins (`data/MetalGear-GeneralUser.sf2`, 7 MB,
licence in `data/LICENSE.GeneralUser.txt`). The instruments and levels per
music are in `music.cfg` in the settings folder, written on first run for
this SoundFont; a `music.cfg` made for another one is kept as
`music.cfg.old` and replaced. Any other General MIDI SoundFont in the
settings folder is used instead.

Make a subset of another SoundFont with the instruments and drums the game
uses:

    tools/sf2/sf2_subset.py GeneralUser-GS.sf2 out.sf2 \
        0:29,0:30,0:33,0:36,0:38,0:48,0:50,0:56,0:61,0:62,0:80,0:81,128:0 \
        -drumkeys 36,38,40,41,42,45,48,49,50

## HD room tiles

Every 8x8 cell of the room is identified by a key: a hash of its 64 colour
numbers plus the room palette, as `tools/gfx_export` writes them. When
`hd/tiles/<key>.png` exists, the cell is painted in HD over the filtered
picture; sprites stay on top. Pixels in a colour that is changed at the
moment (a pulsing lamp, a flash, a fade, the dark) keep the original.

`tools/hd/make_hd.sh` makes the automatic set (`gfx_export` reads the rooms
from the ROM, `tools/hd/hd_auto.py` paints the tiles, `xbr_png` enlarges).
Own drawings go in `hd/tiles` too and take priority; see
[02-hd-tekenen.md](02-hd-tekenen.md) (Dutch) and `mg-hdtool`.

## Saved games

The original saves to cassette. The port writes the same bytes to a tape
image, `~/config/settings/MetalGear/MetalGear.cas`, in the .CAS format that
MSX emulators (openMSX, blueMSX, fMSX) use, so saves can be exchanged with
them. Each save holds its name (up to 6 letters or digits) and the game
progress (#300 bytes). Saving under a name that is already on the tape
replaces that save; `mg-tape file.cas` lists the saves on a tape. Disk saves
of third party disk patches are not supported.

## Tools

    make tools          # all tools and tests below
    ./music_preview MetalGear.rom data/MetalGear-GeneralUser.sf2 out 60
    ./gfx_export MetalGear.rom export
    ./mgnative MetalGear.rom 3000 -shot 2000 -out shots   # the game without a window
    ./sound_diff MetalGear.rom
    ./game_diff MetalGear.rom 20000 -play 1 -warp 300
    ./save_diff MetalGear.rom

`tools/check/haiku_syntax.sh` checks the Haiku specific code against the
real Haiku headers on another system.

## Icon

`data/icon` holds the application icon, an original design (an olive drab
gear with a radar screen): `MetalGear.hvif` (Haiku vector icon, also in
`src/MetalGear.rdef`, attached by `make`) and PNG versions from 16 to 512
pixels. `tools/icon/make_icon.py data/icon src/MetalGear.rdef` rebuilds
them.

## Coding style

The code follows the [Haiku coding guidelines](https://www.haiku-os.org/development/coding-guidelines)
with these deliberate exceptions:

- The game core (`src/game`, `src/audio`) does not depend on Haiku, so the
  tests and tools also build on other systems. It uses the `stdint.h` types
  (`uint8_t`) instead of the SupportDefs.h types (`uint8`).
- Routines rewritten from the disassembly keep the names of the
  disassembly (`VDP_Copy_Byte`, `LoadRoomTiles`, `SNSMAT`), so every one
  can be found in the original and checked against it.
- The sound driver (`src/game/SoundDriver.cpp`) keeps the jump structure of
  the original routine with `goto`, so it can be compared with
  `bgmdriver.asm` line by line.
- `src/msx` and `src/z80` are the reference machine for the tests only;
  they are not part of the game.
