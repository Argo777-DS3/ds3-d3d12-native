import sys,struct,array
data=open(sys.argv[1],'rb').read()
pe=struct.unpack_from('<I',data,0x3c)[0]
nsec=struct.unpack_from('<H',data,pe+6)[0]; opt=struct.unpack_from('<H',data,pe+20)[0]
secs=[]
for i in range(nsec):
    o=pe+24+opt+40*i
    name=data[o:o+8].rstrip(b'\0').decode(errors='replace'); vs,va,rs,ra,_,_,_,_,ch=struct.unpack_from('<IIIIIIHHI',data,o+8)
    secs.append((name,va,vs,ra,rs,ch))
    print(name,hex(va),hex(vs),hex(ch),file=sys.stderr)
targets=[int(t,16) for t in sys.argv[2:]]
for name,va,vs,ra,rs,ch in secs:
    if not (ch & 0x20000000): continue   # executable
    seg=data[ra:ra+rs]; base=0x140000000+va
    for k in range(4):
        n=(len(seg)-k)//4
        a=array.array('i'); a.frombytes(seg[k:k+4*n])
        want={t-base-4-k:t for t in targets}
        # a[i]+4i == w  -> a[i] == w-4i
        for i,v in enumerate(a):
            w=v+4*i
            if w in want:
                print(name,hex(base+k+4*i),'->',hex(want[w]))
