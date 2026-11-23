# I-0113 tranche 7 (ZERO-CONSTANTS-CENSUS §5): the native's constant words against the original's, draw by draw.
# Usage: py -I Compare-I113ZeroCensus.py <native journal> <original journal> <out.tsv> [programme cache]
# The native journal holds native_zero_census rows (zero-census.flag), the original's reference_zero_census and
# reference_zero_instance rows (zero-census.flag with decal-observe.flag and light-observe.flag). A native draw is matched to the
# original's by (model, PS DXBC checksum, index count), else (model, checksum). For each PS buffer b2, b3, b7 and b4 (VC_InstanceData,
# against the record 5B3D0 wrote for the model, pass state type 6 or 7 first), every word the programme reads (the native row's mask)
# is classed: zero natively and not in the original (« zero »), zero in the original and not natively (« extra »), both nonzero and
# different (« differs »); each class counted apart for the full key and the (model, PS) fallback. The variable names come from
# the RDEF of the game's programmes cached by I-0063 (matched by checksum). Output: one row per (buffer, variable, class) with the
# draws counted and three examples; the summary on stdout. Read-only.
import json, os, struct, sys, collections

def rows(path, events):
    out = collections.defaultdict(list)
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"event"' not in line: continue
            try: r = json.loads(line)
            except ValueError: continue
            if r.get('event') in events: out[r['event']].append(r)
    return out

def cstr(b, o):
    e = b.index(b'\0', o); return b[o:e].decode('latin1')

def rdef(path):
    """{register: (cbuffer name, {offset: (name, size)})} of a DXBC programme's constant buffers."""
    b = open(path, 'rb').read()
    if b[:4] != b'DXBC': return None, {}
    n = struct.unpack_from('<I', b, 28)[0]
    for i in range(n):
        off = struct.unpack_from('<I', b, 32 + 4 * i)[0]
        if b[off:off + 4] != b'RDEF': continue
        r = b[off + 8:]
        ncb, cbo, nbind, bindo = struct.unpack_from('<IIII', r, 0)
        regs = {}
        for k in range(nbind):
            name, typ, ret, dim, samples, point, count, flags = struct.unpack_from('<IIIIIIII', r, bindo + 32 * k)
            if typ == 0: regs[cstr(r, name)] = point
        out = {}
        for k in range(ncb):
            name, nvar, varo, size, flags, typ = struct.unpack_from('<IIIIII', r, cbo + 24 * k)
            variables = {}
            for v in range(nvar):
                vn, vo, vs, vf, vt, dv = struct.unpack_from('<IIIIII', r, varo + 40 * v)
                variables[vo] = (cstr(r, vn), vs)
            cb = cstr(r, name)
            if cb in regs: out[regs[cb]] = (cb, variables)
        return b[4:12].hex().upper(), out
    return b[4:12].hex().upper(), {}

def words(hexstr):
    if not hexstr: return None
    if hexstr.startswith('k:'): hexstr = hexstr[2:]
    raw = bytes.fromhex(hexstr)
    return [struct.unpack_from('<I', raw, 4 * i)[0] for i in range(len(raw) // 4)]

def used(mask):
    return [4 * n + k for n, ch in enumerate(mask) for k in range(4) if int(ch, 16) >> k & 1]

# PerInstanceData, one VC_InstanceData element (WHEEL-STUDY §6): its fields by first word; RDEF names only the array.
INSTANCE = [(0, 'mWorld'), (12, 'atlasUvScaleOffset'), (16, 'userClipPlane'), (20, 'matricesData'), (24, 'envData'),
            (25, 'dynamicLightDisable'), (26, 'envPad'), (28, 'mulCol'), (32, 'aSh'), (44, 'ShLightMask'), (48, 'vDominantRotateParam'),
            (52, 'parallaxCorrectedCubemapOrigin'), (56, 'parallaxCorrectedCubemapAabbMin'), (60, 'parallaxCorrectedCubemapAabbMax'),
            (64, 'ditherFadeAlpha'), (65, 'padding')]

def name_of(layout, slot, word):
    if slot == 4:
        first, field = max((f for f in INSTANCE if f[0] <= word % 68), key=lambda f: f[0])
        return 'cbInstanceData', '%s+%d' % (field, 4 * (word % 68 - first)) if word % 68 != first else field
    cb, variables = layout.get(slot, ('b%d' % slot, {}))
    at = 4 * word
    for o, (n, s) in variables.items():
        if o <= at < o + s: return cb, '%s+%d' % (n, at - o) if at != o else n
    return cb, '+0x%X' % at

def main():
    native_path, original_path, out_path = sys.argv[1:4]
    cache = sys.argv[4] if len(sys.argv) > 4 else r'C:\Users\User\.codex\I0063-cache\programs'
    native = rows(native_path, {'native_zero_census'})['native_zero_census']
    original = rows(original_path, {'reference_zero_census', 'reference_zero_instance'})
    layouts = {}
    for f in os.listdir(cache):
        if f.endswith('.ppo'):
            checksum, layout = rdef(os.path.join(cache, f))
            if checksum: layouts[checksum] = (f, layout)
    by_full, by_ps = {}, {}
    for r in original['reference_zero_census']:
        by_full.setdefault((r['model'], r['ps_dxbc'], r['count']), r); by_ps.setdefault((r['model'], r['ps_dxbc']), r)
    instances = collections.defaultdict(dict)
    for r in original['reference_zero_instance']: instances[r['model']].setdefault(r['state_type'], r['record'])
    found = collections.defaultdict(lambda: [0, []])
    matched = unmatched = 0; keys = collections.Counter()
    for n in native:
        o = by_full.get((n['model'], n['ps_dxbc'], n['count'])); key = 'full'
        if not o: o = by_ps.get((n['model'], n['ps_dxbc'])); key = 'fallback'
        if not o: unmatched += 1; continue
        matched += 1; keys[key] += 1
        layout = layouts.get(n['ps_dxbc'], (None, {}))[1]
        for slot in (2, 3, 7, 4):
            mine = n.get('b%d' % slot)
            if not mine: continue
            nw = words(mine['bytes'])
            if slot == 4:
                records = instances.get(n['model'], {})
                record = records.get(6) or records.get(7) or next(iter(records.values()), None)
                ow = words(record) if record else None
            else:
                ow = words(o.get('ps_cb%d' % slot))
            if not nw or not ow: continue
            for w in used(mine['used']):
                ref = w % 68 if slot == 4 else w   # b4: the original's one record against the native's element
                if w >= len(nw) or ref >= len(ow): continue
                a, b = nw[w], ow[ref]
                if a == b: continue
                cls = ('zero' if a == 0 else 'extra' if b == 0 else 'differs') + '/' + key
                cb, var = name_of(layout, slot, ref if slot == 4 else w)
                entry = found[(slot, cb, var, cls)]
                entry[0] += 1
                if len(entry[1]) < 3:
                    f = lambda x: '%.6g' % struct.unpack('<f', struct.pack('<I', x))[0]
                    entry[1].append('%s m%s %s: %s vs %s' % (n['model'], n['mesh'], n['family'].split('\\')[-1][:50], f(a), f(b)))
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write('slot\tbuffer\tvariable\tclass\tdraws\texamples (native vs original)\n')
        for (slot, cb, var, cls), (count, ex) in sorted(found.items(), key=lambda x: (-x[1][0])):
            f.write('b%d\t%s\t%s\t%s\t%d\t%s\n' % (slot, cb, var, cls, count, ' | '.join(ex)))
    print('native draws %d, matched %d (full key %d, fallback %d), unmatched %d; programmes with RDEF %d; findings %d -> %s' %
          (len(native), matched, keys['full'], keys['fallback'], unmatched, len(layouts), len(found), out_path))
    for (slot, cb, var, cls), (count, ex) in sorted(found.items(), key=lambda x: (-x[1][0]))[:40]:
        print('b%d %-16s %-36s %-7s %5d  %s' % (slot, cb, var, cls, count, ex[0] if ex else ''))

if __name__ == '__main__':
    main()
