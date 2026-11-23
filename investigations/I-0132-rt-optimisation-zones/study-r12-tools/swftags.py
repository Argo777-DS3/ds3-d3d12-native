import sys,zlib,struct,collections
def load(p):
    d=open(p,'rb').read()
    sig=d[:3]
    if sig in (b'CFX',b'CWS'): d=d[:8]+zlib.decompress(d[8:])
    return d
def tags(d):
    nb=d[8]>>3; rbits=5+4*nb; o=8+(rbits+7)//8+4
    out=[]
    while o<len(d):
        h=struct.unpack_from('<H',d,o)[0]; o+=2; code=h>>6; ln=h&63
        if ln==63: ln=struct.unpack_from('<I',d,o)[0]; o+=4
        out.append((code,o,ln)); o+=ln
        if code==0: break
    return out
if __name__=='__main__':
  for p in sys.argv[1:]:
    d=load(p); t=tags(d)
    c=collections.Counter(x[0] for x in t)
    print(p, d[:3], "ver", d[3], "len", len(d), dict(sorted(c.items())))
