# I-0113 gap 6: the 3x3 regions' mean luminance of a native run picture against the original's (game window 1280x720 at (8, 31)).
# Usage: python -I gap6_regions.py <ffmpeg.exe> <native.png> <original.png>
import sys,subprocess
FF,a,b=sys.argv[1:4]
def load(p):
    return subprocess.run([FF,'-v','error','-i',p,'-vf','crop=1280:720:8:31','-f','rawvideo','-pix_fmt','gray','-'],capture_output=True,creationflags=0x08000000).stdout
x,y=load(a),load(b);W,H=1280,720
if len(x)!=W*H or len(y)!=W*H:sys.exit('ffmpeg gave no 1280x720 picture for '+a+' or '+b)
out=[]
for r in range(3):
    row=[]
    for c in range(3):
        sa=sb=0
        for yy in range(r*240,(r+1)*240,3):
            for xx in range(c*426,(c+1)*426,3):
                i=yy*W+xx;sa+=x[i];sb+=y[i]
        row.append(round(sa/max(sb,1),2))
    out.append(row)
print(a.split('/')[-1],out)
