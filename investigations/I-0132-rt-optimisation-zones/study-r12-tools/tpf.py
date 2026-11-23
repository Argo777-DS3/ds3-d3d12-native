import sys,struct,zlib
d=open(sys.argv[1],'rb').read()
assert d[:4]==b'DCX\0', d[:4]
fmt=d[0x28:0x2c]; print('dcx',fmt)
i=d.index(b'DCA\0')+8
t=zlib.decompress(d[i:])
assert t[:4]==b'TPF\0'
n=struct.unpack_from('<I',t,8)[0]; enc=t[0xE]
print('textures',n,'enc',enc)
rows=[]
for k in range(n):
    o=0x10+k*0x14
    off,size,fmt,typ,mips,flags=struct.unpack_from('<IIBBBB',t,o); nameoff=struct.unpack_from('<I',t,o+0xC)[0]
    if enc==1:
        e=nameoff
        while t[e:e+2]!=b'\0\0': e+=2
        name=t[nameoff:e].decode('utf-16-le')
    else:
        name=t[nameoff:t.index(b'\0',nameoff)].decode('latin1')
    dds=t[off:off+128]; h,w=struct.unpack_from('<II',dds,12); fourcc=dds[84:88]
    rows.append((name,w,h,fmt,fourcc,size))
want=sys.argv[2:]
for r in rows:
    if not want or any(x.lower() in r[0].lower() for x in want): print(r)
open(sys.argv[1].replace('.dcx','')+'.out','wb').write(t)   # the decompressed TPF, beside its input
