# I-0128 R8d (R8-PLAN §2): how steady the displayed picture is under RR, from a still run's presented pictures (world-pictures.flag:
# world-N.rgba, RGBA8 rows at the presentation extent, every 300th world frame; world-burst.flag: also the 31 frames after 2400).
# Y = 0.2126 R + 0.7152 G + 0.0722 B on the 0-255 values (R7d's figure, compare-r7d4-stability.txt). Prints, over the whole picture and
# each --roi region:
# - burst: the mean |dY| between consecutive frames of the burst (frame-to-frame flicker), and each pixel's standard deviation of Y over
#   the burst's frames, its mean, median and 90th percentile over the pixels (the median is robust to a moving minority: grass, fire);
# - apart: the mean |dY| between the 300-apart pictures at --apart frames (R7d's figure: a slow drift or blotches that outlive RR's history).
# A picture missing is said; a camera that moved is not detected (give a still --roi as a control and read it). Standard library
# only (python -I).
# Usage: python -I Get-Stability.py <run folder> [<run folder> ...] --size WxH [--stride 2] [--burst 2400:32] [--apart 1800,2100,2400,2700]
#        [--roi name=x:y:w:h ...]
import argparse, math, os, sys

def luma(path, w, h, stride):
    with open(path, 'rb') as f:
        data = f.read()
    if len(data) != w * h * 4:
        raise ValueError(f'{path}: {len(data)} bytes, not {w}x{h} RGBA8')
    out = []
    for y in range(0, h, stride):
        row = data[4 * w * y: 4 * w * (y + 1)]
        r, g, b = row[0::4 * stride], row[1::4 * stride], row[2::4 * stride]
        out.append([0.2126 * r[i] + 0.7152 * g[i] + 0.0722 * b[i] for i in range(len(r))])
    return out

def region_cells(rois, w, h, stride):
    regions = [('whole', 0, 0, w, h)]
    for roi in rois:
        name, box = roi.split('=')
        x, y, rw, rh = (int(v) for v in box.split(':'))
        regions.append((name, x, y, rw, rh))
    rows_all, cols_all = range((h + stride - 1) // stride), range((w + stride - 1) // stride)
    return [(name, [r for r in rows_all if y <= r * stride < y + rh], [c for c in cols_all if x <= c * stride < x + rw])
            for name, x, y, rw, rh in regions]

def mean_abs_diff(a, b, rows, cols):
    s = 0.0; n = 0
    for r in rows:
        ra, rb = a[r], b[r]
        for c in cols:
            s += abs(ra[c] - rb[c]); n += 1
    return s / n if n else float('nan')

def percentile(values, q):
    if not values:
        return float('nan')
    v = sorted(values); k = (len(v) - 1) * q; lo = math.floor(k); hi = math.ceil(k)
    return v[lo] + (v[hi] - v[lo]) * (k - lo)

def main():
    p = argparse.ArgumentParser()
    p.add_argument('runs', nargs='+')
    p.add_argument('--size', required=True)
    p.add_argument('--stride', type=int, default=2)
    p.add_argument('--burst', default='2400:32')
    p.add_argument('--apart', default='1800,2100,2400,2700')
    p.add_argument('--roi', action='append', default=[])
    o = p.parse_args()
    w, h = (int(v) for v in o.size.lower().split('x'))
    first, count = (int(v) for v in o.burst.split(':'))
    apart = [int(v) for v in o.apart.split(',') if v]
    cells = region_cells(o.roi, w, h, o.stride)
    for run in o.runs:
        name = os.path.basename(os.path.normpath(run))
        print(f'{name}')
        frames = [s for s in range(first, first + count) if os.path.exists(os.path.join(run, f'world-{s}.rgba'))]
        burst = [luma(os.path.join(run, f'world-{s}.rgba'), w, h, o.stride) for s in frames]
        if len(frames) < count:
            print(f'  burst: {len(frames)} of {count} frames ({frames[0] if frames else "-"} to {frames[-1] if frames else "-"})')
        consecutive = [(i, i + 1) for i in range(len(frames) - 1) if frames[i + 1] == frames[i] + 1]
        present = [s for s in apart if os.path.exists(os.path.join(run, f'world-{s}.rgba'))]
        pics = {s: luma(os.path.join(run, f'world-{s}.rgba'), w, h, o.stride) for s in present}
        for region, rows, cols in cells:
            if not rows or not cols:
                print(f'  {region:10s} outside the picture');continue
            if len(burst) >= 2:
                flicker = [mean_abs_diff(burst[i], burst[j], rows, cols) for i, j in consecutive]
                stds = []
                n = len(burst)
                for r in rows:
                    for c in cols:
                        vals = [b[r][c] for b in burst]
                        m = sum(vals) / n
                        stds.append(math.sqrt(sum((v - m) ** 2 for v in vals) / (n - 1)))
                f_mean = sum(flicker) / len(flicker) if flicker else float('nan')
                f_max = max(flicker) if flicker else float('nan')
                print(f'  {region:10s} burst {len(burst)} frames: consecutive |dY| {f_mean:.3f} (max {f_max:.3f}); per-pixel std mean {sum(stds) / len(stds):.3f} median {percentile(stds, 0.5):.3f} p90 {percentile(stds, 0.9):.3f}')
            if len(present) >= 2:
                step = apart[1] - apart[0] if len(apart) > 1 else 0
                pairs = [mean_abs_diff(pics[a], pics[b], rows, cols) for a, b in zip(present, present[1:]) if b - a == step]   # only the pictures --apart's step apart
                print(f'  {region:10s} apart {",".join(str(s) for s in present)}: ' + ' '.join(f'{x:.3f}' for x in pairs))
            elif region == 'whole':
                print(f'  apart: {len(present)} of {len(apart)} pictures')

if __name__ == '__main__':
    main()
