# I-0114 tranche 8. Usage: python -I Get-TopClimb.py <run> [...]  (the first run is the reference line)
# the climb to the last step (anorlondo-stairs-top): from the first motion row (speed > 1 m/s), 0-19 s: the terrain field's mean |x|
# (all, the main flight 5-13 s, the upper part 13-19 s), the rise, and the path's distance to the first run's line (max, mean).
import json,sys,math,os,statistics as st
D=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','evidence','I-0066','artifacts')+os.sep
def seg(p,a,b):
    dx,dz=b[0]-a[0],b[1]-a[1];L=dx*dx+dz*dz;u=0 if L==0 else max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[1])*dz)/L));return math.hypot(p[0]-a[0]-u*dx,p[1]-a[1]-u*dz)
ref=None
for r in sys.argv[1:]:
    pos=[];tx=[]
    for l in open(D+r+'/startup.jsonl',encoding='utf-8',errors='replace'):
        if '"unlock_player_position"' in l: o=json.loads(l);pos.append((o['t'],o['at']))
        elif '"unlock_terrain"' in l: o=json.loads(l);tx.append((o['t'],abs(o['x'])))
    t0=None
    for (ta,a),(tb,b) in zip(pos,pos[1:]):
        if ta>45 and math.hypot(b[0]-a[0],b[2]-a[2])/(tb-ta)>1.0: t0=ta;break
    w=[(t-t0,(p[0],p[2]),p[1]) for t,p in pos if 0<=t-t0<=19]
    line=[q for _,q,_ in w]
    if ref is None: ref=line
    d=[min(seg(q,ref[i],ref[i+1]) for i in range(len(ref)-1)) for q in line]
    m=lambda lo,hi: st.mean([x for t,x in tx if lo<=t-t0<=hi])
    print(r.ljust(20),'t0 %.2f |x| all %.4f flight %.4f upper %.4f  rise %.2f  off-line max %.2f mean %.2f  rows %d'%(t0,m(0,19),m(5,13),m(13,19),w[-1][2]-w[0][2],max(d),st.mean(d),len([1 for t,x in tx if 0<=t-t0<=19])))
