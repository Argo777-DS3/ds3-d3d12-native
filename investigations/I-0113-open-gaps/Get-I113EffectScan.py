# I-0113 tranche 29: the effect-scan.flag rows of a native journal (native_effect_scan, native_effect_scan_window) summed by stage:
# pictures flagged, NaN, inf and negative halves by channel (R, G, B, A), the largest count a picture, and the bursts (runs of
# consecutive serials with a flagged copy) with their serial span and the union of their boxes.
# Usage: python -I Get-I113EffectScan.py <startup.jsonl> [...]
import json, sys
for path in sys.argv[1:]:
    rows=[]; window=None
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"native_effect_scan' not in line: continue
            try: r=json.loads(line)
            except ValueError: continue
            if r.get('event')=='native_effect_scan': rows.append(r)
            elif r.get('event')=='native_effect_scan_window': window=r
    print(path)
    print('  window:', window)
    for stage in ('before','copy','copy_after','after'):
        s=[r for r in rows if r['stage']==stage]
        if not s: print(f'  {stage}: no row'); continue
        tot={k:[sum(r[k][c] for r in s) for c in range(4)] for k in ('nan','inf','neg')}
        mx={k:[max(r[k][c] for r in s) for c in range(4)] for k in ('nan','inf','neg')}
        print(f'  {stage}: {len(s)} pictures flagged; sums nan {tot["nan"]} inf {tot["inf"]} neg {tot["neg"]}; max a picture neg {mx["neg"]} nan {mx["nan"]} inf {mx["inf"]}')
    copies=sorted({r['serial'] for r in rows if r['stage']=='copy'})
    bursts=[]
    for v in copies:
        if bursts and v-bursts[-1][-1]<=3: bursts[-1].append(v)
        else: bursts.append([v])
    for b in bursts:
        rs=[r for r in rows if r['stage']=='copy' and b[0]<=r['serial']<=b[-1]]
        rs=[r for r in rs if r['neg_box'][2]>0] or rs   # the step review's L5: rows with a negative only (an empty box is {0,0,0,0})
        box=[min(r['neg_box'][0] for r in rs),min(r['neg_box'][1] for r in rs),max(r['neg_box'][2] for r in rs),max(r['neg_box'][3] for r in rs)]
        peak=max(rs,key=lambda r:sum(r['neg']))
        print(f'  copy burst serials {b[0]}-{b[-1]} ({len(b)} frames): negative box {box}, peak {peak["serial"]} neg {peak["neg"]}')
