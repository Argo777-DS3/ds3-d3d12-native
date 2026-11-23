"""Attribution on the decrypted image (disk + overlay935): vtable classes, callers and callees (rel32 calls in decrypted
bodies too), strings referenced rip-relative; labels propagated two rounds; then keyword systems. Read-only."""
import struct, sys, collections, bisect, re
exe, ovl, pdtsv, names_tsv, outp = sys.argv[1:6]
d = bytearray(open(exe, 'rb').read())
IB = 0x140000000
pe = struct.unpack_from('<I', d, 0x3c)[0]; nsec = struct.unpack_from('<H', d, pe+6)[0]
soh = struct.unpack_from('<H', d, pe+20)[0]; opt = pe+24
secs = []
for i in range(nsec):
    o = opt+soh+40*i; nm = d[o:o+8].rstrip(b'\0').decode(); vs, va, rs, raw = struct.unpack_from('<IIII', d, o+8)
    secs.append((nm, va, vs, raw, rs))
def off(r):
    for nm, va, vs, raw, rs in secs:
        if va <= r < va+rs: return raw+r-va
ov = open(ovl, 'rb').read(); pos = 0
while pos+8 <= len(ov):
    r, n = struct.unpack_from('<II', ov, pos); pos += 8; o = off(r); d[o:o+n] = ov[pos:pos+n]; pos += n
d = bytes(d)
T1 = (0x1000, 0x1000+41000448)
rd = next(s for s in secs if s[0] == '.rdata'); RD = (rd[1], rd[1]+rd[4])
exc_rva, exc_size = struct.unpack_from('<II', d, opt+112+3*8)
funcs = sorted(struct.unpack_from('<II', d, off(exc_rva)+12*k) for k in range(exc_size//12))
fb = [b for b, e in funcs]; fend = dict(funcs)
def func_of(r):
    i = bisect.bisect_right(fb, r)-1
    if i >= 0 and funcs[i][0] <= r < funcs[i][1]: return funcs[i][0]
enc = {}
for l in open(pdtsv).read().splitlines()[1:]:
    p = l.split('\t'); enc[int(p[0], 16)-IB] = (p[5], int(p[2]))
# RTTI
rdb = d[rd[3]:rd[3]+rd[4]]
def demangle(n):
    core = n[4:] if n[3] in 'VU' else n[3:]
    if core.startswith('?$'):
        m = re.match(r'\?\$([A-Za-z0-9_]+)@(.*)', core)
        inner = re.findall(r'V([A-Za-z0-9_]+)@([A-Za-z0-9_]+)@@', m.group(2)) if m else []
        return m.group(1)+('<'+','.join(f"{b}::{a}" for a, b in inner)+'>' if inner else '') if m else core
    parts = [p for p in core.rstrip('@').split('@') if p]
    return '::'.join(reversed(parts))
vclass = collections.defaultdict(list)
for k in range(0, len(rdb)-8, 8):
    q = struct.unpack_from('<Q', rdb, k)[0]-IB
    if not (RD[0] <= q < RD[1]): continue
    o = off(q); sig, offs, cdo, ptd, pcd, pself = struct.unpack_from('<IIIIII', d, o)
    if sig != 1 or pself != q: continue
    to = off(ptd)
    if to is None: continue
    s = d[to+16:to+316]; nm = s[:s.find(b'\0')].decode('latin1', 'replace')
    if not nm.startswith('.?A'): continue
    cls = demangle(nm); j = k+8; slot = 0
    while j+8 <= len(rdb) and slot < 2000:
        v = struct.unpack_from('<Q', rdb, j)[0]-IB
        if not (T1[0] <= v < T1[1]): break
        vclass[v].append(cls); j += 8; slot += 1
names = {}
for l in open(names_tsv, encoding='utf-8').read().splitlines()[1:]:
    p = l.split('\t'); names[int(p[0], 16)-IB] = p[1].rsplit('_', 1)[0]
# call graph over .text1 (decrypted)
t1o = off(T1[0]); tb = d[t1o:t1o+(T1[1]-T1[0])]
callers = collections.defaultdict(set); callees = collections.defaultdict(set); indeg = collections.Counter()
for m in re.finditer(b'\xe8', tb):
    i = m.start()
    if i+5 > len(tb): break
    tgt = T1[0]+i+5+struct.unpack_from('<i', tb, i+1)[0]
    if tgt in fend:
        f = func_of(T1[0]+i)
        if f is not None and f != tgt: indeg[tgt] += 1
        if f is not None and f != tgt and (f in enc or tgt in enc):
            callers[tgt].add(f); callees[f].add(tgt)
# strings referenced from encrypted bodies
def string_at(r):
    o = off(r)
    if o is None: return None
    s = d[o:o+80]
    m = re.match(rb'[\x20-\x7e]{5,}\x00', s)
    if m: return m.group(0)[:-1].decode()
    m = re.match(rb'(?:[\x20-\x7e]\x00){5,}\x00\x00', s)
    if m: return m.group(0)[:-2].decode('utf-16-le')
strs = collections.defaultdict(list)
for f in enc:
    o = off(f); body = d[o:o+(fend[f]-f)]
    for i in range(len(body)-4):
        disp = struct.unpack_from('<i', body, i)[0]
        tgt = f+i+4+disp
        if RD[0] <= tgt < RD[1] and (tgt == RD[0] or d[off(tgt)-1] == 0):
            s = string_at(tgt)
            if s and s not in strs[f]: strs[f].append(s)
def own(f):
    if f in vclass: return vclass[f][0]
    if f in names: return names[f]
lab = {f: own(f) for f in enc}
src = {f: ('vtable' if f in vclass else 'name') for f in enc if lab[f]}
for rnd in range(3):
    for f in enc:
        if lab[f]: continue
        c = collections.Counter()
        for x in callers.get(f, ()):
            if indeg[x] > 40: continue
            l = own(x) or lab.get(x)
            if l: c[l] += 1
        if c: lab[f] = c.most_common(1)[0][0]; src[f] = f'caller{rnd}'; continue
        c = collections.Counter()
        for x in callees.get(f, ()):
            if indeg[x] > 40: continue
            l = own(x) or lab.get(x)
            if l: c[l] += 1
        if c: lab[f] = c.most_common(1)[0][0]; src[f] = f'callee{rnd}'
# neighbour round (weaker, INFERRED same compilation unit): nearest labelled function within 0x800 bytes
labelled = sorted(set(list(vclass) + list(names) + [f for f in enc if lab[f]]))
for f in enc:
    if lab[f]: continue
    i = bisect.bisect_left(labelled, f); best = None
    for j in (i-1, i):
        if 0 <= j < len(labelled):
            x = labelled[j]; dist = abs(x-f)
            if dist <= 0x800 and (best is None or dist < best[0]): best = (dist, x)
    if best:
        x = best[1]; lab[f] = own(x) or lab.get(x); src[f] = 'neighbour'
SYS = [
 ('debug_tools', r'debug|dbg|tweaker|luaconsole|guideline|cheat|TGUI|NetworkDebugger|DebugCommand|LogDebugger|DebugDisplay'),
 ('crt_utility', r'^std|_Func_impl|num_put|numpunct|^Crt|Memcpy|Sprintf|Format|CriticalSection|SecurityCookie|ShaderWrapperServiceLookup'),
 ('menus_ui', r'menu|scaleform|dlg|dialog|CSWindow|window|gaitem|caption|textbox|font|GuiFramework|EzDraw_Text'),
 ('input', r'pad|input|keyboard|mouse|manipulator'),
 ('network_online', r'Frpg2|notify|login|session|steam|socket|matching|online|bloodmessage|summon|p2p|packet|FdpSv'),
 ('character_gameplay', r'Chr|Player|Enemy|\bAi|Ai[A-Z]|Behavior|hkb|EzState|Damage|Weapon|Item|Equip|Bullet|Attack|Targeting|ComThink|TurnAnim|ObjIns'),
 ('physics_havok', r'^hk|havok|cloth|ragdoll|navi'),
 ('render_gx_sfx', r'^GX|GX[A-Z]|Rend|Draw|Sfx|Ffx|Model|Shader|Texture|Light|Shadow|Visb|Scene|Skeleton'),
 ('map_event_world', r'Map|Msb|Event|Stage|World|Region|Remo|Movie|Regulation'),
 ('file_resource', r'File|Encrypted|Binder|Archive|Dvd|Bnd|Loader|DLIO'),
 ('engine_core', r'Step|Task|Time|Seed|Logger|System|Language|Profile'),
]
def system(f):
    for l in [lab[f] or ''] + strs.get(f, [])[:6]:
        for nm, pat in SYS:
            if l and re.search(pat, l, re.I if nm != 'character_gameplay' else 0): return nm
    return 'unattributed'
agg = collections.Counter(); aggB = collections.Counter(); hows = collections.Counter(src.get(f, 'none') for f in enc)
with open(outp, 'w', newline='\n', encoding='utf-8') as fo:
    fo.write('entry\tsize\tclass\tsystem\tlabel\tlabel_source\tcallers\tstrings\n')
    for f in sorted(enc):
        sy = system(f); agg[sy] += 1; aggB[sy] += enc[f][1]
        fo.write(f"{IB+f:x}\t{enc[f][1]}\t{enc[f][0]}\t{sy}\t{lab[f] or '-'}\t{src.get(f, 'none')}\t{len(callers.get(f, ()))}\t{' | '.join(strs.get(f, [])[:5])}\n")
print('label sources', dict(hows))
for k, v in agg.most_common(): print(f"{k}\t{v}\t{aggB[k]/1024:.0f} KB")
