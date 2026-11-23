"""I-0141 tranche 25 (RESULT « Suggested next step » 3): where a held pair's luma difference lies.

Usage: python Compare-I141HoldRegions.py <first.png> <second.png> [--grid CxR | --grid=CxR]   (default 6x4)

Both captures must be 1920x1080 window shots (Run-AlphaCast's cutscene-hold-<k>.png, JPEG data under a .png name, read with
-c:v mjpeg); anything else is refused. As in Make-I138HoldPairs.ps1 the window's title bar (rows 0 to 30) is left out (rows 32
on), and columns 0 to 7 (the border) too. ffmpeg decodes each full frame to 8-bit grey (Y of 255, the Y of signalstats' YAVG);
the crop and a 2x2 mean (half size) are made here. Printed: each capture's luma quantiles (5 to 99 %) and their differences
(first - second: a uniform gain lifts every quantile in proportion; a veil or a bloom lifts the low ones; clipped highlights stop
at the top); the means; the mean difference by grid cell (rows top to bottom); and the share of the whole difference carried by
the pixels whose second-capture luma is below 64, 64 to 159 and 160 and above (« n/a » when the whole difference is under 0.5
of 255 a pixel, where a share means nothing).
"""
import subprocess, sys, glob, os
SW, SH = 1920, 1080
X0, Y0 = 8, 32
W, H = (SW - X0) // 2, (SH - Y0) // 2   # 956 x 524
def ffmpeg():
    for p in glob.glob(os.path.expandvars(r'%LOCALAPPDATA%\Microsoft\WinGet\Packages\**\ffmpeg.exe'), recursive=True): return p
    sys.exit('no ffmpeg')
def grey(path, ff):
    run = subprocess.run([ff, '-hide_banner', '-loglevel', 'error', '-c:v', 'mjpeg', '-i', path, '-vf', 'format=gray', '-f', 'rawvideo', '-'],
                         capture_output=True, creationflags=0x08000000)   # CREATE_NO_WINDOW (D-0199)
    if run.returncode: sys.exit('%s: ffmpeg failed: %s' % (path, run.stderr.decode(errors='replace').strip()))
    raw = run.stdout
    if len(raw) != SW * SH: sys.exit('%s: %d bytes decoded, a 1920x1080 capture gives %d: refused' % (path, len(raw), SW * SH))
    out = bytearray(W * H)
    for y in range(H):
        r0 = (Y0 + 2 * y) * SW + X0; r1 = r0 + SW
        for x in range(W):
            a = r0 + 2 * x; b = r1 + 2 * x
            out[y * W + x] = (raw[a] + raw[a + 1] + raw[b] + raw[b + 1] + 2) // 4
    return out
def quantiles(v, qs):
    hist = [0] * 256
    for p in v: hist[p] += 1
    out = []
    for q in qs:
        k = min(len(v) - 1, int(q * len(v))); acc = 0
        for level in range(256):
            acc += hist[level]
            if acc > k: out.append(level); break
    return out
args, gc, gr, rest = [], 6, 4, sys.argv[1:]
i = 0
while i < len(rest):
    a = rest[i]
    if a == '--grid' or a.startswith('--grid='):
        spec = a[7:] if a.startswith('--grid=') else (rest[i + 1] if i + 1 < len(rest) else '')
        if a == '--grid': i += 1
        try: gc, gr = (int(x) for x in spec.split('x'))
        except ValueError: sys.exit('--grid takes CxR, for example 6x4')
        if not (1 <= gc <= W and 1 <= gr <= H): sys.exit('--grid: 1 to %d columns and 1 to %d rows' % (W, H))
    elif a.startswith('--'): sys.exit('unknown option %s' % a)
    else: args.append(a)
    i += 1
if len(args) != 2: sys.exit(__doc__)
ff = ffmpeg(); n = grey(args[0], ff); o = grey(args[1], ff)
qs = (0.05, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99)
print('quantile\t' + '\t'.join('%d%%' % round(q * 100) for q in qs))
qn = quantiles(n, qs); qo = quantiles(o, qs)
print('first\t' + '\t'.join(str(x) for x in qn)); print('second\t' + '\t'.join(str(x) for x in qo))
print('first - second\t' + '\t'.join('%+d' % (a - b) for a, b in zip(qn, qo)))
tot = sum(n) - sum(o)
print('mean\tfirst %.1f\tsecond %.1f\tdifference %+.1f' % (sum(n) / len(n), sum(o) / len(o), tot / len(n)))
print('grid %dx%d, mean first - second by cell (rows top to bottom):' % (gc, gr))
for r in range(gr):
    cells = []
    for c in range(gc):
        x0, x1, y0, y1 = c * W // gc, (c + 1) * W // gc, r * H // gr, (r + 1) * H // gr
        d = s = 0
        for y in range(y0, y1):
            row = y * W
            for x in range(x0, x1): d += n[row + x] - o[row + x]; s += 1
        cells.append('%+6.1f' % (d / s))
    print('\t'.join(cells))
for lo, hi in ((0, 64), (64, 160), (160, 256)):
    idx = [k for k in range(len(o)) if lo <= o[k] < hi]
    d = sum(n[k] - o[k] for k in idx)
    share = 'n/a' if abs(tot) < 0.5 * len(n) else '%.0f%%' % (100.0 * d / tot)
    print('second luma %d..%d: %.1f%% of pixels, mean difference %+.1f, %s of the whole difference' % (
        lo, hi - 1, 100.0 * len(idx) / len(o), d / len(idx) if idx else 0.0, share))
