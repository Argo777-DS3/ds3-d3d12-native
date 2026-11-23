import sys,struct,re
sys.path.insert(0,__import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from swftags import load,tags
for p in sys.argv[1:]:
    d=load(p); imgs=[]; abc=b''; exports=[]
    for code,o,ln in tags(d):
        if code==1009:
            body=d[o:o+ln]
            m=re.findall(rb'[A-Za-z0-9_\-\.]{5,}',body); imgs+= [x.decode() for x in m]
        if code==82: abc+=d[o:o+ln]
        if code==76 or code==56:
            n=struct.unpack_from('<H',d,o)[0]; q=o+2
            for i in range(n):
                q+=2; e=d.index(b'\0',q); exports.append(d[q:e].decode('latin1')); q=e+1
    strs=sorted(set(x.decode() for x in re.findall(rb'[A-Za-z_][A-Za-z0-9_]{3,}',abc)))
    print('==',p.split('/')[-1]); print(' images:',sorted(set(imgs)))
    print(' symbols:',exports[:20])
    print(' abc strings (',len(strs),'):',' '.join(strs)[:3500])
