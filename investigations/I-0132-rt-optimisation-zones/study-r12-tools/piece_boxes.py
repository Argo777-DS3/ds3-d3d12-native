# I-0132 R12a's second tranche: the menu pieces' rectangles, measured on 01_common.tpf's textures decoded offline (decode.py beside
# it, from the study's tpf.py and bcdec.py; the decoded PNGs are licensed bytes, kept in a scratch folder, never in the repository or
# the evidence store). Prints the alpha bounding boxes of 4x4 cells (alpha > 8) and, for MENU_ItemParts' left column, each band's exact
# extent (alpha > 16) and its middle colour. Usage: python -I piece_boxes.py <folder of the decoded PNGs>
import sys,zlib,struct
def load(p):
    d=open(p,'rb').read();o=8;w=h=0;idat=b''
    while o<len(d):
        n=struct.unpack('>I',d[o:o+4])[0];t=d[o+4:o+8];c=d[o+8:o+8+n];o+=12+n
        if t==b'IHDR':w,h=struct.unpack('>II',c[:8])
        if t==b'IDAT':idat+=c
    raw=zlib.decompress(idat);st=w*4+1;px=bytearray()
    for y in range(h):
        assert raw[y*st]==0;px+=raw[y*st+1:(y+1)*st]
    return w,h,px
folder=sys.argv[1]
for name in ['MENU_OptionBase','MENU_ItemParts']:
    w,h,px=load(folder+'/'+name+'.png')
    cw,ch=w//4,h//4
    cell=[[1 if max(px[((cy*4+dy)*w+cx*4+dx)*4+3] for dy in (0,3) for dx in (0,3))>8 else 0 for cx in range(cw)] for cy in range(ch)]
    seen=[[0]*cw for _ in range(ch)];boxes=[]
    for cy in range(ch):
        for cx in range(cw):
            if cell[cy][cx] and not seen[cy][cx]:
                stack=[(cy,cx)];seen[cy][cx]=1;x0=x1=cx;y0=y1=cy;n=0
                while stack:
                    y,x=stack.pop();n+=1;x0=min(x0,x);x1=max(x1,x);y0=min(y0,y);y1=max(y1,y)
                    for yy,xx in ((y+1,x),(y-1,x),(y,x+1),(y,x-1)):
                        if 0<=yy<ch and 0<=xx<cw and cell[yy][xx] and not seen[yy][xx]:seen[yy][xx]=1;stack.append((yy,xx))
                if n>=4:boxes.append((x0*4,y0*4,(x1+1)*4,(y1+1)*4))
    print(name,w,h,'boxes',len(boxes))
    for b in boxes[:12]:print('  box',b)
w,h,px=load(folder+'/MENU_ItemParts.png')
rowmax=lambda y,x0,x1:max(px[(y*w+x)*4+3] for x in range(x0,x1))
colmax=lambda x,y0,y1:max(px[(y*w+x)*4+3] for y in range(y0,y1))
for y0,y1,label in [(0,14,'rule'),(14,70,'glow 1'),(70,122,'glow 2 (the cursor)'),(122,170,'field'),(170,222,'orange field'),(222,256,'brown bar')]:
    ys=[y for y in range(y0,y1) if rowmax(y,0,410)>16]
    if not ys:print(label,'none');continue
    ya,yb=ys[0],ys[-1]+1;xs=[x for x in range(0,412) if colmax(x,ya,yb)>16]
    m=((ya+yb)//2*w+200)*4;print(f'{label}: x {xs[0]}-{xs[-1]+1} y {ya}-{yb} middle {tuple(px[m:m+4])}')
# The cursor glow's own extent (R12a's second tranche's final review M1: the band limits above cut it at 122): its alpha profile down
# column 200 and the mean over x 5-412, rows 70-131; the glow fades in from row 77 and ends hard after row 115 (the field below).
for y in range(70,132):
    a=[px[(y*w+x)*4+3] for x in range(5,412)];print(f'row {y}: alpha at x 200 {px[(y*w+200)*4+3]}, mean {round(sum(a)/len(a))}, rgb {tuple(px[(y*w+200)*4:(y*w+200)*4+3])}')
