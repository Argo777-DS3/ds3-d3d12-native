# I-0113 tranche 14: a world object's region IBL, native against original. Reads the two runs' journals (startup.jsonl) and
# compares, model by model, the region keys, blends and scales 5C0E0 gave (reference_region_ibl) with the native's choice
# (native_region_ibl). Usage: python -I Compare-I113RegionIbl.py <native run folder> <original run folder>
import collections
import json
import sys


def rows(folder, event):
    out = collections.defaultdict(set)
    with open(folder + '/startup.jsonl', encoding='utf-8', errors='replace') as f:
        for line in f:
            if event not in line:
                continue
            try:
                e = json.loads(line)
            except ValueError:
                continue
            if e.get('event') != event:
                continue
            keys = tuple(tuple(k) for k in e['keys'])
            out[e['model']].add((keys, round(e['blend'], 2), tuple(round(s, 3) for s in e['scales'])))
    return out


native = rows(sys.argv[1], 'native_region_ibl')
original = rows(sys.argv[2], 'reference_region_ibl')
both = sorted(set(native) & set(original))
print(f'models: native {len(native)}, original {len(original)}, both {len(both)}')
same_keys = 0
for model in both:
    nk = {r[0] for r in native[model]}
    ok = {r[0] for r in original[model]}
    match = bool(nk & ok)
    same_keys += match
    if not match:
        print('keys differ', model, 'native', sorted(nk)[:4], 'original', sorted(ok)[:4])
print(f'models whose key pairs meet: {same_keys} of {len(both)}')
scales_n = collections.Counter(r[2] for v in native.values() for r in v)
scales_o = collections.Counter(r[2] for v in original.values() for r in v)
print('scales native', scales_n.most_common(4), 'original', scales_o.most_common(4))
print('only native', sorted(set(native) - set(original))[:20])
print('only original', sorted(set(original) - set(native))[:20])

# The rows logged at the same position (rounded to 0.1 m) in both runs: their keys and blends.
def positioned(folder, event):
    out = {}
    with open(folder + '/startup.jsonl', encoding='utf-8', errors='replace') as f:
        for line in f:
            if event not in line:
                continue
            try:
                e = json.loads(line)
            except ValueError:
                continue
            if e.get('event') == event:
                out[(e['model'], tuple(round(x, 1) for x in e['position'][:3]))] = (tuple(tuple(k) for k in e['keys']), e['blend'])
    return out


np, op = positioned(sys.argv[1], 'native_region_ibl'), positioned(sys.argv[2], 'reference_region_ibl')
shared = sorted(set(np) & set(op))
equal = [k for k in shared if np[k][0] == op[k][0] and abs(np[k][1] - op[k][1]) < 0.005]
print(f'rows at the same model and position: {len(shared)}, the same keys and blend: {len(equal)}')
for k in shared:
    if k not in equal:
        print('differs', k, 'native', np[k], 'original', op[k])
