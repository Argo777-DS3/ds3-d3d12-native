# Pure-Python BC1/BC3/BC7 decoder (stdlib only).
import struct, zlib

# 2-subset: 16-bit masks, bit i set => pixel i in subset 1
P2MASK = [0xCCCC,0x8888,0xEEEE,0xECC8,0xC880,0xFEEC,0xFEC8,0xEC80,0xC800,0xFFEC,0xFE80,0xE800,0xFFE8,0xFF00,0xFFF0,0xF000,
 0xF710,0x008E,0x7100,0x08CE,0x008C,0x7310,0x3100,0x8CCE,0x088C,0x3110,0x6666,0x366C,0x17E8,0x0FF0,0x718E,0x399C,
 0xaaaa,0xf0f0,0x5a5a,0x33cc,0x3c3c,0x55aa,0x9696,0xa55a,0x73ce,0x13c8,0x324c,0x3bdc,0x6996,0xc33c,0x9966,0x0660,
 0x0272,0x04e4,0x4e40,0x2720,0xc936,0x936c,0x39c6,0x639c,0x9336,0x9cc6,0x817e,0xe718,0xccf0,0x0fcc,0x7744,0xee22]
ANCH2 = [15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,2,8,2,2,8,8,15,2,8,2,2,8,8,2,2,
 15,15,6,8,2,8,15,15,2,8,2,2,2,15,15,6,6,2,6,8,15,15,2,2,15,15,15,15,15,2,2,15]
P2 = [[(m >> i) & 1 for i in range(16)] for m in P2MASK]
# 3-subset: only partitions 0..15 (all that mode 0 can address); mode 2 needs 0..63 (not available here)
P3_ROWS = [
 "0011001102212222","0001001122112221","0000200122112211","0222002200110111",
 "0000000011221122","0011001100220022","0022002211111111","0011001122112211",
 "0000000011112222","0000111111112222","0000111122222222","0012001200120012",
 "0112011201120112","0122012201220122","0011011211221222","0011200122002220"]
P3 = [[int(c) for c in r] for r in P3_ROWS]
ANCH3_1 = [3,3,15,15,8,3,15,15,8,8,6,6,6,5,3,3]
ANCH3_2 = [15,8,8,3,15,15,3,8,15,15,15,15,15,15,15,8]

def _selfcheck_tables():
    assert len(P2) == 64 and len(ANCH2) == 64
    for p in range(64):
        assert P2[p][0] == 0, p
        assert P2[p][ANCH2[p]] == 1, ("anchor2", p)
        assert sum(P2[p]) not in (0, 16)
    for p in range(16):
        assert P3[p][0] == 0
        assert P3[p][ANCH3_1[p]] == 1 and P3[p][ANCH3_2[p]] == 2, ("anchor3", p)
        assert set(P3[p]) == {0, 1, 2}
_selfcheck_tables()

W2 = [0,21,43,64]; W3 = [0,9,18,27,37,46,55,64]
W4 = [0,4,9,13,17,21,26,30,34,38,43,47,51,55,60,64]
WT = {2: W2, 3: W3, 4: W4}
# mode: (subsets, partbits, rotbits, idxselbit, colbits, alphabits, pbit_kind, idxbits, idx2bits)
# pbit_kind: 0 none, 1 per endpoint, 2 shared per subset
MODES = {0:(3,4,0,0,4,0,1,3,0), 1:(2,6,0,0,6,0,2,3,0), 2:(3,6,0,0,5,0,0,2,0), 3:(2,6,0,0,7,0,1,2,0),
         4:(1,0,2,1,5,6,0,2,3), 5:(1,0,2,0,7,8,0,2,2), 6:(1,0,0,0,7,7,1,4,0), 7:(2,6,0,0,5,5,1,2,0)}

class Stats:
    def __init__(self):
        self.undec_mode2 = 0
        self.reserved = 0
        self.modes = [0]*8

def _expand(v, n):
    if n >= 8:
        return v
    return (v << (8 - n)) | (v >> (2 * n - 8))

def decode_bc7_block(blk, stats=None):
    v = int.from_bytes(blk, 'little')
    mode = 0
    while mode < 8 and not (v >> mode) & 1:
        mode += 1
    if mode == 8:
        if stats is not None: stats.reserved += 1
        return [(0, 0, 0, 0)] * 16
    if stats is not None: stats.modes[mode] += 1
    ns, pb, rb, isb, cb, ab, pk, ib, i2b = MODES[mode]
    pos = mode + 1
    def rd(n):
        nonlocal pos
        r = (v >> pos) & ((1 << n) - 1)
        pos += n
        return r
    part = rd(pb)
    rot = rd(rb)
    idxsel = rd(isb)
    ne = ns * 2
    cols = [[0] * 4 for _ in range(ne)]
    for ch in range(3):
        for e in range(ne):
            cols[e][ch] = rd(cb)
    if ab:
        for e in range(ne):
            cols[e][3] = rd(ab)
    pbits = None
    if pk == 1:
        pbits = [rd(1) for _ in range(ne)]
    elif pk == 2:
        sp = [rd(1) for _ in range(ns)]
        pbits = [sp[e >> 1] for e in range(ne)]
    ep = []
    for e in range(ne):
        c = cols[e]
        out = []
        for ch in range(3):
            if pbits is not None:
                out.append(_expand((c[ch] << 1) | pbits[e], cb + 1))
            else:
                out.append(_expand(c[ch], cb))
        if ab:
            if pbits is not None and mode in (6, 7):
                out.append(_expand((c[3] << 1) | pbits[e], ab + 1))
            else:
                out.append(_expand(c[3], ab))
        else:
            out.append(255)
        ep.append(out)
    if ns == 1:
        smap = [0] * 16; anchors = (0,)
    elif ns == 2:
        smap = P2[part]; anchors = (0, ANCH2[part])
    else:
        if part >= 16:
            if stats is not None: stats.undec_mode2 += 1
            return [(255, 0, 255, 255)] * 16
        smap = P3[part]; anchors = (0, ANCH3_1[part], ANCH3_2[part])
    def read_idx(nb):
        res = []
        for i in range(16):
            n = nb - 1 if i in anchors else nb
            res.append(rd(n))
        return res
    idx1 = read_idx(ib)
    idx2 = read_idx(i2b) if i2b else None
    w1 = WT[ib]
    w2 = WT[i2b] if i2b else None
    px = []
    for i in range(16):
        s = smap[i]
        e0 = ep[2 * s]; e1 = ep[2 * s + 1]
        if i2b:
            if idxsel == 0:
                wc = w1[idx1[i]]; wa = w2[idx2[i]]
            else:
                wc = w2[idx2[i]]; wa = w1[idx1[i]]
        else:
            wc = wa = w1[idx1[i]]
        r = (e0[0] * (64 - wc) + e1[0] * wc + 32) >> 6
        g = (e0[1] * (64 - wc) + e1[1] * wc + 32) >> 6
        b = (e0[2] * (64 - wc) + e1[2] * wc + 32) >> 6
        a = (e0[3] * (64 - wa) + e1[3] * wa + 32) >> 6
        if rot == 1: r, a = a, r
        elif rot == 2: g, a = a, g
        elif rot == 3: b, a = a, b
        px.append((r, g, b, a))
    return px

def _c565(c):
    r = (c >> 11) & 31; g = (c >> 5) & 63; b = c & 31
    return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))

def decode_bc1_color(blk8, allow_alpha=True):
    c0, c1, bits = struct.unpack('<HHI', blk8)
    p0 = _c565(c0); p1 = _c565(c1)
    if c0 > c1 or not allow_alpha:
        p2 = tuple((2 * p0[i] + p1[i]) // 3 for i in range(3))
        p3 = tuple((p0[i] + 2 * p1[i]) // 3 for i in range(3))
        pal = [p0 + (255,), p1 + (255,), p2 + (255,), p3 + (255,)]
    else:
        p2 = tuple((p0[i] + p1[i]) // 2 for i in range(3))
        pal = [p0 + (255,), p1 + (255,), p2 + (255,), (0, 0, 0, 0)]
    return [pal[(bits >> (2 * i)) & 3] for i in range(16)]

def decode_bc3_block(blk):
    a0, a1 = blk[0], blk[1]
    bits = int.from_bytes(blk[2:8], 'little')
    if a0 > a1:
        al = [a0, a1] + [((7 - k) * a0 + k * a1) // 7 for k in range(1, 7)]
    else:
        al = [a0, a1] + [((5 - k) * a0 + k * a1) // 5 for k in range(1, 5)] + [0, 255]
    px = decode_bc1_color(blk[8:16], allow_alpha=False)
    return [(p[0], p[1], p[2], al[(bits >> (3 * i)) & 7]) for i, p in enumerate(px)]

def decode_image(data, w, h, fmt):
    out = bytearray(w * h * 4)
    bw, bh = (w + 3) // 4, (h + 3) // 4
    bs = 8 if fmt == 'bc1' else 16
    stats = Stats()
    for by in range(bh):
        for bx in range(bw):
            off = (by * bw + bx) * bs
            blk = data[off:off + bs]
            if fmt == 'bc7': px = decode_bc7_block(blk, stats)
            elif fmt == 'bc3': px = decode_bc3_block(blk)
            else: px = decode_bc1_color(blk)
            for r in range(4):
                y = by * 4 + r
                if y >= h: break
                row = b''.join(bytes(px[r * 4 + c]) for c in range(4))
                o = (y * w + bx * 4) * 4
                n = min(4, w - bx * 4)
                out[o:o + n * 4] = row[:n * 4]
    return out, stats

def write_png(path, w, h, rgba):
    raw = bytearray()
    stride = w * 4
    for y in range(h):
        raw.append(0)
        raw += rgba[y * stride:(y + 1) * stride]
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(bytes(raw), 6)) + chunk(b'IEND', b''))
    open(path, 'wb').write(png)

def half(rgba, w, h):
    hw, hh = w // 2, h // 2
    out = bytearray(hw * hh * 4)
    stride = w * 4
    for y in range(hh):
        r0 = rgba[(2 * y) * stride:(2 * y + 1) * stride]
        r1 = rgba[(2 * y + 1) * stride:(2 * y + 2) * stride]
        o = y * hw * 4
        for c in range(4):
            a = r0[c::8]; b = r0[c + 4::8]; d = r1[c::8]; e = r1[c + 4::8]
            out[o + c:o + hw * 4:4] = bytes((p + q + r + s + 2) >> 2 for p, q, r, s in zip(a, b, d, e))
    return out, hw, hh

def stats_rgba(rgba):
    n = len(rgba) // 4
    sums = [sum(rgba[c::4]) for c in range(4)]
    tr = rgba[3::4].count(0)
    return [s / n for s in sums], tr / n
