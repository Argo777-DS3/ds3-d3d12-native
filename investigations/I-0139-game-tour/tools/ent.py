import sys; sys.stdout.reconfigure(encoding="utf-8")
import sys,csv,os,re
P=os.environ.get('MSBTSV','evidence/I-0139/artifacts/static/cutscenes-events/msb-entities.tsv')
_ent={}
def load():
    if _ent: return
    for l in open(P,encoding='utf-8'):
        c=l.rstrip('\n').split('\t')
        if len(c)<7 or not re.fullmatch(r'-?\d+',c[3]) or c[3] in('-1','0'): continue
        _ent.setdefault(int(c[3]),[]).append((c[0],c[1],c[2],c[4],c[5],c[6],c[7] if len(c)>7 else ''))
def get(i):
    load(); return _ent.get(int(i),[])
if __name__=='__main__':
    for a in sys.argv[1:]:
        for r in get(a): print(a,*r,sep=' | ')

_start=[]
def starts():
    if _start: return _start
    for l in open(P,encoding='utf-8'):
        c=l.rstrip('\n').split('\t')
        if len(c)>=7 and (c[2].startswith('c0000_')) and c[1]=='part':
            try: _start.append((c[0][:6],c[2],float(c[4]),float(c[5]),float(c[6])))
            except: pass
    return _start
def nearest_start(mp,x,y,z):
    best=None
    for m,n,a,b,c in starts():
        if m!=mp[:6]: continue
        d=((a-x)**2+(b-y)**2+(c-z)**2)**.5
        if best is None or d<best[0]: best=(d,n,a,b,c)
    return best
