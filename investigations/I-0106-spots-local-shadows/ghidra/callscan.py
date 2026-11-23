# I-0106: read-only raw scan of the image: every E8 (call) or E9 (jmp) rel32 in the executable sections whose target is
# one of the given addresses, and every 8-byte absolute pointer in any section equal to one of them. Finds callers in
# code Ghidra left undisassembled. Usage: py -I callscan.py <exe> <hex VA>...
import struct,sys
data=open(sys.argv[1],'rb').read()
pe=struct.unpack_from('<I',data,0x3c)[0]
nsec=struct.unpack_from('<H',data,pe+6)[0]; opt=struct.unpack_from('<H',data,pe+20)[0]
base=0x140000000; sec=pe+24+opt
targets=set(int(x,16) for x in sys.argv[2:])
for k in range(nsec):
    name,vs,va,rs,ro=struct.unpack_from('<8sIIII',data,sec+40*k)
    ch=struct.unpack_from('<I',data,sec+40*k+36)[0]
    blob=data[ro:ro+rs]
    nm=name.rstrip(b'\0').decode()
    if ch&0x20000000:
        for i in range(len(blob)-5):
            if blob[i] in (0xE8,0xE9):
                d=struct.unpack_from('<i',blob,i+1)[0]
                t=base+va+i+5+d
                if t in targets: print(nm,hex(base+va+i),'E8' if blob[i]==0xE8 else 'E9','->',hex(t))
    for i in range(0,len(blob)-7,8):
        q=struct.unpack_from('<Q',blob,i)[0]
        if q in targets: print(nm,hex(base+va+i),'qword ->',hex(q))
