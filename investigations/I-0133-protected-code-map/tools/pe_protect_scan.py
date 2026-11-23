"""Static scan of DarkSoulsIII.exe for anti-tamper regions (read-only).

Usage: python -I pe_protect_scan.py <exe> <overlay.bin> <out_dir>
Outputs: sections.tsv, overlay_runs.tsv (run -> section), entropy_windows.tsv (high-entropy windows in code sections),
summary.txt.
"""
import math, struct, sys, os, collections

exe, ovl, out = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(out, exist_ok=True)
d = open(exe, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
soh = struct.unpack_from('<H', d, pe + 20)[0]
opt = pe + 24
image_base = struct.unpack_from('<Q', d, opt + 24)[0]
entry = struct.unpack_from('<I', d, opt + 16)[0]
secs = []
for i in range(nsec):
    o = opt + soh + 40 * i
    name = d[o:o + 8].rstrip(b'\0').decode('latin1')
    vsize, va, rsize, raw = struct.unpack_from('<IIII', d, o + 8)
    ch = struct.unpack_from('<I', d, o + 36)[0]
    secs.append(dict(name=name, va=va, vsize=vsize, raw=raw, rsize=rsize, ch=ch))


def sec_of(rva):
    for s in secs:
        if s['va'] <= rva < s['va'] + max(s['vsize'], s['rsize']):
            return s
    return None


def off(rva):
    s = sec_of(rva)
    if not s or rva - s['va'] >= s['rsize']:
        return None
    return s['raw'] + rva - s['va']


def H(b):
    if not b:
        return 0.0
    c = collections.Counter(b)
    n = len(b)
    return -sum(v / n * math.log2(v / n) for v in c.values())


lines = []
with open(os.path.join(out, 'sections.tsv'), 'w', newline='\n') as f:
    f.write('name\trva\tvsize\trawsize\tchar\texec\twrite\tentropy\n')
    for s in secs:
        b = d[s['raw']:s['raw'] + s['rsize']]
        e = H(b)
        s['entropy'] = e
        ex = bool(s['ch'] & 0x20000000)
        wr = bool(s['ch'] & 0x80000000)
        f.write(f"{s['name']}\t{s['va']:x}\t{s['vsize']}\t{s['rsize']}\t{s['ch']:08x}\t{int(ex)}\t{int(wr)}\t{e:.3f}\n")
        lines.append(f"section {s['name']:8s} rva {s['va']:08x} vsize {s['vsize']:>10} ({s['vsize']/2**20:6.2f} MB) raw {s['rsize']:>10} exec {int(ex)} write {int(wr)} entropy {e:.3f}")
lines.append(f"image_base {image_base:x} entry rva {entry:x} in {sec_of(entry)['name'] if sec_of(entry) else '?'}")

# Overlay runs: u32 rva, u32 len, bytes
ov = open(ovl, 'rb').read()
pos = 0
runs = []
while pos + 8 <= len(ov):
    rva, n = struct.unpack_from('<II', ov, pos)
    pos += 8
    b = ov[pos:pos + n]
    pos += n
    o = off(rva)
    disk = d[o:o + n] if o is not None else b''
    runs.append((rva, n, b, disk))
by_sec = collections.defaultdict(lambda: [0, 0])
size_hist = collections.Counter()
with open(os.path.join(out, 'overlay_runs.tsv'), 'w', newline='\n') as f:
    f.write('rva\tlen\tsection\tdisk_entropy\tmem_entropy\n')
    for rva, n, b, disk in runs:
        s = sec_of(rva)
        nm = s['name'] if s else '?'
        by_sec[nm][0] += 1
        by_sec[nm][1] += n
        bucket = '<=16' if n <= 16 else '<=64' if n <= 64 else '<=256' if n <= 256 else '<=1024' if n <= 1024 else '<=4096' if n <= 4096 else '>4096'
        size_hist[bucket] += 1
        f.write(f"{rva:x}\t{n}\t{nm}\t{H(disk):.2f}\t{H(b):.2f}\n")
for k, v in by_sec.items():
    lines.append(f"overlay section {k}: runs {v[0]} bytes {v[1]} ({v[1]/2**20:.2f} MB)")
for k in ['<=16', '<=64', '<=256', '<=1024', '<=4096', '>4096']:
    lines.append(f"overlay run size {k}: {size_hist[k]}")

# Entropy windows over executable sections (4 KB windows, step 4 KB)
W = 4096
with open(os.path.join(out, 'entropy_windows.tsv'), 'w', newline='\n') as f:
    f.write('section\trva\tentropy\n')
    for s in secs:
        if not s['ch'] & 0x20000000:
            continue
        hist = collections.Counter()
        hi = 0
        tot = 0
        for a in range(0, s['rsize'] - W + 1, W):
            e = H(d[s['raw'] + a:s['raw'] + a + W])
            hist[round(e, 1)] += 1
            tot += 1
            if e >= 7.2:
                hi += 1
            f.write(f"{s['name']}\t{s['va']+a:x}\t{e:.3f}\n")
        lines.append(f"entropy {s['name']}: {tot} windows of 4 KB, {hi} with entropy >= 7.2 ({hi*W/2**20:.2f} MB)")
        top = sorted(hist.items())
        lines.append('  histogram ' + ' '.join(f"{k}:{v}" for k, v in top))

# Jumps from .text into the other executable sections: E9 rel32 at any offset whose target lies in another exec section
text = next(s for s in secs if s['name'] == '.text')
others = [s for s in secs if s['ch'] & 0x20000000 and s is not text]
tb = d[text['raw']:text['raw'] + text['rsize']]
cnt = collections.Counter()
import re
for m in re.finditer(bytes([0xE9]), tb):
    i = m.start()
    if i + 5 > len(tb):
        break
    if True:
        rel = struct.unpack_from('<i', tb, i + 1)[0]
        tgt = text['va'] + i + 5 + rel
        for s in others:
            if s['va'] <= tgt < s['va'] + s['vsize']:
                cnt[s['name']] += 1
                break
for k, v in cnt.items():
    lines.append(f"E9 rel32 byte patterns in .text targeting {k}: {v} (raw pattern count, includes false positives)")

open(os.path.join(out, 'summary.txt'), 'w', newline='\n').write('\n'.join(lines) + '\n')
print('\n'.join(lines))
