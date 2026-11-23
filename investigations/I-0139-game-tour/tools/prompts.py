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
for f in sorted(glob.glob(base+'m*.txt')):
    mp=os.path.basename(f)[:-4]
    for e,L in evs(mp).items():
        pr=[a.split(',') for i,a in L if i=='3[24]']
        if not pr: continue
        ops=collections.Counter(i for i,a in L if i in('2005[3]','2005[6]','2005[7]','2005[1]','2003[14]','2002[3]','2002[4]','2012[1]','2011[1]','2003[59]','2005[8]','2006[4]','2010[2]'))
        s=[]
        for p in pr[:2]:
            if len(p)>=3 and p[2].lstrip('-').isdigit() and int(p[2])>0:
                r=[x for x in ent.get(p[2]) if x[0].startswith(mp[:6])] or ent.get(p[2])
                s.append(f"text {p[1]} ent {p[2]} "+(f"{r[0][2]}@({r[0][3]},{r[0][4]},{r[0][5]})" if r else ''))
            else: s.append(f"text {p[1] if len(p)>1 else p}")
        print(mp[:6],e,dict(ops),'|',' ; '.join(s))
