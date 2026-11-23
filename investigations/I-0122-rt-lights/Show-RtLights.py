# I-0122 R4b: the local lights' check pictures of a run (Compare-RtLights.py's inputs) as PNGs to look at: per picture, three panels
# side by side, the original's batches (the shadowed lights from the local atlas), the native pass (their shadows traced), and their
# difference (red where the native is darker, green where it is lighter, its strength the difference's). Each panel the diffuse plus
# the specular, tone-mapped c / (1 + c) then gamma 1 / 2.2 (a picture to look at, not a measure). Pure Python (no numpy).
# Usage: python -I Show-RtLights.py <width> <height> <out folder> <run folder> [<run folder> ...]
import glob,os,struct,sys,zlib
def channel(bits,mantissa):
    m=bits&((1<<mantissa)-1);e=bits>>mantissa
    if e==0:return m/(1<<mantissa)*2.0**-14
    if e==31:return 0.0
    return (1+m/(1<<mantissa))*2.0**(e-15)
def decode(path):
    data=open(path,'rb').read();words=struct.unpack('<%dI'%(len(data)//4),data)
    return [(channel(w&0x7FF,6),channel((w>>11)&0x7FF,6),channel(w>>22,5)) for w in words]
def tone(c):return int(round(255*(max(c,0.0)/(1+max(c,0.0)))**(1/2.2)))
def png(path,width,height,rows):
    raw=b''.join(b'\x00'+bytes(r) for r in rows)
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xFFFFFFFF)
    open(path,'wb').write(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b''))
width,height,out=int(sys.argv[1]),int(sys.argv[2]),sys.argv[3]
os.makedirs(out,exist_ok=True)
for folder in sys.argv[4:]:
    for batch in sorted(glob.glob(os.path.join(folder,'rt-lights-*-batch-diffuse.r11g11b10'))):
        base=batch[:-len('-batch-diffuse.r11g11b10')]
        sides=[]
        for side in ('batch','native'):
            d=decode(base+'-'+side+'-diffuse.r11g11b10');s=decode(base+'-'+side+'-specular.r11g11b10')
            if len(d)!=width*height or len(s)!=width*height:raise SystemExit('%s: not %dx%d'%(base,width,height))
            sides.append([(a[0]+b[0],a[1]+b[1],a[2]+b[2]) for a,b in zip(d,s)])
        rows=[]
        for y in range(height):
            row=[]
            for side in sides:
                for x in range(width):row.extend(tone(c) for c in side[y*width+x])
            for x in range(width):
                p,q=sides[0][y*width+x],sides[1][y*width+x];a=sum(p)/3;b=sum(q)/3;v=min(255,tone(abs(b-a))*2)
                row.extend((v,0,0) if b<a else (0,v,0))
            rows.append(row)
        name=os.path.basename(os.path.normpath(folder))+'-'+os.path.basename(base)+'.png'
        png(os.path.join(out,name),3*width,height,rows);print(name)
