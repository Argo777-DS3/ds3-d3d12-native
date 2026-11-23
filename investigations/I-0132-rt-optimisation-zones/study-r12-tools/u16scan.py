import sys,re
data=open(sys.argv[1],'rb').read()
# PE section mapping
import struct
pe=struct.unpack_from('<I',data,0x3c)[0]
nsec=struct.unpack_from('<H',data,pe+6)[0]; opt=struct.unpack_from('<H',data,pe+20)[0]
secs=[]
for i in range(nsec):
    o=pe+24+opt+40*i
    name=data[o:o+8].rstrip(b'\0').decode(); vs,va,rs,ra=struct.unpack_from('<IIII',data,o+8)
    secs.append((name,va,vs,ra,rs))
def rva(off):
    for n,va,vs,ra,rs in secs:
        if ra<=off<ra+rs: return 0x140000000+va+off-ra,n
    return None,None
pat=re.compile(rb'(?:[\x20-\x7e]\x00){4,}')
needles=[s.lower() for s in sys.argv[2:]]
for m in pat.finditer(data):
    s=m.group().decode('utf-16-le')
    if any(n in s.lower() for n in needles):
        a,n=rva(m.start()); print(hex(a) if a else '?',n,s)
