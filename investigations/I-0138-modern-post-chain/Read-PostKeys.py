# I-0138 tranche 8h: the post_keys rows of a run's journal (07 LogPostKeys, shaft-observe.flag), one line a row in the order written:
# site, call, thread, tick, scene cut and frame, the context, the gate, the original's draw gate (root +0xCA, timing +0x27B and +0x278;
# rows from 8h's fix on), the set's tokens (apply only), and the keys and weights of the
# groups asked (default 0 Setting Info, 1 Tone Map, 2 Dof, 3 Bloom, 9 LightShaft). -Cuts keeps only the rows written within -Window
# rows of a site's cut change (the cut starts) plus each site's first rows of the scene.
# python -I Read-PostKeys.py <startup.jsonl> [--groups 0,1,2,3,9] [--sites apply,fb890,collect] [--cuts] [--window 10]
import argparse, json, sys

def main():
    p = argparse.ArgumentParser()
    p.add_argument('journal')
    p.add_argument('--groups', default='0,1,2,3,9')
    p.add_argument('--sites', default='apply,fb890,collect')
    p.add_argument('--cuts', action='store_true')
    p.add_argument('--window', type=int, default=10)
    a = p.parse_args()
    groups = [int(g) for g in a.groups.split(',') if g]
    sites = set(a.sites.split(','))
    rows = []
    with open(a.journal, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"post_keys"' not in line:
                continue
            try:
                r = json.loads(line)
            except ValueError:
                continue
            if r.get('event') == 'post_keys' and r.get('site') in sites:
                rows.append(r)
    last_cut = {}
    left = {}
    for r in rows:
        site = r['site']
        cut = r.get('scene_cut', -1)
        if cut != last_cut.get(site):
            last_cut[site] = cut
            left[site] = a.window
        keep = not a.cuts or left.get(site, 0) > 0
        if left.get(site, 0) > 0:
            left[site] -= 1
        if not keep:
            continue
        keys = r.get('keys', [])
        weights = r.get('weights', [])
        cells = []
        for g in groups:
            k = keys[g] if g < len(keys) else []
            w = weights[g] if g < len(weights) else None
            def key(x, y):
                return '-1' if x == 4294967295 and y == 4294967295 else f'{x}:{y}'
            cells.append(f'g{g}={key(k[0], k[1])}>{key(k[2], k[3])}@{w}' if len(k) == 4 else f'g{g}=?')
        frame = r.get('scene_frame')
        clock = f'cut {cut} f {frame:.2f}' if frame is not None else 'no clock'
        extra = ''
        if 'tokens' in r:
            extra = f" set {r.get('set')} tokens {r['tokens']} exp {r['expected']} single {r['single']}"
        draw = f" draw {r['draw_gate']}" if 'draw_gate' in r else ''
        print(f"{site:7} {r['call']:6} t{r['thread']} {r['tick']} {clock:18} ctx {r['context']} gate {r['gate']}{draw}{extra} | " + ' '.join(cells))

if __name__ == '__main__':
    sys.exit(main())
