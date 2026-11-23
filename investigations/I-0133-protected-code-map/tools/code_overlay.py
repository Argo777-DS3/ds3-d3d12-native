# I-0066 cycle935: turn a code_capture journal (<journal>.code.bin) into an overlay of
# decrypted executable bytes at their RVAs, de-relocated to the preferred image base so
# it can be laid over the static database. Records are applied in capture order (later
# wins); every distinct version per RVA is also counted. Output: overlay.bin (RVA, length,
# bytes records, merged), overlay.txt summary. Usage: code_overlay.py exe code.bin delta out_dir
import struct, sys, os, collections
exe, code, delta, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
data = open(exe, 'rb').read()
pe = struct.unpack_from('<I', data, 0x3c)[0]
nsec = struct.unpack_from('<H', data, pe + 6)[0]; opt = pe + 24
soh = struct.unpack_from('<H', data, pe + 20)[0]
reloc_rva, reloc_size = struct.unpack_from('<II', data, opt + 112 + 5 * 8)
secs = []
for i in range(nsec):
    o = opt + soh + 40 * i
    vsz, va, rsz, raw = struct.unpack_from('<IIII', data, o + 8)
    secs.append((va, vsz, raw, rsz))
def rva_to_off(r):
    for va, vsz, raw, rsz in secs:
        if va <= r < va + min(vsz, rsz): return raw + r - va
    return None
relocs = set()
at = reloc_rva
while at < reloc_rva + reloc_size:
    o = rva_to_off(at); page, size = struct.unpack_from('<II', data, o)
    if size < 8: break
    for k in range((size - 8) // 2):
        e = struct.unpack_from('<H', data, o + 8 + 2 * k)[0]
        if e >> 12 == 10: relocs.add(page + (e & 0xfff))
    at += size
raw = open(code, 'rb').read(); pos = 0; overlay = {}; versions = collections.defaultdict(set); recs = 0
by_source = collections.Counter()
while pos + 24 <= len(raw):
    rva, n, tick, source, _ = struct.unpack_from('<IIQII', raw, pos); pos += 24
    b = bytearray(raw[pos:pos + n]); pos += n; recs += 1; by_source[source] += 1
    for r in range(max(rva - 7, 0), rva + n):
        if r in relocs:
            lo, hi = max(r, rva), min(r + 8, rva + n)
            if hi - lo == 8:
                v = struct.unpack_from('<Q', b, r - rva)[0]; struct.pack_into('<Q', b, r - rva, (v - delta) & (2**64 - 1))
    for i, x in enumerate(b):
        overlay[rva + i] = x
    versions[rva].add(bytes(b))
os.makedirs(out, exist_ok=True)
# differing bytes only (against disk), merged into runs
diff = sorted(r for r, x in overlay.items() if rva_to_off(r) is not None and data[rva_to_off(r)] != x)
runs = []
for r in diff:
    if runs and r <= runs[-1][1] + 16: runs[-1][1] = r + 1
    else: runs.append([r, r + 1])
with open(os.path.join(out, 'overlay.bin'), 'wb') as f:
    for a, e in runs:
        f.write(struct.pack('<II', a, e - a)); f.write(bytes(overlay.get(r, data[rva_to_off(r)]) for r in range(a, e)))
with open(os.path.join(out, 'overlay.txt'), 'w') as f:
    f.write(f'records {recs} by_source {dict(by_source)} differing_bytes {len(diff)} runs {len(runs)} multi_version_rvas {sum(1 for v in versions.values() if len(v) > 1)}\n')
    for a, e in runs: f.write(f'{a:08x} {e - a}\n')
print(open(os.path.join(out, 'overlay.txt')).readline())
