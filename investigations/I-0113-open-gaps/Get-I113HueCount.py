# I-0113 tranche 26 (gap 16): per frame of a film's crop, the pixels of saturated magenta (hue 280-330 deg) and cyan-blue
# (hue 175-215 deg) with saturation >= 0.5 and value >= 0.35 (HSV), read from ffmpeg's raw RGB. Usage: film start dur w:h:x:y fps (ffmpeg's crop order)
import subprocess,sys,colorsys
film,start,dur,crop,fps=sys.argv[1:6];w,h=int(crop.split(':')[0]),int(crop.split(':')[1])
cmd=['ffmpeg','-hide_banner','-loglevel','error','-ss',start,'-t',dur,'-i',film,'-vf',f'fps={fps},crop={crop}','-f','rawvideo','-pix_fmt','rgb24','-']
p=subprocess.run(cmd,capture_output=True,check=True).stdout;n=w*h*3;k=0
print(f'# {film} from {start} s for {dur} s, crop {crop} (w:h:x:y), {fps} frames/s')
for i in range(0,len(p)-n+1,n):
    f=p[i:i+n];mag=cyan=0
    for j in range(0,n,3):
        r,g,b=f[j]/255,f[j+1]/255,f[j+2]/255
        mx=max(r,g,b);mn=min(r,g,b)
        if mx<0.35 or (mx-mn)/mx<0.5: continue
        hh=colorsys.rgb_to_hsv(r,g,b)[0]*360
        if 280<=hh<=330: mag+=1
        elif 175<=hh<=215: cyan+=1
    print(f'{float(start)+k/float(fps):.3f} magenta {mag} cyan {cyan}');k+=1
