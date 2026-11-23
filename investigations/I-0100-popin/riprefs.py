# I-0100 LOD scale study: read-only scan of an image (py -I riprefs.py <exe> <hex VA>...): every 4-byte RIP-relative
# displacement in the executable sections that lands within 4 bytes of a target, for 0-4 bytes of immediate after it; finds
# code Ghidra left undisassembled (the initializer at 142640640 writing 14473F6CC).
import struct,sys
data=open(sys.argv[1],'rb').read()
pe=struct.unpack_from('<I',data,0x3c)[0]
nsec=struct.unpack_from('<H',data,pe+6)[0]; opt=struct.unpack_from('<H',data,pe+20)[0]
base=0x140000000; sec=pe+24+opt
targets=[int(x,16) for x in sys.argv[2:]]
for k in range(nsec):
    name,vs,va,rs,ro=struct.unpack_from('<8sIIII',data,sec+40*k)
    ch=struct.unpack_from('<I',data,sec+40*k+36)[0]
    if not ch&0x20000000: continue
    blob=data[ro:ro+rs]
    for i in range(len(blob)-4):
        d=struct.unpack_from('<i',blob,i)[0]
        for tail in range(0,5):
            t=base+va+i+4+tail+d
            for tg in targets:
                if tg-4<=t<=tg+4:
                    print(name.rstrip(b'\0').decode(),hex(base+va+i),'tail',tail,'->',hex(t),blob[max(0,i-4):i+4+tail].hex())
