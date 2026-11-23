# I-0100 LOD scale study: read-only; prints 64 bytes of the image at each hex VA (py -I bytesat.py <exe> <VA>...).
import struct,sys
data=open(sys.argv[1],'rb').read()
pe=struct.unpack_from('<I',data,0x3c)[0]
nsec=struct.unpack_from('<H',data,pe+6)[0]; opt=struct.unpack_from('<H',data,pe+20)[0]; sec=pe+24+opt
def off(va):
    r=va-0x140000000
    for k in range(nsec):
        name,vs,v,rs,ro=struct.unpack_from('<8sIIII',data,sec+40*k)
        if v<=r<v+rs: return ro+(r-v)   # not the uninitialised tail
for a in sys.argv[2:]:
    va=int(a,16); o=off(va); print(hex(va),data[o:o+64].hex(' '))
