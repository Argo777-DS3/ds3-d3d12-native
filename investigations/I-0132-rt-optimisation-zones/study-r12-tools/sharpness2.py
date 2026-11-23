# The high-frequency energy of a region of a screenshot: ffmpeg crops it to 8-bit luma (gray), then the mean absolute 4-neighbour
# Laplacian and the mean luma. Usage: python -I sharpness2.py <ffmpeg> x y w h a.png [b.png ...]
import sys,subprocess
ff=sys.argv[1];x,y,w,h=map(int,sys.argv[2:6])
for p in sys.argv[6:]:
    px=subprocess.run([ff,'-v','error','-i',p,'-vf',f'crop={w}:{h}:{x}:{y},format=gray','-f','rawvideo','-'],capture_output=True,check=True).stdout
    assert len(px)==w*h,(len(px),w*h)
    s=0;m=0;n=0
    for j in range(1,h-1):
        r=j*w
        for i in range(1,w-1):
            c=px[r+i];s+=abs(4*c-px[r+i-1]-px[r+i+1]-px[r+i-w]-px[r+i+w]);m+=c;n+=1
    print(p.replace(chr(92),'/').split('/')[-2],'laplacian %.3f'%(s/n),'luma %.2f'%(m/n))
