"""I-0130: read a Havok 2014.1.0-r1 packfile (64-bit, little endian) as DS3 ships it, without the Havok SDK.

Reads the sections, the fixups (local, global, virtual), lists the objects by class, and decodes
`hkaSkeleton` (bone names, parents, reference pose) and `hkaSplineCompressedAnimation` (header, then the
blocks: every track's position / rotation / scale, static or B-spline, sampled at each frame).
Standard library only. Read-only on the input; prints statistics, never the game's data in bulk.

Usage: python -I hkx_read.py <file.hkx> [--sample N] [--track T] [--check]
  --sample N  print the decoded transform of track T at frame N
  --check     decode every frame of every track and report quaternion norms (a decode sanity check)
"""
import math
import struct
import sys


class Packfile:
    def __init__(self, data):
        self.d = data
        m0, m1, user_tag, version = struct.unpack_from('<IIii', data, 0)
        if (m0, m1) != (0x57E0E057, 0x10C0C010):
            raise ValueError('not a Havok packfile')
        self.version = version
        self.layout = data[16:20]
        (self.num_sections, self.contents_section, self.contents_offset,
         self.contents_cn_section, self.contents_cn_offset) = struct.unpack_from('<iiiii', data, 20)
        self.sdk = data[40:56].split(b'\0')[0].decode()
        flags, maxpred, predpad = struct.unpack_from('<ihh', data, 56)
        pos = 64 + (predpad if version == 11 else 0)
        self.sections = []
        for i in range(self.num_sections):
            tag = data[pos:pos + 20].split(b'\0')[0].decode()
            vals = struct.unpack_from('<7i', data, pos + 20)
            start, local, glob, virt, exp, imp, end = vals
            self.sections.append(dict(tag=tag, start=start, local=local, glob=glob, virt=virt, exp=exp, imp=imp, end=end))
            pos += 48 + (16 if version == 11 else 0)
        self.ptr = {}       # (section, offset) of a pointer -> (section, offset) of its target
        self.objects = []   # (section, offset, class name)
        for si, s in enumerate(self.sections):
            base = s['start']
            for o in range(s['local'], s['glob'] - 7, 8):      # each table ends in 0xFF padding
                src, dst = struct.unpack_from('<II', data, base + o)
                if src != 0xFFFFFFFF:
                    self.ptr[(si, src)] = (si, dst)
            for o in range(s['glob'], s['virt'] - 11, 12):
                src, dsec, dst = struct.unpack_from('<III', data, base + o)
                if src != 0xFFFFFFFF:
                    self.ptr[(si, src)] = (dsec, dst)
            for o in range(s['virt'], s['exp'] - 11, 12):
                src, csec, coff = struct.unpack_from('<III', data, base + o)
                if src != 0xFFFFFFFF:
                    cs = self.sections[csec]
                    name = data[cs['start'] + coff:data.index(b'\0', cs['start'] + coff)].decode()
                    self.objects.append((si, src, name))

    def abs(self, sec, off):
        return self.sections[sec]['start'] + off

    def deref(self, sec, off):
        return self.ptr.get((sec, off))

    def array(self, sec, off):
        """hkArray: pointer (fixup), int size, int capacityAndFlags."""
        size = struct.unpack_from('<i', self.d, self.abs(sec, off) + 8)[0]
        return self.deref(sec, off), size

    def cstr(self, sec, off):
        t = self.deref(sec, off)
        if not t:
            return None
        a = self.abs(*t)
        return self.d[a:self.d.index(b'\0', a)].decode('ascii', 'replace')


def read_skeleton(pf, sec, off):
    name = pf.cstr(sec, off + 0x10)
    parents_t, nparents = pf.array(sec, off + 0x18)
    bones_t, nbones = pf.array(sec, off + 0x28)
    pose_t, npose = pf.array(sec, off + 0x38)
    parents = list(struct.unpack_from('<%dh' % nparents, pf.d, pf.abs(*parents_t))) if parents_t else []
    names = []
    for i in range(nbones):   # hkaBone: hkStringPtr name, hkBool lockTranslation, 16 bytes
        names.append(pf.cstr(bones_t[0], bones_t[1] + i * 16))
    pose = []
    for i in range(npose):    # hkQsTransform: translation (4 floats), rotation (4), scale (4)
        pose.append(struct.unpack_from('<12f', pf.d, pf.abs(pose_t[0], pose_t[1] + i * 48)))
    return dict(name=name, parents=parents, bones=names, pose=pose)


def read_root_motion(pf, sec, off):
    """hkaDefaultAnimatedReferenceFrame: up, forward, duration, one hkVector4 a frame (x, y, z, yaw)."""
    d = pf.d; a = pf.abs(sec, off)
    up = struct.unpack_from('<4f', d, a + 0x20); fwd = struct.unpack_from('<4f', d, a + 0x30)
    duration = struct.unpack_from('<f', d, a + 0x40)[0]
    t, n = pf.array(sec, off + 0x48)
    samples = [struct.unpack_from('<4f', d, pf.abs(t[0], t[1] + 16 * i)) for i in range(n)] if t else []
    return dict(up=up[:3], forward=fwd[:3], duration=round(duration, 4), count=n, samples=samples)


# ---- hkaSplineCompressedAnimation ------------------------------------------------------------------
ROT_SIZE = {0: 4, 1: 5, 2: 6, 3: 3, 4: 2, 5: 16}     # POLAR32, THREECOMP40, THREECOMP48, THREECOMP24, STRAIGHT16, UNCOMPRESSED
ROT_ALIGN = {0: 4, 1: 1, 2: 2, 3: 1, 4: 2, 5: 4}
ROT_NAMES = {0: 'POLAR32', 1: 'THREECOMP40', 2: 'THREECOMP48', 3: 'THREECOMP24', 4: 'STRAIGHT16', 5: 'UNCOMPRESSED'}


def align(p, a):
    return (p + a - 1) // a * a


class Reader:
    def __init__(self, d, p):
        self.d, self.p = d, p

    def u8(self):
        v = self.d[self.p]; self.p += 1; return v

    def u16(self):
        v = struct.unpack_from('<H', self.d, self.p)[0]; self.p += 2; return v

    def f32(self):
        v = struct.unpack_from('<f', self.d, self.p)[0]; self.p += 4; return v

    def align(self, base, a):
        self.p = base + align(self.p - base, a)


def find_span(n, p, u, knots):
    if u >= knots[n + 1]:
        return n
    low, high = p, n + 1
    mid = (low + high) // 2
    while u < knots[mid] or u >= knots[mid + 1]:
        if u < knots[mid]:
            high = mid
        else:
            low = mid
        mid = (low + high) // 2
    return mid


def basis(span, u, p, knots):
    n = [0.0] * (p + 1); left = [0.0] * (p + 1); right = [0.0] * (p + 1)
    n[0] = 1.0
    for j in range(1, p + 1):
        left[j] = u - knots[span + 1 - j]
        right[j] = knots[span + j] - u
        saved = 0.0
        for r in range(j):
            denom = right[r + 1] + left[j - r]
            temp = n[r] / denom if denom else 0.0
            n[r] = saved + right[r + 1] * temp
            saved = left[j - r] * temp
        n[j] = saved
    return n


def eval_spline(ctrl, knots, degree, u):
    num = len(ctrl) - 1
    span = find_span(num, degree, u, knots)
    b = basis(span, u, degree, knots)
    out = [0.0] * len(ctrl[0])
    for i in range(degree + 1):
        c = ctrl[span - degree + i]
        for k in range(len(out)):
            out[k] += b[i] * c[k]
    return out


def read_quat(r, qtype):
    d = r.d
    if qtype == 5:
        q = struct.unpack_from('<4f', d, r.p); r.p += 16; return list(q)
    if qtype == 1:   # THREECOMP40: 3 x 12 bits + 2-bit dropped index + sign
        v = int.from_bytes(d[r.p:r.p + 5], 'little'); r.p += 5
        mask, frac = (1 << 12) - 1, 0.000345436 * 2.0   # (1/sqrt2) / 2047, the published constant pair
        a, b, c = v & mask, (v >> 12) & mask, (v >> 24) & mask
        shift = (v >> 36) & 3
        neg = (v >> 38) & 1
        comps = [(x - 2047) * (1.0 / 2047) * (1 / math.sqrt(2)) for x in (a, b, c)]
        w2 = 1.0 - sum(x * x for x in comps)
        w = math.sqrt(max(0.0, w2)) * (-1 if neg else 1)
        comps.insert(shift, w)
        return comps
    if qtype == 2:   # THREECOMP48: 3 x 16 bits, the dropped index in the top bits of two of them
        x, y, z = struct.unpack_from('<3H', d, r.p); r.p += 6
        shift = ((y >> 14) & 2) | ((x >> 15) & 1)
        neg = (z >> 15) & 1
        m = 0x7FFF
        comps = [((v & m) - 16383) * (1.0 / 16383) * (1 / math.sqrt(2)) for v in (x, y, z)]
        w = math.sqrt(max(0.0, 1.0 - sum(c * c for c in comps))) * (-1 if neg else 1)
        comps.insert(shift, w)
        return comps
    raise NotImplementedError(ROT_NAMES.get(qtype, qtype))


def read_vec_track(r, base, flags, qbits):
    """Position or scale: flags bits 0..2 static X/Y/Z, bits 4..6 spline X/Y/Z. Returns a callable of u."""
    spline = flags & 0x70
    if spline:
        nitems = r.u16(); degree = r.u8()
        knots = [r.u8() for _ in range(nitems + degree + 2)]
        r.align(base, 4)
        lo, hi, stat = [0.0] * 3, [0.0] * 3, [None] * 3
        for k in range(3):
            if flags & (0x10 << k):
                lo[k], hi[k] = r.f32(), r.f32()
            elif flags & (0x01 << k):
                stat[k] = r.f32()
        ctrl = []
        for _ in range(nitems + 1):
            c = []
            for k in range(3):
                if flags & (0x10 << k):
                    if qbits == 0:
                        q = r.u8() / 255.0
                    else:
                        q = r.u16() / 65535.0
                    c.append(lo[k] + (hi[k] - lo[k]) * q)
                else:
                    c.append(stat[k] if stat[k] is not None else 0.0)
            ctrl.append(c)
        r.align(base, 4)
        return ('spline', degree, nitems, lambda u: eval_spline(ctrl, knots, degree, u))
    vals = [None] * 3
    for k in range(3):
        if flags & (0x01 << k):
            vals[k] = r.f32()
    r.align(base, 4)
    return ('static' if any(v is not None for v in vals) else 'identity', 0, 0, vals)


def read_rot_track(r, base, flags, qtype):
    if flags & 0xF0:
        nitems = r.u16(); degree = r.u8()
        knots = [r.u8() for _ in range(nitems + degree + 2)]
        r.align(base, ROT_ALIGN[qtype])
        ctrl = [read_quat(r, qtype) for _ in range(nitems + 1)]
        r.align(base, 4)
        return ('spline', degree, nitems, lambda u: eval_spline(ctrl, knots, degree, u))
    if flags & 0x0F:
        r.align(base, ROT_ALIGN[qtype])
        q = read_quat(r, qtype)
        r.align(base, 4)
        return ('static', 0, 0, q)
    return ('identity', 0, 0, [0, 0, 0, 1])


def read_spline_anim(pf, sec, off):
    d = pf.d; a = pf.abs(sec, off)
    atype, duration, ntt, nft = struct.unpack_from('<ifii', d, a + 0x10)
    hdr = dict(type=atype, duration=duration, transform_tracks=ntt, float_tracks=nft)
    (hdr['frames'], hdr['blocks'], hdr['max_frames_per_block'], hdr['mask_quant_size'],
     hdr['block_duration'], hdr['block_inv_duration'], hdr['frame_duration']) = struct.unpack_from('<iiiifff', d, a + 0x38)
    bo_t, nbo = pf.array(sec, off + 0x58)
    data_t, ndata = pf.array(sec, off + 0x98)
    block_offsets = list(struct.unpack_from('<%dI' % nbo, d, pf.abs(*bo_t)))
    data_base = pf.abs(*data_t)
    hdr['data_bytes'] = ndata
    hdr['extracted_motion'] = pf.deref(sec, off + 0x20) is not None
    blocks = []
    stats = {'pos': {}, 'rot': {}, 'scale': {}, 'rot_quant': {}}
    for bi, bo in enumerate(block_offsets):
        base = data_base + bo
        r = Reader(d, base)
        masks = [(r.u8(), r.u8(), r.u8(), r.u8()) for _ in range(ntt)]
        r.align(base, 4)   # the float tracks' masks follow (none in the files read here)
        r.p += nft
        r.align(base, 4)
        tracks = []
        for t in range(ntt):
            qt, pflags, rflags, sflags = masks[t]
            pq, rq, sq = qt & 3, (qt >> 2) & 0xF, (qt >> 6) & 3
            pos = read_vec_track(r, base, pflags, pq)
            rot = read_rot_track(r, base, rflags, rq)
            scl = read_vec_track(r, base, sflags, sq)
            for k, v in (('pos', pos[0]), ('rot', rot[0]), ('scale', scl[0])):
                stats[k][v] = stats[k].get(v, 0) + 1
            if rot[0] != 'identity':
                stats['rot_quant'][ROT_NAMES[rq]] = stats['rot_quant'].get(ROT_NAMES[rq], 0) + 1
            tracks.append((pos, rot, scl))
        blocks.append(tracks)
    return hdr, blocks, stats


def sample(hdr, blocks, track, frame, pose=None):
    mfpb = hdr['max_frames_per_block']
    bi = min(frame // (mfpb - 1), len(blocks) - 1)
    local = frame - bi * (mfpb - 1)
    u = local   # the knots are in frames inside the block
    pos, rot, scl = blocks[bi][track]

    def ev(tr, default):
        kind, deg, n, f = tr
        if kind == 'spline':
            return f(float(u))
        if kind == 'static':
            return [default[k] if v is None else v for k, v in enumerate(f)] if len(f) == 3 else f
        return default
    p = ev(pos, pose[0:3] if pose else [0, 0, 0])
    q = ev(rot, pose[4:8] if pose else [0, 0, 0, 1])
    s = ev(scl, pose[8:11] if pose else [1, 1, 1])
    return p, q, s


def main(argv):
    path = argv[1]
    data = open(path, 'rb').read()
    pf = Packfile(data)
    print('sdk', pf.sdk, 'version', pf.version, 'layout', list(pf.layout), 'sections', [s['tag'] for s in pf.sections])
    counts = {}
    for _, _, n in pf.objects:
        counts[n] = counts.get(n, 0) + 1
    print('objects', counts)
    for sec, off, name in pf.objects:
        if name == 'hkaSkeleton':
            sk = read_skeleton(pf, sec, off)
            print('skeleton', sk['name'], 'bones', len(sk['bones']), 'roots', [sk['bones'][i] for i, p in enumerate(sk['parents']) if p < 0])
        if name == 'hkaSplineCompressedAnimation':
            hdr, blocks, stats = read_spline_anim(pf, sec, off)
            print('animation', {k: (round(v, 4) if isinstance(v, float) else v) for k, v in hdr.items()})
            print('tracks', stats)
            if '--check' in argv:
                worst = 0.0; n = 0
                for t in range(hdr['transform_tracks']):
                    for f in range(hdr['frames']):
                        _, q, _ = sample(hdr, blocks, t, f)
                        norm = math.sqrt(sum(c * c for c in q)); n += 1
                        worst = max(worst, abs(norm - 1.0))
                print('check: %d samples, worst |quaternion norm - 1| = %.4f' % (n, worst))
            if '--sample' in argv:
                f = int(argv[argv.index('--sample') + 1])
                t = int(argv[argv.index('--track') + 1]) if '--track' in argv else 0
                print('track', t, 'frame', f, sample(hdr, blocks, t, f))
        if name == 'hkaDefaultAnimatedReferenceFrame':
            rm = read_root_motion(pf, sec, off)
            print('root motion', {k: v for k, v in rm.items() if k != 'samples'}, 'last sample', [round(x, 3) for x in rm['samples'][-1]] if rm['samples'] else None)
        if name == 'hkaAnimationBinding':
            a = pf.abs(sec, off)
            skel = pf.cstr(sec, off + 0x10)
            tt_t, ntt = pf.array(sec, off + 0x20)
            print('binding skeleton', skel, 'track->bone entries', ntt)


if __name__ == '__main__':
    main(sys.argv)
