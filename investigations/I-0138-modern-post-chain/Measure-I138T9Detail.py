# I-0138 tranche 9: the static detail and the crawl of the Firelink west entrance's fixed view (Run-I138T9Detail.ps1), by run.
# Detail: per hold still (hold-1..5.png), the mean Sobel gradient (ffmpeg's sobel then signalstats YAVG on the grey picture) of
# I-0113 tranche 39's crops: a stone pillar (90x200 at 1100,150), Eta's hair (160x120 at 920,450), the foliage (160x160 at
# 1240,200), the arch's mist (160x200 at 925,150); printed as the five stills' minimum-maximum and mean.
# Crawl: from hold.mp4 (I-0113 tranche 39's frame-diff.py, the same rule), the mean absolute luma difference between consecutive
# distinct frames from 3 s to 22 s (a frame whose whole-picture difference is 0, a capture duplicate, left out), by crop; in a
# fixed view of static stone and foliage that change is the jitter's residue (and the lighting's noise), not motion.
# Usage: py -I Measure-I138T9Detail.py <run dir>...
import subprocess,sys,os,re,statistics,tempfile
CROPS={'stone':(1100,150,90,200),'hair':(920,450,160,120),'foliage':(1240,200,160,160),'mist':(925,150,160,200)}
def sobel(png,r):
    out=subprocess.run(['ffmpeg','-hide_banner','-nostats','-i',png,'-vf',f'crop={r[2]}:{r[3]}:{r[0]}:{r[1]},format=gray,sobel,signalstats,metadata=print:key=lavfi.signalstats.YAVG','-f','null','-'],capture_output=True,text=True).stderr
    m=re.search(r'YAVG=([0-9.]+)',out);return float(m.group(1)) if m else None
def crawl(mp4):
    tmp=tempfile.mkdtemp();vals={}
    for k,r in [('whole',None)]+list(CROPS.items()):
        crop='' if r is None else f'crop={r[2]}:{r[3]}:{r[0]}:{r[1]},'
        log=f'{k}.txt'
        subprocess.run(['ffmpeg','-v','error','-ss','3','-t','19','-i',os.path.abspath(mp4),'-vf',f'{crop}format=gray,tblend=all_mode=difference,signalstats,metadata=print:key=lavfi.signalstats.YAVG:file={log}','-f','null','-'],check=True,cwd=tmp)
        vals[k]=[float(x) for x in re.findall(r'YAVG=([0-9.]+)',open(os.path.join(tmp,log)).read())]
    keep=[i for i,v in enumerate(vals['whole']) if v>0]
    return len(vals['whole']),len(keep),{k:statistics.mean([vals[k][i] for i in keep if i<len(vals[k])]) for k in CROPS} if keep else {}
for d in sys.argv[1:]:
    name=os.path.basename(d.rstrip('/\\'));row=[name]
    stills=[os.path.join(d,f'hold-{i}.png') for i in range(1,6) if os.path.isfile(os.path.join(d,f'hold-{i}.png'))]
    for k,r in CROPS.items():
        v=[x for x in (sobel(p,r) for p in stills) if x is not None]
        row.append(f'{k} {min(v):.1f}-{max(v):.1f} ({statistics.mean(v):.2f})' if v else f'{k} -')
    mp4=os.path.join(d,'hold.mp4')
    if os.path.isfile(mp4):
        n,kept,c=crawl(mp4);row.append(f'film {n} frames {kept} distinct; change '+' '.join(f'{k} {v:.2f}' for k,v in c.items()))
    print(f'stills {len(stills)}  '+'  '.join(row))
