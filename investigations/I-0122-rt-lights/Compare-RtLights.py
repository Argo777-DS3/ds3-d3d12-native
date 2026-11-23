# I-0122 R4a: the local lights' check pictures of a run (rt-lights-<serial>-{batch,native}-{diffuse,specular}.r11g11b10, rows of
# 4 bytes at the render extent; Run-R4ZoneTour.ps1 takes them) compared target by target: the original's unshadowed batches against the native pass on the same
# frame. A pixel counts when either side is above zero; the error is |native - batch| in steps of 1/255 of max(1, the larger value)
# (the accumulation's range above 1 compared relatively); the sign of the largest channel difference counted (native above or
# below the batches, and those over 2 steps: the batches' R11G11B10 rounding after each add predicts native above). Prints one JSON row a picture pair and the totals. Pure Python (no numpy).
# Usage: python -I Compare-RtLights.py <run folder> [<run folder> ...]
import glob,json,os,struct,sys
def channel(bits,mantissa):
    m=bits&((1<<mantissa)-1);e=bits>>mantissa
    if e==0:return m/(1<<mantissa)*2.0**-14
    if e==31:return float('inf')
    return (1+m/(1<<mantissa))*2.0**(e-15)
def decode(path):
    data=open(path,'rb').read();words=struct.unpack('<%dI'%(len(data)//4),data)
    return [(channel(w&0x7FF,6),channel((w>>11)&0x7FF,6),channel(w>>22,5)) for w in words]
def compare(a,b):
    lit=0;within={1:0,2:0,4:0,8:0};worst=0.0;signed=0.0;rel=0.0;rel_n=0;nonfinite=0;above=below=above2=below2=0
    for p,q in zip(a,b):
        if max(p)<=0 and max(q)<=0:continue
        lit+=1;e=0.0;big=0.0
        for x,y in zip(p,q):
            if x!=x or y!=y or x==float('inf') or y==float('inf'):nonfinite+=1;e=float('inf');continue
            d=abs(y-x)/max(1.0,x,y)*255;signed+=(y-x)
            if d>e:e=d;big=y-x
            if max(x,y)>4/255:rel+=abs(y-x)/max(x,y);rel_n+=1
        worst=max(worst,e)
        if big>0:above+=1;above2+=e>2
        elif big<0:below+=1;below2+=e>2
        for k in within:
            if e<=k:within[k]+=1
    return {'lit':lit,'within':{str(k):(round(v/lit,5) if lit else None) for k,v in within.items()},'worst_steps':round(worst,2),
        'mean_signed':(signed/(3*lit) if lit else 0),'mean_relative':(rel/rel_n if rel_n else 0),'nonfinite':nonfinite,
        'native_above':above,'native_below':below,'above_over2':above2,'below_over2':below2}
totals={}
for folder in sys.argv[1:]:
    for batch in sorted(glob.glob(os.path.join(folder,'rt-lights-*-batch-diffuse.r11g11b10'))):
        base=batch[:-len('-batch-diffuse.r11g11b10')]
        for target in ('diffuse','specular'):
            a=decode(base+'-batch-'+target+'.r11g11b10');b=decode(base+'-native-'+target+'.r11g11b10')
            r=compare(a,b);r.update({'run':os.path.basename(os.path.normpath(folder)),'picture':os.path.basename(base),'target':target})
            print(json.dumps(r))
            t=totals.setdefault(target,{'lit':0,'within1':0,'within2':0,'within4':0,'within8':0,'worst_steps':0.0})
            t['lit']+=r['lit'];t['worst_steps']=max(t['worst_steps'],r['worst_steps'])
            for k in ('1','2','4','8'):t['within'+k]+=round((r['within'][k] or 0)*r['lit'])
for target,t in totals.items():
    print(json.dumps({'total':target,'lit':t['lit'],'worst_steps':t['worst_steps'],**{k:(round(t[k]/t['lit'],5) if t['lit'] else None) for k in ('within1','within2','within4','within8')}}))
