"""I-0141 tranche 26 (RESULT « Suggested next step » 3): the native shares of a held cutscene moment, every variant shot in one hold.

Usage: python Compare-I141HoldVariants.py <run folder> [<run folder> ...] [--out <file.tsv>]

Each run folder is a Run-AlphaCast run with -CutsceneHolds and -CutsceneHoldVariants: cutscene-hold-<k>.png (the hold's first shot,
all shares on), cutscene-hold-<k>-v<bits>-<j>.png (entry j, the module's share variant bits), cutscene-hold-variants.tsv (k, j, bits,
ms, the shot's ticks) and startup.jsonl (the witness_cutscene_hold rows: the hold's cut, frame and release; the witness_share_variant
rows: the bits and the scene clock the module read when it took them). Every shot's luma is ffmpeg signalstats' YAVG of the capture
below the window's title bar (rows 32 on, Make-I138HoldPairs.ps1's Get-Luma; JPEG data under a .png name, read with -c:v mjpeg).

Checks, each printed and making the hold « refused » when it fails (the step review's M1, L4, L5): the hold released by its shot
(« shot », not a timeout or a scene change, so every shot was taken before the release) with no drift and at the hold's own frame
(Make-I138HoldPairs.ps1's rule); the last variant row before the hold 0 (no share still off at its first shot); every variant row of
the hold read while the scene clock stood at the hold's cut and target scene frame (within 0.05 frame); each shot present; a base
shot after the last variant (the variants' list ending with 0); the hold's variants those of alphacast-args.json's
-CutsceneHoldVariants, in order (the final review's L8: a skipped entry leaves no tsv line); no idle reset row (I-0141 tranche 27,
bits back to 0) inside the hold. The rows are ordered by their tick, then their serial (a Reference run's rows are logged after the
module's lock, both taken under it); a reset row counts as bits 0 before a hold. Printed per run and hold: the luma of each shot in its order, the
base's drift (its last shot minus its first) and range (its largest minus its smallest: the world's own motion during the hold) and each variant minus the mean of the base
shots just before and just after it. With several runs, the per-hold mean and range over the runs of each variant's delta.
"""
import csv, glob, json, os, re, subprocess, sys

def ffmpeg():
    for p in glob.glob(os.path.expandvars(r'%LOCALAPPDATA%\Microsoft\WinGet\Packages\**\ffmpeg.exe'), recursive=True): return p
    sys.exit('no ffmpeg')

def luma(path, ff):
    run = subprocess.run([ff, '-hide_banner', '-c:v', 'mjpeg', '-i', path, '-vf', 'crop=iw:ih-32:0:32,signalstats,metadata=print:key=lavfi.signalstats.YAVG',
                          '-f', 'null', '-'], capture_output=True, creationflags=0x08000000)   # CREATE_NO_WINDOW (D-0199)
    m = re.search(r'lavfi\.signalstats\.YAVG=([0-9.]+)', run.stderr.decode(errors='replace'))
    if not m: sys.exit('%s: no YAVG (ffmpeg exit %d)' % (path, run.returncode))
    return float(m.group(1))

def read_run(folder, ff):
    holds, releases, variants = {}, {}, []
    with open(os.path.join(folder, 'startup.jsonl'), encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"witness_cutscene_hold"' in line or '"witness_share_variant"' in line:
                try: row = json.loads(line)
                except ValueError: continue
                if row.get('event') == 'witness_cutscene_hold':
                    if row.get('phase') == 'hold': holds[row['index']] = row
                    elif row.get('phase') in ('release', 'missed'): releases[row['index']] = row
                elif row.get('event') == 'witness_share_variant' and 'tick' in row:   # not the leftover rows (the third step review's M2)
                    if 'reset' in row: row.update(bits=0, parsed=True)   # I-0141 tranche 27's idle reset (the final review's L4): bits back to 0
                    variants.append(row)
    variants.sort(key=lambda v: (v['tick'], v.get('serial', 0)))   # logged after the module's lock; tick and serial both taken under it
    entries = {}
    vt = os.path.join(folder, 'cutscene-hold-variants.tsv')
    if os.path.exists(vt):
        for k, j, bits, ms, s0, s1 in csv.reader(open(vt), delimiter='\t'):
            entries.setdefault(int(k), []).append((int(j), int(bits), int(ms)))
    # The final review's L8: the runner skips an entry whose row it does not see in 3 s without a line in the tsv; the list it was given.
    listed = None
    args_path = os.path.join(folder, 'alphacast-args.json')
    if os.path.exists(args_path):
        given = json.load(open(args_path, encoding='utf-8-sig')).get('CutsceneHoldVariants') or ''
        listed = [int(e.split(':')[0]) for e in given.split()]
    out = {}
    for k in sorted(holds):
        h = holds[k]; problems = []
        rel = releases.get(k)
        if not rel or rel.get('why') != 'shot': problems.append('released by %s' % (rel.get('why') if rel else 'nothing'))
        # The variant rows taken during this hold: between the hold's tick and its release's (the module's own clock).
        t0, t1 = h['tick'], rel['tick'] if rel else float('inf')
        inside = [v for v in variants if t0 <= v['tick'] <= t1]
        before = [v for v in variants if v['tick'] < t0 and v.get('parsed')]
        if before and before[-1]['bits'] != 0: problems.append('shares still off (bits %d) at the first shot' % before[-1]['bits'])
        if rel and rel.get('why') == 'shot' and (rel.get('drift', 0) != 0 or abs(rel.get('frame', -1e9) - h['frame']) > 1e-3):
            problems.append('release drift %s, frame %s against %s' % (rel.get('drift'), rel.get('frame'), h['frame']))
        for v in inside:
            if 'reset' in v: problems.append('share variant reset to 0 during the hold (was %s)' % v.get('was')); continue
            if v.get('scene_cut') != h['cut'] or abs(v.get('scene_frame', -1e9) - h['target']) > 0.05:
                problems.append('variant %d read at cut %s scene %s' % (v['bits'], v.get('scene_cut'), v.get('scene_frame')))
        shots = [('base', 0, os.path.join(folder, 'cutscene-hold-%d.png' % k))]
        for j, bits, ms in entries.get(k, []):
            shots.append(('v%d' % bits, bits, os.path.join(folder, 'cutscene-hold-%d-v%d-%d.png' % (k, bits, j))))
        if entries.get(k) and entries[k][-1][1] != 0: problems.append('no base shot after the last variant')
        if listed is None: problems.append('no alphacast-args.json: the variants given unknown')
        elif [b for _, b, _ in entries.get(k, [])] != listed: problems.append('variants shot %s, given %s' % ([b for _, b, _ in entries.get(k, [])], listed))
        values = []
        for name, bits, path in shots:
            if not os.path.exists(path): problems.append('%s missing' % os.path.basename(path)); values.append(None)
            else: values.append(luma(path, ff))
        out[k] = dict(cut=h['cut'], frame=h['frame'], shots=[(n, b, v) for (n, b, _), v in zip(shots, values)], problems=problems,
                      variant_rows=len(inside))
    return out

def main():
    args, out_path = [], None
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '--out': out_path = sys.argv[i + 1]; i += 2; continue
        if sys.argv[i].startswith('--'): sys.exit('unknown option %s' % sys.argv[i])
        args.append(sys.argv[i]); i += 1
    if not args: sys.exit(__doc__)
    ff = ffmpeg(); lines = []
    deltas = {}   # (hold, label) -> [delta per run]
    for folder in args:
        run = read_run(folder, ff); name = os.path.basename(os.path.normpath(folder))
        lines.append('# %s' % name)
        lines.append('hold\tcut\tframe\tstatus\tshots (label=luma)\tbase_drift\tbase_range\tdeltas vs the bases\' mean')
        for k, r in sorted(run.items()):
            vals = r['shots']; bases = [v for n, b, v in vals if b == 0 and v is not None]
            status = 'refused: ' + '; '.join(r['problems']) if r['problems'] else 'ok (%d variant rows)' % r['variant_rows']
            cells = ' '.join('%s=%s' % (n, '%.1f' % v if v is not None else '-') for n, b, v in vals)
            drift = bases[-1] - bases[0] if len(bases) >= 2 else float('nan')
            spread = max(bases) - min(bases) if len(bases) >= 2 else float('nan')   # the integrity review's M1: the middle bases too
            seen = {}; ds = []
            for at, (n, b, v) in enumerate(vals):
                if b == 0 or v is None: continue
                prev = [x for _, bb, x in vals[:at] if bb == 0 and x is not None][-1:]
                nxt = [x for _, bb, x in vals[at + 1:] if bb == 0 and x is not None][:1]
                if not prev or not nxt: continue
                base = (prev[0] + nxt[0]) / 2
                seen[n] = seen.get(n, 0) + 1; label = n if seen[n] == 1 else '%s#%d' % (n, seen[n])
                ds.append('%s %+.1f' % (label, v - base))
                if not r['problems']: deltas.setdefault((k, r['cut'], r['frame'], label), []).append(v - base)
            lines.append('%d\t%d\t%.2f\t%s\t%s\t%+.1f\t%.1f\t%s' % (k, r['cut'], r['frame'], status, cells, drift, spread, ', '.join(ds)))
        lines.append('')
    if len(args) > 1:
        lines.append('# over the runs: each variant minus its hold\'s bases, mean [range] (runs)')
        lines.append('hold\tcut\tframe\tvariant\tmean\tmin\tmax\truns')
        for (k, cut, frame, label), v in sorted(deltas.items()):
            lines.append('%d\t%d\t%.2f\t%s\t%+.1f\t%+.1f\t%+.1f\t%d' % (k, cut, frame, label, sum(v) / len(v), min(v), max(v), len(v)))
    text = '\n'.join(lines) + '\n'
    print(text, end='')
    if out_path: open(out_path, 'w', encoding='utf-8', newline='\n').write(text)

if __name__ == '__main__': main()
