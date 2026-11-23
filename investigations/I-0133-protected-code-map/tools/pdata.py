"""Functions from .pdata (RUNTIME_FUNCTION), crossed with the high-entropy regions and the decrypted overlay; entry thunks
into the protection's code section. Read-only."""
import struct, sys, collections, bisect
exe, ovl, regs_tsv, out = sys.argv[1:5]
d = open(exe, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]; nsec = struct.unpack_from('<H', d, pe+6)[0]
soh = struct.unpack_from('<H', d, pe+20)[0]; opt = pe+24
secs = []
for i in range(nsec):
    o = opt+soh+40*i; vs, va, rs, raw = struct.unpack_from('<IIII', d, o+8); secs.append((va, vs, raw, rs))
def off(r):
    for va, vs, raw, rs in secs:
        if va <= r < va+rs: return raw+r-va
exc_rva, exc_size = struct.unpack_from('<II', d, opt+112+3*8)
funcs = []
for k in range(exc_size//12):
    b, e, u = struct.unpack_from('<III', d, off(exc_rva)+12*k)
    funcs.append((b, e))
funcs.sort()
T1 = (0x1000, 0x1000+41000448); T2 = (0x4dbd000, 0x4dbd000+9452544); BIND = (0x56c1000, 0x56c1000+206616); INTERP = (0x271b000, 0x271b000+49664)
def where(r):
    for nm, (a, b) in (('.text1', T1), ('.text2', T2), ('.bind', BIND), ('.interpr', INTERP)):
        if a <= r < b: return nm
    return 'other'
loc = collections.Counter(where(b) for b, e in funcs)
# coverage arrays over .text1
n1 = T1[1]-T1[0]
hi = bytearray(n1); dec = bytearray(n1)
for l in open(regs_tsv).read().splitlines()[1:]:
    r, n, _ = l.split('\t'); r = int(r, 16); n = int(n); hi[r-T1[0]:r-T1[0]+n] = b'\1'*n
ov = open(ovl, 'rb').read(); pos = 0
while pos+8 <= len(ov):
    r, n = struct.unpack_from('<II', ov, pos); pos += 8+n; dec[r-T1[0]:r-T1[0]+n] = b'\1'*n
cls = collections.Counter(); clsB = collections.Counter(); thunk = collections.Counter()
rows = []
for b, e in funcs:
    if where(b) != '.text1': continue
    a0, a1 = b-T1[0], e-T1[0]
    size = a1-a0
    hd = sum(dec[a0:a1]); hh = sum(hi[a0:a1])
    o = off(b); first = d[o]
    tgt = None
    if first == 0xE9:
        tgt = b+5+struct.unpack_from('<i', d, o+1)[0]
        thunk[where(tgt)] += 1
    if hd == 0: c = 'plain'
    elif hd >= 0.5*size: c = 'encrypted_body'
    else: c = 'partly_changed'
    cls[c] += 1; clsB[c] += size
    if c != 'plain': rows.append((b, e, size, hd, hh, c, where(tgt) if tgt else '-'))
L = [f".pdata functions: {len(funcs)} by section {dict(loc)}"]
for c in cls: L.append(f".text1 functions {c}: {cls[c]} ({clsB[c]/2**20:.2f} MB of bodies)")
L.append(f".text1 functions whose first byte is E9 (jmp rel32), by target section: {dict(thunk)}")
# decrypted bytes outside any .pdata function
inside = bytearray(n1)
for b, e in funcs:
    if where(b) == '.text1': inside[b-T1[0]:e-T1[0]] = b'\1'*(e-b)
outside = sum(1 for i in range(n1) if dec[i] and not inside[i]) if False else None
with open(out, 'w', newline='\n') as f:
    f.write('begin\tend\tsize\tdecrypted_bytes\thigh_entropy_bytes\tclass\tentry_jmp_target\n')
    for r in rows: f.write(f"{0x140000000+r[0]:x}\t{0x140000000+r[1]:x}\t{r[2]}\t{r[3]}\t{r[4]}\t{r[5]}\t{r[6]}\n")
print('\n'.join(L))
