# I-0114 tranche 7: the moves of a run read from jump-observe.flag's per-step rows (unlock_jump_step: every game step for 120 s
# from the first motion; the player's position [mover+0x80] and the step's dt), so no row sampling and no key alignment enter.
# The horizontal speed of each step is its displacement over its dt. A move is a run of steps above a threshold:
# - rolls: above 8 m/s (the roll's peak is 10-11 m/s at every rate, the sprint 5.7, a jump's held 7.5), widened to the steps
#   above 4.6 m/s around the peak (series 21's roll bound): distance and duration between the interpolated 4.6 m/s crossings;
# - motions: above 0.5 m/s (a held move from its start to its stop): distance, duration and the median speed of its middle half;
# - falls: the steps whose vertical speed is under -3 m/s: their count, duration and drop.
# Usage: python -I Get-Moves.py <startup.jsonl> [...]
import json, math, sys, statistics as st

def steps(path):
    rows = [json.loads(l) for l in open(path, encoding='utf-8') if '"unlock_jump_step"' in l]
    out = []
    for a, b in zip(rows, rows[1:]):
        dt = b['t'] - a['t']
        if dt <= 0:
            continue
        dx, dy, dz = (b['at'][i] - a['at'][i] for i in range(3))
        out.append((b['t'], dt, math.hypot(dx, dz) / dt, dy / dt, math.hypot(dx, dz)))
    return out

def crossing(s0, s1, t0, t1, level):
    return t0 + (t1 - t0) * (level - s0) / (s1 - s0) if s1 != s0 else t1

def runs_above(s, level):
    i = 0
    while i < len(s):
        if s[i][2] > level:
            j = i
            while j + 1 < len(s) and s[j + 1][2] > level:
                j += 1
            yield i, j
            i = j + 1
        else:
            i += 1

def rolls(s):
    # Each step's speed is constant over its dt; the roll runs from the time the speed crosses 4.6 m/s upward to the time it crosses
    # it downward, interpolated between the two steps' middles (the step review's L1: whole edge steps grew with the step's length).
    out = []
    for i, j in runs_above(s, 8.0):
        a, b = i, j
        while a > 0 and s[a - 1][2] > 4.6:
            a -= 1
        while b + 1 < len(s) and s[b + 1][2] > 4.6:
            b += 1
        if out and out[-1][0] == a:
            continue
        mid = lambda k: s[k][0] - 0.5 * s[k][1]
        t0 = crossing(s[a - 1][2], s[a][2], mid(a - 1), mid(a), 4.6) if a > 0 else s[a][0] - s[a][1]
        t1 = crossing(s[b][2], s[b + 1][2], mid(b), mid(b + 1), 4.6) if b + 1 < len(s) else s[b][0]
        # the distance between t0 and t1: each step's speed over the part of its interval inside [t0, t1]
        dist = 0.0
        for k in range(max(a - 1, 0), min(b + 2, len(s))):
            lo, hi = max(s[k][0] - s[k][1], t0), min(s[k][0], t1)
            if hi > lo:
                dist += s[k][2] * (hi - lo)
        out.append((a, t0, t1, dist, max(x[2] for x in s[a:b + 1])))
    return out

def motions(s):
    out = []
    for i, j in runs_above(s, 0.5):
        if j - i < 3:
            continue
        seg = s[i:j + 1]
        dur = seg[-1][0] - (seg[0][0] - seg[0][1])
        mid = [x[2] for x in seg[len(seg) // 4: 3 * len(seg) // 4]] or [seg[0][2]]
        out.append((seg[0][0] - seg[0][1], dur, sum(x[4] for x in seg), st.median(mid)))
    return out

def falls(s):
    out = []
    i = 0
    while i < len(s):
        if s[i][3] < -3.0:
            j = i
            while j + 1 < len(s) and s[j + 1][3] < -3.0:
                j += 1
            out.append((s[i][0], s[j][0] - (s[i][0] - s[i][1]), sum(x[3] * x[1] for x in s[i:j + 1])))
            i = j + 1
        else:
            i += 1
    return out

for path in sys.argv[1:]:
    s = steps(path)
    if not s:
        print(path, 'no unlock_jump_step rows')
        continue
    dt = st.median(x[1] for x in s)
    print(f'== {path}  steps {len(s)}  median dt {dt * 1000:.2f} ms')
    for k, (_, t0, t1, d, peak) in enumerate(rolls(s)):
        print(f'roll {k}: t {t0:.3f}  {d:.3f} m in {(t1 - t0) * 1000:.0f} ms, peak {peak:.2f} m/s')
    for k, (t0, dur, d, v) in enumerate(motions(s)):
        print(f'motion {k}: t {t0:.3f}  {d:.3f} m in {dur * 1000:.0f} ms, middle speed {v:.3f} m/s')
    for k, (t0, dur, drop) in enumerate(falls(s)):
        print(f'fall {k}: t {t0:.3f}  {dur * 1000:.0f} ms, drop {drop:.3f} m')
