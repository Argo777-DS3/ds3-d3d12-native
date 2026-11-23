# I-0141 tranche 24 (RESULT « Suggested next step » 4): the passes each model is drawn in, per 60-second window, from the
# reference_model_pass_window rows of an original run and the native_model_pass_window rows of a native run (pixel-keys.flag).
# Usage: python Compare-I141PassWindows.py <original startup.jsonl> [<native startup.jsonl>] [--models a,b,...]
# Windows are the rows' QPC minutes (PixelKeyWindow); « the arrival's » is the first holding an m32 model; « later » the whole
# windows after it (the run's last, partial window left out), as tranche 18's models.py counts. For each named model (default:
# tranche 22's 8), its passes in each window of each run; then, for the later windows, the models each run draws and the native-only
# ones by pass. A run's pass ids are its own (the original's view +0x124, the native's engine pass); they are printed, not mapped.
import json,sys,collections
EIGHT=['m32_00_00_00_030000','m32_00_00_00_030010','m32_00_00_00_030020','m32_00_00_00_030030','o328120','HR_A_0101','FG_A_2012','FG_A_7002']
def load(path,event):
    w=collections.defaultdict(lambda:collections.defaultdict(set));full=[]
    for line in open(path,encoding='utf-8',errors='replace'):
        if '"event":"'+event not in line:continue
        try:r=json.loads(line)
        except Exception:continue
        if r.get('event')==event+'_full':full.append(r.get('window'));continue   # a window whose 20,000-row budget ran out (its id is QPC minutes)
        if r.get('event')!=event:continue
        w[int(r['qpc_ms']//60000)][r['model']].add(r['pass'])
    return w,full
def later(w):
    ks=sorted(w);arrive=next((i for i,k in enumerate(ks) if any(m.startswith('m32') for m in w[k])),None)
    return (ks[arrive] if arrive is not None else None),(ks[arrive+1:-1] if arrive is not None else [])
def main(a):
    models=EIGHT
    if '--models' in a:i=a.index('--models');models=a[i+1].split(',');a=a[:i]+a[i+2:]
    runs=[('original',*load(a[0],'reference_model_pass_window'))]
    if len(a)>1:runs.append(('native',*load(a[1],'native_model_pass_window')))
    for name,w,full in runs:
        arrive,rest=later(w);print(f'{name}: {len(w)} windows {sorted(w)}, the arrival {arrive}, later {rest}, budget ran out in windows {full}')
        for k in sorted(w):
            print(f'  window {k}: {len(w[k])} models'+(' (arrival)' if k==arrive else '')+(' (budget reached: rows may be lost)' if k in full else ''))
        for m in models:
            print(f'  {m}: '+'; '.join(f'{k}:{sorted(w[k][m])}' for k in sorted(w) if m in w[k]) if any(m in w[k] for k in w) else f'  {m}: no row')
    if len(runs)==2:
        (_,wo,fo),(_,wn,fn)=runs;_,lo=later(wo);_,ln=later(wn)
        # A row proves its model drawn, so the windows whose budget was reached still count; but a model absent from the other run
        # is « only » this one's as proven only when the other run reached no budget in its later windows (windows are each run's own).
        po=not any(k in fo for k in lo);pn=not any(k in fn for k in ln)
        if not lo or not ln:print('no later window in',('the original' if not lo else 'the native'),'- no comparison');return
        so=set().union(*[set(wo[k]) for k in lo]);sn=set().union(*[set(wn[k]) for k in ln])
        if not (po and pn):print('budget reached in the later windows: original',sorted(k for k in lo if k in fo),'native',sorted(k for k in ln if k in fn))
        print(f'later windows: the original draws {len(so)} models, the native {len(sn)}; native-only {len(sn-so)}'+('' if po else ' (unproven: the original may have lost rows)')+
              f', original-only {len(so-sn)}'+('' if pn else ' (unproven: the native may have lost rows)'))
        byp=collections.Counter()
        for m in sn-so:
            for k in ln:
                for p in wn[k].get(m,()):byp[p]+=1
        print('native-only (model, window) rows by native pass:',dict(sorted(byp.items())))
        print('of the named models, native-only in the later windows'+('' if po else ' (unproven)')+':',[m for m in models if m in sn-so])
if __name__=='__main__':main(sys.argv[1:])
