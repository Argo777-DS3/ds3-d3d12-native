# I-0114 tranche 8: the stairs path's three walking legs (whole legs: 0-8.3, 10-18.3 and 20-28.3 s from the first leg's start,
# which Get-Stairs.py prints) per run: the terrain field's mean |x| (unlock_terrain rows), leg 3's heading (degrees of its
# horizontal displacement), its end (x, z) and its distance to the first run's leg-3 line (max, mean), and the camera's yaw
# at leg 3's start (19.9 s). Usage: python -I Get-StairsLines.py <run>=<first leg's start, s> [...]  (runs under evidence/I-0066/artifacts)
import json,sys,math,os,statistics as st
base=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','evidence','I-0066','artifacts')
def seg(p,a,b):
    dx,dz=b[0]-a[0],b[1]-a[1];L=dx*dx+dz*dz;u=0 if L==0 else max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[1])*dz)/L))
    return math.hypot(p[0]-a[0]-u*dx,p[1]-a[1]-u*dz)
ref=None
for arg in sys.argv[1:]:
    run,t0=arg.split('=');t0=float(t0);pos=[];tx=[]
    for l in open(os.path.join(base,run,'startup.jsonl'),encoding='utf-8',errors='replace'):
        if '"unlock_player_position"' in l: o=json.loads(l);pos.append((o['t']-t0,o['at'][0],o['at'][2],o['cam']['yaw']))
        elif '"unlock_terrain"' in l: o=json.loads(l);tx.append((o['t']-t0,abs(o['x'])))
    legs=[st.mean([x for t,x in tx if lo<=t<=hi]) for lo,hi in ((0,8.3),(10,18.3),(20,28.3))]
    leg3=[(x,z) for t,x,z,_ in pos if 20-0.05<=t<=28.3]
    if ref is None: ref=leg3
    d=[min(seg(q,ref[i],ref[i+1]) for i in range(len(ref)-1)) for q in leg3]
    heading=math.degrees(math.atan2(leg3[-1][1]-leg3[0][1],leg3[-1][0]-leg3[0][0]))
    yaw=math.degrees(min(pos,key=lambda q:abs(q[0]-19.9))[3])
    print('%-20s legs |x| %.4f %.4f %.4f  leg3 heading %+.1f end (%.2f, %.2f) off-line max %.2f mean %.2f  yaw %.1f'%(run,*legs,heading,*leg3[-1],max(d),st.mean(d),yaw))
