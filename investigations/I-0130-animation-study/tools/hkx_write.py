"""I-0130: write an hkaSplineCompressedAnimation's curves back into a DS3 packfile, without the Havok SDK.

The encoder needs no curve fitting: every animated channel becomes a degree-1 B-spline with one control point a frame
(the clamped knots 0, 0, 1, ..., n, n), which DS3's own files already use (26 212 of the 39 785 splines read by
hkx_census.py are degree 1). Positions and scales are quantised on 16 bits between their minimum and maximum, the
rotations as THREECOMP40 (the only rotation quantisation DS3's files use). The new data array is spliced into the
original packfile: the fixups and the section offsets after it are shifted, the block offsets rewritten.

Usage:
  python -I hkx_write.py <in.hkx> <out.hkx> [--edit EDIT]
EDIT (a demonstration of a retouch, optional): 'bone:T:deg' adds a rotation of deg degrees about Y to transform track T on every frame.
After writing, the output is decoded again with hkx_read and compared with the input (or with the edited target).
"""
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hkx_read as H  # noqa: E402

SQRT2 = math.sqrt(2.0)


def quat_mul(a, b):
    ax, ay, az, aw = a; bx, by, bz, bw = b
    return [aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz]


def quat_norm(q):
    n = math.sqrt(sum(c * c for c in q)) or 1.0
    return [c / n for c in q]


def slerp(a, b, t):
    d = sum(x * y for x, y in zip(a, b))
    if d < 0:
        b = [-x for x in b]; d = -d
    if d > 0.9995:
        return quat_norm([x + (y - x) * t for x, y in zip(a, b)])
    th = math.acos(d); s = math.sin(th)
    return [(math.sin((1 - t) * th) * x + math.sin(t * th) * y) / s for x, y in zip(a, b)]


def enc_threecomp40(q):
    q = quat_norm(q)
    i = max(range(4), key=lambda k: abs(q[k]))
    neg = 1 if q[i] < 0 else 0
    rest = [q[k] for k in range(4) if k != i]
    vals = [min(4095, max(0, int(round(c * SQRT2 * 2047)) + 2047)) for c in rest]
    v = vals[0] | (vals[1] << 12) | (vals[2] << 24) | (i << 36) | (neg << 38)
    return v.to_bytes(5, 'little')


class W:
    def __init__(self):
        self.b = bytearray()

    def pad(self, a):
        while len(self.b) % a:
            self.b.append(0)

    def u8(self, v): self.b.append(v)
    def u16(self, v): self.b += struct.pack('<H', v)
    def f32(self, v): self.b += struct.pack('<f', v)


def knots_deg1(n):          # n control points -> nitems = n - 1, knots = nitems + degree + 2 = n + 2 bytes
    return [0] + list(range(n)) + [n - 1]


def enc_vec(w, vals, identity, flags_out):
    """vals: one [x, y, z] a frame. Returns the flags byte; writes the track."""
    n = len(vals); flags = 0
    per = [[v[k] for v in vals] for k in range(3)]
    anim = [max(c) - min(c) > 1e-6 for c in per]
    if any(anim):
        for k in range(3):
            flags |= (0x10 << k) if anim[k] else (0x01 << k)
        w.u16(n - 1); w.u8(1)
        for kn in knots_deg1(n):
            w.u8(kn)
        w.pad(4)
        for k in range(3):
            if anim[k]:
                w.f32(min(per[k])); w.f32(max(per[k]))
            else:
                w.f32(per[k][0])
        for f in range(n):
            for k in range(3):
                if anim[k]:
                    lo, hi = min(per[k]), max(per[k])
                    w.u16(int(round((per[k][f] - lo) / (hi - lo) * 65535)))
        w.pad(4)
    else:
        for k in range(3):
            if abs(per[k][0] - identity[k]) > 1e-7:
                flags |= 0x01 << k
                w.f32(per[k][0])
        w.pad(4)
    flags_out.append(flags)


def enc_rot(w, quats, flags_out):
    q = [quat_norm(x) for x in quats]
    for i in range(1, len(q)):                 # keep the hemisphere continuous for the interpolation
        if sum(a * b for a, b in zip(q[i - 1], q[i])) < 0:
            q[i] = [-c for c in q[i]]
    same = all(max(abs(a - b) for a, b in zip(q[0], x)) < 1e-6 for x in q)
    if same:
        if max(abs(a - b) for a, b in zip(q[0], [0, 0, 0, 1])) < 1e-6:
            flags_out.append(0); return
        w.b += enc_threecomp40(q[0]); w.pad(4); flags_out.append(0x0F); return
    w.u16(len(q) - 1); w.u8(1)
    for kn in knots_deg1(len(q)):
        w.u8(kn)
    for x in q:
        w.b += enc_threecomp40(x)
    w.pad(4)
    flags_out.append(0xF0)


def encode_block(frames_tt):
    """frames_tt[t] = list of (pos, quat, scale) for the block's frames. Returns the block's bytes and its transform size."""
    ntt = len(frames_tt)
    body = W(); masks = []
    for t in range(ntt):
        fr = frames_tt[t]
        pf, rf, sf = [], [], []
        enc_vec(body, [p for p, _, _ in fr], [0, 0, 0], pf)
        enc_rot(body, [q for _, q, _ in fr], rf)
        enc_vec(body, [s for _, _, s in fr], [1, 1, 1], sf)
        quant = 1 | (1 << 2) | (1 << 6)       # positions 16 bits, rotations THREECOMP40, scales 16 bits
        masks.append((quant, pf[0], rf[0], sf[0]))
    # the per-track layout puts position, rotation and scale of one track in sequence; masks first
    out = W()
    for m in masks:
        out.b += bytes(m)
    out.pad(4)
    head = len(out.b)
    out.b += body.b
    return bytes(out.b), head


def splice(pf, sec, off, blocks_bytes):
    d = bytearray(pf.d)
    s = pf.sections[sec]
    data_t, ndata = pf.array(sec, off + 0x98)
    start = data_t[1]
    old_len = H.align(ndata, 16)
    new = bytearray(); block_offs = []; float_offs = []
    for b in blocks_bytes:
        block_offs.append(len(new)); float_offs.append(len(b))
        new += b
        while len(new) % 16:
            new.append(0)
    new_len = len(new)
    delta = new_len - old_len
    base = s['start']
    d[base + start: base + start + old_len] = new
    # the section offsets (relative to the section start) and the fixups that point past the data array
    tail_from = start + old_len

    def sh(x):
        return x + delta if x >= tail_from and x != 0xFFFFFFFF else x
    loc, glo, vir, exp, imp, end = (s[k] + delta for k in ('local', 'glob', 'virt', 'exp', 'imp', 'end'))
    for o in range(loc, glo - 7, 8):
        a = base + o; src, dst = struct.unpack_from('<II', d, a)
        if src != 0xFFFFFFFF:
            struct.pack_into('<II', d, a, sh(src), sh(dst))
    for o in range(glo, vir - 11, 12):
        a = base + o; src, ds, dst = struct.unpack_from('<III', d, a)
        if src != 0xFFFFFFFF:
            struct.pack_into('<III', d, a, sh(src), ds, sh(dst) if ds == sec else dst)
    for o in range(vir, exp - 11, 12):
        a = base + o; src, cs, co = struct.unpack_from('<III', d, a)
        if src != 0xFFFFFFFF:
            struct.pack_into('<III', d, a, sh(src), cs, co)
    # the section header in the file header
    pos = 64 + struct.unpack_from('<ihh', d, 56)[2] + sec * 64
    struct.pack_into('<6i', d, pos + 24, loc, glo, vir, exp, imp, end)
    # the arrays' sizes and contents (objects before the data array do not move)
    a = base + off
    cap_flags = struct.unpack_from('<i', d, a + 0x98 + 12)[0] & ~0x3FFFFFFF
    struct.pack_into('<ii', d, a + 0x98 + 8, new_len, new_len | cap_flags)
    for arr, vals in ((0x58, block_offs), (0x68, float_offs)):
        t, n = pf.array(sec, off + arr)
        assert n == len(vals)
        struct.pack_into('<%dI' % n, d, base + t[1], *vals)
    return bytes(d)


def main(argv):
    src, dst = argv[1], argv[2]
    edit = argv[argv.index('--edit') + 1] if '--edit' in argv else None
    pf = H.Packfile(open(src, 'rb').read())
    sec, off = next((s_, o_) for s_, o_, n in pf.objects if n == 'hkaSplineCompressedAnimation')
    hdr, blocks, _ = H.read_spline_anim(pf, sec, off)
    ntt, nfr, m = hdr['transform_tracks'], hdr['frames'], hdr['max_frames_per_block']
    frames = [[H.sample(hdr, blocks, t, f) for f in range(nfr)] for t in range(ntt)]
    target = frames
    if edit and edit.startswith('bone:'):
        _, t, deg = edit.split(':'); t = int(t); r = math.radians(float(deg))
        qy = [0, math.sin(r / 2), 0, math.cos(r / 2)]
        target = [list(tr) for tr in frames]
        target[t] = [(p, quat_mul(q, qy), s) for p, q, s in frames[t]]
    nblocks = hdr['blocks']
    out_blocks = []
    for b in range(nblocks):
        f0 = b * (m - 1); f1 = min(nfr - 1, f0 + m - 1)
        bb, _ = encode_block([[target[t][f] for f in range(f0, f1 + 1)] for t in range(ntt)])
        out_blocks.append(bb)
    out = splice(pf, sec, off, out_blocks)
    open(dst, 'wb').write(out)
    # read back
    pf2 = H.Packfile(out)
    sec2, off2 = next((s_, o_) for s_, o_, n in pf2.objects if n == 'hkaSplineCompressedAnimation')
    hdr2, blocks2, st2 = H.read_spline_anim(pf2, sec2, off2)
    worst_deg = 0.0; worst_pos = 0.0
    for t in range(ntt):
        for f in range(nfr):
            p2, q2, s2 = H.sample(hdr2, blocks2, t, f)
            p1, q1, s1 = target[t][f]
            dot = abs(sum(a * b for a, b in zip(quat_norm(q1), quat_norm(q2))))
            worst_deg = max(worst_deg, math.degrees(2 * math.acos(min(1.0, dot))))
            worst_pos = max(worst_pos, max(abs(a - b) for a, b in zip(p1, p2)))
    print('wrote', dst, 'bytes', len(pf.d), '->', len(out), '; data', hdr['data_bytes'], '->', hdr2['data_bytes'],
          '; objects', [n for _, _, n in pf2.objects])
    print('read back: %d tracks x %d frames, worst rotation error %.3f deg, worst position error %.5f m' % (ntt, nfr, worst_deg, worst_pos))
    print('tracks', st2)


if __name__ == '__main__':
    main(sys.argv)
