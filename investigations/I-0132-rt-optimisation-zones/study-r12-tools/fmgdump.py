import sys,struct,zlib,re
def dcx(p):
    d=open(p,'rb').read(); i=d.index(b'DCA\0')+8; return zlib.decompress(d[i:])
b=dcx(sys.argv[1]); pat=re.compile(sys.argv[2],re.I); lo,hi=int(sys.argv[3]),int(sys.argv[4])
o=0;seen=set()
while True:
    o=b.find(b'\x00\x00\x02\x00',o)
    if o<0: break
    try:
        size,unk,gc,sc=struct.unpack_from('<IIii',b,o+4)
        soff=struct.unpack_from('<q',b,o+0x18)[0]
        if 0<gc<100000 and 0<sc<200000 and 0x28+16*gc<size<=len(b)-o and 0<soff<size:
            for g in range(gc):
                idx,first,last,_=struct.unpack_from('<iiii',b,o+0x28+16*g)
                for k,i in enumerate(range(first,last+1)):
                    so=struct.unpack_from('<q',b,o+soff+8*(idx+k))[0]
                    if so<=0: continue
                    e=o+so
                    while b[e:e+2]!=b'\0\0': e+=2
                    s=b[o+so:e].decode('utf-16-le')
                    if (lo<=i<=hi or pat.search(s)) and (i,s) not in seen:
                        seen.add((i,s)); print(i,repr(s).encode('ascii','backslashreplace').decode()[:110])
    except Exception: pass
    o+=4
