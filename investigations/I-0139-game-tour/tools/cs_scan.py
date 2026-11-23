import re,sys,collections
src='evidence/I-0069/queries/remo/emevd-bank2002-all.txt'
ids=set()
for l in open(src,encoding='utf-8',newline=''):
    m=re.match(r'(\S+) ev=(\d+) (\d+)\[(\d+)\] args=(.*)',l.strip())
    if not m: continue
    f,ev,b,i,a=m.groups(); args=a.split(',')
    for x in args:
        if re.fullmatch(r'(30|31|32|33|34|35|36|37|38|39|40|41|45|50|51)\d{6}',x) and x[2:5] in('000','010') or re.fullmatch(r'(3[0-9]|4[015]|5[01])0[01]00\d\d',x):
            if not (b=='2002'): print(f,ev,b,i,a)
