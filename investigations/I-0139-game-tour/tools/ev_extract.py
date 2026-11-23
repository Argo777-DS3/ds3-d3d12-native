import re,sys,collections
src='evidence/I-0069/queries/remo/emevd-bank2002-all.txt'
ev=collections.OrderedDict()
for l in open(src,encoding='utf-8',newline=''):
    m=re.match(r'(\S+) ev=(\d+) (\d+\[\d+\]) args=(.*)',l.strip())
    if not m: continue
    f,e,i,a=m.groups(); ev.setdefault((f,int(e)),[]).append((i,a))
def show(f,e,maxl=80):
    L=ev.get((f,e),[]);print('##',f,e,len(L),'instr')
    for i,a in L[:maxl]: print('  ',i,a)
if __name__=='__main__':
    sel=sys.argv[1]
    if sel=='cs':
        for (f,e),L in ev.items():
            if any(i.startswith('2002[') for i,a in L) and e!=0: show(f,e,int(sys.argv[2]) if len(sys.argv)>2 else 60)
    else:
        for x in sys.argv[2:]: show(sel,int(x),200)
