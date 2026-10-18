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
lightly modified versions (translations, small hacks) are accepted with a
warning when at least 12 of the 16 banks equal one of the two. Check a ROM
with `mg-romcheck`: it prints only checksums, so its output can be shared.

    g++ -O2 -std=c++17 -o mg-romcheck tools/romcheck.cpp src/game/game_data.cpp src/game/rom_symbols.cpp
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
  ("enhanced"), or with the original square wave sound ("classic").
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
| Demo and game logic in native C++ | in progress |
| Haiku window, input, sound output | first version (from the Maze of Galious port) |

## Building and running on Haiku

    make
    mkdir -p ~/config/settings/MetalGear
    cp MetalGear.rom ~/config/settings/MetalGear/
    ./MetalGear

Optional: put a General MIDI SoundFont (`.sf2`, for example FluidR3_GM.sf2)
in the same folder for the enhanced music; without it the original sound is
used. Instruments per music are in `music.cfg` there (written on first run).

Keys: arrows, space (fire), M or N (second button), F1-F5 and Return as on
the MSX. F8 music classic/enhanced, F9 timing, F10 filter, F11 full screen,
F12 screenshot.

## Building the ROM (needed for everything)

    git clone https://github.com/GuillianSeed/MetalGear
    cd MetalGear
    sjasm MetalGear.asm MetalGear.rom      # Sjasm 0.39

## Tools

    # music previews (needs a General MIDI SoundFont, e.g. FluidR3_GM.sf2)
    g++ -O2 -std=c++17 -o music_preview tools/music_preview.cpp src/audio/*.cpp src/game/*.cpp
    ./music_preview MetalGear.rom FluidR3_GM.sf2 out 60

    # graphics export, HD tool
    g++ -O2 -std=c++17 -o gfx_export tools/gfx_export.cpp src/msx/*.cpp src/z80/z80.cpp
    ./gfx_export MetalGear.rom export
    g++ -O2 -std=c++17 -o mg-hdtool tools/hdtool.cpp

    # sound driver test against the original
    g++ -O2 -std=c++17 -o sound_diff tests/sound/sound_diff.cpp src/game/*.cpp src/msx/*.cpp src/z80/z80.cpp
    ./sound_diff MetalGear.rom

Port by Akakor, developed with the help of Claude (Anthropic).
