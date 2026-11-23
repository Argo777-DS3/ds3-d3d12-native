# I-0141 tranche 24 (RESULT « Suggested next step » 3): the repeat spread of the cutscene holds against the native-original gap.
# Usage: python Compare-I141HoldSpread.py <pairs.tsv> <pairs.tsv> [...]   (Make-I138HoldPairs.ps1's tables, one per pair of runs)
# For each hold compared in every table with the same target, cut and frame: each mode's YAVG per pair, its range across the pairs
# (max - min: the within-mode spread), the mean native-original difference and its range; « beyond the spread » when |mean
# difference| exceeds both modes' spreads. A hold whose target, cut or frame differ between the tables is refused; a row without a
# number for a luma counts as not compared in that table.
import csv,sys
def num(x):
    try:return float(x)
    except (TypeError,ValueError):return None
def load(p):
    with open(p,encoding='utf-8-sig') as f:   # a BOM from Windows PowerShell's utf8
        return {r['index']:r for r in csv.DictReader(f,delimiter='\t')
                if r['status']=='compared' and num(r['native_luma']) is not None and num(r['original_luma']) is not None}
if len(sys.argv)<3:sys.exit('usage: Compare-I141HoldSpread.py <pairs.tsv> <pairs.tsv> [...]  (two tables at least: one has no spread)')
tables=[load(p) for p in sys.argv[1:]]
keys=sorted(set.intersection(*[set(t) for t in tables]),key=int)
print('hold\tcut\tframe\tnative\toriginal\tnative_spread\toriginal_spread\tmean_diff\tdiff_range\tverdict')
for k in keys:
    ends={(t[k]['target'],t[k]['cut'],t[k]['frame']) for t in tables}
    if len(ends)>1:print(f'{k}\trefused: target, cut or frame differ between the tables {sorted(ends)}');continue
    _,cut,frame=next(iter(ends))
    n=[num(t[k]['native_luma']) for t in tables];o=[num(t[k]['original_luma']) for t in tables];d=[a-b for a,b in zip(n,o)]
    sn=max(n)-min(n);so=max(o)-min(o);md=sum(d)/len(d)
    v='beyond the spread' if abs(md)>max(sn,so) else 'within the spread'
    print(f"{k}\t{cut}\t{frame}\t{'/'.join(f'{x:.1f}' for x in n)}\t{'/'.join(f'{x:.1f}' for x in o)}\t{sn:.1f}\t{so:.1f}\t{md:+.1f}\t{min(d):+.1f}..{max(d):+.1f}\t{v}")
missing=sorted(set.union(*[set(t) for t in tables])-set(keys),key=int)
if missing:print('not compared in every table:',missing)
