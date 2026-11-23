# I-0113 gap 4: the grading's output predicted from the dumps (the tone-mapped pixel through the 16-cube volume, trilinear and
# nearest) against the display intermediate, over the dark pixels (run folder, frame).
import sys,struct,math
d,f=sys.argv[1],sys.argv[2]
tm=open(f'{d}/world-{f}-tonemap.rgba','rb').read();dp=open(f'{d}/world-{f}-display.rgba','rb').read()
v=struct.unpack('<%df'%(16**3*4),open(f'{d}/world-{f}-lut.rgba32f','rb').read())
def cell(x,y,z,c):return v[((z*16+y)*16+x)*4+c]
def lin(p,c):
    x,y,z=[min(max(t*15,0),15) for t in p];x0,y0,z0=int(x),int(y),int(z);x1,y1,z1=min(x0+1,15),min(y0+1,15),min(z0+1,15);fx,fy,fz=x-x0,y-y0,z-z0
    a=lambda X,Y,Z:cell(X,Y,Z,c)
    c00=a(x0,y0,z0)*(1-fx)+a(x1,y0,z0)*fx;c10=a(x0,y1,z0)*(1-fx)+a(x1,y1,z0)*fx
    c01=a(x0,y0,z1)*(1-fx)+a(x1,y0,z1)*fx;c11=a(x0,y1,z1)*(1-fx)+a(x1,y1,z1)*fx
    return (c00*(1-fy)+c10*fy)*(1-fz)+(c01*(1-fy)+c11*fy)*fz
def near(p,c):return cell(*[min(max(int(math.floor(t*15+0.5)),0),15) for t in p],c)
el=en=n=0
for i in range(0,len(tm)//4,37):
    q=i*4;p=[tm[q]/255,tm[q+1]/255,tm[q+2]/255]
    if max(tm[q:q+3])>60:continue
    for c in range(3):
        o=dp[q+c];el+=abs(round(lin(p,c)*255)-o);en+=abs(round(near(p,c)*255)-o)
    n+=1
print(d,f,'pixels',n,'mean |linear-display|',round(el/n/3,2),'mean |nearest-display|',round(en/n/3,2))
