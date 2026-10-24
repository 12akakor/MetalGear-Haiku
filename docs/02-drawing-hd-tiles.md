# Drawing HD graphics for Metal Gear

Besides the automatic HD tiles (`tools/hd/make_hd.sh`) you can draw your
own. You draw over enlarged rooms, a tool cuts the drawing into tiles, and
the game loads them automatically. Where a tile has no HD version, the game
shows the refined (xBR) version of the original.

## Making the kit

The kit is made from your own ROM:

    make gfx_export mg-hdtool
    ./gfx_export MetalGear.rom kit

| Folder or file | Contents |
|---|---|
| `rooms/room_NNN.png` | the 251 rooms as the game draws them (256x192) |
| `rooms_x4/room_NNN.png` | the same rooms enlarged 4x (1024x768), to draw on |
| `rooms/room_NNN.keys` | for every 8x8 cell the name (key) of its tile |
| `tiles/` | every tile on its own, original size |
| `sheets/` | all tiles per tile set and palette, enlarged 8x, in a grid |
| `tiles.csv` | list of all tiles: key, tile set, palette, position, rooms |

A room is made of 32 by 24 tiles of 8x8 pixels. In the 4x version every tile
is a square of 32x32 pixels. There are 1221 different tiles. About 10 rooms
cover half of them, 32 rooms 90 percent, 62 rooms all.

## How to work

1. See which room is best to draw first:

       ./mg-hdtool coverage kit ~/config/settings/MetalGear/hd

2. Open `kit/rooms_x4/room_NNN.png` in a drawing program with layers. Draw
   over it in a new layer. Keep the size exactly 1024x768. For finer work,
   enlarge the file to 2048x1536 first (8x, "nearest neighbour") and draw
   on that.

3. Save the result as a PNG, for example `my_room_008.png`.

4. Cut it into HD tiles:

       ./mg-hdtool slice kit 8 my_room_008.png ~/config/settings/MetalGear/hd

   The tiles go to `hd/tiles/`. Tiles that exist already are kept; with
   `-force` at the end they are overwritten. Your drawings take priority
   over the automatic tiles: `make_hd.sh` does not overwrite existing
   tiles.

5. Repeat with the next room from the list of step 1.

Restart the game to see the new tiles (F6 switches HD on or off).

## Rules for drawing

- **Stay in the grid.** Every 32x32 square (at 4x) becomes a separate tile.
  Do not draw anything that runs over the edge of a square and only fits in
  this one room.
- **The same tile is the same everywhere.** When a tile occurs more often
  (a wall, a floor), the game uses the same HD version everywhere. The tool
  takes the first place where it finds the tile. Draw repeating tiles so
  that they join seamlessly.
- **Move nothing.** Walls, doors and edges must stay in the same place,
  because the game works with those squares (collisions, doors).
- **Colours are free**, but keep it recognisable. The same tile in another
  area with another palette has another key (it ends in `_p` plus the
  palette number), so it is drawn separately.
- **Dark rooms**: the kit shows them as with the flashlight. The game makes
  the dark and the night vision itself, and shows the original picture
  there.

## Help from AI image tools

You can let an AI tool enlarge a room first and use that as the base. Mind
two things: the size must stay exactly 4x or 8x, and repeating tiles
(floors, walls) often come out slightly differently per place with such a
tool. Check those and make them equal before cutting.

## Not in this kit yet

Sprites (Snake, enemies, items), the HUD, letters and menu screens.
