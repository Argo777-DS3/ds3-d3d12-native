# I-0113 gap 4: per-stage reader of one native world picture's lossless dumps (run folder, frame): the dark pixels' distinct
# colours at the tone map's output, the display intermediate and the presented picture, and the grading volume's smoothness.
import sys,os,struct,collections
d,frame=sys.argv[1],sys.argv[2]
def rgba8(name):
    p=f'{d}/world-{frame}{name}'
    return open(p,'rb').read() if os.path.exists(p) else None
def stats(b,label):
    if b is None:print(label,'missing');return
    cols=collections.Counter();per=[collections.Counter() for _ in range(3)];n=0
    for i in range(0,len(b),4):
        r,g,bb=b[i],b[i+1],b[i+2]
        if max(r,g,bb)<40:n+=1;cols[(r,g,bb)]+=1;per[0][r]+=1;per[1][g]+=1;per[2][bb]+=1
    top=cols.most_common(1)[0][1]/n if n else 0
    print(f'{label:10s} dark {n:7d} distinct {len(cols):5d} top-colour share {top:.3f} R-values {len(per[0])}')
for name,label in (('-tonemap.rgba','tonemap'),('-display.rgba','display'),('.rgba','presented')):stats(rgba8(name),label)
p=f'{d}/world-{frame}-lut.rgba32f'
if os.path.exists(p):
    v=struct.unpack('<%df'%(16*16*16*4),open(p,'rb').read())
    cell=lambda x,y,z:v[((z*16+y)*16+x)*4:((z*16+y)*16+x)*4+3]
    eq=sum(1 for z in range(16) for y in range(16) for x in range(15) if cell(x,y,z)==cell(x+1,y,z))
    print('lut: equal x-neighbours',eq,'of',16*16*15,' diagonal R:',[round(cell(i,i,i)[0],3) for i in range(16)])
