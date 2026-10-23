# Metal Gear for Haiku and RenkuOS (work in progress)

A native C++ port of **Metal Gear** (Konami, MSX2, 1987) for Haiku and
RenkuOS on x86_64, with optional enhanced music and graphics.

**No game data is included.** This repository contains only the engine and
tools. The game itself is read at run time from a ROM you provide: build it
yourself from the annotated disassembly by Manuel Pazos
([GuillianSeed/MetalGear](https://github.com/GuillianSeed/MetalGear)), or use
a dump of a cartridge you own.

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
plays that SCC layer itself (F10 switches it on for any ROM).

Check a ROM with `mg-romcheck`: it prints only checksums, so its output can
be shared.

    g++ -O2 -std=c++17 -o mg-romcheck tools/romcheck.cpp src/game/GameData.cpp src/game/RomSymbols.cpp
    ./mg-romcheck MetalGear.rom

Metal Gear is a trademark of Konami. This project is not affiliated with or
endorsed by Konami.

## How it works

The original program is rewritten routine by routine in C++. Every rewritten
routine is checked against the original code with a differential test: a Z80
reference machine (used only for testing, it is not part of the game) runs the
original routine on the same state, and the variables and output must match
bit for bit.

- **Music:** the original sound driver, rewritten in C++ (bit exact). The notes
  can be played on General MIDI SoundFont instruments with reverb
  ("enhanced"), or with the original square wave sound ("classic"), with or
  without the SCC layer of the SCC versions.
- **Graphics:** HD tiles are generated from the ROM on your own computer
  (crisp surfaces, smooth object contours, realistic colours for known objects,
  original brightness kept). Your own drawings in `hd/tiles` take priority.

## Status

| Part | State |
|---|---|
| Z80 reference machine for tests (zexdoc/zexall pass) | done |
| Sound driver in C++, bit exact | done |
| SoundFont music and classic sound | done (preview tool) |
| Graphics export and HD tile generator | done (tools) |
| Start up, Konami logo, Metal Gear logo, menu | done, tick for tick equal |
| Save and load (tape, .CAS images) | done, tick for tick equal; reachable once the pause screen is rewritten |
| Demo and game logic in native C++ | in progress |
| Haiku window, input, sound output | first version (from the Maze of Galious port) |

## Building and running on Haiku

Tested on Haiku R1/beta6 x86_64 (hrev59866+92), in VirtualBox.

    make
    mkdir -p ~/config/settings/MetalGear
    cp MetalGear.rom ~/config/settings/MetalGear/
    ./MetalGear

The enhanced music uses `data/MetalGear-FluidR3.sf2` (20 MB, included): the
instruments and drums of the Fluid R3 General MIDI SoundFont (MIT licence)
that the game uses. The instruments per music are in `music.cfg` in the
settings folder (written on first run). To choose other instruments there,
put the complete FluidR3_GM.sf2 (or any General MIDI SoundFont) in the
settings folder; it is then used instead. Without any SoundFont the
original sound plays.

Make a subset of another SoundFont with:

    tools/sf2/sf2_subset.py FluidR3_GM.sf2 out.sf2 0:61,0:33,128:0 -drumkeys 36,38

Keys: arrows, space (fire), M or N (second button), F1-F5 and Return as on
the MSX. Port keys (as in the Maze of Galious port where the game allows;
Metal Gear uses F1-F5 itself):

| Key | Function |
|---|---|
| F7 | Next filter (xBR, nearest, sharp) |
| F8 | Show or hide the timing display |
| F9 or Print Screen | Screenshot (PNG in the home folder) |
| F10 | Music: enhanced, original sound, original sound with SCC |
| F11 | Full screen on or off |
| F12 or Alt+Q | Quit |

## Building the ROM (needed for everything)

    git clone https://github.com/GuillianSeed/MetalGear
    cd MetalGear
    sjasm MetalGear.asm MetalGear.rom      # Sjasm 0.39

## Icon

`data/icon` holds the application icon, an original design (an olive drab
gear with a radar screen): `MetalGear.hvif` (Haiku vector icon, also in
`src/MetalGear.rdef`, attached by `make`) and PNG versions from 16 to 512
pixels. `tools/icon/make_icon.py data/icon src/MetalGear.rdef` rebuilds
them; the PNGs are drawn from the HVIF data the way Haiku draws icons.

## Saved games

The original saves to cassette. The port writes the same bytes to a tape
image, `~/config/settings/MetalGear/MetalGear.cas`, in the .CAS format
that MSX emulators (openMSX, blueMSX, fMSX) use. Saves are
interchangeable: put a .CAS with Metal Gear saves from an emulator there
under that name to load them in the port, or insert the port's file as a
tape in an emulator and load it with F4 in the original. Each save holds
its name (up to 6 letters or digits) and the game progress (#300 bytes).
Saving under a name that is already on the tape replaces that save; `mg-tape
file.cas` lists the saves on a tape.

Disk saves made with third party disk patches are not supported (their
format is unknown here); a sample file would be needed.

## Tools

    # music previews (needs a General MIDI SoundFont, e.g. FluidR3_GM.sf2)
    g++ -O2 -std=c++17 -o music_preview tools/music_preview.cpp src/audio/*.cpp src/game/*.cpp
    ./music_preview MetalGear.rom FluidR3_GM.sf2 out 60

    # graphics export, HD tool
    g++ -O2 -std=c++17 -o gfx_export tools/gfx_export.cpp src/game/*.cpp src/msx/*.cpp src/z80/*.cpp
    ./gfx_export MetalGear.rom export
    g++ -O2 -std=c++17 -o mg-hdtool tools/hdtool.cpp

    # sound driver test against the original
    make tools          # tests and tools: sound_diff, room_diff, game_diff, mgnative, ...
    ./sound_diff MetalGear.rom
    ./game_diff MetalGear.rom 700
    ./save_diff MetalGear.rom

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

`tools/check/haiku_syntax.sh` checks the Haiku specific code against the
real Haiku headers on another system.

Port by Akakor, developed with the help of Claude (Anthropic).
