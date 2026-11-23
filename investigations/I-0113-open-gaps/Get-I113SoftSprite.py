# I-0113 tranche 33 (gap 16, the native's extra red negatives): the 2AFA soft billboards' words (ffx_soft_sprite_series, tranche 32's
# witness, both modes) from a Reference journal and a native one. PS 2AFA takes its colour as v0.rgb + softSpriteParam.w x
# (g_vAmbientSH[c].w - v0.rgb): per journal, the rows' count, the distribution of the three SH .w words (sh[3], sh[7], sh[11]: red,
# green, blue), of softSpriteParam.w, of the colour scale, and how many rows have one SH .w word below another by more than 0.05
# (the channel the lerp pulls down). Usage: python -I Get-I113SoftSprite.py <original startup.jsonl> <native startup.jsonl>
import json, sys


def rows(path):
    out = []
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"ffx_soft_sprite_series"' in line:
                try:
                    out.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    return out


def q(values):
    v = sorted(x for x in values if x is not None)
    if not v:
        return 'none'
    pick = lambda p: v[min(len(v) - 1, int(p * (len(v) - 1)))]
    return f'n={len(v)} min={v[0]:.4g} p10={pick(.1):.4g} med={pick(.5):.4g} p90={pick(.9):.4g} max={v[-1]:.4g}'


def report(label, rs):
    print(f'== {label}: {len(rs)} rows')
    if not rs:
        return
    sh = [r['sh'] for r in rs if len(r.get('sh', [])) == 12]
    for name, i in (('SH red .w', 3), ('SH green .w', 7), ('SH blue .w', 11)):
        print(f'  {name}: {q([s[i] for s in sh])}')
    print(f'  soft.w: {q([r["soft"][3] for r in rs if len(r.get("soft", [])) == 4])}')
    for name, i in (('scale r', 0), ('scale g', 1), ('scale b', 2), ('scale a', 3)):
        print(f'  {name}: {q([r["scale"][i] for r in rs if len(r.get("scale", [])) == 4])}')
    low = sum(1 for s in sh if None not in (s[3], s[7], s[11]) and s[3] < max(s[7], s[11]) - 0.05)
    print(f'  rows whose red .w is below green or blue by > 0.05: {low} of {len(sh)}')
    for key in ('blend', 'colour_blend', 'fog', 'pre_alpha'):
        counts = {}
        for r in rs:
            counts[r.get(key)] = counts.get(r.get(key), 0) + 1
        print(f'  {key}: {dict(sorted(counts.items(), key=lambda kv: -kv[1]))}')


if __name__ == '__main__':
    for label, path in zip(('original', 'native'), sys.argv[1:3]):
        report(label, [r for r in rows(path)])
