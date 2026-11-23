# I-0114 tranche 3: the lock-on path (scripts/replay/settlement-lockon.path.json) read by run name from locomotion-observe.flag and
# mechanics-observe.flag's rows (every 1/30 s of the logic's time): for each window (times from the motion's start,
# taken as the first row the player moves faster than 1 m/s, less 0.15 s): the follow camera's yaw turned and the time it took to
# reach 90 % of it, and the player's distance covered. Windows: lock (the press at 3.3 s, to 4.0 s), strafe-right (4.0-6.0),
# strafe-left (6.0-8.0), switch (the mouse move at 8.0, to 9.0), strafe-right-2 (9.0-11.0), strafe-left-2
# (11.0-13.0): one cycle since series 26 (series 25's runs had two, the second cut by a lost foreground; their first reads
# the same). The key times and the logic's time drift apart by a few frames: compare runs window by window.
# Usage: python Get-Lockon.py <run> [<run> ...]   (runs under evidence/I-0066/artifacts)
import json,sys,math,os
base=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','evidence','I-0066','artifacts')
windows=[('lock',3.3,4.0),('strafe-right',4.0,6.0),('strafe-left',6.0,8.0),('switch',8.0,9.0),('strafe-right-2',9.0,11.0),('strafe-left-2',11.0,13.0)]
def wrap(a):
    while a>math.pi:a-=2*math.pi
    while a<-math.pi:a+=2*math.pi
    return a
for run in sys.argv[1:]:
    rows=[]
    for l in open(os.path.join(base,run,'startup.jsonl'),encoding='utf-8',errors='replace'):
        if '"event":"unlock_player_position"' in l:
            try:r=json.loads(l)
            except Exception:continue
            rows.append((r['t'],r['at'],r['cam']['yaw']))
    if not rows:print(run,'no rows');continue
    start=next((a[0] for a,b in zip(rows,rows[1:]) if b[0]>a[0] and math.hypot(b[1][0]-a[1][0],b[1][2]-a[1][2])/(b[0]-a[0])>1.0),None)
    if start is None:print(run,'no motion');continue
    start-=0.15
    print(f"{run}: {len(rows)} rows, motion from {start:.2f} s")
    for name,a,b in windows:
        t0,t1=start+a,start+b
        w=[r for r in rows if t0<=r[0]<=t1]
        if len(w)<2:print(f"  {name}: no rows");continue
        turned=0.0;path=[]
        for p,q in zip(w,w[1:]):turned+=wrap(q[2]-p[2]);path.append(turned)
        reach=next((w[i+1][0]-t0 for i,v in enumerate(path) if abs(v)>=0.9*abs(turned)),float('nan')) if abs(turned)>1e-3 else float('nan')
        dist=sum(math.hypot(q[1][0]-p[1][0],q[1][2]-p[1][2]) for p,q in zip(w,w[1:]))
        print(f"  {name:15s} yaw {turned:+.3f} rad, 90 % at {reach:.2f} s, moved {dist:.2f} m")
