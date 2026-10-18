# Metal Gear for Haiku and RenkuOS (work in progress)

A native C++ port of **Metal Gear** (Konami, MSX2, 1987) for Haiku and
RenkuOS on x86_64, with optional enhanced music and graphics.

**No game data is included.** This repository contains only the engine and
tools. The game itself is read at run time from a ROM you provide: build it
yourself from the annotated disassembly by Manuel Pazos
([GuillianSeed/MetalGear](https://github.com/GuillianSeed/MetalGear)), or use
a dump of a cartridge you own. Only the English version is supported (CRC32
`E85C5731`).

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
| Game logic in native C++ | in progress |
| Haiku window, input, settings | planned |

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
