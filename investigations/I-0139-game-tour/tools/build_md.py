import sys,os,json
sys.stdout.reconfigure(encoding='utf-8')
here=os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0,here)
import make_sheet as m
C,E=m.C,m.E
def tab(rows,letter):
    out=["| Row | Name | Map | What to look at | How to reach it with the test launcher | Source and status |","|---|---|---|---|---|---|"]
    for i,r in enumerate(rows,1):
        out.append(f"| {letter}{i:02d} | "+" | ".join(x.replace('|','/') for x in r)+" |")
    return "\n".join(out)
head=open(os.path.join(here,'header.md'),encoding='utf-8').read()
gaps=open(os.path.join(here,'gaps.md'),encoding='utf-8').read()
text=head+"\n## Cutscenes and movies (C rows)\n\n"+tab(C,'C')+"\n\n## Scripted world events (E rows)\n\n"+tab(E,'E')+"\n\n"+gaps
p=os.path.join(here,'..','sheets','cutscenes-events.md')
open(p,'w',encoding='utf-8',newline='\n').write(text)
print(len(C),len(E),len(text))
