"""I-0115: the near characters' rows of runs (popin_native_near natively, popin_reference_near in the original; popin-observe.flag).

For each character entity within 8 m of the camera: its gaps (not drawn or not emitted while near, then drawn again), each with its
start, length in frames (native serials, original steps), distance and why; and the changes of its fade-group words (k, dc, o) and
category. Usage: python -I Get-NearRows.py <run> [<run> ...] [--model c1410]
"""
import json, sys, os, collections

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def rows(run):
    path = run if os.path.isfile(run) else os.path.join(REPO, 'evidence', 'I-0066', 'artifacts', run, 'startup.jsonl')
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"popin_native_near"' in line or '"popin_reference_near"' in line:
                try:
                    yield json.loads(line)
                except ValueError:
                    pass


def summarise(run, model):
    gaps, words, last = [], collections.Counter(), {}
    clock = None
    for r in rows(run):
        if model and r.get('model') != model:
            continue
        clock = clock or ('serial' if 'serial' in r else 'step')
        t, e, why = r[clock], r['entity'], r['why']
        visible = why in ('drawn', 'emitted')
        state = last.get(e)
        if visible and state and state['off'] is not None:
            gaps.append((e[-5:], r['model'], state['off'], t - state['off'], state['why'], round(state['d'], 2)))
            state['off'] = None
        if not visible and why != 'far' and state and state['off'] is None:
            state.update(off=t, why=why)
        if why == 'far':
            last.pop(e, None)
            continue
        if visible:
            key = (r['model'], r.get('category', '-'), tuple(r['k']), round(r['dc'], 2), round(r['o'], 2))
            words[key] += 1
            last[e] = dict(off=None, why=why, d=r['d'])
    print(f'== {run} ({clock})')
    print('gaps while near (entity, model, start, frames, why, last d):')
    for g in gaps:
        print('  ', *g)
    print('drawn states (model, category, k, dc, o): rows')
    for k, n in words.most_common(20):
        print('  ', k, n)


if __name__ == '__main__':
    args = sys.argv[1:]
    model = None
    if '--model' in args:
        i = args.index('--model'); model = args[i + 1]; del args[i:i + 2]
    for run in args:
        summarise(run, model)
