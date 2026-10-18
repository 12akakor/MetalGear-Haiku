#!/usr/bin/env python3
"""
Makes a small SoundFont with only the presets the game uses.

  sf2_subset.py <in.sf2> <out.sf2> <bank:program,...> [-drumkeys k,k,...]

Presets are given as bank:program (bank 128 is the drum kit). With
-drumkeys only the drum kit zones that cover these keys are kept.
Samples that are not used are left out; linked stereo samples are kept
together.
"""
import struct, sys

def chunks(data, off, end):
    while off < end:
        cid = data[off:off+4]; size = struct.unpack_from('<I', data, off+4)[0]
        yield cid, off+8, size
        off += 8 + size + (size & 1)

def parse(path):
    d = open(path, 'rb').read()
    assert d[:4] == b'RIFF' and d[8:12] == b'sfbk'
    out = {}
    for cid, off, size in chunks(d, 12, len(d)):
        if cid == b'LIST':
            kind = d[off:off+4]
            out[kind] = {}
            for c2, o2, s2 in chunks(d, off+4, off+size):
                out[kind][c2] = d[o2:o2+s2]
    return out

def records(raw, fmt):
    n = struct.calcsize(fmt)
    return [list(struct.unpack_from(fmt, raw, i)) for i in range(0, len(raw), n)]

PHDR = '<20sHHHIII'; BAG = '<HH'; MOD = '<HHhHH'; GEN = '<HH'; INST = '<20sH'
SHDR = '<20sIIIIIBbHH'

def main():
    src, dst, want = sys.argv[1], sys.argv[2], sys.argv[3]
    wanted = set(tuple(int(x) for x in p.split(':')) for p in want.split(','))
    drumkeys = None
    if '-drumkeys' in sys.argv:
        drumkeys = set(int(k) for k in sys.argv[sys.argv.index('-drumkeys')+1].split(','))
    sf = parse(src)
    pd = sf[b'pdta']
    phdr = records(pd[b'phdr'], PHDR); pbag = records(pd[b'pbag'], BAG)
    pmod = records(pd[b'pmod'], MOD); pgen = records(pd[b'pgen'], GEN)
    inst = records(pd[b'inst'], INST); ibag = records(pd[b'ibag'], BAG)
    imod = records(pd[b'imod'], MOD); igen = records(pd[b'igen'], GEN)
    shdr = records(pd[b'shdr'], SHDR)
    smpl = sf[b'sdta'][b'smpl']

    def zones(bags, gens, mods, first, last):
        z = []
        for b in range(first, last):
            g0, m0 = bags[b]; g1, m1 = bags[b+1]
            z.append((gens[g0:g1], mods[m0:m1]))
        return z

    # presets
    new_presets = []
    used_inst = []
    for i, p in enumerate(phdr[:-1]):
        if (p[2], p[1]) not in wanted:
            continue
        zs = zones(pbag, pgen, pmod, p[3], phdr[i+1][3])
        for g, m in zs:
            for gen in g:
                if gen[0] == 41 and gen[1] not in used_inst:
                    used_inst.append(gen[1])
        new_presets.append((p, zs))
    found = set((p[2], p[1]) for p, _ in new_presets)
    for w in sorted(wanted - found):
        print('warning: preset %d:%d not in the SoundFont' % w)

    # instruments (drum kit zones filtered by key)
    drum_inst = set()
    for p, zs in new_presets:
        if p[2] == 128:
            for g, m in zs:
                for gen in g:
                    if gen[0] == 41:
                        drum_inst.add(gen[1])
    new_inst = []
    used_smp = []
    for idx in used_inst:
        zs = zones(ibag, igen, imod, inst[idx][1], inst[idx+1][1])
        keep = []
        for g, m in zs:
            sample = [gen[1] for gen in g if gen[0] == 53]
            if drumkeys is not None and idx in drum_inst and sample:
                kr = [gen[1] for gen in g if gen[0] == 43]
                if kr:
                    lo, hi = kr[0] & 0xff, kr[0] >> 8
                    if not any(lo <= k <= hi for k in drumkeys):
                        continue
            keep.append((g, m))
            for s in sample:
                for t in (s, shdr[s][8]):
                    if t < len(shdr) - 1 and t not in used_smp and (t == s or shdr[s][9] & 0x6):
                        used_smp.append(t)
        new_inst.append((inst[idx], keep))

    # samples
    smap = {}
    new_shdr = []
    data = bytearray()
    for s in used_smp:
        h = list(shdr[s])
        start, end = h[1], h[2]
        base = len(data) // 2
        data += smpl[start*2:end*2] + bytes(46*2)
        h[3] = h[3] - start + base; h[4] = h[4] - start + base
        h[1] = base; h[2] = base + (end - start)
        smap[s] = len(new_shdr)
        new_shdr.append(h)
    for h in new_shdr:
        if h[8] in smap:
            h[8] = smap[h[8]]
        else:
            h[8] = 0; h[9] = h[9] & 1 if h[9] & 1 else 1   # mono
    new_shdr.append([b'EOS'.ljust(20, b'\0'), 0, 0, 0, 0, 0, 0, 0, 0, 0])
    imap = {idx: n for n, idx in enumerate(used_inst)}

    def build(items, remap_gen, remap_to):
        hdr, bags, gens, mods = [], [], [], []
        for h, zs in items:
            hdr.append((h, len(bags)))
            for g, m in zs:
                bags.append((len(gens), len(mods)))
                for gen in g:
                    gen = list(gen)
                    if gen[0] == remap_gen:
                        gen[1] = remap_to[gen[1]]
                    gens.append(gen)
                mods.extend(m)
        bags.append((len(gens), len(mods)))
        gens.append([0, 0]); mods.append([0, 0, 0, 0, 0])
        return hdr, bags, gens, mods

    ph, pb, pg, pm = build(new_presets, 41, imap)
    ih, ib, ig, im = build(new_inst, 53, smap)
    pack = lambda fmt, rows: b''.join(struct.pack(fmt, *r) for r in rows)
    phdr_rows = [[h[0], h[1], h[2], bag, h[4], h[5], h[6]] for h, bag in ph]
    phdr_rows.append([b'EOP'.ljust(20, b'\0'), 0, 0, len(pb) - 1, 0, 0, 0])
    inst_rows = [[h[0], bag] for h, bag in ih]
    inst_rows.append([b'EOI'.ljust(20, b'\0'), len(ib) - 1])

    def chunk(cid, body):
        return cid + struct.pack('<I', len(body)) + body + (b'\0' if len(body) & 1 else b'')
    def lst(kind, body):
        return chunk(b'LIST', kind + body)
    info = b''.join(chunk(k, v) for k, v in sf[b'INFO'].items())
    pdta = b''.join([chunk(b'phdr', pack(PHDR, phdr_rows)), chunk(b'pbag', pack(BAG, pb)),
                     chunk(b'pmod', pack(MOD, pm)), chunk(b'pgen', pack(GEN, pg)),
                     chunk(b'inst', pack(INST, inst_rows)), chunk(b'ibag', pack(BAG, ib)),
                     chunk(b'imod', pack(MOD, im)), chunk(b'igen', pack(GEN, ig)),
                     chunk(b'shdr', pack(SHDR, new_shdr))])
    body = b'sfbk' + lst(b'INFO', info) + lst(b'sdta', chunk(b'smpl', bytes(data))) + lst(b'pdta', pdta)
    open(dst, 'wb').write(b'RIFF' + struct.pack('<I', len(body)) + body)
    print('%d presets, %d instruments, %d samples, %.1f MB' % (len(new_presets), len(new_inst),
          len(new_shdr) - 1, (len(body) + 8) / 1e6))

main()
