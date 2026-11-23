# I-0139 npcs-merchants: joins the static dumps (msb-enemies.tsv, npc-nameid.tsv, FMG NPC names, msb-bonfires.tsv,
# BonfireWarpParam rows, menu text FMG) into one JSON used to write the sheet. Run: python -I Build-NpcData.py <staticdir>
import sys,json,glob,math,collections
D=sys.argv[1]
def rd(p): return open(p,encoding='utf-8',newline='').read().replace('\r','').split('\n')
fm={}
for f in ['NPC名.fmg.tsv','NPC名_dlc1.fmg.tsv','NPC名_dlc2.fmg.tsv']:
    for l in rd(D+'/item_dlc2/'+f):
        a=l.split('\t')
        if len(a)==2 and a[1].strip(): fm[int(a[0])]=a[1]
nid={}
for l in rd(D+'/npc-nameid.tsv'):
    a=l.split('\t')
    if len(a)>=2 and a[0]: nid[int(a[0])]=int(a[1])
txt={}
for f in glob.glob(D+'/menu_dlc2/FDP_*メニューテキスト*.tsv'):
    for l in rd(f):
        a=l.split('\t')
        if len(a)==2 and a[1].strip(): txt[int(a[0])]=a[1]
r=collections.defaultdict(dict)
for l in rd(D+'/BonfireWarpParam.rows.tsv')[2:]:
    p=l.split('\t')
    if len(p)>=5: r[int(p[0])][int(p[1],16)]=int(p[4])
e2n={v[4]:txt.get(v[8],'?') for v in r.values()}
bf=[]
for l in rd(D+'/msb-bonfires.tsv')[1:]:
    p=l.split('\t')
    if len(p)<7: continue
    m=p[0].replace('map_MapStudio_','').replace('.msb.dcx','')
    n=e2n.get(int(p[3]))
    if n: bf.append((m,n,float(p[4]),float(p[5]),float(p[6])))
rows=[]
for l in rd(D+'/msb-enemies.tsv')[1:]:
    p=l.split('\t')
    if len(p)<11: continue
    f,k,name,model,ent,npc,talk,ci,x,y,z=p[:11]
    npc=int(npc); talk=int(talk or 0); n=nid.get(npc)
    if n is None: continue
    m=f.replace('map_MapStudio_','').replace('.msb.dcx','')
    pos=(float(x),float(y),float(z))
    best=min(((math.dist(pos,b[2:]),b[1]) for b in bf if b[0]==m),default=(None,None))
    rows.append(dict(map=m,part=name,model=model,entity=ent,npcparam=npc,talk=talk,nameid=n,name=fm.get(n,''),pos=pos,bonfire=best[1],bdist=best[0]))
json.dump(rows,open(D+'/npc-parts-joined.json','w',encoding='utf-8'),indent=0,ensure_ascii=False)
json.dump(bf,open(D+'/bonfires-joined.json','w',encoding='utf-8'),ensure_ascii=False)
print(len(rows),len(bf))
