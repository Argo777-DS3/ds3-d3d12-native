# I-0114 tranche 3: the jumps path (scripts/replay/firelink-jumps.path.json) read by run name from locomotion-observe.flag and
# mechanics-observe.flag's rows (every 1/30 s of the logic's time; times from the motion's start, the first row faster than 1 m/s,
# less 0.15 s): for each half cycle h (out at 6c s, back at 6c+3 s), the jump's window [h+1.55, h+2.7] (the distance and the
# rise: the window's highest row less its first) and its fastest row; the roll windows and the sprint-and-stop windows.
# Usage: python Get-Jumps.py <run> [<run> ...]   (runs under evidence/I-0066/artifacts)
import json,sys,math,os
base=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','evidence','I-0066','artifacts')
def dist(w):return sum(math.hypot(b[1][0]-a[1][0],b[1][2]-a[1][2]) for a,b in zip(w,w[1:]))
def peak(w):return max((math.hypot(b[1][0]-a[1][0],b[1][2]-a[1][2])/(b[0]-a[0]) for a,b in zip(w,w[1:]) if b[0]>a[0]),default=0.0)
for run in sys.argv[1:]:
    P=[]
    for l in open(os.path.join(base,run,'startup.jsonl'),encoding='utf-8',errors='replace'):
        if '"event":"unlock_player_position"' in l:
            try:r=json.loads(l)
            except Exception:continue
            P.append((r['t'],r['at']))
    if not P:print(run,'no rows');continue
    s=next((a[0] for a,b in zip(P,P[1:]) if b[0]>a[0] and math.hypot(b[1][0]-a[1][0],b[1][2]-a[1][2])/(b[0]-a[0])>1.0),None)
    if s is None:print(run,'no motion');continue
    s-=0.15
    win=lambda a,b:[p for p in P if s+a<=p[0]<=s+b]
    jumps=[]
    for c in range(4):
        for h in (6*c,6*c+3):
            w=win(h+1.55,h+2.7);jumps.append((dist(w),max(p[1][1] for p in w)-w[0][1][1],peak(w)))
    rolls=[dist(win(24+5*c+a,24+5*c+a+1.2)) for c in range(2) for a in (0.75,3.25)]
    sprints=[dist(win(34+5*c+a,34+5*c+a+2.45)) for c in range(2) for a in (0,2.5)]
    print(f"{run}: jumps (m, rise, fastest m/s) {' '.join(f'{d:.2f}/+{h:.2f}/{v:.2f}' for d,h,v in jumps)}")
    print(f"  roll windows {' '.join(f'{d:.2f}' for d in rolls)} m; sprint-and-stop windows {' '.join(f'{d:.2f}' for d in sprints)} m")
