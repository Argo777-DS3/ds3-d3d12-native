# I-0100 LOD scale study: read-only; finds each hex VA stored as an 8-byte pointer in the image (py -I ptr.py <exe> <VA>...).
import struct,sys
data=open(sys.argv[1],'rb').read()
pe=struct.unpack_from('<I',data,0x3c)[0];nsec=struct.unpack_from('<H',data,pe+6)[0];opt=struct.unpack_from('<H',data,pe+20)[0];sec=pe+24+opt
secs=[struct.unpack_from('<8sIIII',data,sec+40*k) for k in range(nsec)]
def va(o):
    for n,vs,v,rs,ro in secs:
        if ro<=o<ro+rs: return 0x140000000+v+(o-ro),n.rstrip(b'\0').decode()
for a in sys.argv[2:]:
    pat=struct.pack('<Q',int(a,16)); i=data.find(pat)
    while i>=0:
        w=va(i); print(a,'at',[hex(x) if isinstance(x,int) else x for x in w] if w else 'outside every section'); i=data.find(pat,i+1)
