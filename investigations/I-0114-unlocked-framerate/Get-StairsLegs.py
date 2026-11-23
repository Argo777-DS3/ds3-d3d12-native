# I-0114 tranche 7: the terrain field [+0x2DC] over the stairs path's three first climbing legs (28 s of the logic's time from
# the first leg's start, which Get-Stairs.py prints): mean |x| of the unlock_terrain rows and their variation a second.
# Usage: python -I Get-StairsLegs.py <startup.jsonl> <first leg's start, s> [...pairs]
import json, sys, statistics as st
args = sys.argv[1:]
for path, t0 in zip(args[0::2], args[1::2]):
    t0 = float(t0); xs = []; last = None; var = 0.0
    for l in open(path, encoding='utf-8', errors='replace'):
        if '"unlock_terrain"' in l:
            r = json.loads(l)
            if t0 <= r['t'] <= t0 + 28:
                xs.append(abs(r['x']))
                if last is not None: var += abs(r['x'] - last)
                last = r['x']
    print(path, 'rows', len(xs), 'mean |x| %.4f' % st.mean(xs), 'variation a second %.3f' % (var / 28))