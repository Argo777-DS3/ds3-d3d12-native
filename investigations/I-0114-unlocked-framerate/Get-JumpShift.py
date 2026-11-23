# I-0114 tranche 7: the jump's root-motion lag against 60. For each jump (the jump state's start, [+0x1DB] 0 -> 1, in the
# unlock_jump_step rows), the requested speed [ctrl+0x260] placed at each step's middle and fitted to the first 60 run's first jump over
# 40-110 ms of the state by least squares on a 0.1 ms grid: the lag in ms (positive: later than 60). RESULT « Seventh tranche ».
# Usage: python -I Get-JumpShift.py <60 startup.jsonl> <other startup.jsonl> [...]
import json,sys,math,os
def jumps(path):
    rows=[json.loads(l) for l in open(path,encoding='utf-8',errors='replace') if 'unlock_jump_step' in l]
    out=[];cur=None
    for i in range(1,len(rows)):
        a,b=rows[i-1],rows[i]
        if not a['1db'] and b['1db']:
            cur=[];out.append(cur)
        if cur is not None:
            cur.append((b['t'],b['speed']))
    res=[]
    for c in out:
        t0=c[0][0]; res.append([(t-t0,s) for t,s in c if t-t0<=0.18])
    return res
def interp(p,t):
    for (a,sa),(b,sb) in zip(p,p[1:]):
        if a<=t<=b: return sa+(sb-sa)*(t-a)/(b-a) if b>a else sa
    return None
ref=jumps(sys.argv[1])
# the 60 profile: the requested speed is the step's displacement over the step ending at t; place it at mid-step
def mid(p,dt): return [(t-dt/2,s) for t,s in p]
r=[mid(p,1/60) for p in ref[:6]]
for path in sys.argv[2:]:
    J=jumps(path)
    best=[]
    for p in J[:8]:
        dt=(p[-1][0]-p[0][0])/(len(p)-1)
        q=mid(p,dt)
        bs=None
        for k in range(0,800):
            s=k/10000
            err=0;n=0
            for t,v in q:
                if 0.04<=t-s<=0.11:
                    w=interp(r[0],t-s)
                    if w is not None: err+=(w-v)**2;n+=1
            if n>=3 and (bs is None or err/n<bs[1]): bs=(s,err/n)
        best.append(round(bs[0]*1000,1) if bs else None)
    print(os.path.basename(os.path.dirname(os.path.abspath(path))),'dt=%.4f'%dt,'shift ms',best)
