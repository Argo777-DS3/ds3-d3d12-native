"""I-0138 tranche 8b: the library's 256x1 spectral ramp (FUN_141719770's ctx +0x216F0), emulated in float32 from the
executable's words, as read statically (evidence/I-0138/artifacts/t8b-readings/corona-static.txt). Writes mip 0 and the
mip chain as RGBA8 bytes. Usage: corona_ramp.py <DarkSoulsIII.exe> <out.raw>"""
import struct, sys, math
class F(float):
    # float32: every operation rounded to single precision (double then round is exact for +, -, *, /)
    def __new__(cls, x): return float.__new__(cls, struct.unpack('<f', struct.pack('<f', float(x)))[0])
    def __add__(a, b): return F(float(a) + float(b))
    __radd__ = __add__
    def __sub__(a, b): return F(float(a) - float(b))
    def __rsub__(a, b): return F(float(b) - float(a))
    def __mul__(a, b): return F(float(a) * float(b))
    __rmul__ = __mul__
    def __truediv__(a, b): return F(float(a) / float(b))
    def __rtruediv__(a, b): return F(float(b) / float(a))
    def __neg__(a): return F(-float(a))
b = open(sys.argv[1], 'rb').read()
pe = struct.unpack_from('<I', b, 0x3c)[0]; n = struct.unpack_from('<H', b, pe + 6)[0]; opt = struct.unpack_from('<H', b, pe + 20)[0]
secs = [struct.unpack_from('<IIII', b, pe + 24 + opt + 40 * i + 8) for i in range(n)]
def off(a):
    r = a - 0x140000000
    for vs, va, rs, ra in secs:
        if va <= r < va + rs: return ra + r - va
    raise KeyError(hex(a))
def f(a): return F(struct.unpack_from('<f', b, off(a))[0])

cie = [[f(0x14469ae70 + 16 * i + 4 * k) for k in range(3)] for i in range(97)]
alpha_t = [f(0x14469abd0 + 8 * i) for i in range(83)]
one, five = f(0x143d7ee10), f(0x143d7eee0)
m = dict(a4=f(0x142b4d140), f4=f(0x142b4d188), d8=f(0x142b4d17c), e4=f(0x142b4d0f0), d4=f(0x142b4cff8), f0=f(0x142b4cffc),
         e8=f(0x142b4d190), d0=f(0x142b4d088), e0=f(0x142b4d170))
# FUN_1416DC870's maxima of the second set, its own constants and operation order
c = {k: f(a) for k, a in dict(d140=0x142b4d140, d0ac=0x142b4d0ac, d064=0x142b4d064, d0f0=0x142b4d0f0, d084=0x142b4d084,
                              cff8=0x142b4cff8, cffc=0x142b4cffc, d034=0x142b4d034, d088=0x142b4d088).items()}
mr = mg = mb = F(-3.4028234663852886e+38)
for X, Y, Z in cie:
    r = ((X * c['d140'] - Y * c['d0ac']) - Z * c['d064']) + F(0)
    g = ((Y * c['d0f0'] - X * c['d084']) + Z * c['cff8']) + F(0)
    bl = ((X * c['cffc'] - Y * c['d034']) + Z * c['d088']) + F(0)
    mr = max(mr, r); mg = max(mg, g); mb = max(mb, bl)
top = max(max(mr, mg), mb)
lum = (f(0x142b4d074), f(0x142b4d038), f(0x142b4d010))
gamma_pow, soft = f(0x142b4d060), f(0x143d7ed94)
def xyz(l):
    v = F(355.0)
    lo, hi = f(0x14469ae7c), f(0x14469b47c)
    v = lo if not (lo <= l) else (hi if not (l <= hi) else l)
    t = F((v - lo) / five); i = int(math.floor(t)); j = min(i + 1, 96); t = F(t - F(i))
    return [F(cie[i][k] + (cie[j][k] - cie[i][k]) * t) for k in range(3)]
def alpha(l):
    lo, hi = f(0x14469abd4), f(0x14469ae64)
    v = lo if not (lo <= l) else (hi if not (l <= hi) else l)
    t = F((v - lo) / five); i = int(math.floor(t)); j = min(i + 1, 82)
    return F((alpha_t[j] - alpha_t[i]) * F(t - F(i)) + alpha_t[i])
def colour(l):
    X, Y, Z = xyz(l)
    o = [F(F(F(Y * m['e8'] + X * m['a4']) + Z * m['d8']) + F(0)),
         F(F(F(X * m['f4'] + Y * m['e4']) + Z * m['d4']) + F(0)),
         F(F(F(X * m['f0'] + Y * m['e0']) + Z * m['d0']) + F(0)), F(0)]
    k = F(one / top); o = [F(k * x) for x in o]
    s = F(F(F(max(o[1], F(0)) * lum[0] + max(o[0], F(0)) * lum[1]) + max(o[2], F(0)) * lum[2]) * soft)
    if s <= 0: o = [max(x, F(0)) for x in o]
    else:
        e0 = F(math.exp(F(F(0) / s)))
        for ch in range(3):
            x = o[ch]; y = F(F(math.log(F(F(math.exp(F(x / s))) + e0))) * s)
            if y < x or top < y: y = max(x, F(0))
            o[ch] = y
    o[3] = alpha(l)
    o = [F(math.pow(x, gamma_pow)) if x > 0 else x for x in o]
    return o
q = f(0x142b60ce4)
def pack(o): return bytes(255 if not (x < one) else (int(F(x * q)) if x > 0 else 0) for x in o)
step = F(f(0x142b60cec) / F(f(0x143d7eff4) - F(0))); base = F(f(0x142b60ce8) - F(step * F(0)))
mip0 = bytearray()
for i in range(256):
    o = colour(F(F(F(i) * step) + base))
    if i in (0, 255): o = [F(0)] * 4
    mip0 += pack(o)
# The mip chain (YebisBuildCpuMipChain_141711350): a 2x2 box, with its 4x4 table added where the level is at least 4 texels wide.
table = [F(0.25) * F(0) + F(0), f(0x143d7ee7c), f(0x143d7edb4), f(0x1427cdab4)] + [f(0x142b60d20 + 4 * k) for k in range(4)] + \
        [f(0x143d7ede8), f(0x142b60cd4), f(0x143d7ed9c), f(0x14278d024)] + [f(0x142b60d10 + 4 * k) for k in range(4)]
levels = [bytes(mip0)]; w = 256
while w > 1:
    src = levels[-1]; w //= 2; dst = bytearray()
    for x in range(w):
        a, c2 = src[8 * x:8 * x + 4], src[8 * x + 4:8 * x + 8]
        d = table[(x & 3) * 4] if w >= 4 else f(0x143d7ee7c)
        dst += bytes(int(F(F(d + F(a[k] + a[k] + c2[k] + c2[k])) * f(0x143d7ed9c))) for k in range(4))
    levels.append(bytes(dst))
open(sys.argv[2], 'wb').write(b''.join(levels))
print('top', top, 'mip0 first texels', mip0[4:20].hex(), 'levels', len(levels))
