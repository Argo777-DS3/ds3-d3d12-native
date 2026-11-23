# I-0113 gap 8: the local-shadow rasterizer words of every BTL light (140018490 reads them for versions 4 and later: entry +0xA2
# the depth bias, a signed byte, into light +0xB4; +0xA3 the cull flip, into light +0xB2). Reads every *.btl.dcx (DCX DFLT) of a
# folder and prints the counts by (BTL version, light type, bias, flip). Usage: py -I Get-I113BtlBias.py <folder>. Read-only.
import collections, glob, os, struct, sys, zlib

counts = collections.Counter()
for path in glob.glob(os.path.join(sys.argv[1], '*.btl.dcx')):
    data = open(path, 'rb').read()
    body = zlib.decompress(data[data.find(b'DCA\0') + 8:])
    major, minor, count, names = struct.unpack_from('<iiii', body, 0)
    for k in range(count):
        entry = 0x3C + names + 200 * k   # the 200-byte entries of 140018490's second branch
        kind = struct.unpack_from('<i', body, entry + 0x18)[0]
        counts[(minor, kind, struct.unpack_from('<b', body, entry + 0xA2)[0], body[entry + 0xA3])] += 1
print('version type bias flip count')
for key, n in sorted(counts.items()):
    print(*key, n)
