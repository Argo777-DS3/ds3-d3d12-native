# I-0114 (UNLOCK-READY §5): the mechanics of scripts/replay/firelink-mechanics.path.json read from mechanics-observe.flag's rows
# (unlock_player_position every 1/30 s of the logic's time: position "at", the physics module's heading phys70[1], the follow
# camera's yaw "cam.yaw"), by run name, so 60 and the unlocked rates compare event by event.
# Usage: python Get-Mechanics.py <run> [<run> ...]   (runs under evidence/I-0066/artifacts); two lines a run: by key windows, then
# « events » from the speeds (the raw events RESULT series 21's ranges were read from, not sensitive to the keys' drift against the
# logic's time; their windows are this path's layout: 0-20 s the rolls, 56-72 s the attacks and backsteps).
# The path's times are aligned on the first motion (the roll segment's Z at 0 ms): t0 = the first row moved over 1 cm of the first
# sustained motion (0.5 m within a second), less the same 0.1 s at every rate. Per event, over a fixed window from its key time:
# - roll: the distance covered from the Space tap to 1.2 s after it (the walk's share included, the same at every rate);
# - stop: the distance from the keys' release to the stop (the first row under 0.2 m/s), and the time to it;
# - turn: the time from the key until the heading is within 0.05 rad of where it ends the leg, and the heading turned;
# - attack, backstep: the distance covered in the 1.5 s after the button or the tap;
# - camera: the time from each turn key until the follow camera's yaw stays within 0.02 rad of its value 0.95 s after the key
#   (the follow lag's settle), and the yaw it turned;
# - recentre: the same settle and yaw for the middle button's press (lock-on without a target), within 1.5 s.
import json,sys,math,os
base=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','evidence','I-0066','artifacts')
path=json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','scripts','replay','firelink-mechanics.path.json'),encoding='utf-8'))
def wrap(a):return (a+math.pi)%(2*math.pi)-math.pi
for run in sys.argv[1:]:
    rows=[]
    for l in open(os.path.join(base,run,'startup.jsonl'),encoding='utf-8',errors='replace'):
        if '"event":"unlock_player_position"' in l:
            try:r=json.loads(l)
            except Exception:continue
            rows.append((r['t'],r['at'],r['phys70'][1],r['cam']['yaw'] if 'cam' in r else 0.0))
    if len(rows)<10:print(run,'no rows');continue
    # I-0132 (workstream 2's step review M1): without a follow camera seen (mechanics-observe alone with no-unlock-fix: neither camera_laws
    # nor the observing thunk) every row's camera is 0, so the camera's figures are not read for that run.
    if all(not r[3] for r in rows):print(run,'note: no follow camera seen in the rows (camera_lag and recentre not meaningful)')
    # The first sustained motion (over 0.5 m within the next second: a settle's small shift at the start is not the path), then
    # back to its first row moved over 1 cm.
    t0=None
    for i,a in enumerate(rows):
        later=[r for r in rows[i:] if r[0]<=a[0]+1.0]
        if math.hypot(later[-1][1][0]-a[1][0],later[-1][1][2]-a[1][2])>0.5:
            for x,y in zip(rows[i:],rows[i+1:]):
                if math.hypot(y[1][0]-x[1][0],y[1][2]-x[1][2])>0.01:t0=x[0]-0.1;break
            break
    if t0 is None:print(run,'no motion');continue
    def at(t):   # the row at or just after t
        for r in rows:
            if r[0]>=t:return r
        return rows[-1]
    def dist(t1,t2):
        d=0.0;prev=None
        for r in rows:
            if t1<=r[0]<=t2:
                if prev:d+=math.hypot(r[1][0]-prev[1][0],r[1][2]-prev[1][2])
                prev=r
        return d
    out={'roll':[],'stop':[],'turn':[],'turn_rad':[],'camera_lag':[],'attack':[],'backstep':[],'recentre':[]}
    start=0.0
    for seg in path['segments']:
        for c in range(seg['cycles']):
            s=t0+(start+c*seg['cycle_ms'])/1000.0
            ev=seg['events']
            if seg['name']=='roll':
                for e in ev:
                    if e.get('key')=='Space' and e['down']:out['roll'].append(round(dist(s+e['t']/1000,s+e['t']/1000+1.2),2))
            elif seg['name']=='stop':
                for e in ev:
                    if e.get('key') in ('Z','S') and not e['down']:
                        tr=s+e['t']/1000;stop=None;prev=at(tr)
                        for r in rows:
                            if r[0]>tr and r[0]-prev[0]>0:
                                v=math.hypot(r[1][0]-prev[1][0],r[1][2]-prev[1][2])/(r[0]-prev[0])
                                if v<0.2:stop=r;break
                                prev=r
                        if stop:out['stop'].append((round(dist(tr,stop[0]),2),round(stop[0]-tr,2)))
            elif seg['name']=='turn':
                for e in ev:
                    if e.get('key') and e['down']:
                        tk=s+e['t']/1000;end=at(tk+0.55);h0=at(tk)[2];reach=None
                        for r in rows:
                            if tk<=r[0]<=tk+0.55 and abs(wrap(r[2]-end[2]))<0.05:reach=r;break
                        out['turn'].append(round(reach[0]-tk,3) if reach else None);out['turn_rad'].append(round(wrap(end[2]-h0),2))
                        cend=at(tk+0.95)[3];settle=None
                        for r in rows:
                            if tk<=r[0]<=tk+0.95 and all(abs(wrap(q[3]-cend))<0.02 for q in rows if r[0]<=q[0]<=tk+0.95):settle=r;break
                        out['camera_lag'].append((round(settle[0]-tk,3) if settle else None,round(wrap(cend-at(tk)[3]),3)))
            elif seg['name']=='recentre':
                for e in ev:
                    if e.get('button')==3 and e['down']:
                        tk=s+e['t']/1000;cend=at(tk+1.5)[3];settle=None
                        for r in rows:
                            if tk<=r[0]<=tk+1.5 and all(abs(wrap(q[3]-cend))<0.02 for q in rows if r[0]<=q[0]<=tk+1.5):settle=r;break
                        out['recentre'].append((round(settle[0]-tk,3) if settle else None,round(wrap(cend-at(tk)[3]),3)))
            elif seg['name'] in ('attack','backstep'):
                for e in ev:
                    if e['down']:
                        tk=s+e['t']/1000;out[seg['name']].append(round(dist(tk,tk+1.5),2))
        start+=seg['cycles']*seg['cycle_ms']
    # By event, from the speeds (the figures of RESULT series 21: the keys' times drift against the logic's, the speeds do not):
    # rolls (above 4.6 m/s in the roll segment), the sprint's stops (the last row at 5 m/s to under 0.2 m/s), and the attack and
    # backstep moves (above 0.5 m/s in their segments): distance, duration, peak.
    sp=[]
    for a,b in zip(rows,rows[1:]):
        dt=b[0]-a[0]
        if dt>0:d=math.hypot(b[1][0]-a[1][0],b[1][2]-a[1][2]);sp.append((b[0],d/dt,d))
    def moves(lo,hi,th):
        found=[];i=0
        while i<len(sp):
            if lo<=sp[i][0]-t0<=hi and sp[i][1]>th:
                j=i;d=0.0;peak=0.0
                while j<len(sp) and sp[j][1]>th:d+=sp[j][2];peak=max(peak,sp[j][1]);j+=1
                found.append((round(d,2),round(sp[j-1][0]-sp[i][0],2),round(peak,1)));i=j
            else:i+=1
        return found
    stops=[];i=0
    while i<len(sp):
        if sp[i][1]>5.0:
            j=i
            while j<len(sp) and sp[j][1]>5.0:j+=1
            if sp[j-1][0]-sp[i][0]>0.5:
                k=j;d=0.0
                while k<len(sp) and sp[k][1]>=0.2:d+=sp[k][2];k+=1
                if k<len(sp):stops.append((round(d,2),round(sp[k][0]-sp[j-1][0],2)))
            i=j
        else:i+=1
    events={'rolls':moves(0,20,4.6),'sprint_stops':stops,'attack_backstep':moves(56,72,0.5)}
    print(run,json.dumps(out))
    print(run,'events',json.dumps(events))
