# I-0115 (the enemies gone and back within half a second): the blinks of character models in a run's popin-observe rows.
# A blink is a fade episode of a c#### model that was already drawn at full opacity: natively, popin_native_fade rows (the main
# draw's fade intent under 1, one a frame, keyed by entity since the step review) after the entity's « drawn » row and after it was
# last seen at 1; in the original, popin_reference_fade rows (one a window, the lowest emit under 1, keyed by model and place), with
# what each view emitted (o5: the pass-5 view, o45: the 0x45 view, 2 = not emitted; the screen shows the more opaque one, INFERRED).
# Its first row is the drop, its last the frame before the return; an episode down to 0 or ended by the model leaving is a
# fade-out, counted apart (final review M2). Usage: python -I Get-Blinks.py <run> [<run>...]
# (evidence/I-0066/artifacts/<run>/startup.jsonl) or a journal file (the test launcher's %LOCALAPPDATA%/DS3-DX12-Test/test-*.jsonl).
import json, sys, os
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'evidence', 'I-0066', 'artifacts')
def character(m): return len(m) == 5 and m[0] == 'c' and m[1:].isdigit()
for run in sys.argv[1:]:
    full, open_ep, episodes, native, last_fade = {}, {}, [], False, {}
    def gone(key, clock):   # the model leaves: an episode still running (its last row within 3 frames) was a fade-out, not a blink
        ep = open_ep.pop(key, None)
        if ep and clock - ep[3] <= 3: ep[9] = True
        full.pop(key, None); last_fade.pop(key, None)
    path = run if os.path.isfile(run) else os.path.join(root, run, 'startup.jsonl')   # a run name, or a journal (the test launcher's test-*.jsonl)
    for line in open(path, encoding='utf-8', errors='replace'):
        if '"popin_' not in line or '"c' not in line: continue
        try: r = json.loads(line)
        except ValueError: continue   # a row with a non-finite number
        ev, m = r['event'], r.get('model', '')
        if not character(m): continue
        if ev == 'popin_native':   # keyed by entity and, for runs before the fade rows had one, by model and place
            native = True
            for key in (r['entity'], (m, round(r['centre'][0], 1), round(r['centre'][2], 1))):
                if r['on'] and r.get('o', 1) >= 0.999: full[key] = r['serial']
                elif not r['on']: gone(key, r['serial'])
        elif ev == 'popin_reference' and 'centre' in r:
            key = (m, round(r['centre'][0], 1), round(r['centre'][2], 1))
            # final review M2: an « on » row in the same window as a fade row is a fade-in, not a full draw; « off » ends it all
            if r['on'] and last_fade.get(key) != r['step']: full.setdefault(key, r['step'])
            elif not r['on']: gone(key, r['step'])
        elif ev in ('popin_native_fade', 'popin_reference_fade'):
            clock = r.get('serial', r.get('step'))
            key = r.get('entity') or (m, round(r['centre'][0], 1), round(r['centre'][2], 1))
            # a fade ended more than 3 frames ago (the first appearance's at the draw distance too): the model is at 1 since
            if key not in full and key in last_fade and clock - last_fade[key] > 3: full[key] = last_fade[key] + 1
            last_fade[key] = clock
            seen = r['o'] if ev == 'popin_native_fade' else max(v for v in (r.get('o5', 2), r.get('o45', 2), r['o']) if v <= 1.0)
            ep = open_ep.get(key)
            if ep and clock - ep[3] <= 3: ep[3] = clock; ep[4] += 1; ep[5] = min(ep[5], seen); continue
            open_ep.pop(key, None)
            if key in full and full[key] < clock - 3 and seen < 0.999:
                ep = [m, key, clock, clock, 1, seen, r['d'], r.get('o5'), r.get('o45'), False]; open_ep[key] = ep; episodes.append(ep)
    outs = [e for e in episodes if e[9] or e[5] <= 0.0]   # ended by the model leaving, or down to 0: a fade-out
    episodes = [e for e in episodes if e not in outs]
    print(f'{run} ({"native" if native else "original"}): {len(episodes)} blink(s) of an already drawn character, {len(outs)} fade-out(s) set apart')
    for m, key, first, last, n, low, d, o5, o45, out in episodes:
        views = f', o5 {o5:.3f} o45 {o45:.3f}' if o5 is not None else ''
        print(f'  {m} {key[-5:] if isinstance(key, str) else key[1:]} frames {first}-{last} ({n} rows): lowest {low:.3f} at {d:.1f} m{views}')
