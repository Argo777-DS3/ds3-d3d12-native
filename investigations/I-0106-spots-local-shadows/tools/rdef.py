# Lists a DXBC programme's RDEF: bound resources (type, register) and constant buffers with their variables (offset, size).
# Usage: py -I rdef.py <file.ppo|vpo> ... (the files are the engine's own programmes, read-only)
import os, struct, sys
def cstr(b, o):
    e = b.index(b'\0', o); return b[o:e].decode('latin1')
def rdef(path):
    b = open(path, 'rb').read()
    assert b[:4] == b'DXBC'
    n = struct.unpack_from('<I', b, 28)[0]
    for i in range(n):
        off = struct.unpack_from('<I', b, 32 + 4 * i)[0]
        if b[off:off + 4] != b'RDEF': continue
        r = b[off + 8:]
        ncb, cbo, nbind, bindo = struct.unpack_from('<IIII', r, 0)
        print(os.path.basename(path))
        kinds = {0: 'cbuffer', 1: 'tbuffer', 2: 'texture', 3: 'sampler', 4: 'uav', 5: 'structured', 7: 'byteaddr'}
        for k in range(nbind):
            name, typ, ret, dim, samples, point, count, flags = struct.unpack_from('<IIIIIIII', r, bindo + 32 * k)
            print('  bind %-12s %-34s reg %d count %d dim %d' % (kinds.get(typ, typ), cstr(r, name), point, count, dim))
        for k in range(ncb):
            name, nvar, varo, size, flags, typ = struct.unpack_from('<IIIIII', r, cbo + 24 * k)
            print('  cbuffer %s size %d' % (cstr(r, name), size))
            for v in range(nvar):
                vn, vo, vs, vf, vt, dv = struct.unpack_from('<IIIIII', r, varo + 40 * v)  # SM5: 40-byte variable records
                print('    %-36s +%-5d %d%s' % (cstr(r, vn), vo, vs, '' if vf & 2 else ' (unused)'))
for p in sys.argv[1:]: rdef(p)
