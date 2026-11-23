# I-0113 gap 4: the run pictures' (JPEG) dark-pixel histogram peaks on the game window, beside each run's pipeline-library state.
# Usage: python -I gap4_survey.py <ffmpeg.exe> <out.jsonl>
import sys,os,glob,re,subprocess,collections,time,json
FF=sys.argv[1];root='C:/Users/User/Desktop/DS3_evidences/I-0066/artifacts';out=open(sys.argv[2],'w')
def metric(b,W,H,x0=330,y0=130,x1=1280,y1=660):
    per=[collections.Counter() for _ in range(3)];dark=0
    for y in range(y0,min(y1,H)):
        row=y*W*3
        for x in range(x0,min(x1,W)):
            i=row+x*3;r,g,bb=b[i],b[i+1],b[i+2]
            if r<50 and g<50 and bb<50:
                dark+=1;per[0][r]+=1;per[1][g]+=1;per[2][bb]+=1
    sp=[]
    for c in range(3):
        h=per[c];s=0
        for v in range(256):
            avg=sum(h.get(v+k,0) for k in range(-6,7) if k)/12
            s+=max(0,h.get(v,0)-2*avg)
        sp.append(round(s/(dark or 1),3))
    return dark,sp
runs=[]
for e in glob.glob(root+'/*/end.png'):
    if os.path.getmtime(e)<time.mktime((2026,10,24,0,0,0,0,0,-1)):continue
    runs.append(e)
runs.sort(key=os.path.getmtime)
for e in runs:
    d=os.path.dirname(e);run=os.path.basename(d);lib=None;flags=''
    sj=os.path.join(d,'startup.jsonl')
    if os.path.exists(sj):
        with open(sj,encoding='utf-8',errors='ignore') as f:
            for line in f:
                if '"native_pipeline_library"' in line:
                    m=re.search(r'"file_loaded":(\w+)',line);lib=m.group(1) if m else None
                    m=re.search(r'"discarded":(\d+)',line);flags=m.group(1) if m else ''
                    break
    try:
        raw=subprocess.run([FF,'-v','error','-i',e,'-f','rawvideo','-pix_fmt','rgb24','-'],capture_output=True,creationflags=0x08000000).stdout
        if len(raw)!=1920*1080*3:res=('size',len(raw))
        else:res=metric(raw,1920,1080)
    except Exception as ex:res=('err',str(ex))
    out.write(json.dumps([time.strftime('%m-%d %H:%M',time.localtime(os.path.getmtime(e))),run,lib,flags,res])+'\n');out.flush()
out.write('DONE\n')
