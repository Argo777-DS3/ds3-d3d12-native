"""Video sheet: a game film turned into a few pictures and numbers a thread can read in one turn (VIDEO-STUDY, Thomas 2026-11-07).

Claude sees stills, never motion: this tool turns time into space. Python 3 standard library only; ffmpeg does the pixel work.
For one film, or a pair (A = native, B = original), it writes into --out:
  report.txt        the numbers: per region, the motion's quantiles; its own jumps (not at a camera turn, not with a control
                    region, not at an encoder keyframe) with their period; the frames frozen while the rest moves; the blinks
                    (a brightness step that comes back within 15 frames; a flicker is many short ones); the camera turns, the
                    whole-picture hitches and the repeated pictures; without a region, the frames where many blocks of the
                    picture jump at once; in a pair, region by region, A against B
  report.json       the same, for a script
  views-A.png       one frame per view (the middle of each stretch between camera turns)
  curves.png        one band per region (and the whole frame): motion over time, log scale; red ticks = the region's own
                    jumps; grey ticks every second, white every ten
  footprint-A.png   where the jumps are: each cell's motion on the jump frames over its median, red over a frame of the film
  slit-A-x<X>.png   slit-scans: one column (--slit-x) or row (--slit-y) of pixels taken at every frame (every n-th past 1,600
                    frames) and laid side by side; continuous motion draws smooth lines, a freeze flat stretches, a restart a seam
Frame numbers count the decoded frames from the analysed start (frame 0 = the first decoded frame at or after --start); motion at
frame i is the change from frame i-1 to frame i. Units: grey steps on the 0-255 scale ffmpeg's gray16 conversion gives (a film in
limited range is expanded to full range: one step here is about 0.86 of the film's luma code), averaged over the cells.

Usage: python -I videosheet.py FILM [FILM2] --out DIR [--start S] [--dur D] [--roi name=x:y:w:h ...] [--control name=x:y:w:h ...]
       [--shift dx:dy] [--slit-x X] [--slit-y Y] [--k 3]
A control is a region that moves all the time and should not show the defect (an animated character, foliage, snow): a jump it
shares is not the region's own. With two films, --shift moves every region and slit for the second film.
"""
import argparse, bisect, glob, json, math, os, random, shutil, statistics as st, struct, subprocess, sys, tempfile, zlib
from array import array

NOWIN = getattr(subprocess, 'CREATE_NO_WINDOW', 0)
STEP = 257.0  # one grey step in gray16
SLIT_MAX = 1600


def find_ffmpeg():
    p = shutil.which('ffmpeg')
    if p: return p
    g = glob.glob(os.path.join(os.environ.get('LOCALAPPDATA', ''), 'Microsoft', 'WinGet', 'Packages', 'Gyan.FFmpeg_*', 'ffmpeg-*', 'bin', 'ffmpeg.exe'))
    if not g: sys.exit('videosheet: ffmpeg not found')
    return g[0]


def ffprobe_of(ff):
    return os.path.join(os.path.dirname(ff), 'ffprobe.exe' if ff.lower().endswith('.exe') else 'ffprobe')


def probe(ff, film):
    r = subprocess.run([ffprobe_of(ff), '-v', 'error', '-select_streams', 'v:0', '-show_entries', 'stream=width,height,r_frame_rate,start_time,codec_name',
                        '-of', 'csv=p=0', film], capture_output=True, text=True, creationflags=NOWIN)
    out = r.stdout.strip().split(',')
    if r.returncode or len(out) < 5: sys.exit(f'videosheet: cannot read {film}: {r.stderr.strip()[:200]}')
    codec = out[0]
    out = out[1:]  # csv order: codec_name, width, height, r_frame_rate, start_time
    a, b = out[2].split('/')
    if float(a) <= 0 or float(b) <= 0: sys.exit(f'videosheet: no frame rate in {film}')
    try: t0 = float(out[3])
    except ValueError: t0 = 0.0
    return int(out[0]), int(out[1]), float(a) / float(b), t0, codec


def keyframes(ff, film, start, n):
    """The encoder's keyframes as frame numbers of the analysed stretch, from the packets (no decode): a decoded frame's number
    is the count of packets at or after --start before it, as the decode keeps every frame. x264's default interval is 250
    frames, NVIDIA's clips 30: a still region changes a little at each, which looks like a periodic jump."""
    r = subprocess.run([ffprobe_of(ff), '-v', 'error', '-select_streams', 'v:0', '-show_entries', 'packet=pts_time,flags',
                        '-of', 'csv=p=0', film], capture_output=True, text=True, creationflags=NOWIN)
    if r.returncode: return None
    pk = []
    for line in r.stdout.split():
        f = line.split(',')
        try: pk.append((float(f[0]), 'K' in f[1]))
        except (ValueError, IndexError): continue
    pk.sort()
    if not pk: return []
    t0 = pk[0][0]
    pk = [p for p in pk if p[0] - t0 >= start - 1e-6]
    return [i for i, (_, key) in enumerate(pk[:n]) if key]


def ff_raw(ff, film, vf, frame_bytes, start, dur):
    cmd = [ff, '-hide_banner', '-loglevel', 'error']
    if start: cmd += ['-ss', str(start)]
    cmd += ['-i', film]
    if dur: cmd += ['-t', str(dur)]
    cmd += ['-vf', vf, '-fps_mode', 'passthrough', '-f', 'rawvideo', '-']
    with tempfile.TemporaryFile() as errf:  # a file, not a pipe: a film with decode errors writes much to stderr
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=errf, creationflags=NOWIN)
        out = []
        while True:
            b = p.stdout.read(frame_bytes)
            if len(b) < frame_bytes: break
            out.append(array('H', b))
        rc = p.wait()
        errf.seek(0)
        err = errf.read().decode('utf-8', 'replace')
    if rc != 0: sys.exit(f'videosheet: ffmpeg failed on {film}: {err.strip()[:300]}')
    return out, err.count(chr(10))


def ff_png(ff, film, vf, out, start, dur=None):
    cmd = [ff, '-hide_banner', '-loglevel', 'error', '-y']
    if start: cmd += ['-ss', str(start)]
    cmd += ['-i', film]
    if dur: cmd += ['-t', str(dur)]
    cmd += ['-vf', vf, '-frames:v', '1', '-fps_mode', 'passthrough', out]
    r = subprocess.run(cmd, creationflags=NOWIN, capture_output=True, text=True)
    if r.returncode: raise RuntimeError(r.stderr.strip()[:300])


def write_png(path, w, h, rgb):
    """rgb: bytearray of w*h*3."""
    raw = b''.join(b'\x00' + bytes(rgb[y * w * 3:(y + 1) * w * 3]) for y in range(h))
    ck = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + ck(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + ck(b'IDAT', zlib.compress(raw, 6)) + ck(b'IEND', b''))


def running_median(v, half=15):
    out, win = [], sorted(v[:min(len(v), half + 1)])
    lo_i, hi_i = 0, min(len(v), half + 1)
    for i in range(len(v)):
        want_lo, want_hi = max(0, i - half), min(len(v), i + half + 1)
        while hi_i < want_hi: bisect.insort(win, v[hi_i]); hi_i += 1
        while lo_i < want_lo: win.pop(bisect.bisect_left(win, v[lo_i])); lo_i += 1
        out.append(win[len(win) // 2])
    return out


def local_events(v, k, floor):
    """Indices where v jumps over k times its running median (window 31) and by more than floor; neighbours within 2 merged.
    'frame' is the event's first index, 'ratio' its peak over the running median (floored at 0.25 step)."""
    m = running_median(v)
    hits = [i for i in range(len(v)) if v[i] > k * m[i] and v[i] - m[i] > floor]
    ev = []
    for i in hits:
        r = v[i] / max(m[i], 0.25 * STEP)
        if ev and i - ev[-1]['last'] <= 2:
            ev[-1]['last'] = i
            if v[i] > ev[-1]['peak']: ev[-1]['peak'], ev[-1]['ratio'] = v[i], r
        else:
            ev.append({'frame': i, 'last': i, 'peak': v[i], 'ratio': r})
    return ev


def _best_lattice(fr, pmax):
    """P in [4, pmax] (by quarter frames under 64, half frames under 200, whole frames above), whose lattice (phase free, tolerance max(1, 4 % of P)) is the least likely by chance:
    the score is -log10 of the binomial tail P(X >= hits | n events, one event's chance (2 tol + 1) / P), that chance under
    25 %. A fraction of the true period can still score best
    on the tolerance's rounding alone: period_of multiplies it back (the gcd of the on-lattice steps)."""
    n = len(fr)
    best = []
    grid = ([q / 4 for q in range(16, min(4 * pmax, 255) + 1)] + [q / 2 for q in range(128, min(2 * pmax, 399) + 1)]
            + [float(q) for q in range(200, pmax + 1)])
    for P in grid:
        tol = max(1, int(round(0.04 * P)))
        chance = (2 * tol + 1) / P
        if chance > 0.25: continue
        ph = sorted(e % P for e in fr)
        ph2 = ph + [x + P for x in ph]
        hit, j = 0, 0
        for i in range(n):  # the most phases in a window [x, x + 2 tol] starting at a phase, circular
            if j < i: j = i
            while j < len(ph2) and ph2[j] <= ph[i] + 2 * tol: j += 1
            hit = max(hit, j - i)
        best.append((_tail_score(n, hit, chance), P, hit, chance))
    if not best: return None
    return max(best)


def _tail_score(n, k, c):
    """-log10 P(X >= k) for X binomial(n, c)."""
    t = sum(math.comb(n, i) * c ** i * (1 - c) ** (n - i) for i in range(k, n + 1))
    return -math.log10(max(t, 1e-300))


def period_of(frames, fps, alpha=0.05, sims=500):
    """A period of the events: the best lattice (_best_lattice, P up to half the span and 10 s), refined to a fractional P by a
    least-squares fit of its on-lattice events; when every on-lattice step is a multiple m of P (the whole-frame tolerance can
    make P / m score best), P is m times longer. Reported only with at least 6 events, half of them on it, and when under alpha of
    random trains (as many events, uniform over the same span, the same search) score as well: the false-alarm share. A burst
    (a chain of events under 10 frames apart) counts as one event."""
    fr, last = [], None
    for f in sorted(set(frames)):  # a burst (a chain of events under 10 frames apart) counts once, at its first frame
        if last is None or f - last >= 10: fr.append(f)
        last = f
    fr = fr[:80]  # bounded cost: the first 80 events
    n = len(fr)
    if n < 6: return None
    pmax = min((fr[-1] - fr[0]) // 2, int(10 * fps))
    b = _best_lattice(fr, pmax)
    if not b: return None
    score, P, hit, chance = b
    rng = random.Random(1)
    beat, done = 0, 0
    if hit / n >= 0.5:
        while done < sims:  # stops early once the share is clearly over alpha
            r = _best_lattice(sorted(rng.sample(range(fr[0], fr[-1] + 1), n)), pmax)
            done += 1
            if r is not None and r[0] >= score: beat += 1
            if done >= 50 and beat / done > 3 * alpha: break  # clearly over the gate
            if (beat + 1.645 * math.sqrt(beat + 1) + 2) / done < alpha: break  # clearly under it (about a 95 % bound)
    false_alarm = beat / done if done else 1.0
    if hit / n < 0.5 or false_alarm >= alpha:
        return {'interval': None, 'events': n, 'on_lattice': hit, 'false_alarm': false_alarm, 'beat': beat, 'draws': done, 'alpha': alpha}
    tol = max(1, int(round(0.04 * P)))
    near = lambda e, a0: min((e - a0) % P, P - (e - a0) % P) <= tol
    anchor = max(fr, key=lambda a0: sum(1 for e in fr if near(e, a0)))
    on = [(round((e - anchor) / P), e) for e in fr if near(e, anchor)]
    m = 0
    for k, _ in on: m = math.gcd(m, abs(k))
    if m > 1:  # every on-lattice event m steps apart: the period is m P
        P *= m
        on = [(k // m, e) for k, e in on]
    mk, me = st.mean(k for k, _ in on), st.mean(e for _, e in on)
    var = sum((k - mk) ** 2 for k, _ in on)
    Pf = sum((k - mk) * (e - me) for k, e in on) / var if var else float(P)
    return {'interval': Pf, 'lattice': P, 'on_lattice': hit, 'events': n, 'fit': hit / n, 'false_alarm': false_alarm,
            'beat': beat, 'draws': done, 'alpha': alpha}


def period_text(per, fps):
    if not per: return 'no period (under 6 events)'
    if per['interval'] is None:
        if not per.get('draws'): return f"no clear period ({per['events']} events, at most {per['on_lattice']} on one lattice)"
        return (f"no clear period ({per['events']} events, at most {per['on_lattice']} on one lattice; random trains of as many "
                f"events did as well in {per['beat']} of {per['draws']} draws)")
    return (f"period {per['interval']:.1f} frames ({per['interval']/fps:.2f} s): {per['on_lattice']} of {per['events']} events on it; "
            f"random trains of as many events did as well in {per['beat']} of {per['draws']} draws (gate {100*per['alpha']:g} %)")


def parse_rect(s):
    name, r = s.split('=', 1) if '=' in s else (s, s)
    try: x, y, w, h = (int(t) for t in r.split(':'))
    except ValueError: sys.exit(f'videosheet: a region is name=x:y:w:h, not {s}')
    if w <= 0 or h <= 0: sys.exit(f'videosheet: empty region {s}')
    return name, (x, y, w, h)


class Film:
    def __init__(self, ff, path, a, shift=(0, 0)):
        if not os.path.isfile(path): sys.exit(f'videosheet: no film {path}')
        self.path, self.shift = path, shift
        self.W, self.H, self.fps, _, self.codec = probe(ff, path)
        self.cs = max(8, self.W // 160)  # cell size in pixels
        self.gw, self.gh = self.W // self.cs, self.H // self.cs
        cw, ch = self.gw * self.cs, self.gh * self.cs
        n = self.gw * self.gh
        vf = f'crop={cw}:{ch}:0:0,format=gray16le,tblend=all_mode=difference,scale={self.gw}:{self.gh}:flags=area,format=gray16le'
        self.mot, e1 = ff_raw(ff, path, vf, n * 2, a.start, a.dur)  # mot[j] = change from frame j to frame j+1
        vf = f'crop={cw}:{ch}:0:0,format=gray16le,scale={self.gw}:{self.gh}:flags=area,format=gray16le'
        self.lum, e2 = ff_raw(ff, path, vf, n * 2, a.start, a.dur)
        self.decode_errors = max(e1, e2)
        if len(self.lum) < 3: sys.exit(f'videosheet: under 3 frames decoded from {path} (--start past the end?)')
        self.mot = self.mot[:len(self.lum) - 1]
        self.N = len(self.lum)
        self.glob = [sum(f) / n for f in self.mot]
        # the share of cells moving well over their usual motion (4 x their median, at least 2 steps), per change: a camera
        # turn moves most of the picture; snow, water or an animated scene that always moves does not count
        sample = self.mot[::max(1, len(self.mot) // 600)]
        lim = [max(4 * sorted(f[c] for f in sample)[len(sample) // 2], 2 * STEP) for c in range(n)]
        self.moving = [sum(1 for c in range(n) if f[c] > lim[c]) / n for f in self.mot]
        ks = keyframes(ff, path, a.start or 0.0, self.N)
        self.keys_read = ks is not None
        self.keys = ks or []
        # an encoder keyframe matters only where it shows: the still cells (median motion under 0.05 step) jump at it (x264,
        # NVIDIA's encoder; not a lossless codec). Such a keyframe gets a bound: a jump there is set apart only under
        # max(1.5 steps, 4 x the still cells' median change at it), at most 3 steps; a bigger one stays a jump
        still = [c for c in range(n) if sorted(f[c] for f in sample)[len(sample) // 2] < 0.05 * STEP]
        self.still_cells = len(still)
        self.key_bound = {}
        self.lossless = self.codec in ('ffv1', 'utvideo', 'huffyuv', 'ffvhuff', 'magicyuv', 'png', 'rawvideo', 'qtrle')
        if len(still) >= max(10, n // 100) and not self.lossless:
            # the lower quartile of the still cells' change: at a keyframe that shows, nearly every still cell changes a
            # little; a region of them jumping by itself moves the mean, not the lower quartile
            q1 = lambda f: sorted(f[c] for c in still)[len(still) // 4]
            base = st.median(q1(self.mot[j]) for j in range(0, len(self.mot), max(1, len(self.mot) // 300)))
            for kf in self.keys:
                if 1 <= kf < self.N:
                    qk = q1(self.mot[kf - 1])
                    if qk > max(3 * base, 0.01 * STEP):
                        med = sorted(self.mot[kf - 1][c] for c in still)[len(still) // 2]
                        for f in (kf - 1, kf, kf + 1): self.key_bound[f] = min(3 * STEP, max(1.5 * STEP, 4 * med))

    def cells(self, rect):
        x, y, w, h = rect
        x += self.shift[0]; y += self.shift[1]
        c0, c1 = max(0, x // self.cs), min(self.gw, (x + w + self.cs - 1) // self.cs)
        r0, r1 = max(0, y // self.cs), min(self.gh, (y + h + self.cs - 1) // self.cs)
        return [r * self.gw + c for r in range(r0, r1) for c in range(c0, c1)]

    def series(self, cells):
        return [sum(f[c] for c in cells) / len(cells) for f in self.mot]

    def lseries(self, cells):
        return [sum(f[c] for c in cells) / len(cells) for f in self.lum]


def blinks(L, skip, W=15):
    """Brightness steps that come back: a step over the threshold at frame i, then the level back within 35 % of the step of
    its level before, within W frames. The threshold is max(0.5 step, 6 x the lower quartile of the one- and two-frame changes):
    the quiet part of the series, so a flicker present most of the time does not raise its own threshold."""
    d1 = [abs(L[i + 1] - L[i]) for i in range(len(L) - 1)]
    d2 = [abs(L[i + 2] - L[i]) for i in range(len(L) - 2)]
    q = lambda xs: sorted(xs)[len(xs) // 4] if xs else 0
    thr = max(0.5 * STEP, 6 * min(q(d1), q(d2)))
    out = []
    i = 1
    while i < len(L) - 1:
        step = L[i] - L[i - 1]
        if abs(step) > thr and i not in skip:
            back = next((j for j in range(i + 1, min(len(L), i + W + 1)) if abs(L[j] - L[i - 1]) < 0.35 * abs(step)), None)
            if back is not None:
                out.append((i, back - i, step / STEP))
                i = back + 1  # one blink per excursion: a flicker counts its cycles, a dip in several steps once
                continue
        i += 1
    return out, thr


def analyse_region(film, name, rect, controls, k, turn_frames, ctrl_events):
    cells = film.cells(rect)
    if not cells: return {'name': name, 'error': 'outside the frame'}
    v = film.series(cells)
    ev = local_events(v, k, 1.0 * STEP)
    fr = lambda e: e['frame'] + 1  # the later frame of the change
    cev = set()
    for cn, _ in controls:
        if cn != name: cev |= ctrl_events.get(cn, set())
    own, at_key, at_turn, with_ctrl = [], 0, 0, 0
    for e in ev:
        f = fr(e)
        if f in turn_frames: at_turn += 1
        elif f in cev: with_ctrl += 1
        elif f in film.key_bound and e['peak'] - st.median(v) <= film.key_bound[f]: at_key += 1
        else: own.append(e)
    q = sorted(v)
    pick = lambda p: q[int(p * (len(q) - 1))] / STEP
    live = [i for i in range(len(v)) if (i + 1) not in turn_frames]
    # frozen: the region under 0.1 step while the whole frame moves over 0.3 step (meaningful for a region that should move)
    frozen = sum(1 for i in live if v[i] < 0.1 * STEP and film.glob[i] > 0.3 * STEP)
    bl, thr = blinks(film.lseries(cells), turn_frames)
    secs = len(live) / film.fps
    return {
        'name': name, 'rect': rect, 'cells': len(cells),
        'median': pick(0.5), 'p90': pick(0.9), 'p99': pick(0.99), 'max': pick(1.0),
        'events': [fr(e) for e in ev], 'own_events': [fr(e) for e in own],
        'own_peaks': [round(e['ratio'], 1) for e in own],
        'removed': {'camera_turn': at_turn, 'with_control': with_ctrl, 'keyframe': at_key},
        'period': period_of([fr(e) for e in own], film.fps),
        'frozen_frames': frozen, 'frozen_share': frozen / max(1, len(live)),
        'blinks': [{'frame': b[0], 'frames': b[1], 'step': round(b[2], 2)} for b in bl],
        'blinks_per_s': len(bl) / secs if secs else 0, 'blink_threshold': thr / STEP,
        'series': v,
    }


def turns(film):
    """Camera turns: at least 5 changes where over half of the cells move well over their usual motion, merged across gaps
    under 10 frames. A shorter whole-picture jump is a hitch (of the capture, the present or the light), reported, not a turn.
    A change over half the picture for 5 frames or more that is not the camera (a light change, a fog fade) counts as a turn too."""
    runs = []
    for i, m in enumerate(film.moving):
        if m > 0.5:
            if runs and i - runs[-1][1] < 10: runs[-1][1] = i
            else: runs.append([i, i])
    tr = [(a + 1, b + 1) for a, b in runs if b - a + 1 >= 5 and sum(1 for j in range(a, b + 1) if film.moving[j] > 0.5) >= 5]
    hitches = [(a + 1, b + 1) for a, b in runs if (a + 1, b + 1) not in tr]
    return tr, hitches


def auto_blocks(film, k, turn_frames, fps, top=8):
    """Blocks of 4x4 cells and their jumps (not at a camera turn or an encoder keyframe), for a film without given regions; then
    the frames where many blocks jump together (a layer or an area of the picture updated at once)."""
    bw, bh = max(1, film.gw // 4), max(1, film.gh // 4)
    res = []
    for by in range(bh):
        for bx in range(bw):
            cells = [(by * 4 + r) * film.gw + bx * 4 + c for r in range(4) for c in range(4) if by * 4 + r < film.gh and bx * 4 + c < film.gw]
            ser = film.series(cells)
            ev = local_events(ser, k, 1.5 * STEP)
            md = st.median(ser)
            own = [e['frame'] + 1 for e in ev if e['frame'] + 1 not in turn_frames
                   and not (e['frame'] + 1 in film.key_bound and e['peak'] - md <= film.key_bound[e['frame'] + 1])]
            if own: res.append({'rect': (bx * 4 * film.cs, by * 4 * film.cs, 4 * film.cs, 4 * film.cs), 'own_events': own})
    res.sort(key=lambda r: -len(r['own_events']))
    for r in res[:top]: r['period'] = period_of(r['own_events'], fps, alpha=0.01)  # many lines tested: a stricter gate
    by_frame = {}
    for bi, r in enumerate(res):
        for f in r['own_events']: by_frame.setdefault(f, set()).add(bi)
    clusters = []  # frames at most 2 after the cluster's first frame; each block counted once in a cluster
    for f in sorted(by_frame):
        if clusters and f - clusters[-1]['first'] <= 2:
            clusters[-1]['blocks'] |= by_frame[f]
            if len(by_frame[f]) > clusters[-1]['peakn']: clusters[-1]['peak'], clusters[-1]['peakn'] = f, len(by_frame[f])
        else: clusters.append({'first': f, 'peak': f, 'peakn': len(by_frame[f]), 'blocks': set(by_frame[f])})
    need = max(5, int(0.02 * bw * bh))
    common = [(c['peak'], len(c['blocks'])) for c in clusters if len(c['blocks']) >= need]
    return res[:top], res, common, bw * bh, need


def draw_curves(path, bands, frames, fps):
    W = min(frames, 1600)
    BH, GAP = 80, 6
    H = len(bands) * (BH + GAP) + 14
    img = bytearray(W * H * 3)
    def px(x, y, c):
        if 0 <= x < W and 0 <= y < H: img[(y * W + x) * 3:(y * W + x) * 3 + 3] = bytes(c)
    top = max(max(b['series']) for b in bands) or 1
    for bi, b in enumerate(bands):
        y0 = bi * (BH + GAP)
        s = b['series']
        for x in range(W):
            a, z = x * len(s) // W, max(x * len(s) // W + 1, (x + 1) * len(s) // W)
            val = max(s[a:z]) if a < len(s) else 0
            hgt = int((BH - 4) * math.log1p(val) / math.log1p(top))
            for y in range(BH - hgt, BH): px(x, y0 + y, b.get('colour', (200, 200, 200)))
            px(x, y0 + BH - 1, (60, 60, 60))
        for f in b.get('marks', []):
            x = f * W // max(1, frames)
            for y in range(0, 6): px(x, y0 + y, (255, 40, 40))
    for f in range(0, frames, max(1, int(round(fps)))):
        x = f * W // max(1, frames)
        ten = (f // max(1, int(round(fps)))) % 10 == 0
        for y in range(H - 14, H - (2 if ten else 8)): px(x, y, (255, 255, 255) if ten else (120, 120, 120))
    write_png(path, W, H, img)


def footprint(ff, film, frames_hit, out, a):
    """Each cell's mean motion on the hit frames over its median (floored at 0.25 step), as red over a frame of the film."""
    n = film.gw * film.gh
    hit = [f - 1 for f in frames_hit if 0 < f < film.N]
    if not hit: return None
    sample = film.mot[::max(1, len(film.mot) // 600)]
    med = [max(sorted(f[c] for f in sample)[len(sample) // 2], 0.25 * STEP) for c in range(n)]
    ratio = [sum(film.mot[j][c] for j in hit) / len(hit) / med[c] for c in range(n)]
    hot = sum(1 for r in ratio if r > 4)
    s = max(2, 800 // film.gw)  # background: the film's frame at the first hit, grey, about 800 px wide
    bgw, bgh = film.gw * s, film.gh * s
    tmp = out + '.bg.raw'
    cmd = [ff, '-hide_banner', '-loglevel', 'error', '-y']
    if a.start: cmd += ['-ss', str(a.start)]
    cmd += ['-i', film.path, '-vf', f"select='eq(n\\,{hit[0] + 1})',scale={bgw}:{bgh}:flags=area,format=gray", '-frames:v', '1',
            '-fps_mode', 'passthrough', '-f', 'rawvideo', tmp]
    r = subprocess.run(cmd, creationflags=NOWIN, capture_output=True, text=True)
    if r.returncode: raise RuntimeError(r.stderr.strip()[:300])
    bg = open(tmp, 'rb').read(); os.remove(tmp)
    img = bytearray(bgw * bgh * 3)
    for y in range(bgh):
        for x in range(bgw):
            g = bg[y * bgw + x] if y * bgw + x < len(bg) else 0
            red = max(0, min(1, (ratio[(y // s) * film.gw + x // s] - 1) / 7))
            img[(y * bgw + x) * 3:(y * bgw + x) * 3 + 3] = bytes((min(255, int(g * 0.6 + 255 * red)), int(g * 0.6 * (1 - red)), int(g * 0.6 * (1 - red))))
    write_png(out, bgw, bgh, img)
    return {'cells_over_4': hot, 'cells': n, 'frames': list(frames_hit)}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('films', nargs='+')
    ap.add_argument('--out', required=True)
    ap.add_argument('--start', type=float, default=0.0)
    ap.add_argument('--dur', type=float)
    ap.add_argument('--roi', action='append', default=[], help='name=x:y:w:h, the region under study')
    ap.add_argument('--control', action='append', default=[], help='name=x:y:w:h, a region that always moves and should not show the defect')
    ap.add_argument('--shift', default='0:0', help='dx:dy applied to every region and slit of the second film')
    ap.add_argument('--slit-x', type=int, action='append', default=[])
    ap.add_argument('--slit-y', type=int, action='append', default=[])
    ap.add_argument('--k', type=float, default=3.0, help='a jump is k times the running median (window 31 frames)')
    a = ap.parse_args()
    if len(a.films) > 2: sys.exit('videosheet: one film or a pair')
    os.makedirs(a.out, exist_ok=True)
    ff = find_ffmpeg()
    try: dx, dy = (int(t) for t in a.shift.split(':'))
    except ValueError: sys.exit(f'videosheet: --shift is dx:dy, not {a.shift}')
    rois = [parse_rect(s) for s in a.roi]
    ctrls = [parse_rect(s) for s in a.control]
    films = [Film(ff, a.films[0], a)] + ([Film(ff, a.films[1], a, (dx, dy))] if len(a.films) == 2 else [])
    labels = ['A', 'B'] if len(films) == 2 else ['A']
    roi_names = {n for n, _ in rois}
    rep, js = [], {'films': [], 'start': a.start, 'k': a.k}
    rep.append(f'videosheet: start {a.start} s, k {a.k}; frames count the decoded frames from the start; units grey steps (0-255)')
    bands, fps = [], films[0].fps
    report_path = os.path.join(a.out, 'report.txt')

    def save():
        open(report_path, 'w', encoding='utf-8').write('\n'.join(rep) + '\n')
        json.dump(js, open(os.path.join(a.out, 'report.json'), 'w'), indent=1)

    for lab, film in zip(labels, films):
        fj = {'label': lab, 'path': film.path, 'size': [film.W, film.H], 'fps': film.fps, 'frames': film.N, 'cell': film.cs,
              'shift': list(film.shift), 'keyframes': film.keys}
        tr, hitches = turns(film)
        fj['turns'], fj['hitches'] = tr, hitches
        tf = set()
        for t0, t1 in tr: tf.update(range(t0 - 10, t1 + 11))
        rep.append(f'\n[{lab}] {film.path}\n  {film.W}x{film.H} at {film.fps:g} frames/s, {film.N} frames, cells {film.cs} px, shift {film.shift}')
        shown = sorted(f for f in film.key_bound if f in film.keys)
        rep.append(f'  encoder keyframes: {len(film.keys)}{" (unread)" if not film.keys_read else ""}; those where the {film.still_cells} still cells '
                   f'jump (a small jump there is set apart): {shown[:20]}{" ..." if len(shown) > 20 else ""}')
        if film.decode_errors: rep.append(f'  ffmpeg wrote {film.decode_errors} lines of decode errors: a damaged film, read it with care')
        rep.append(f'  camera turns (5 changes or more with over half the cells moving over 4x their median): {tr}')
        rep.append(f'  whole-picture hitches (the same, under 5 changes): {len(hitches)} {hitches[:20]}{" ..." if len(hitches) > 20 else ""}')
        gm = running_median(film.glob)
        reps = [i + 1 for i in range(len(film.glob)) if gm[i] > 1.0 * STEP and film.glob[i] < 0.15 * gm[i]]
        fj['repeats'] = reps
        rep.append(f'  repeated pictures while the scene moves (whole frame under 0.15 of its running median; a capture or present repeat): '
                   f'{len(reps)} {reps[:30]}{" ..." if len(reps) > 30 else ""}')
        gq = sorted(film.glob)
        rep.append(f'  whole frame motion p50 {gq[len(gq)//2]/STEP:.3f} p99 {gq[int(0.99*(len(gq)-1))]/STEP:.3f}')
        bands.append({'series': film.glob, 'colour': (110, 110, 160), 'marks': []})
        # the controls' jumps, once per film
        ctrl_events = {}
        for cn, cr in ctrls:
            cc = film.cells(cr)
            s = set()
            if cc:
                for e in local_events(film.series(cc), a.k, 1.0 * STEP): s.update(range(e['frame'], e['last'] + 3))
            ctrl_events[cn] = s
        fj['regions'] = []
        for name, rect in rois + ctrls:
            kind = 'region' if name in roi_names else 'control'
            r = analyse_region(film, name, rect, ctrls if kind == 'region' else [], a.k, tf, ctrl_events)
            if 'error' in r:
                rep.append(f'  {name}: {r["error"]}'); continue
            rm = r['removed']
            rep.append(f"  {kind} {name} {rect}: median {r['median']:.2f} p90 {r['p90']:.2f} p99 {r['p99']:.2f} max {r['max']:.2f}")
            rep.append(f"    own jumps {len(r['own_events'])} {r['own_events'][:30]}{' ...' if len(r['own_events'])>30 else ''}")
            rep.append(f"    their size (peak over the running median) {r['own_peaks'][:30]}")
            rep.append(f"    jumps not counted: at a camera turn {rm['camera_turn']}, with a control {rm['with_control']}, at a keyframe {rm['keyframe']}; "
                       + period_text(r['period'], film.fps))
            bl = r['blinks']
            rep.append(f"    frozen while the frame moves {r['frozen_frames']} frames ({100*r['frozen_share']:.1f} %, outside the turns); "
                       f"blinks {len(bl)} ({r['blinks_per_s']:.2f}/s, threshold {r['blink_threshold']:.2f} step)"
                       + (f": " + ', '.join(f"{b['frame']}+{b['frames']} {b['step']:+.1f}" for b in bl[:12]) + (' ...' if len(bl) > 12 else '') if bl else ''))
            bands.append({'series': r['series'], 'colour': (220, 200, 120) if kind == 'region' else (120, 200, 140), 'marks': r['own_events']})
            r = dict(r); r.pop('series'); r['kind'] = kind; fj['regions'].append(r)
        if not rois:
            topb, allb, common, nb, need = auto_blocks(film, a.k, tf, film.fps)
            fj['auto_blocks'], fj['auto_common'] = topb, common
            layer = [c for c in common if c[1] <= 0.3 * nb]
            wide = [c for c in common if c[1] > 0.3 * nb]
            rep.append(f"  auto: frames where at least {need} of {nb} blocks of {4*film.cs} px jump at once (frame:blocks): "
                       + (', '.join(f'{f}:{n}' for f, n in layer[:40]) if layer else 'none') + (' ...' if len(layer) > 40 else ''))
            rep.append('    ' + period_text(period_of([f for f, _ in layer], film.fps), film.fps))
            if wide:
                rep.append(f"  auto: over 30 % of the blocks at once (the whole picture: a hitch or a light change): " + ', '.join(f'{f}:{n}' for f, n in wide[:30]))
                rep.append('    ' + period_text(period_of([f for f, _ in wide], film.fps), film.fps))
            rep.append(f'  auto: {len(allb)} blocks with jumps; the most:')
            for b in topb:
                rep.append(f"    {b['rect']}: {len(b['own_events'])} jumps {b['own_events'][:16]}; " + period_text(b.get('period'), film.fps))
            fj['auto_layer'] = layer
        save()
        # footprint: the first region's own jumps, else the frames where many blocks jump together (not the whole picture)
        hit = next((r['own_events'] for r in fj['regions'] if r['kind'] == 'region' and r['own_events']), None)
        if hit is None and fj.get('auto_layer'): hit = [f for f, _ in fj['auto_layer']]
        if hit:
            try:
                fpj = footprint(ff, film, hit, os.path.join(a.out, f'footprint-{lab}.png'), a)
                fj['footprint'] = fpj
                rep.append(f"  footprint-{lab}.png: {fpj['cells_over_4']} of {fpj['cells']} cells over 4x their median on frames {hit[:12]}{' ...' if len(hit) > 12 else ''}")
            except RuntimeError as e: rep.append(f'  footprint-{lab}.png failed: {e}')
        # views: the middle of each stretch between camera turns (at most 12)
        stretches, prev = [], 0
        for t0, t1 in tr:
            stretches.append((prev, t0)); prev = t1
        stretches.append((prev, film.N))
        mids = [(p + q) // 2 for p, q in stretches if q - p > 20][:12]
        fj['views'] = mids
        if mids:
            sel = '+'.join(f'eq(n\\,{m})' for m in mids)
            cols = min(4, len(mids)); rows = (len(mids) + cols - 1) // cols
            try:
                ff_png(ff, film.path, f"select='{sel}',scale=480:-2,tile={cols}x{rows}:padding=4", os.path.join(a.out, f'views-{lab}.png'), a.start, a.dur)
                rep.append(f'  views-{lab}.png: frames {mids}')
            except RuntimeError as e: rep.append(f'  views-{lab}.png failed: {e}')
        js['films'].append(fj)
        save()
    if len(films) == 2:
        rep.append('\n[pair] A against B')
        for ra in js['films'][0]['regions']:
            rb = next((r for r in js['films'][1]['regions'] if r['name'] == ra['name']), None)
            if not rb: continue
            rep.append(f"  {ra['name']}: own jumps A {len(ra['own_events'])} B {len(rb['own_events'])}; frozen A {100*ra['frozen_share']:.1f} % B {100*rb['frozen_share']:.1f} %; "
                       f"blinks A {ra['blinks_per_s']:.2f} B {rb['blinks_per_s']:.2f}/s; median A {ra['median']:.2f} B {rb['median']:.2f}")
        ta, tb = js['films'][0]['turns'], js['films'][1]['turns']
        if not ta and not tb: rep.append('  no camera turn in either film')
        elif len(ta) == len(tb):
            off = st.median([b[0] - x[0] for x, b in zip(ta, tb)])
            rep.append(f'  turns align with B = A + {off:g} frames (same number of turns)')
            js['turn_offset'] = off
        else:
            rep.append(f'  turns: A {len(ta)}, B {len(tb)}: not aligned automatically')
    draw_curves(os.path.join(a.out, 'curves.png'), bands, max(f.N for f in films), fps)
    order = []
    for lab, fj in zip(labels, js['films']):
        order += [f'{lab} whole frame'] + [f"{lab} {r['kind']} {r['name']}" for r in fj['regions']]
    rep.append('\ncurves.png bands, top to bottom: ' + '; '.join(order))
    for lab, film in zip(labels, films):
        every = max(1, -(-film.N // SLIT_MAX))
        pick = f"select='not(mod(n\\,{every}))'," if every > 1 else ''
        cols = -(-film.N // every)
        for x in a.slit_x:
            xx = x + film.shift[0]; o = os.path.join(a.out, f'slit-{lab}-x{x}.png')
            if not 0 <= xx < film.W: rep.append(f'slit-{lab}-x{x}: column {xx} outside the frame'); continue
            try:
                ff_png(ff, film.path, f'{pick}format=rgb24,crop=1:{film.H}:{xx}:0,tile={cols}x1', o, a.start, a.dur)
                rep.append(f'slit-{lab}-x{x}.png: column {xx}, one frame in {every}, time left to right')
            except RuntimeError as e: rep.append(f'slit-{lab}-x{x}.png failed: {e}')
        for y in a.slit_y:
            yy = y + film.shift[1]; o = os.path.join(a.out, f'slit-{lab}-y{y}.png')
            if not 0 <= yy < film.H: rep.append(f'slit-{lab}-y{y}: row {yy} outside the frame'); continue
            try:
                ff_png(ff, film.path, f'{pick}format=rgb24,crop={film.W}:1:0:{yy},transpose=1,tile={cols}x1', o, a.start, a.dur)
                rep.append(f'slit-{lab}-y{y}.png: row {yy}, one frame in {every}, time left to right')
            except RuntimeError as e: rep.append(f'slit-{lab}-y{y}.png failed: {e}')
    save()
    print('\n'.join(rep))


if __name__ == '__main__':
    main()
