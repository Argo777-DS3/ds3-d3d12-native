# I-0141 tranche 11 (the ceremony's smoke, « The instances created alike »): the library's instance creations 141ACCA20 in both
# modes. Reads two run journals (native and original, Run-I141SmokePix -Dump -Light) and prints, by caller and FXR id, the calls and
# the instances made since the start (the first ffx_spawn_tally row of each journal, written at the first F9 or F13 press) side by
# side, the keys whose counts differ first; then the ffx_spawn rows of the `--window` seconds before that press (its tick) by FXR
# id, with a warning when the rows' bound (8000 a session) ran out before the window's start; then the entities offered on pass 0x45
# (ffx_entity_offered) by kind. Rows that do not parse are counted and left out.
# Usage: python Compare-I141Spawns.py <native startup.jsonl> <original startup.jsonl> [--window 20]
import json, sys, collections, argparse

def read(path):
    tally, spawns, entities, presses, bad = None, [], [], [], 0
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"ffx_spawn' not in line and '"ffx_entity_offered"' not in line:
                continue
            try:
                r = json.loads(line)
            except ValueError:
                bad += 1
                continue
            e = r.get('event')
            if e == 'ffx_spawn_tally':
                if tally is None:
                    tally = r
                presses.append(r.get('ms'))
            elif e == 'ffx_spawn':
                spawns.append(r)
            elif e == 'ffx_entity_offered':
                entities.append(r)
    return tally, spawns, entities, presses, bad

def main():
    a = argparse.ArgumentParser()
    a.add_argument('native'); a.add_argument('original'); a.add_argument('--window', type=float, default=20.0)
    o = a.parse_args()
    runs = {'native': read(o.native), 'original': read(o.original)}
    for name, (tally, spawns, entities, presses, bad) in runs.items():
        print(f'{name}: spawn rows {len(spawns)}, entity rows {len(entities)}, presses (ms) {presses}, malformed {bad}')
    keys = collections.OrderedDict()
    counts = {}
    for name, (tally, *_rest) in runs.items():
        counts[name] = {}
        for caller, fxr, calls, made in (tally or {}).get('rows', []):
            counts[name][(caller, fxr)] = (calls, made)
            keys[(caller, fxr)] = True
    rows = []
    for k in keys:
        n = counts['native'].get(k, (0, 0)); r = counts['original'].get(k, (0, 0))
        rows.append((n != r, abs(n[1] - r[1]), k, n, r))
    rows.sort(key=lambda x: (not x[0], -x[1]))
    print('\ncaller      fxr         native calls/made   original calls/made')
    for differ, _, (caller, fxr), n, r in rows:
        print(f'{caller:>10} {fxr:>10}   {n[0]:>7}/{n[1]:<7}       {r[0]:>7}/{r[1]:<7}{"   *" if differ else ""}')
    print(f'\nspawns in the {o.window:g} s before the first press, by FXR id (made):')
    window = {}
    for name, (tally, spawns, *_rest) in runs.items():
        end = runs[name][3][0] if runs[name][3] else None
        if end is None:
            print(f'  {name}: no press tick (no tally row with ms): no window'); window[name] = collections.Counter(); continue
        before = [s for s in spawns if s['ms'] <= end]
        # the step review's L1: the bound read from the press's own row (spawn_rows_left), and where its last row fell
        if (tally or {}).get('spawn_rows_left', 1) <= 0 and spawns:
            last, start = spawns[-1]['ms'], end - 1000 * o.window
            where = 'before the window' if last < start else 'inside the window: its counts are short'
            print(f'  WARNING {name}: the 8000 spawn rows ran out at {last:.0f} ms, {where} ({start:.0f} to {end:.0f} ms)')
        window[name] = collections.Counter(s['fxr'] for s in before if s['ms'] >= end - 1000 * o.window and s.get('made'))
    for fxr in sorted(set(window['native']) | set(window['original'])):
        n, r = window['native'][fxr], window['original'][fxr]
        print(f'  {fxr:>10}: native {n:>4}, original {r:>4}{"   *" if n != r else ""}')
    print('\nentities offered on pass 0x45 by kind (rows; recorded):')
    for name, (_t, _s, entities, *_rest) in runs.items():
        c = collections.Counter((e['kind'], e['recorded'] > 0) for e in entities)
        print(f'  {name}: ' + ', '.join(f'kind {k} {"rec" if rec else "not"} {v}' for (k, rec), v in sorted(c.items())))

if __name__ == '__main__':
    sys.exit(main())
