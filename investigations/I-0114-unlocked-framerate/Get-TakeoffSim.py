# I-0114 tranche 7: the speed the original would hold after a takeoff delay of n·(1/60 - dt) (TakeoffHand's rounding: a call
# handed while at least half of it is owed), simulated on unlock_jump_step rows: the requested speed of the last handed step (the
# read-back the first held call takes), per run, its mean and range, for n = 2 to 5. RESULT « Seventh tranche ».
# Usage: python -I Get-TakeoffSim.py <startup.jsonl> [...]
import json,sys,math,os,statistics as st
runs={os.path.basename(os.path.dirname(os.path.abspath(p))):[json.loads(l) for l in open(p,encoding='utf-8',errors='replace') if 'unlock_jump_step' in l] for p in sys.argv[1:]}
for n in (2,3,3.5,4,4.5,5):
    line=[]
    for name,rows in runs.items():
        helds=[];i=1
        while i<len(rows):
            a,b=rows[i-1],rows[i]
            if a['1da'] and not b['1da'] and b['1db']:
                dt=b['dt'];owed=n*(1/60-dt);j=i
                while owed>=0.5*dt and j<len(rows): owed-=dt;j+=1
                helds.append(rows[j-1]['speed']);i=j
            i+=1
        line.append(f"{name}: mean {st.mean(helds):.3f} [{min(helds):.3f},{max(helds):.3f}]")
    print(n,' | '.join(line))
