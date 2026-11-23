# I-0113 gap 4: share of a R11G11B10 dump's lit pixels equal to their right neighbour (blocks from a point-sampled upscale).
import sys,struct
def r11(p):
    b=open(p,'rb').read();n=len(b)//4;w=struct.unpack('<%dI'%n,b)
    def f11(x):
        e=(x>>6)&31;m=x&63
        return 0.0 if e==0 and m==0 else (m/64)*2**-14 if e==0 else (1+m/64)*2**(e-15)
    return [f11(x&0x7ff) for x in w]
W=int(sys.argv[1])   # the picture's width (the dump holds rows only)
for p in sys.argv[2:]:
    R=r11(p);H=len(R)//W
    eqh=sum(1 for y in range(0,H,3) for x in range(0,W-1) if R[y*W+x]==R[y*W+x+1] and R[y*W+x]>0)
    tot=sum(1 for y in range(0,H,3) for x in range(0,W-1) if R[y*W+x]>0)
    print(p.split('artifacts/')[-1],'nonzero',tot,'equal right-neighbour share',round(eqh/max(tot,1),3),'mean',round(sum(R)/len(R),4))
