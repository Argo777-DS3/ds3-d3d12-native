# I-0113 tranche 31 (gap 16): the 608 radial-blur quads at a Fire Orb burst, from ffx-series.flag's rows (both modes): the
# ffx_distortion_series rows whose FXR blend type (+0x960) is not 0 (the burst's blended quads), and the copies made in the same
# window (the original's ffx_distortion_series_copy region copies with their boxes, the native's ffx_distortion_series_copyrect
# rectangles, turned into pixels as the renderer does: floored and ceiled, widened by 16 pixels, clamped). Per window (one a
# cast): the quads by blend type, their screen rectangles' size in pixels (g_worldViewProjectionMatrix on the unit quad's
# corners), g_screenCenter, g_radialDistance, the sampling clamp (g_ps_RadialBlurSamplingLengthMaxX/Y), g_ps_ColorBlendType,
# g_color and g_vColorScale as min / median / max; the copies' box sizes alike; the reach of the 16 taps in pixels
# (|centre - quad centre| x radialDistance, clamped) against the box's 16-pixel margin.
# Usage: python -I Get-I113RadialSeries.py <startup.jsonl> <width> <height> (the run's render size: the rows carry none).
import json
import math
import sys
from collections import defaultdict


def corners(m):
    out = []
    for x, y in ((-0.5, 0.5), (0.5, 0.5), (-0.5, -0.5), (0.5, -0.5)):
        p = (x, y, 0.0, 1.0)
        cx = sum(p[i] * m[i] for i in range(4)); cy = sum(p[i] * m[4 + i] for i in range(4)); cw = sum(p[i] * m[12 + i] for i in range(4))
        if not cw:
            return None
        out.append((0.5 + 0.5 * cx / cw, 0.5 - 0.5 * cy / cw))
    return out


def stats(values):
    v = sorted(x for x in values if x is not None and math.isfinite(x))
    if not v:
        return 'none'
    return '%.4g / %.4g / %.4g (n %d)' % (v[0], v[len(v) // 2], v[-1], len(v))


def main(path, width=1920, height=1080):
    windows = defaultdict(lambda: {'quads': [], 'copies': [], 'rects': []})
    window = -1
    last_ms = None
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"ffx_distortion_series' not in line:
                continue
            try:
                r = json.loads(line)
            except ValueError:
                continue
            ms = r.get('ms')
            if ms is None:
                continue
            if last_ms is None or ms < last_ms - 100:
                window += 1
            last_ms = ms
            w = windows[window]
            if r['event'] == 'ffx_distortion_series' and r.get('blend'):
                w['quads'].append(r)
            elif r['event'] == 'ffx_distortion_series_copy' and 'box' in r:
                w['copies'].append(r)
            elif r['event'] == 'ffx_distortion_series_copyrect':
                w['rects'].append(r)
    print(path)
    for k in sorted(windows):
        w = windows[k]
        q = w['quads']
        by_blend = defaultdict(int)
        sizes_w, sizes_h, reach = [], [], []
        for r in q:
            by_blend[r['blend']] += 1
            c = corners(r['wvp']) if r.get('wvp') and None not in r['wvp'] else None
            rad = r.get('radial') or [None] * 4
            if c:
                xs = [p[0] for p in c]; ys = [p[1] for p in c]
                sizes_w.append((max(xs) - min(xs)) * width); sizes_h.append((max(ys) - min(ys)) * height)
                if None not in rad[:3]:
                    mx = (max(xs) + min(xs)) / 2; my = (max(ys) + min(ys)) / 2
                    dx = abs(rad[0] - mx) * rad[2]; dy = abs(rad[1] - my) * rad[2]
                    lim = r.get('radial_max') or [0, 0]
                    if lim[0] is not None and lim[0] >= 1e-4:
                        dx = min(dx, lim[0])
                    if lim[1] is not None and lim[1] >= 1e-4:
                        dy = min(dy, lim[1])
                    reach.append(max(dx * width, dy * height))
        print('  window %d: blended quads %d %s' % (k, len(q), dict(by_blend)))
        if not q:
            continue
        col = lambda i, key: [(r.get(key) or [None] * 4)[i] for r in q]
        print('    rect width px  ', stats(sizes_w))
        print('    rect height px ', stats(sizes_h))
        print('    screenCenter x ', stats(col(0, 'radial')), ' y ', stats(col(1, 'radial')))
        print('    radialDistance ', stats(col(2, 'radial')))
        print('    sampling max x ', stats(col(0, 'radial_max')), ' y ', stats(col(1, 'radial_max')))
        print('    colour blend    %s' % dict((b, sum(1 for r in q if r.get('colour_blend') == b)) for b in set(r.get('colour_blend') for r in q)))
        for i, n in enumerate('rgba'):
            print('    g_color.%s       %s   g_vColorScale.%s %s' % (n, stats(col(i, 'color')), n, stats(col(i, 'scale'))))
        print('    taps reach px  ', stats(reach), '(against the copy box margin of 16)')
        if w['copies']:
            bw = [c['box'][2] - c['box'][0] for c in w['copies']]; bh = [c['box'][3] - c['box'][1] for c in w['copies']]
            print('    original region copies %d: box w %s, h %s' % (len(w['copies']), stats(bw), stats(bh)))
        if w['rects']:
            pw, ph = [], []
            for c in w['rects']:
                if c['kind'] != 0 or any(v is None for v in c['rect']):   # the final review's L2: null (non-finite) rectangles copy nothing
                    continue
                l, t, rr, b = c['rect']
                left = max(0, math.floor(l * width) - 16); top = max(0, math.floor(t * height) - 16)
                right = min(width, math.ceil(rr * width) + 16); bottom = min(height, math.ceil(b * height) + 16)
                pw.append(right - left); ph.append(bottom - top)
            print('    native copy rects %d (kind 0 %d): box w %s, h %s' % (len(w['rects']), len(pw), stats(pw), stats(ph)))


if __name__ == '__main__':
    a = sys.argv[1:]
    if len(a) != 3:
        sys.exit('usage: Get-I113RadialSeries.py <startup.jsonl> <width> <height>')
    main(a[0], int(a[1]), int(a[2]))
