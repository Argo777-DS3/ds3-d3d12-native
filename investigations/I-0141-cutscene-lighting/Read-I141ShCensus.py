"""I-0141 tranche 28: the original's instance SH census (reference_sh_census rows, bit 512 of the share variant, 04c's
ReferenceShareCensus) read from a Reference run's startup trace.

Each row: a model, the pass kind (0, 3 or 4), the source 5B3D0 took (0 the global override, 1 the animation block's own rows,
2 the light-set records of two keys blended), the keys and blend, the view's w scales, and the record's bytes +0x80..+0xBF as
written (source -1: a read the module needed failed; the source is the choice the inputs make when read again after 5B3D0's call; a
non-finite blend, strength or scale is written 0 and the row marked, a non-finite SH listed as « sh nonfinite »; aSh[3]: three float4 rows, then ShLightMask). The G-buffer's ambient is (1 - F0) * max(0, (N, 1) . aSh[c]) per channel c
(READ, t27-original-ambient-reading.md §1): printed for the constant term (w, the mean over normals of a linear SH is w) and for
the six axis normals, each the mean of the three channels.

Usage: python Read-I141ShCensus.py <run folder or trace.jsonl> [--model-prefix c] [--epoch N]
Each census (epoch) is listed with the witness_share_variant row that opened it (its scene cut and frame), then its rows sorted by
model. Rows are de-duplicated by (model, kind) inside a census by the module itself.
"""
import json, math, os, struct, sys, glob, argparse

def trace_path(p):
    if os.path.isfile(p):
        return p
    for name in ('startup.jsonl', 'startup-trace.jsonl', 'trace.jsonl'):
        q = os.path.join(p, name)
        if os.path.isfile(q):
            return q
    found = sorted(glob.glob(os.path.join(p, '**', '*.jsonl'), recursive=True))
    for q in found:
        with open(q, encoding='utf-8', errors='replace') as f:
            if any('reference_sh_census' in line for line in f):
                return q
    sys.exit('no trace with reference_sh_census rows under %s' % p)

BAD = [0]

def rows(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.strip()
            if not line.startswith('{'):
                continue
            try:
                yield json.loads(line)
            except ValueError:
                if 'reference_sh_census' in line:
                    BAD[0] += 1   # the step review's L3: counted, not dropped silently
                continue

AXES = [('+x', (1, 0, 0)), ('-x', (-1, 0, 0)), ('+y', (0, 1, 0)), ('-y', (0, -1, 0)), ('+z', (0, 0, 1)), ('-z', (0, 0, -1))]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('run')
    ap.add_argument('--model-prefix', default='')
    ap.add_argument('--epoch', type=int, default=None)
    a = ap.parse_args()
    path = trace_path(a.run)
    opened = {}
    census = {}
    epoch = 0
    for r in rows(path):
        if r.get('event') == 'witness_share_variant' and r.get('parsed') and (r.get('bits', 0) & 512):
            epoch += 1
            opened[epoch] = (r.get('scene_cut'), r.get('scene_frame'))
        elif r.get('event') == 'reference_sh_census':
            census.setdefault(r['epoch'], []).append(r)
    print('trace', path)
    if a.epoch is not None and a.epoch not in census and a.epoch not in opened:
        print('census %d: not opened, no rows' % a.epoch)
    total = 0   # rows of every census before this one, whatever --epoch shows (the final review's L6)
    for e in sorted(set(census) | set(opened)):   # an opened census without rows is listed too (the third step review's L3)
        got = census.get(e, [])
        before = total
        total += len(got)
        if a.epoch is not None and e != a.epoch:
            continue
        cut, frame = opened.get(e, (None, None))
        items = [r for r in got if r['model'].startswith(a.model_prefix)]
        sources = {}
        for r in got:
            sources[r['source']] = sources.get(r['source'], 0) + 1
        # the module's caps: 800 rows a census, 4000 a run (04c's ReferenceShareCensus)
        # (rows that did not parse are not counted in `before`: BAD, below)
        cap = (' (at the census cap of 800: possibly truncated)' if len(got) >= 800 else
               ' (none: the run cap of 4000 reached?)' if not got and before >= 4000 else
               ' (the run cap of 4000 reached during it: truncated)' if before + len(got) >= 4000 else '')
        print('\ncensus %d (cut %s, frame %s): %d rows%s, sources %s' % (e, cut, frame, len(got), cap, dict(sorted(sources.items()))))
        print('model\tkind\tsource\tkeys\tblend\tscale_w\tw(r,g,b)\tmean w\t' + '\t'.join(n for n, _ in AXES) + '\tmask')
        for r in sorted(items, key=lambda r: (r['model'], r['kind'])):
            b = bytes.fromhex(r['sh']) if r.get('sh') else b''
            if len(b) < 64:
                print('%s\t%d\t%d\tsh unread' % (r['model'], r['kind'], r['source']))
                continue
            f = struct.unpack('<16f', b)
            if not all(math.isfinite(x) for x in f):   # the module logs the SH as written (the second step review's L2)
                print('%s\t%d\t%d\tsh nonfinite %s' % (r['model'], r['kind'], r['source'], r['sh']))
                continue
            sh = [f[0:4], f[4:8], f[8:12]]
            mask = f[12:16]
            w = [row[3] for row in sh]
            axes = []
            for _, n in AXES:
                axes.append(sum(max(0.0, row[0] * n[0] + row[1] * n[1] + row[2] * n[2] + row[3]) for row in sh) / 3)
            print('%s\t%d\t%d\t%s\t%.3g\t%s\t%s\t%.4g\t%s\t%s' % (r['model'], r['kind'], r['source'], '/'.join(r['keys']), r['blend'],
                  ','.join('%.3g' % x for x in r['scale']), ','.join('%.4g' % x for x in w), sum(w) / 3,
                  '\t'.join('%.4g' % x for x in axes), ','.join('%.3g' % x for x in mask)) + ('\tNONFINITE' if r.get('nonfinite') else ''))
    if BAD[0]:
        print('\n%d census lines not parsed as JSON' % BAD[0])

if __name__ == '__main__':
    main()
