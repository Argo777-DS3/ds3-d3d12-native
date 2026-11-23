# I-0114: the player's legs (distances between stops) and speed from locomotion-observe.flag's rows, by run name.
# Usage: python Get-LocomotionLegs.py <run> [<run> ...]   (runs under evidence/I-0066/artifacts)
import json,sys,math,os
base=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','evidence','I-0066','artifacts')
for run in sys.argv[1:]:
    rows=[]
    for l in open(f'{base}\\{run}\\startup.jsonl',encoding='utf-8',errors='replace'):
        if '"event":"unlock_player_position"' in l:
            try:r=json.loads(l)
            except Exception:continue
            rows.append((r['t'],r['at']))
    if not rows:print(run,'no rows');continue
    # The motion: from the first move to the last move (horizontal displacement over 1 cm between rows).
    steps=[]
    for (t0,a),(t1,b) in zip(rows,rows[1:]):
        d=math.hypot(b[0]-a[0],b[2]-a[2]);steps.append((t0,t1,d))
    moving=[s for s in steps if s[2]>0.01]
    if not moving:print(run,'no motion');continue
    t_start,t_end=moving[0][0],moving[-1][1]
    path=sum(s[2] for s in steps if t_start<=s[0] and s[1]<=t_end)
    # Segments between reversals of the dominant axis direction: the walk's legs (Z 4 s, S 4 s).
    legs=[];cur=0.0;last=None
    for (t0,a),(t1,b) in zip(rows,rows[1:]):
        dx,dz=b[0]-a[0],b[2]-a[2];d=math.hypot(dx,dz)
        if d<0.01:
            if cur>0.2:legs.append(round(cur,2))
            cur=0.0;last=None;continue
        cur+=d
    if cur>0.2:legs.append(round(cur,2))
    speed=[s[2]/(s[1]-s[0]) for s in moving if s[1]>s[0]]
    speed.sort()
    print(f'{run}: rows {len(rows)}, motion {t_end-t_start:.1f} s logic, path {path:.2f} m, median speed {speed[len(speed)//2]:.3f} m/s, legs {legs}')
