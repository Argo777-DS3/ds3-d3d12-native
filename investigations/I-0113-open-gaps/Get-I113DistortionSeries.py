# I-0113 tranche 23 (gap 22): reads the ffx_distortion_series rows (and, Reference with ffx-draw-observe.flag, the
# ffx_distortion_series_copy rows) of a run's startup.jsonl and prints, frame by frame (a frame: the draws of one burst, a gap of
# more than 4 ms between two rows starts the next), each distortion quad in draw order: its screen centre and size (from
# g_worldViewProjectionMatrix applied to the unit quad's corners, x right, y down, in [0,1]), g_color.w, g_waveStrength and the two
# scroll distances; then, over the window, how many frames had two quads whose screen rectangles overlap, and which of them was
# drawn last (the younger by strength trend, or the older). Usage: python -I Get-I113DistortionSeries.py <startup.jsonl> [frames]
import json, sys

def corners(m):
    # HLSL: dp4 r.x = (p,1)·row12 ... rows are cb0[12..15] = words 0..3, 4..7, 8..11, 12..15 of the matrix as logged
    out = []
    for x, y in ((-0.5, 0.5), (0.5, 0.5), (-0.5, -0.5), (0.5, -0.5)):
        p = (x, y, 0.0, 1.0)
        cx = sum(p[i] * m[i] for i in range(4)); cy = sum(p[i] * m[4 + i] for i in range(4)); cw = sum(p[i] * m[12 + i] for i in range(4))
        if not cw:
            return None
        out.append((0.5 + 0.5 * cx / cw, 0.5 - 0.5 * cy / cw))
    return out

def main():
    path = sys.argv[1]; limit = int(sys.argv[2]) if len(sys.argv) > 2 else 12
    rows = []
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"ffx_distortion_series' not in line:
                continue
            try:
                rows.append(json.loads(line))
            except ValueError:
                pass
    draws = [r for r in rows if r['event'] == 'ffx_distortion_series']
    copies = [r for r in rows if r['event'] == 'ffx_distortion_series_copy']
    print(f'{len(draws)} distortion draws, {len(copies)} colour copies in the window, mode {draws[0]["mode"] if draws else "?"}')
    events = sorted([(r['ms'], 'draw', r) for r in draws] + [(r['ms'], 'copy', r) for r in copies], key=lambda e: e[0])
    frames, current, last = [], [], None
    for ms, kind, r in events:
        if last is not None and ms - last > 4 and current:
            frames.append(current); current = []
        current.append((kind, r)); last = ms
    if current:
        frames.append(current)
    overlap_frames = 0
    for n, frame in enumerate(frames):
        quads = []
        for kind, r in frame:
            if kind == 'copy':
                if n < limit:
                    print(f'  copy {r["kind"]} pass {r["pass"]:X} format {r["format"]} {r["size"]} thread {r["thread"]}')
                continue
            c = corners(r['wvp']) if r.get('wvp') and None not in r['wvp'] else None
            if c:
                xs = [p[0] for p in c]; ys = [p[1] for p in c]
                rect = (min(xs), min(ys), max(xs), max(ys))
            else:
                rect = None
            quads.append((r, rect))
        over = False
        for i in range(len(quads)):
            for j in range(i + 1, len(quads)):
                a, b = quads[i][1], quads[j][1]
                if a and b and a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]:
                    over = True
        overlap_frames += over
        if n < limit:
            print(f'frame {n} at {frame[0][1]["ms"]:.1f} ms: {len(quads)} quads, overlap {over}')
            for r, rect in quads:
                w = r['wave']
                where = f'[{rect[0]:.3f},{rect[1]:.3f} - {rect[2]:.3f},{rect[3]:.3f}]' if rect else 'none'
                print(f'  o{r["o"]} t{r["thread"]} blend {r["blend"]} a {r["color"][3]:.3f} strength {w[6]:.3f} scroll {w[8]:.4f},{w[9]:.4f} rect {where}')
    print(f'{len(frames)} frames, {overlap_frames} with overlapping quads')

if __name__ == '__main__':
    main()
