import sys,struct
sys.path.insert(0,sys.argv[1])
import bcdec
t=open(sys.argv[2],'rb').read()
n=struct.unpack_from('<I',t,8)[0]
want=sys.argv[4:]
for k in range(n):
    o=0x10+k*0x14
    off,size,fmt,typ,mips,flags=struct.unpack_from('<IIBBBB',t,o);nameoff=struct.unpack_from('<I',t,o+0xC)[0]
    e=nameoff
    while t[e:e+2]!=b'\0\0':e+=2
    name=t[nameoff:e].decode('utf-16-le')
    if name not in want:continue
    dds=t[off:off+size];h,w=struct.unpack_from('<II',dds,12);fourcc=dds[84:88]
    head=128+(20 if fourcc==b'DX10' else 0)
    kind='bc7' if fourcc==b'DX10' else 'bc1'
    blocks=((w+3)//4)*((h+3)//4)*(16 if kind=='bc7' else 8)
    rgba,_=bcdec.decode_image(dds[head:head+blocks],w,h,kind)
    bcdec.write_png(sys.argv[3]+'/'+name+'.png',w,h,rgba)
    print(name,w,h,kind,flush=True)
