"""Read only pinned installed CPU shader descriptors; no runtime/GPU input."""
import hashlib, json, pathlib, struct, sys

pe = pathlib.Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(pe).hexdigest().upper() == 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'
h = struct.unpack_from('<I', pe, 60)[0]
n = struct.unpack_from('<H', pe, h + 6)[0]
opt = struct.unpack_from('<H', pe, h + 20)[0]
sections = [struct.unpack_from('<III', pe, h + 24 + opt + k * 40 + 12) for k in range(n)]
def read(va, size):
    rva = va - 0x140000000
    rows = [pe[off + rva - at:off + rva - at + size] for at, count, off in sections if at <= rva and rva - at + size <= count]
    assert len(rows) == 1 and len(rows[0]) == size
    return rows[0]
def wide(va): return struct.unpack('<Q', read(va, 8))[0]
rows = []
for stage, table, count in [('vs', 0x1446f90c0, 511), ('ps', 0x1446fc330, 1339)]:
    for index in range(count):
        descriptor = wide(table + index * 8)
        if not descriptor: continue
        mode = struct.unpack('<I', read(descriptor + 4, 4))[0]
        if mode != 3 or not wide(descriptor + 32): continue
        uniforms = read(wide(descriptor + 16), 250)
        semantics = read(wide(descriptor + 24), 75)
        code = wide(descriptor + 32)
        # Actual container extent, independently declared by DXBC; descriptor
        #+28 is not admitted as an on-disk length by this inventory.
        payload = read(code, struct.unpack_from('<I', read(code, 32), 24)[0])
        assert payload[:4] == b'DXBC'
        constants = []
        for part in range(struct.unpack_from('<I', payload, 28)[0]):
            at = struct.unpack_from('<I', payload, 32 + part * 4)[0]
            if payload[at:at + 4] != b'RDEF': continue
            body = payload[at + 8:at + 8 + struct.unpack_from('<I', payload, at + 4)[0]]
            def name(offset): return body[offset:body.index(b'\0', offset)].decode('ascii')
            cb_count, cb_offset = struct.unpack_from('<II', body)
            for cb in range(cb_count):
                nm, variable_count, variable_offset, cb_size, cb_flags, cb_type = struct.unpack_from('<6I', body, cb_offset + cb * 24)
                variables = []
                for variable in range(variable_count):
                    vn, start, size, flags, typ = struct.unpack_from('<5I', body, variable_offset + variable * 40)
                    variables.append({'name': name(vn), 'offset': start, 'bytes': size, 'flags': flags,
                                      'type': list(struct.unpack_from('<6H', body, typ))})
                constants.append({'name': name(nm), 'bytes': cb_size, 'type': cb_type, 'variables': variables})
        rows.append({'stage': stage, 'index': index, 'descriptor': hex(descriptor),
                     'uniforms': [list(struct.unpack_from('<hhBBhBB', uniforms, k * 10)) for k in range(25)],
                     'semantics': [list(struct.unpack_from('<bbb', semantics, k * 3)) for k in range(25)], 'constants': constants})
families = {}
for row in rows:
    shapes = sorted({tuple(v['type']) for cb in row['constants'] for v in cb['variables']})
    family = 'no-constants' if not shapes else ('float4-only' if all(t[0:4] == (1, 3, 1, 4) and not t[5] for t in shapes) else 'scalar-or-other-vector')
    families.setdefault(family, []).append({'stage': row['stage'], 'index': row['index'], 'types': shapes})
out = {'executable_sha256': hashlib.sha256(pe).hexdigest().upper(), 'constant_layout_families': families, 'programmes': rows}
pathlib.Path(sys.argv[2]).write_text(json.dumps(out, separators=(',', ':')) + '\n', encoding='utf-8')
print('Owned CPU descriptor inventory:', len(rows), 'mode3 programmes')
print('Constant families:', {key: len(value) for key, value in families.items()})
for row in rows:
    if (row['stage'], row['index']) in [('vs',340),('ps',909)]:
        print(row)
