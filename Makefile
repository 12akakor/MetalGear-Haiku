# Metal Gear for Haiku and RenkuOS
#   make          builds MetalGear (Haiku)
#   make tools    builds the test and helper tools (any POSIX system)

CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall
GAME = src/game/GameData.cpp src/game/Rom.cpp src/game/RomSymbols.cpp \
	src/game/Screen.cpp src/game/ScreenRender.cpp src/game/GfxBasic.cpp \
	src/game/RoomGfx.cpp src/game/SoundDriver.cpp src/game/Input.cpp \
	src/game/GameMain.cpp src/game/Title.cpp src/game/CasTape.cpp \
	src/game/SaveLoad.cpp src/game/Common.cpp src/game/Hud.cpp src/game/GameGfx.cpp src/game/Sprites.cpp src/game/Random.cpp src/game/Actors.cpp src/game/Text.cpp src/game/Player.cpp src/game/Screens.cpp src/game/Enemies.cpp src/game/Game.cpp src/game/NextRoom.cpp src/game/Doors.cpp src/game/Items.cpp src/game/Weapons.cpp src/game/Contact.cpp src/game/Demo.cpp src/game/Checkpoints.cpp src/game/Events.cpp src/game/Guards.cpp src/game/GuardAlert.cpp src/game/Cameras.cpp src/game/Shots.cpp src/game/IntroScene.cpp src/game/Dogs.cpp src/game/Bosses.cpp src/game/Soldiers.cpp src/game/ElevatorRoom.cpp src/game/Hazards.cpp src/game/Prisoners.cpp src/game/CaptureScene.cpp src/game/EquipMenu.cpp src/game/Passwords.cpp src/game/RoomActors.cpp src/game/Ending.cpp src/game/HdTiles.cpp
AUDIO = src/audio/GameAudio.cpp src/audio/MusicRenderer.cpp \
	src/audio/Reverb.cpp src/audio/ToneSynth.cpp
PLATFORM = src/platform/PlatformHaiku.cpp src/platform/PlatformCommon.cpp \
	src/platform/VideoScaler.cpp src/platform/KeyNames.cpp
REF = src/msx/Machine.cpp src/msx/Psg.cpp src/msx/Vdp.cpp src/z80/Z80.cpp

MetalGear: $(GAME) $(AUDIO) $(PLATFORM) src/main.cpp src/MetalGear.rdef
	$(CXX) $(CXXFLAGS) -o $@ $(filter %.cpp,$^) -lbe -lmedia -ltranslation -ldevice
	rc -o MetalGear.rsrc src/MetalGear.rdef
	xres -o $@ MetalGear.rsrc
	mimeset -f $@

tools: music_preview gfx_export xbr_png mgnative mg-romcheck mg-hdtool mg-tape sound_diff room_diff game_diff save_diff routine_diff

mgnative: tools/mgnative.cpp $(GAME) $(AUDIO) src/platform/VideoScaler.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread
gfx_export: tools/gfx_export.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^
xbr_png: tools/xbr_png.cpp src/platform/VideoScaler.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread
music_preview: tools/music_preview.cpp $(GAME) $(AUDIO)
	$(CXX) $(CXXFLAGS) -o $@ $^
mg-romcheck: tools/romcheck.cpp src/game/GameData.cpp src/game/RomSymbols.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
mg-tape: tools/tapetool.cpp $(GAME)
	$(CXX) $(CXXFLAGS) -o $@ $^
mg-hdtool: tools/hdtool.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
sound_diff: tests/sound/sound_diff.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^
room_diff: tests/gfx/room_diff.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^
game_diff: tests/game/game_diff.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^
save_diff: tests/save/save_diff.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^
routine_diff: tests/routine/routine_diff.cpp tests/routine/Registry.cpp $(GAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f MetalGear MetalGear.rsrc mgnative mg-romcheck mg-hdtool mg-tape sound_diff room_diff game_diff save_diff routine_diff

.PHONY: tools clean
