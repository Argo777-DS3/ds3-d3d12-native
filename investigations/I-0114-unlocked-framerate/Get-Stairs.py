# I-0114 tranche 3: the stairs path (scripts/replay/anorlondo-stairs.path.json) read by run name from locomotion-observe.flag and
# mechanics-observe.flag's rows (every 1/30 s of the logic's time, so every rate is sampled alike): each leg (the motion between two
# stops) with its horizontal distance, its rise and its duration; the vertical jitter (the mean |y(n) - 2 y(n-1) + y(n-2)| of the
# rows while moving: a hop shows there); the player's terrain response [+0x2DC] (unlock_terrain): its total variation a second of
# the logic's time over the motion, the branches seen, the corrected and mismatched calls; the end position.
# Usage: python Get-Stairs.py <run> [<run> ...]   (runs under evidence/I-0066/artifacts)
import json,sys,math,os
from collections import Counter
base=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','evidence','I-0066','artifacts')
for run in sys.argv[1:]:
    rows=[];terrain=[]
    for l in open(os.path.join(base,run,'startup.jsonl'),encoding='utf-8',errors='replace'):
        if '"event":"unlock_player_position"' in l or '"event":"unlock_terrain"' in l:
            try:r=json.loads(l)
            except Exception:continue
            (rows if r['event']=='unlock_player_position' else terrain).append(r)
    if not rows:print(run,'no rows');continue
    pos=[(r['t'],r['at']) for r in rows]
    moving=[(t0,t1) for (t0,a),(t1,b) in zip(pos,pos[1:]) if math.hypot(b[0]-a[0],b[2]-a[2])>0.01]
    if not moving:print(run,'no motion');continue
    t_start,t_end=moving[0][0],moving[-1][1]
    legs=[];cur=None
    for (t0,a),(t1,b) in zip(pos,pos[1:]):
        d=math.hypot(b[0]-a[0],b[2]-a[2])
        if d<=0.01:
            if cur and cur['h']>0.5:legs.append(cur)
            cur=None;continue
        if cur is None:cur={'t':t0,'h':0.0,'y0':a[1]}
        cur['h']+=d;cur['y1']=b[1];cur['t1']=t1
    if cur and cur['h']>0.5:legs.append(cur)
    jitter=[];window=[p for p in pos if t_start<=p[0]<=t_end]
    for (_,a),(_,b),(_,c) in zip(window,window[1:],window[2:]):jitter.append(abs(c[1]-2*b[1]+a[1]))
    print(f"{run}: {len(rows)} rows, motion {t_start:.2f}-{t_end:.2f} s, end ({pos[-1][1][0]:.2f}, {pos[-1][1][1]:.2f}, {pos[-1][1][2]:.2f})")
    for g in legs:
        dt=g['t1']-g['t'];rise=g['y1']-g['y0']
        print(f"  leg at {g['t']:.2f} s: {g['h']:.2f} m in {dt:.2f} s ({g['h']/dt if dt else 0:.2f} m/s), rise {rise:+.2f} m ({rise/dt if dt else 0:+.3f} m/s)")
    if jitter:print(f"  vertical jitter while moving: mean {sum(jitter)/len(jitter):.5f} m, largest {max(jitter):.4f} m over {len(jitter)} rows")
    seen=[r for r in terrain if t_start<=r['t']<=t_end]
    if len(seen)>=2:
        span=seen[-1]['t']-seen[0]['t'];var=seen[-1]['variation']-seen[0]['variation']
        xs=[r['x'] for r in seen]
        print(f"  terrain [+0x2DC]: variation {var/span if span else 0:.4f} a second over {span:.1f} s, x from {min(xs):+.3f} to {max(xs):+.3f}, mean |x| {sum(abs(x) for x in xs)/len(xs):.4f}; "
              f"rows by site/branch {dict(Counter((r['site'],r['branch']) for r in seen))}; corrected {terrain[-1]['corrected']}, mismatched {terrain[-1]['mismatched']}")
    else:print('  no terrain rows')
