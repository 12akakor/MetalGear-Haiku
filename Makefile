# Metal Gear for Haiku and RenkuOS
#   make          builds MetalGear (Haiku)
#   make tools    builds the test and helper tools (any POSIX system)

CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall
GAME = src/game/GameData.cpp src/game/Rom.cpp src/game/RomSymbols.cpp \
	src/game/Screen.cpp src/game/ScreenRender.cpp src/game/GfxBasic.cpp \
	src/game/RoomGfx.cpp src/game/SoundDriver.cpp src/game/Input.cpp \
	src/game/GameMain.cpp src/game/Title.cpp
AUDIO = src/audio/GameAudio.cpp src/audio/MusicRenderer.cpp \
	src/audio/Reverb.cpp src/audio/ToneSynth.cpp
PLATFORM = src/platform/PlatformHaiku.cpp src/platform/PlatformCommon.cpp \
	src/platform/VideoScaler.cpp src/platform/KeyNames.cpp
REF = src/msx/Machine.cpp src/msx/Psg.cpp src/msx/Vdp.cpp src/z80/Z80.cpp

MetalGear: $(GAME) $(AUDIO) $(PLATFORM) src/main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -lbe -lmedia -ltranslation -ldevice

tools: mgnative mg-romcheck mg-hdtool sound_diff room_diff game_diff

mgnative: tools/mgnative.cpp $(GAME) $(AUDIO)
	$(CXX) $(CXXFLAGS) -o $@ $^
mg-romcheck: tools/romcheck.cpp src/game/GameData.cpp src/game/RomSymbols.cpp
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
