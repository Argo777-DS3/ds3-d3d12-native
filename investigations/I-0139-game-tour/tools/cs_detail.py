import sys,re,os,glob
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
def pos(i,mp):
    r=[x for x in ent.get(i) if x[0].startswith(mp[:6])] or ent.get(i)
    return '; '.join(f"{x[1]} {x[2]} ({x[3]} {x[4]} {x[5]})" for x in r[:2]) or '-'
for f in sorted(glob.glob(base+'m*.txt'))+[base+'common.txt']:
    mp=os.path.basename(f)[:-4]
    for e,L in evs(mp).items():
        if e==0 or not any(i.startswith('2002[') for i,a in L): continue
        print('###',mp,e)
        seen=set()
        for i,a in L:
            p=a.split(',')
            if i=='3[2]' and len(p)>=3: k=int(p[2]); 
            elif i=='3[24]' and len(p)>=3: k=int(p[2])
            elif i.startswith('2002[') :
                for x in p:
                    if re.fullmatch(r'\d{7}',x): k=int(x)
                    else: continue
                    if k in seen: continue
                    seen.add(k); print('  2002 ent',k,pos(k,mp))
                continue
            else: continue
            if k in seen or k<1000: continue
            seen.add(k); print('  ',i,a,'->',pos(k,mp))
