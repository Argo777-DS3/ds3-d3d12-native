"""I-0141 tranche 14: the ceremony's newborn heads in a native and an original journal (ffx-entity-life.flag).

Usage: python -I Compare-I141Newborn.py <native startup.jsonl> <original startup.jsonl>
Prints per mode and window (0 s, 14.4 s): the node draws (1ADDE80) by offered/refused, box kind (non-finite, reset, box) and
age 0 or not; the first updates at age 0 (count before -> after); the order of updates (u) and pass-0x45 draws (G) on the scene
clock; the frame rings (ffx_frame_ring); the life rows by their owner's age at the last offer whose owner was read
(ffx_entity_life's owner[2]), by window. « age0 » in the draw and update rows is an age printed 0 (%.4f: under 5e-5 s); in the
life rows (%.6g) an exact 0; a life's age printed null (not finite) is its own class.
"""
import collections
import json
import sys


rejected = collections.Counter()


def rows(path):
    for line in open(path, encoding='utf-8', errors='replace'):
        if '"ffx_newborn_' in line or '"ffx_draw_gate"' in line or '"ffx_frame_ring"' in line or '"ffx_entity_life"' in line:
            try:
                yield json.loads(line)
            except ValueError:
                rejected[path] += 1   # a row that is not JSON (a non-finite field printed nan or inf, a truncated line): counted below


def box(r):
    b = r['bounds']
    if any(x != x or abs(x) == float('inf') or x == -1e30 for x in b):
        return 'nonfinite'
    return 'reset' if b[0] > b[4] else 'box'


def window(ms):
    return '0s' if ms < 10000 else '14.4s'


for path in sys.argv[1:]:
    rejected[path] = 0
    data = list(rows(path))
    mode = next((r.get('mode') for r in data if 'mode' in r), '?')
    print('==', mode, path)
    draws = collections.Counter()
    first = collections.Counter()
    for r in data:
        if r['event'] == 'ffx_newborn_draw':
            draws[(window(r['scene_ms']), 'offered' if r['offered'] else 'refused', box(r), 'age0' if r['age'] == 0 else 'age>0')] += 1
        elif r['event'] == 'ffx_newborn_update' and r['age'] == 0:
            first[(window(r['scene_ms']), box(r), r['before'], r['count'])] += 1
    for k, v in sorted(draws.items()):
        print('  draw', k, v)
    for k, v in sorted(first.items()):
        print('  first update', k, v)
    for lo, hi in ((0, 80), (14370, 14440)):
        ev = sorted((r['scene_ms'], 'G' if r['event'] == 'ffx_draw_gate' else 'u') for r in data
                    if r['event'] in ('ffx_draw_gate', 'ffx_newborn_update') and lo <= r['scene_ms'] <= hi)
        out = []
        for t, k in ev:
            if out and out[-1][0] == k and t - out[-1][1] < 3:
                continue
            out.append((k, t))
        print('  order', lo, ' '.join(f'{k}{t:.1f}' for k, t in out))
    for r in data:
        if r['event'] == 'ffx_frame_ring':
            print('  ring at', r['scene_ms'], 'last draws', r['draws'][-3:], 'updates', r['updates'][-4:])
    life = collections.Counter()
    for r in data:
        if r['event'] == 'ffx_entity_life' and r.get('owner'):
            w = '0s' if r['first'] < 100 else ('14.4s' if 14300 < r['first'] < 14500 else 'other')
            age = r['owner'][2]
            life[(w, 'null' if age is None else ('age0' if age == 0 else 'age>0'))] += 1
    print('  lives by owner age at the last offer read', sorted(life.items()))
    print('  rows not parsed', rejected[path])
