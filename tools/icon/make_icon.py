#!/usr/bin/env python3
# Copyright 2026, Akakor. Distributed under the terms of the MIT License.
"""Builds the Metal Gear application icon (an original design: an olive
drab steel gear with a green radar screen and an enemy blip).

Writes the HVIF (Haiku Vector Icon Format) icon, an .rdef for the
application resources and, by decoding the HVIF again and drawing it the
way Haiku's icon renderer does, PNG images in several sizes.

    make_icon.py data/icon src/MetalGear.rdef
"""
import math
import struct
import sys

# --- model ---------------------------------------------------------------

K = 0.5522847498


class Path:
	def __init__(self, points, closed=True, curves=False):
		# points: (x, y) or (x, y, inx, iny, outx, outy)
		self.points = points
		self.closed = closed
		self.curves = curves


def polygon(points):
	return Path(points)


def circle(cx, cy, rx, ry=None):
	ry = rx if ry is None else ry
	pts = []
	for (dx, dy, tx, ty) in ((1, 0, 0, 1), (0, 1, -1, 0), (-1, 0, 0, -1), (0, -1, 1, 0)):
		x, y = cx + dx * rx, cy + dy * ry
		hx, hy = tx * rx * K, ty * ry * K
		pts.append((x, y, x - hx, y - hy, x + hx, y + hy))
	return Path(pts, curves=True)


def rect(x0, y0, x1, y1):
	return polygon([(x0, y0), (x1, y0), (x1, y1), (x0, y1)])


def gear(cx, cy, outer, root, teeth, phase=0.0, dx=0.0, dy=0.0):
	pts = []
	step = 2 * math.pi / teeth
	for i in range(teeth):
		a = phase + i * step
		# root, flank up, tip, tip, flank down
		for (frac, r) in ((0.00, root), (0.14, outer), (0.36, outer), (0.50, root)):
			t = a + frac * step
			pts.append((cx + dx + r * math.cos(t), cy + dy + r * math.sin(t)))
	# remove duplicate root points (end of a tooth = start of the next gap)
	return polygon(pts)


def solid(r, g, b, a=255):
	return ("solid", (r, g, b, a))


def linear(p0, p1, stops):
	return ("linear", p0, p1, stops)


def radial(c, radius, stops):
	return ("radial", c, radius, stops)


# --- the design (64 x 64 units) -------------------------------------------

CX, CY = 31.0, 30.0
OUTER, ROOT, TEETH, PHASE = 29.0, 23.5, 10, -math.pi / 2 - 0.07 * 2 * math.pi / 10 * 0
PHASE = -math.pi / 2 - 0.25 * 2 * math.pi / TEETH  # a tooth at the top

styles = [
	solid(0, 0, 0, 70),                                           # 0 shadow
	solid(30, 36, 16),                                            # 1 outline
	linear((CX, CY - 20), (CX, CY + 33), [(0, (74, 86, 38)), (255, (34, 41, 18))]),  # 2 side
	linear((8, 4), (54, 58), [(0, (176, 186, 112)), (110, (122, 136, 66)),
		(255, (70, 82, 36))]),                                    # 3 face
	linear((14, 12), (46, 48), [(0, (52, 60, 28)), (255, (150, 162, 92))]),  # 4 hub bevel
	radial((CX + 2, CY + 2), 14, [(0, (40, 150, 70)), (150, (14, 70, 32)),
		(255, (6, 30, 14))]),                                     # 5 screen
	solid(120, 255, 150, 90),                                     # 6 grid lines
	linear((CX, CY), (CX + 11, CY - 9), [(0, (120, 255, 150, 0)),
		(255, (150, 255, 170, 170))]),                            # 7 sweep
	radial((CX + 4.3, CY - 4.6), 3.0, [(0, (255, 200, 170)), (90, (255, 64, 40)),
		(255, (170, 20, 10))]),                                   # 8 blip
	linear((CX - 8, CY - 10), (CX + 2, CY + 2), [(0, (255, 255, 255, 110)),
		(255, (255, 255, 255, 0))]),                              # 9 glass
	solid(255, 255, 240, 120),                                    # 10 rim light
]

paths = []
shapes = []


def add(style, *ps):
	idx = []
	for p in ps:
		paths.append(p)
		idx.append(len(paths) - 1)
	shapes.append((style, idx))


add(0, gear(CX, CY, OUTER, ROOT, TEETH, PHASE, 2.0, 4.0))            # shadow
add(1, gear(CX, CY, OUTER + 0.8, ROOT + 0.8, TEETH, PHASE, 0, 3.0))  # side outline
add(2, gear(CX, CY, OUTER, ROOT, TEETH, PHASE, 0, 3.0))              # side
add(1, gear(CX, CY, OUTER + 0.8, ROOT + 0.8, TEETH, PHASE))          # face outline
add(3, gear(CX, CY, OUTER, ROOT, TEETH, PHASE))                      # face
# light on the upper left edges of the teeth
add(10, gear(CX, CY, OUTER, ROOT, TEETH, PHASE, -0.6, -0.6))
add(3, gear(CX, CY, OUTER - 0.4, ROOT - 0.4, TEETH, PHASE, 0.3, 0.3))
add(1, circle(CX, CY, 16.2))                                         # hub
add(4, circle(CX, CY, 15.2))
add(1, circle(CX, CY, 12.6))                                         # screen
add(5, circle(CX, CY, 11.8))
add(6, rect(CX - 11.0, CY - 0.35, CX + 11.0, CY + 0.35),
	rect(CX - 0.35, CY - 11.0, CX + 0.35, CY + 11.0))                # cross
ring = circle(CX, CY, 6.6)
ring_in = circle(CX, CY, 6.0)
ring_in.points = list(reversed([(p[0], p[1], p[4], p[5], p[2], p[3]) for p in ring_in.points]))
add(6, ring, ring_in)                                                # range ring
a0, a1 = math.radians(-80), math.radians(-25)
wedge = [(CX, CY)]
for i in range(7):
	t = a0 + (a1 - a0) * i / 6
	wedge.append((CX + 11.6 * math.cos(t), CY + 11.6 * math.sin(t)))
add(7, polygon(wedge))                                               # sweep
add(1, circle(CX + 4.3, CY - 4.6, 2.6))                              # blip
add(8, circle(CX + 4.3, CY - 4.6, 2.0))
add(9, circle(CX - 3.5, CY - 4.5, 6.5, 4.2))                         # glass


# --- HVIF -------------------------------------------------------------------

def coord(v):
	if abs(v * 100 - round(v) * 100) < 1e-6 and -32 <= v <= 95 and v == int(v):
		return bytes([int(v + 32)])
	v = max(-128.0, min(192.0, v))
	val = int((v + 128.0) * 102.0) | 0x8000
	return bytes([val >> 8, val & 0xff])


def float24(f):
	if f == 0:
		return b"\0\0\0"
	bits = struct.unpack("<I", struct.pack("<f", f))[0]
	sign = bits >> 31
	exp = ((bits >> 23) & 0xff) - 127
	man = bits & 0x7fffff
	if exp >= 32 or exp < -32:
		return b"\0\0\0"
	v = (sign << 23) | ((exp + 32) << 17) | (man >> 6)
	return bytes([v >> 16, (v >> 8) & 0xff, v & 0xff])


def gradient_matrix(style):
	if style[0] == "linear":
		(x0, y0), (x1, y1) = style[1], style[2]
		dx, dy = x1 - x0, y1 - y0
		s = math.hypot(dx, dy) / 128.0
		c, sn = dx / math.hypot(dx, dy), dy / math.hypot(dx, dy)
		return [s * c, s * sn, -s * sn, s * c, (x0 + x1) / 2, (y0 + y1) / 2]
	(cx, cy), r = style[1], style[2]
	s = r / 64.0
	return [s, 0, 0, s, cx, cy]


def encode():
	out = bytearray(b"ncif")
	out.append(len(styles))
	for st in styles:
		if st[0] == "solid":
			r, g, b, a = st[1]
			if a == 255:
				out += bytes([3, r, g, b])
			else:
				out += bytes([1, r, g, b, a])
			continue
		stops = st[3]
		alpha = any(len(c) == 4 and c[3] != 255 for _, c in stops)
		flags = 0x02 | (0 if alpha else 0x04)
		out += bytes([2, 0 if st[0] == "linear" else 1, flags, len(stops)])
		for m in gradient_matrix(st):
			out += float24(m)
		for off, c in stops:
			out.append(off)
			out += bytes(c[:3])
			if alpha:
				out.append(c[3] if len(c) == 4 else 255)
	out.append(len(paths))
	for p in paths:
		flags = 0x02 if p.closed else 0
		if not p.curves:
			flags |= 0x08
		out += bytes([flags, len(p.points)])
		for pt in p.points:
			for v in pt:
				out += coord(v)
	out.append(len(shapes))
	for style, idx in shapes:
		out += bytes([10, style, len(idx)]) + bytes(idx) + bytes([0])
	return bytes(out)


# --- decoder and renderer (as Haiku's FlatIconImporter and IconRenderer) ---

def decode(data):
	pos = 0

	def u8():
		nonlocal pos
		pos += 1
		return data[pos - 1]

	def rcoord():
		v = u8()
		if v & 0x80:
			return (((v & 0x7f) << 8) | u8()) / 102.0 - 128.0
		return v - 32.0

	def rf24():
		v = (u8() << 16) | (u8() << 8) | u8()
		if v == 0:
			return 0.0
		sign = v >> 23
		exp = ((v >> 17) & 0x3f) - 32
		man = (v & 0x1ffff) << 6
		return struct.unpack("<f", struct.pack("<I", (sign << 31) | ((exp + 127) << 23) | man))[0]

	assert data[:4] == b"ncif"
	pos = 4
	sts = []
	for _ in range(u8()):
		t = u8()
		if t == 1:
			sts.append(("solid", (u8(), u8(), u8(), u8())))
		elif t == 3:
			sts.append(("solid", (u8(), u8(), u8(), 255)))
		elif t == 2:
			gtype, flags, n = u8(), u8(), u8()
			m = [rf24() for _ in range(6)] if flags & 2 else [1, 0, 0, 1, 0, 0]
			alpha = not flags & 4
			stops = []
			for _ in range(n):
				off = u8()
				c = (u8(), u8(), u8(), u8() if alpha else 255)
				stops.append((off / 255.0, c))
			sts.append(("grad", gtype, m, stops))
		else:
			raise ValueError("style type %d" % t)
	pts = []
	for _ in range(u8()):
		flags, n = u8(), u8()
		ps = []
		for _ in range(n):
			if flags & 8:
				x, y = rcoord(), rcoord()
				ps.append((x, y, x, y, x, y))
			else:
				ps.append(tuple(rcoord() for _ in range(6)))
		pts.append((ps, bool(flags & 2)))
	shs = []
	for _ in range(u8()):
		assert u8() == 10
		st, n = u8(), u8()
		idx = [u8() for _ in range(n)]
		assert u8() == 0
		shs.append((st, idx))
	assert pos == len(data), (pos, len(data))
	return sts, pts, shs


def render(data, size, path):
	import cairocffi as cairo
	sts, pts, shs = decode(data)
	surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, size, size)
	ctx = cairo.Context(surface)
	ctx.scale(size / 64.0, size / 64.0)
	ctx.set_fill_rule(cairo.FILL_RULE_WINDING)
	for st, idx in shs:
		ctx.new_path()
		for i in idx:
			ps, closed = pts[i]
			ctx.move_to(ps[0][0], ps[0][1])
			n = len(ps)
			for k in range(1, n + (1 if closed else 0)):
				a, b = ps[k - 1], ps[k % n]
				ctx.curve_to(a[4], a[5], b[2], b[3], b[0], b[1])
			if closed:
				ctx.close_path()
		s = sts[st]
		if s[0] == "solid":
			r, g, b, a = s[1]
			ctx.set_source_rgba(r / 255, g / 255, b / 255, a / 255)
		else:
			gtype, m, stops = s[1], s[2], s[3]
			if gtype == 0:
				pat = cairo.LinearGradient(-64, 0, 64, 0)
			else:
				pat = cairo.RadialGradient(0, 0, 0, 0, 0, 64)
			for off, (r, g, b, a) in stops:
				pat.add_color_stop_rgba(off, r / 255, g / 255, b / 255, a / 255)
			mat = cairo.Matrix(m[0], m[1], m[2], m[3], m[4], m[5])
			mat.invert()
			pat.set_matrix(mat)
			pat.set_extend(cairo.EXTEND_PAD)
			ctx.set_source(pat)
		ctx.fill()
	surface.write_to_png(path)


def rdef(data):
	hexdata = data.hex().upper()
	lines = ['\t$"%s"' % hexdata[i:i + 64] for i in range(0, len(hexdata), 64)]
	return """/*
 * Metal Gear for Haiku: application resources (generated by
 * tools/icon/make_icon.py).
 */

resource app_signature "application/x-vnd.Akakor-MetalGear";

resource app_flags B_SINGLE_LAUNCH;

resource app_version {
	major = 0,
	middle = 1,
	minor = 0,
	variety = B_APPV_DEVELOPMENT,
	internal = 0,
	short_info = "Metal Gear",
	long_info = "Metal Gear (MSX2, 1987) rewritten natively for Haiku"
};

resource vector_icon array {
%s
};
""" % "\n".join(lines)


def main():
	outdir = sys.argv[1] if len(sys.argv) > 1 else "."
	data = encode()
	decode(data)
	with open(outdir + "/MetalGear.hvif", "wb") as f:
		f.write(data)
	with open(sys.argv[2] if len(sys.argv) > 2 else outdir + "/MetalGear.rdef", "w") as f:
		f.write(rdef(data))
	for size in (16, 32, 64, 256, 512):
		render(data, size, "%s/MetalGear-%d.png" % (outdir, size))
	print("%d bytes, %d styles, %d paths, %d shapes" % (len(data), len(styles),
		len(paths), len(shapes)))


if __name__ == "__main__":
	main()
