import sys,re,os,glob,collections
sys.stdout.reconfigure(encoding='utf-8')
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
import ent
base='evidence/I-0139/artifacts/static/cutscenes-events/emevd/'
def evs(f):
    d={};cur=None
    for l in open(base+f+'.txt',encoding='utf-8'):
        if l.startswith('EVENT'): cur=int(l.split()[1]); d[cur]=[]
        else:
            m=re.match(r'\s+\[(\d+)\] (\d+\[\d+\]) ?(.*)',l.rstrip())
            if m: d[cur].append((m.group(2),m.group(3)))
    return d
OPS=sys.argv[2].split(',')
mp=sys.argv[1]
def nm(k,mp):
    r=[x for x in ent.get(k) if x[0].startswith(mp[:6])] or ent.get(k)
    return ','.join(f"{x[2]}@({x[3]},{x[4]},{x[5]})" for x in r[:1])
for e,L in evs(mp).items():
    hit=[(i,a) for i,a in L if i in OPS]
    if not hit: continue
    s=[]
    for i,a in hit[:int(sys.argv[3]) if len(sys.argv)>3 else 4]:
        p=a.split(',');ent_id=p[0] if i.startswith(('2005','2012','2011')) else None
        n=nm(int(ent_id),mp) if ent_id and ent_id.lstrip('-').isdigit() and int(ent_id)>0 else ''
        s.append(f"{i} {a} {n}")
    print(mp,e,len(L),'|',' ; '.join(s))
