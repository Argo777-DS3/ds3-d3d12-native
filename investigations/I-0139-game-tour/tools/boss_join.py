# I-0139 bosses: joins EMEVD template calls (20005500 bonfire-after-boss, 20005800 boss fight) with the MSB dumps. Run with python -I.
import re,glob,os,sys,io
E='evidence/I-0139/artifacts/static/bosses'
def msb(m):
    f=glob.glob(f'{E}/*mapstudio_{m}_00_00.msb.txt')[0]
    d={}
    for l in io.open(f,encoding='utf-8-sig'):
        p=l.rstrip('\n').split('\t')
        if len(p)>=5 and p[3] not in('0','-1',''): d.setdefault(p[3],[]).append(p)
    return d
def calls(m,tid):
    out=[]
    for l in io.open(f'{E}/event_{m}_00_00.emevd.txt',encoding='utf-8'):
        mm=re.search(r'\] 2000\[6\] (\d+),(.*)',l)
        if mm and mm.group(1)==str(tid): out.append(mm.group(2).split(','))
    return out
if __name__=='__main__':
    sys.stdout.reconfigure(encoding='utf-8')
    for m in sys.argv[1:]:
        d=msb(m); print('=====',m)
        for tid in (5500,5800,5801,5810,5820,5831,5837):
            for a in calls(m,20000000+tid):
                s=[]
                for x in a:
                    if x in d: s.append(f'{x}={d[x][0][0]}:{d[x][0][1]}@{d[x][0][4]}')
                print(tid,','.join(a),'|',' ; '.join(s))
