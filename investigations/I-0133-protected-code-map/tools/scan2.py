"""Second pass: name the two .text sections apart, map overlay runs to them, build a static map of high-entropy
regions in the first .text (256-byte windows), and compare it with the decrypted overlay. Read-only."""
import math, struct, sys, os, collections
exe, ovl, out = sys.argv[1:4]
d = open(exe, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]; nsec = struct.unpack_from('<H', d, pe+6)[0]
soh = struct.unpack_from('<H', d, pe+20)[0]; opt = pe+24
secs = []; seen = collections.Counter()
for i in range(nsec):
    o = opt+soh+40*i; nm = d[o:o+8].rstrip(b'\0').decode('latin1'); seen[nm] += 1
    if seen[nm] > 1: nm = nm+str(seen[nm])
    vs, va, rs, raw = struct.unpack_from('<IIII', d, o+8); ch = struct.unpack_from('<I', d, o+36)[0]
    secs.append(dict(name=nm, va=va, vsize=vs, raw=raw, rsize=rs, ch=ch))
def sec_of(r):
    for s in secs:
        if s['va'] <= r < s['va']+max(s['vsize'], s['rsize']): return s
def H(b):
    c = collections.Counter(b); n = len(b)
    return -sum(v/n*math.log2(v/n) for v in c.values()) if n else 0
t1 = secs[0]; assert t1['name'] == '.text'
W = 256
tb = d[t1['raw']:t1['raw']+t1['rsize']]
hi = bytearray(len(tb)//W)
for k in range(len(hi)):
    hi[k] = 1 if H(tb[k*W:(k+1)*W]) >= 7.0 else 0   # 256-byte windows: x64 code sits ~5.0-6.2, ciphertext ~7.1-7.3 max for 256
# merge into regions
regs = []; k = 0
while k < len(hi):
    if hi[k]:
        j = k
        while j < len(hi) and hi[j]: j += 1
        regs.append((t1['va']+k*W, (j-k)*W)); k = j
    else: k += 1
# overlay
ov = open(ovl, 'rb').read(); pos = 0; runs = []
while pos+8 <= len(ov):
    r, n = struct.unpack_from('<II', ov, pos); pos += 8+n; runs.append((r, n))
by = collections.Counter(); byb = collections.Counter()
covered = bytearray(len(tb))
for r, n in runs:
    s = sec_of(r); by[s['name']] += 1; byb[s['name']] += n
    if s is t1:
        a = r-t1['va']; covered[a:a+n] = b'\1'*n
hiB = sum(n for _, n in regs)
inter = 0
for r, n in regs:
    a = r-t1['va']; inter += sum(covered[a:a+n])
cov_total = sum(covered)
L = []
for k in by: L.append(f"overlay runs in {k}: {by[k]} runs, {byb[k]} bytes ({byb[k]/2**20:.2f} MB)")
L.append(f".text1 high-entropy 256-byte windows (H>=7.0): {sum(hi)} windows, {len(regs)} regions, {hiB} bytes ({hiB/2**20:.2f} MB) of {t1['rsize']/2**20:.2f} MB")
L.append(f"overlay bytes inside .text1: {cov_total}; of which inside high-entropy regions: {inter} ({inter/max(1,cov_total)*100:.1f}%); high-entropy bytes covered by overlay: {inter/max(1,hiB)*100:.1f}%")
sz = collections.Counter()
for r, n in regs:
    sz['256' if n <= 256 else '<=1K' if n <= 1024 else '<=4K' if n <= 4096 else '<=16K' if n <= 16384 else '>16K'] += 1
L.append('region sizes ' + ' '.join(f"{k}:{v}" for k, v in sz.items()))
big = sorted(regs, key=lambda x: -x[1])[:25]
L.append('largest regions: ' + ', '.join(f"{0x140000000+r:x}+{n:#x}" for r, n in big))
# distribution of high-entropy bytes by 1 MB of .text1
mb = collections.Counter()
for r, n in regs: mb[(r-t1['va'])//2**20] += n
L.append('per MB of .text1 (offset MB: KB high-entropy): ' + ' '.join(f"{k}:{v//1024}" for k, v in sorted(mb.items())))
with open(os.path.join(out, 'text1-regions.tsv'), 'w', newline='\n') as f:
    f.write('rva\tlen\toverlay_covered\n')
    for r, n in regs:
        a = r-t1['va']; f.write(f"{r:x}\t{n}\t{sum(covered[a:a+n])}\n")
open(os.path.join(out, 'summary2.txt'), 'w', newline='\n').write('\n'.join(L)+'\n'); print('\n'.join(L))
