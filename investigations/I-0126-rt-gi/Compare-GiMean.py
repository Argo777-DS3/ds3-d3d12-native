# I-0126 R7d (R7-PLAN §8): the GI's bias check. Reads two `-gi-mean.rgba32f` files (rt-gi-mean.flag: per pixel the sums of the GI's
# demodulated estimate in rgb and its frames in a, raw rows at the render extent) of the same still view, the reference first (the PT
# level's one ray, unbiased) and the tested one second, and prints how the tested mean departs from the reference: the whole picture's
# luminance ratio, the ratio over blocks (their spread), and the ratio over the pixels next to a strong edge of the reference's mean (the
# shadow edges, where R4c's reuse darkened) against the others. The reference is the PT level's one ray (no-rt-restir.flag since I-0132 R11i, ReSTIR GI the default): the RT
# level lights the hits' local lights otherwise (R7d's step review M1). Standard library only (python -I).
# Usage: python -I Compare-GiMean.py <reference.rgba32f> <tested.rgba32f> --size WxH [--block 32] [--edge 0.5] [--band 2] [--min-frames 200]
import argparse, array, math, sys

def load(path, w, h):
    a = array.array('f')
    with open(path, 'rb') as f:
        a.fromfile(f, w * h * 4)
    if sys.byteorder != 'little':
        a.byteswap()
    return a

def means(a, w, h, min_frames):
    lum = [0.0] * (w * h)
    ok = [False] * (w * h)
    frames = []
    for i in range(w * h):
        n = a[4 * i + 3]
        if n >= min_frames:
            r, g, b = a[4 * i] / n, a[4 * i + 1] / n, a[4 * i + 2] / n
            if all(math.isfinite(x) for x in (r, g, b)):
                lum[i] = 0.2126 * r + 0.7152 * g + 0.0722 * b
                ok[i] = True
        frames.append(n)
    return lum, ok, frames

def main():
    p = argparse.ArgumentParser()
    p.add_argument('reference'); p.add_argument('tested')
    p.add_argument('--size', required=True); p.add_argument('--block', type=int, default=32)
    p.add_argument('--edge', type=float, default=0.5); p.add_argument('--band', type=int, default=2)
    p.add_argument('--min-frames', type=float, default=200)
    p.add_argument('--blocks-out', default='')   # each read block's ratio, one 'bx by ratio' line: two tests' files correlated tell a local bias from noise
    o = p.parse_args()
    w, h = (int(x) for x in o.size.lower().split('x'))
    ra, ta = load(o.reference, w, h), load(o.tested, w, h)
    rl, rok, rf = means(ra, w, h, o.min_frames)
    tl, tok, tf = means(ta, w, h, o.min_frames)
    both = [rok[i] and tok[i] for i in range(w * h)]
    print(f'frames: reference {min(rf):.0f}..{max(rf):.0f}, tested {min(tf):.0f}..{max(tf):.0f}; pixels compared {sum(both)} of {w*h}')
    # The edges of the reference's mean: a relative central difference over one pixel, then a band of `band` pixels about them.
    edge = [False] * (w * h)
    for y in range(1, h - 1):
        for x in range(1, w - 1):
            i = y * w + x
            if not rok[i]:
                continue
            l = rl[i]
            g = abs(rl[i + 1] - rl[i - 1]) + abs(rl[i + w] - rl[i - w])
            if g > o.edge * (l + 1e-4) and g > 1e-3:
                edge[i] = True
    band = [False] * (w * h)
    for y in range(h):
        for x in range(w):
            if edge[y * w + x]:
                for dy in range(-o.band, o.band + 1):
                    yy = y + dy
                    if 0 <= yy < h:
                        for dx in range(-o.band, o.band + 1):
                            xx = x + dx
                            if 0 <= xx < w:
                                band[yy * w + xx] = True
    def ratio(sel):
        sr = st = 0.0; n = 0
        for i in range(w * h):
            if both[i] and sel(i):
                sr += rl[i]; st += tl[i]; n += 1
        return (st / sr if sr > 0 else float('nan')), n, sr / max(n, 1)
    whole = ratio(lambda i: True)
    near = ratio(lambda i: band[i])
    far = ratio(lambda i: not band[i])
    print(f'whole picture: tested / reference {whole[0]:.4f} over {whole[1]} pixels (reference mean luminance {whole[2]:.4g})')
    print(f'next to edges: {near[0]:.4f} over {near[1]} pixels; elsewhere: {far[0]:.4f} over {far[1]} pixels')
    # Blocks: each block's ratio (its sums), with enough reference light to be read.
    ratios = []; rows = []
    for by in range(0, h - o.block + 1, o.block):
        for bx in range(0, w - o.block + 1, o.block):
            sr = st = 0.0; n = 0
            for y in range(by, by + o.block):
                for x in range(bx, bx + o.block):
                    i = y * w + x
                    if both[i]:
                        sr += rl[i]; st += tl[i]; n += 1
            if n >= o.block * o.block // 2 and sr / n > 0.1 * whole[2]:
                ratios.append(st / sr); rows.append(f'{bx} {by} {st / sr:.6f}')
    if o.blocks_out:
        open(o.blocks_out, 'w').write(chr(10).join(rows) + chr(10))
    if ratios:
        ratios.sort(); k = len(ratios)
        q = lambda f: ratios[min(k - 1, int(f * k))]
        print(f'blocks {o.block}x{o.block}: {k}; ratio p5 {q(0.05):.4f}, median {q(0.5):.4f}, p95 {q(0.95):.4f}, min {ratios[0]:.4f}, max {ratios[-1]:.4f}')

if __name__ == '__main__':
    main()
