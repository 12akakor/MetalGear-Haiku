# Metal Gear for Haiku and RenkuOS
#   make          builds MetalGear
#   make tools    builds the test and helper tools (any POSIX system)

CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall
GAME = src/game/game_data.cpp src/game/rom.cpp src/game/rom_symbols.cpp \
	src/game/screen.cpp src/game/screen_render.cpp src/game/gfx_basic.cpp \
	src/game/room_gfx.cpp src/game/sound_driver.cpp src/game/input.cpp \
	src/game/game_main.cpp src/game/title.cpp
AUDIO = src/audio/game_audio.cpp src/audio/music_renderer.cpp \
	src/audio/reverb.cpp src/audio/tone_synth.cpp
PLATFORM = src/platform/platform_haiku.cpp src/platform/platform_common.cpp \
	src/platform/video_scaler.cpp src/platform/keynames.cpp
REF = src/msx/machine.cpp src/msx/psg.cpp src/msx/vdp.cpp src/z80/z80.cpp

MetalGear: $(GAME) $(AUDIO) $(PLATFORM) src/main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -lbe -lmedia -ltranslation -ldevice

tools: mgnative mg-romcheck mg-hdtool sound_diff room_diff game_diff

mgnative: tools/mgnative.cpp $(GAME) $(AUDIO)
	$(CXX) $(CXXFLAGS) -o $@ $^
mg-romcheck: tools/romcheck.cpp src/game/game_data.cpp src/game/rom_symbols.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
mg-hdtool: tools/hdtool.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
sound_diff: tests/sound/sound_diff.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^
room_diff: tests/gfx/room_diff.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^
game_diff: tests/game/game_diff.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f MetalGear mgnative mg-romcheck mg-hdtool sound_diff room_diff game_diff

.PHONY: tools clean
