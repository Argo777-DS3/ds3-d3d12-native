# I-0132, ROADMAP workstream 2 step 4 (D-0273): how steady the displayed picture is while the camera turns, from Run-GiMotion.ps1's
# runs (world-burst-after-900.flag: 32 consecutive lossless pictures world-N.rgba, RGBA8 rows at the presentation extent, from the
# serial the row world_burst_window names; the same camera turn a frame in every run, unlock-30.flag). For each pair of consecutive
# burst pictures, on a band with the character's column left out (--exclude: the player stays put on screen while the world moves),
# the horizontal shift that best aligns them (the least mean |dY| over shifts within --search pixels), then:
# - raw: the mean |dY| of the pair unaligned (motion and noise together);
# - aligned: the mean |dY| at the best shift, and the median over tiles (--tiles, each tile's mean |dY| at that shift): what one
#   sideways shift does not explain. That is the GI's and the denoiser's noise and lag, but also the follow camera's orbit (it turns
#   around the player, so near ground and walls move with parallax), the perspective (a yaw's shift varies across the band) and the
#   whole-pixel alignment. Those grow with the motion and are the same in every run whose turn is the same, so only the difference
#   between the GI qualities is read, against the spread between runs of the same quality (the repeats);
# - shift: the best shift in pixels (the motion's share: equal in every run when the turn is the same);
# - the camera: the yaw turned over the burst from the rows world_burst_yaw (the follow camera's yaw at each call in the burst).
# A run is marked INVALID when its yaw rows are missing, its yaw turned is under --min-yaw, a pair's shift reached the search bound,
# or its mean shift is more than 10 % from the median of the runs (a run below 30 frames/s, or the burst outside the turn); a note when its
# burst starts more than 1.5 frames' turn from the runs' median yaw. The figure compared is the one over the yaw range every run covers
# (the pairs keyed by the camera's yaw: the scene changes along the turn, so bursts that start apart are not compared as wholes).
# Y = 0.2126 R + 0.7152 G + 0.0722 B on the 0-255 values (Get-Stability.py's). Standard library only (python -I).
# Usage: python -I Get-GiMotion.py <run folder> [<run folder> ...] [--size 1280x720] [--band 0.2:0.8:0.08:0.92] [--exclude 0.38:0.62]
#        [--search 64] [--rowstep 3] [--tiles 6x3] [--min-yaw 0.05] [--first N]
import argparse, json, math, os, sys

def luma_rows(path, w, h, rows, x0, x1):
    with open(path, 'rb') as f:
        data = f.read()
    if len(data) != w * h * 4:
        raise ValueError(f'{path}: {len(data)} bytes, not {w}x{h} RGBA8')
    out = []
    for y in rows:
        row = data[4 * (w * y + x0): 4 * (w * y + x1)]
        r, g, b = row[0::4], row[1::4], row[2::4]
        out.append([0.2126 * r[i] + 0.7152 * g[i] + 0.0722 * b[i] for i in range(len(r))])
    return out

def shifted_diff(a, b, s, cols):
    # mean |a[x] - b[x + s]| over the columns given (the same columns at every shift, the margin kept)
    total = 0.0; n = 0
    for ra, rb in zip(a, b):
        for x in cols:
            total += abs(ra[x] - rb[x + s]); n += 1
    return total / n if n else float('nan')

def burst_window(run):
    first = None; yaw = {}
    trace = os.path.join(run, 'startup.jsonl')
    if os.path.exists(trace):
        with open(trace, encoding='utf-8', errors='replace') as f:
            for line in f:
                if '"world_burst_window"' in line:
                    try: first = json.loads(line)['from']
                    except Exception: pass
                elif '"world_burst_yaw"' in line:
                    try: r = json.loads(line); yaw[r['serial']] = r['yaw']
                    except Exception: pass
    return first, yaw

def median(v):
    s = sorted(v); n = len(s)
    return float('nan') if not n else (s[n // 2] if n % 2 else 0.5 * (s[n // 2 - 1] + s[n // 2]))

def read_run(run, o, w, h, rows, x0, x1, cols, tiles):
    name = os.path.basename(os.path.normpath(run))
    first, yaw = burst_window(run)
    if first is None: first = o.first
    if first is None:
        print(f'{name}: no world_burst_window row'); return None
    serials = [s for s in range(first, first + o.frames) if os.path.exists(os.path.join(run, f'world-{s}.rgba'))]
    if len(serials) < o.frames: print(f'{name}: {len(serials)} of {o.frames} burst pictures')
    pics = {s: luma_rows(os.path.join(run, f'world-{s}.rgba'), w, h, rows, x0, x1) for s in serials}
    raws, aligned, tiled, shifts = [], [], [], []
    guess = None; bound = False
    for s in serials:
        if s + 1 not in pics: continue
        a, b = pics[s], pics[s + 1]
        # coarse every 4 pixels over the whole search, then +-4 around the best (or around the previous pair's shift)
        candidates = range(-o.search, o.search + 1, 4) if guess is None else range(max(-o.search, guess - 8), min(o.search, guess + 8) + 1)
        best = min(candidates, key=lambda d: shifted_diff(a, b, d, cols))
        fine = range(max(-o.search, best - 4), min(o.search, best + 4) + 1)
        scored = {d: shifted_diff(a, b, d, cols) for d in fine}
        best = min(scored, key=scored.get); guess = best
        if abs(best) >= o.search: bound = True
        tile_values = []
        for (r0, r1, tcols) in tiles:
            if tcols: tile_values.append(shifted_diff(a[r0:r1], b[r0:r1], best, tcols))
        raws.append(shifted_diff(a, b, 0, cols)); aligned.append(scored[best]); tiled.append(median(tile_values)); shifts.append(best)
    if not raws:
        print(f'{name}: no consecutive pair'); return None
    n = len(raws)
    steps = []
    if yaw:
        k = sorted(yaw); steps = [(yaw[b] - yaw[a] + math.pi) % (2 * math.pi) - math.pi for a, b in zip(k, k[1:])]   # wrapped to +-pi
    turned = sum(steps)
    reasons = []
    if not yaw: reasons.append('no yaw rows')
    elif abs(turned) < o.min_yaw: reasons.append(f'yaw turned {turned:.4f} rad')
    if bound: reasons.append('a shift at the search bound')
    # Each pair keyed by the camera's yaw at its first picture (the final review's M1: the bursts start at different points of the turn, and
    # the scene, not the GI, sets most of the figure; the pairs are compared over the yaw range every run covers).
    pair_serials = [s for s in serials if s + 1 in pics]
    unwrapped = {}   # the yaw unwrapped from the burst's first row (the step review's L2: a turn across +-pi stays one range)
    for a in sorted(yaw):
        unwrapped[a] = yaw[a] if not unwrapped else unwrapped[max(unwrapped)] + (yaw[a] - yaw[max(unwrapped)] + math.pi) % (2 * math.pi) - math.pi
    keyed = [(unwrapped.get(s), v, t) for s, v, t in zip(pair_serials, aligned, tiled)]
    r = dict(name=name, first=first, pictures=len(serials), pairs=n, raw=sum(raws) / n, aligned=sum(aligned) / n, aligned_median=median(aligned),
             tiles=sum(tiled) / n, shift=sum(shifts) / n, turned=turned, reasons=reasons, notes=[], keyed=keyed,
             first_yaw=yaw[min(yaw)] if yaw else None, step=median([abs(v) for v in steps]) if steps else None)
    yaw_text = (f'; camera yaw turned {turned:.4f} rad over {len(yaw)} rows (a step {min(steps):.4f} to {max(steps):.4f})' if steps else '; no camera yaw rows')
    print(f'{name}: burst {first}+{len(serials)}, {n} pairs; raw |dY| {r["raw"]:.3f}; aligned |dY| mean {r["aligned"]:.3f} median {r["aligned_median"]:.3f} '
          f'max {max(aligned):.3f}; tiles median (mean over pairs) {r["tiles"]:.3f}; shift px mean {r["shift"]:.2f} (min {min(shifts)}, max {max(shifts)}){yaw_text}')
    print('  aligned per pair: ' + ' '.join(f'{v:.2f}' for v in aligned))
    print('  shift per pair:   ' + ' '.join(str(v) for v in shifts))
    hundreds = [s for s in serials if s % 300 == 0]
    if hundreds: print(f'  note: serial {hundreds} is a 300th-frame picture inside the burst')
    return r

def main():
    p = argparse.ArgumentParser()
    p.add_argument('runs', nargs='+')
    p.add_argument('--size', default='1280x720')
    p.add_argument('--band', default='0.2:0.8:0.08:0.92', help='y0:y1:x0:x1 as fractions of the picture')
    p.add_argument('--exclude', default='0.38:0.62', help='x0:x1 fractions of the picture left out over the band (the character)')
    p.add_argument('--search', type=int, default=64)
    p.add_argument('--rowstep', type=int, default=3)
    p.add_argument('--tiles', default='6x3')
    p.add_argument('--min-yaw', type=float, default=0.05)
    p.add_argument('--frames', type=int, default=32)
    p.add_argument('--first', type=int, default=None, help='the first serial of the burst when the run has no world_burst_window row (world-burst.flag alone: 2400)')
    o = p.parse_args()
    w, h = (int(v) for v in o.size.lower().split('x'))
    fy0, fy1, fx0, fx1 = (float(v) for v in o.band.split(':'))
    rows = list(range(int(fy0 * h), int(fy1 * h), o.rowstep)); x0, x1 = int(fx0 * w), int(fx1 * w)
    if x1 - x0 <= 2 * o.search: sys.exit('the band is narrower than twice the search')
    ex0, ex1 = (int(float(v) * w) - x0 for v in o.exclude.split(':')) if o.exclude else (0, 0)
    cols = [x for x in range(o.search, x1 - x0 - o.search) if not (ex0 <= x < ex1)]
    tx, ty = (int(v) for v in o.tiles.lower().split('x'))
    tiles = []
    for j in range(ty):
        r0, r1 = j * len(rows) // ty, (j + 1) * len(rows) // ty
        for i in range(tx):
            c0, c1 = o.search + i * (x1 - x0 - 2 * o.search) // tx, o.search + (i + 1) * (x1 - x0 - 2 * o.search) // tx
            tiles.append((r0, r1, [x for x in range(c0, c1) if not (ex0 <= x < ex1)]))
    results = []
    for run in o.runs:
        try:
            r = read_run(run, o, w, h, rows, x0, x1, cols, tiles)
        except (OSError, ValueError) as e:
            print(f'{os.path.basename(os.path.normpath(run))}: unreadable ({e})'); r = None
        if r: results.append(r)
    if results:
        m = median([r['shift'] for r in results])
        for r in results:
            if m and abs(r['shift'] - m) > 0.1 * abs(m): r['reasons'].append(f'mean shift {r["shift"]:.2f} against the runs\' median {m:.2f}')
        # The yaw range every valid run covers, and each run's pairs inside it (the figure compared between the GI qualities); an INVALID
        # run neither narrows the range nor is compared (the step review's L2).
        valid = [r for r in results if not r['reasons'] and any(y is not None for y, _, _ in r['keyed'])]
        common = None
        if len(valid) > 1:
            lo = max(min(y for y, _, _ in r['keyed'] if y is not None) for r in valid)
            hi = min(max(y for y, _, _ in r['keyed'] if y is not None) for r in valid)
            if lo < hi: common = (lo, hi)
            starts = [r['first_yaw'] for r in valid]; step = median([r['step'] for r in valid if r['step']])
            ms = median(starts)
            for r in valid:
                if step and abs(r['first_yaw'] - ms) > 1.5 * step:
                    r['notes'].append(f'the burst starts {abs(r["first_yaw"] - ms) / step:.1f} frames from the runs\' median yaw: compare over the common range')
        for r in results:
            if common and r in valid:
                sel = [(v, t) for y, v, t in r['keyed'] if y is not None and common[0] <= y <= common[1]]
                r['common'] = (sum(v for v, _ in sel) / len(sel), sum(t for _, t in sel) / len(sel), len(sel)) if sel else None
            else: r['common'] = None
        print('summary (run, aligned mean, aligned median, tiles median, mean shift, yaw turned, first yaw, validity):')
        for r in results:
            print(f'  {r["name"]:40s} {r["aligned"]:.3f} {r["aligned_median"]:.3f} {r["tiles"]:.3f} {r["shift"]:7.2f} {r["turned"]:8.4f} '
                  + (f'{r["first_yaw"]:8.4f}  ' if r['first_yaw'] is not None else '       -  ')
                  + ('INVALID: ' + '; '.join(r['reasons']) if r['reasons'] else 'valid') + ''.join('; note: ' + t for t in r['notes']))
        if common:
            print(f'over the yaw range every valid run covers ({common[0]:.4f} to {common[1]:.4f} rad; the figure to compare):')
            for r in results:
                c = r['common']
                print(f'  {r["name"]:40s} ' + ('INVALID, not compared' if r['reasons'] else f'aligned mean {c[0]:.3f}, tiles median {c[1]:.3f}, {c[2]} pairs' if c else 'no pair in the range'))
        else:
            print('no yaw range common to the valid runs (fewer than two, yaw rows missing or bursts apart): the bursts cannot be compared')

if __name__ == '__main__':
    main()
