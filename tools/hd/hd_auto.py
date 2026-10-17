#!/usr/bin/env python3
"""
Automatic HD tiles for the Metal Gear port.

Reads the export of gfx_export (rooms, keys, tiles.csv), and writes for every
tile key an HD version (4x) to <hd>/tiles/<key>.png:
  - shapes: xBR contours for objects, crisp square shapes for repeating
    surface tiles (floors, walls), so patterns stay rectangular
  - colours: the original brightness is kept; known objects get realistic
    colours (tank and trucks army green, crates wood), the rest keeps the
    original hue with a richer tint
  - relief light from the top left, soft shadow in corners, fine texture
Tiles that already exist in <hd>/tiles are not touched (own drawings win).

  hd_auto.py <export dir> <hd dir> <xbr tool> [-preview room,room,...] [-force]
"""
import sys, os, csv, subprocess, zlib, colorsys
import numpy as np
from PIL import Image
from scipy import ndimage as nd

S = 4

# Known objects: (room, col0, row0, col1, row1, class). The tiles inside the
# rectangles of a class that do not also occur outside them in that room get
# the class; it then applies to that tile position of the tileset in every
# room.
OBJECTS = [
    (8, 20, 8, 26, 15, 'tank'), (0, 0, 0, 10, 10, 'tank'), (121, 0, 4, 10, 11, 'tank'),
    (121, 18, 4, 28, 11, 'tank'),
    (8, 26, 4, 32, 16, 'crate'), (7, 0, 8, 4, 16, 'crate'), (7, 20, 8, 24, 20, 'crate'),
    (18, 12, 16, 22, 24, 'crate'), (121, 0, 16, 8, 20, 'crate'),
    (5, 4, 4, 8, 12, 'truck'), (5, 12, 8, 16, 16, 'truck'), (5, 20, 4, 24, 12, 'truck'),
    (7, 4, 12, 8, 20, 'truck'), (7, 8, 8, 16, 16, 'truck'), (64, 4, 8, 12, 16, 'truck'),
]

# Realistic colour ramps (dark to light) for object classes; the original
# brightness of each pixel picks the place on the ramp.
RAMPS = {
    'tank':  [(14, 17, 9), (42, 50, 26), (78, 88, 46), (118, 128, 76), (176, 182, 130)],
    'truck': [(16, 18, 10), (46, 54, 30), (84, 92, 52), (126, 132, 84), (184, 186, 140)],
    'crate': [(26, 15, 8), (74, 46, 24), (122, 82, 44), (168, 122, 72), (214, 176, 120)],
}
TEXTURE = {'tank': 'brush', 'truck': 'brush', 'crate': 'wood', 'surface': 'speckle',
           'default': 'speckle'}


def luma(rgb):
    return rgb[..., 0]*0.299 + rgb[..., 1]*0.587 + rgb[..., 2]*0.114


def ramp_colour(ramp, l):
    """Colour on the ramp with luminance l (0..1)."""
    pts = np.array(ramp, np.float32)/255
    pl = luma(pts)
    idx = np.clip(np.searchsorted(pl, l), 1, len(pts)-1)
    t = np.clip((l - pl[idx-1])/np.maximum(pl[idx]-pl[idx-1], 1e-4), 0, 1)[..., None]
    c = pts[idx-1]*(1-t) + pts[idx]*t
    # exact luminance of the original
    return np.clip(c * (l/np.maximum(luma(c), 1e-4))[..., None], 0, 1)


def richer(rgb):
    """Original hue, a bit more saturation, same luminance."""
    out = np.empty_like(rgb)
    flat = rgb.reshape(-1, 3)
    o = out.reshape(-1, 3)
    for i, (r, g, b) in enumerate(flat):
        h, l, s = colorsys.rgb_to_hls(r, g, b)
        o[i] = colorsys.hls_to_rgb(h, l, min(1.0, s*1.25))
    lo = luma(out)
    return np.clip(out * (luma(rgb)/np.maximum(lo, 1e-4))[..., None], 0, 1)


def texture(kind, seed, n):
    rng = np.random.default_rng(seed)
    z = rng.standard_normal((n, n)).astype(np.float32)
    if kind == 'brush':
        return nd.gaussian_filter(z, (0.4, 4), mode='wrap')*2.5
    if kind == 'wood':
        return nd.gaussian_filter(z, (6, 0.5), mode='wrap')*3 + nd.gaussian_filter(z, 1)*0.6
    return nd.gaussian_filter(z, 0.8, mode='wrap')*1.4


def main():
    exp, hd, xbr = sys.argv[1], sys.argv[2], sys.argv[3]
    preview = []
    if '-preview' in sys.argv:
        preview = [int(v) for v in sys.argv[sys.argv.index('-preview')+1].split(',')]
    force = '-force' in sys.argv
    os.makedirs(hd + '/tiles', exist_ok=True)
    info = {r['key']: r for r in csv.DictReader(open(exp + '/tiles.csv'))}
    keys = {}
    for r in range(251):
        keys[r] = [l.split() for l in open('%s/rooms/room_%03d.keys' % (exp, r))]
    count = {}
    for r in keys:
        for row in keys[r]:
            for k in row:
                count[k] = count.get(k, 0) + 1

    # object classes per (tileset, position)
    cls = {}
    groups = {}
    for room, c0, r0, c1, r1, name in OBJECTS:
        groups.setdefault((room, name), []).append((c0, r0, c1, r1))
    for (room, name), rects in groups.items():
        inside, outside = set(), set()
        for y in range(24):
            for x in range(32):
                k = keys[room][y][x]
                if k == '-':
                    continue
                p = (info[k]['tileset'], info[k]['position'])
                hit = any(c0 <= x < c1 and r0 <= y < r1 for c0, r0, c1, r1 in rects)
                (inside if hit else outside).add(p)
        for p in inside - outside:
            cls[p] = name
    # The crates are loaded at the same place for every tileset except the
    # Metal Gear one (LoadGfxCrates), so their positions apply everywhere.
    for (ts, pos), name in list(cls.items()):
        if name == 'crate':
            for t in ('0', '1', '2', '3', '4', '5'):
                cls.setdefault((t, pos), 'crate')

    def tile_class(k):
        p = (info[k]['tileset'], info[k]['position'])
        if p in cls:
            return cls[p]
        return 'surface' if count[k] >= 12 else 'default'

    written = 0
    done = set()
    tmp = hd + '/.tmp_xbr.png'
    for r in range(251):
        src_img = Image.open('%s/rooms/room_%03d.png' % (exp, r)).convert('RGB')
        src = np.asarray(src_img).astype(np.float32)/255
        need = [k for row in keys[r] for k in row if k != '-' and k not in done and
                (force or not os.path.exists('%s/tiles/%s.png' % (hd, k)))]
        if not need:
            continue
        subprocess.run([xbr, '%s/rooms/room_%03d.png' % (exp, r), tmp, str(S)], check=True)
        xb = np.asarray(Image.open(tmp).convert('RGB')).astype(np.float32)/255
        near = np.repeat(np.repeat(src, S, 0), S, 1)
        H, W = near.shape[:2]
        # shape per cell: crisp for surfaces, xBR for the rest
        shape = xb.copy()
        clsmap = np.empty((24, 32), object)
        for y in range(24):
            for x in range(32):
                k = keys[r][y][x]
                c = tile_class(k) if k != '-' else 'surface'
                clsmap[y, x] = c
                if c == 'surface':
                    shape[y*32:(y+1)*32, x*32:(x+1)*32] = near[y*32:(y+1)*32, x*32:(x+1)*32]
        # snap to the original colours (clean regions), then colour mapping
        pal = np.unique(src.reshape(-1, 3), axis=0)
        lab = ((shape[:, :, None, :]-pal[None, None])**2).sum(3).argmin(2)
        base_l = luma(pal)[lab]
        col = np.zeros((H, W, 3), np.float32)
        rich = richer(pal)[lab]
        for y in range(24):
            for x in range(32):
                ys, xs = slice(y*32, (y+1)*32), slice(x*32, (x+1)*32)
                c = clsmap[y, x]
                if c in RAMPS:
                    col[ys, xs] = ramp_colour(RAMPS[c], base_l[ys, xs])
                else:
                    col[ys, xs] = rich[ys, xs]
                k = keys[r][y][x]
                seed = zlib.crc32(k.encode())
                t = texture(TEXTURE.get(c, 'speckle'), seed, 32)
                amount = 0.07 if c in RAMPS else 0.05
                col[ys, xs] *= (1 + t*amount)[..., None]
        # soften the colour edges a little (anti aliasing)
        col = nd.gaussian_filter(col, (0.6, 0.6, 0))
        # relief
        h = nd.gaussian_filter(base_l, 1.4)
        gy, gx = np.gradient(h)
        shade = np.clip(-(gx*-0.6 + gy*-0.8)*10, -0.35, 0.35)
        ao = np.clip((nd.gaussian_filter(base_l, 4)-h)*0.8, 0, 0.25)
        out = col*(1+shade[..., None]*0.8)*(1-ao[..., None])
        out += np.clip(shade-0.18, 0, None)[..., None]*0.25
        # keep the brightness of the original
        gain = luma(near).mean()/max(luma(out).mean(), 1e-4)
        out = np.clip(out*gain, 0, 1)
        img = (out*255+0.5).astype(np.uint8)
        for y in range(24):
            for x in range(32):
                k = keys[r][y][x]
                path = '%s/tiles/%s.png' % (hd, k)
                if k == '-' or k in done or k not in need:
                    continue
                Image.fromarray(img[y*32:(y+1)*32, x*32:(x+1)*32]).save(path)
                done.add(k)
                written += 1
    if os.path.exists(tmp):
        os.remove(tmp)
    print('%d HD tiles written' % written)

    # previews: rooms assembled from the HD tiles, as the game will draw them
    for r in preview:
        img = Image.new('RGB', (256*S, 192*S))
        for y in range(24):
            for x in range(32):
                k = keys[r][y][x]
                if k != '-':
                    img.paste(Image.open('%s/tiles/%s.png' % (hd, k)), (x*32, y*32))
        img.save('%s/preview_room_%03d.png' % (hd, r))
        o = np.asarray(Image.open('%s/rooms/room_%03d.png' % (exp, r)).convert('RGB')).astype(np.float32)/255
        n = np.asarray(img).astype(np.float32)/255
        print('room %d: brightness original %.3f, HD %.3f' % (r, luma(o).mean(), luma(n).mean()))


main()
