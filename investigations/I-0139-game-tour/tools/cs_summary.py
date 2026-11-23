import sys,os
sys.path.insert(0,os.path.dirname(__file__))
from ev_extract import ev
FT={'3[0]','1003[1]','1003[2]','1003[101]','1003[3]','1003[4]','1003[5]','1003[14]'}
def st(a):
    p=a.split(',');
    try: v=int(p[0])
    except: return None
    return v
for (f,e),L in ev.items():
    if e==0 or not any(i.startswith('2002[') for i,a in L): continue
    cs=[];tested=[];setf=[];ent=[]
    for i,a in L:
        p=a.split(',')
        if i.startswith('2002['): cs.append(i+' '+a)
        elif i in('3[0]','1003[1]','1003[2]','1003[101]') and len(p)>=2 and p[1].isdigit() and int(p[1])>1000: tested.append(f"{i}:{p[0]}:{p[1]}")
        elif i in('2003[2]','2003[62]'): setf.append(f"{i}:{a}")
        elif i in('2003[22]','2003[63]'): setf.append(f"{i}:{a}")
        elif i.startswith('3[') and i!='3[0]' and len(p)>=2: ent.append(f"{i}:{a}")
    print(f"{f} ev={e}\n   CS {cs}\n   TEST {tested}\n   SET {setf}\n   OTHER3 {ent[:6]}")
