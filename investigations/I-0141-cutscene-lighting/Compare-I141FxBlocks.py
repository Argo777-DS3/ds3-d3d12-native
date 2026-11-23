# I-0141 (the ceremony's smoke): compares the F9 effect blocks of a native run and an original run word by word.
# Rows: ffx_ps_image (the kind: blend, colour scale, fog mode, pre-alpha), ffx_ps_block (the same n: PSCommonBuffer 140 words,
# PSLightBuffer 100, cbFog 56, each word as 8 hex digits) and ffx_vs_image (VSCommonBuffer 84 floats, by writer). The draws are
# grouped by kind; for each kind present in both, each word in each mode. A word with a non-zero sample whose exponent field is 0
# or all ones (a small integer, -1) in a mode is an integer there and compared exactly, by its most frequent value; otherwise a float,
# by its median with the zeros and its median without them (draws that leave a word at 0), listed when either median differs by more than 1 % of the larger (or 1e-12 absolute); a word an
# integer in one mode and a float in the other is always listed. A block section whose read failed is left out. PSLightBuffer is uploaded for the lit programmes only:
# elsewhere it holds an earlier draw's (the step review's M2), so its section is read with the kind's programmes in mind.
# The VSCommonBuffer rows word by word per writer, the writers both modes logged.
# Usage: python -I Compare-I141FxBlocks.py <native startup.jsonl> <original startup.jsonl> [out.txt]
import collections, json, statistics, struct, sys


def load(path):
    image, block, vs, rejected = {}, {}, [], 0
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"ffx_ps_image"' not in line and '"ffx_ps_block"' not in line and '"ffx_vs_image"' not in line:
                continue
            try:
                r = json.loads(line)
            except ValueError:
                rejected += 1
                continue
            e = r.get('event')
            if e == 'ffx_ps_image':
                image[r['n']] = r
            elif e == 'ffx_ps_block':
                try:
                    for name, count in (('ps', 140), ('light', 100), ('fog', 56)):
                        text = r.get(name)
                        if not isinstance(text, str) or len(text) != 8 * count:
                            raise ValueError(name)
                        r[name] = [int(text[i:i + 8], 16) for i in range(0, len(text), 8)]
                except ValueError:
                    rejected += 1
                    continue
                for name, ok in zip(('ps', 'light', 'fog'), r.get('read', [1, 1, 1])):
                    if not ok:
                        r[name] = None
                block[r['n']] = r
            elif e == 'ffx_vs_image':
                vs.append(r)
    kinds = {}
    for n, r in image.items():
        k = (r['blend'], tuple(round(v, 4) for v in r['colour_scale']), r['fog'], r['pre_alpha'])
        kinds.setdefault(k, []).append(block.get(n))
    counts = {'image rows': len(image), 'block rows': len(block), 'image rows without a block': len(set(image) - set(block)),
              'block rows without an image': len(set(block) - set(image)), 'unreadable lines': rejected,
              'blocks with a failed read': sum(1 for r in block.values() if 0 in r.get('read', [1, 1, 1]))}
    return kinds, vs, counts


def as_float(bits):
    return struct.unpack('<f', struct.pack('<I', bits))[0]


def integer_like(bits):
    return (bits >> 23) & 0xFF in (0, 0xFF)


def describe(values):
    """values: one word's 32-bit patterns in one mode. Returns (kind, summary text, key)."""
    if not values:
        return None
    if any(v not in (0, 0x80000000) and integer_like(v) for v in values):
        common = collections.Counter(values).most_common(3)
        return 'int', ', '.join(f'0x{v:08X}x{c}' for v, c in common) + f' of {len(values)}', common[0][0]
    f = [as_float(v) for v in values]
    m = statistics.median(f)
    nz = [x for x in f if x != 0]
    m_nz = statistics.median(nz) if nz else 0.0
    return 'float', f'{m:.6g} [{min(f):.6g}, {max(f):.6g}] n={len(f)}, non-zero {len(nz)} median {m_nz:.6g}', (m, m_nz)


def differ(a, b):
    if a is None or b is None:
        return a is not b
    if a[0] != b[0]:
        return True
    if a[0] == 'int':
        return a[2] != b[2]
    return any(abs(x - y) > max(1e-12, 0.01 * max(abs(x), abs(y))) for x, y in zip(a[2], b[2]))


def words(rows, name):
    rows = [r for r in rows if r and r.get(name)]
    if not rows:
        return []
    count = len(rows[0][name])
    return [describe([r[name][i] for r in rows]) for i in range(count)]


def text(x):
    return x[1] if x else 'no value'


def main():
    nat_kinds, nat_vs, nat_counts = load(sys.argv[1])
    orig_kinds, orig_vs, orig_counts = load(sys.argv[2])
    out = ['rows: native / original']
    for key in nat_counts:
        out.append(f'  {key}: {nat_counts[key]} / {orig_counts[key]}')
    out.append('kinds (blend, colour scale, fog, pre-alpha): native / original draws')
    for k in sorted(set(nat_kinds) | set(orig_kinds), key=str):
        out.append(f'  {k}: {len(nat_kinds.get(k, []))} / {len(orig_kinds.get(k, []))}')
    for k in sorted(set(nat_kinds) & set(orig_kinds), key=str):
        out.append(f'\nkind {k}')
        for name in ('ps', 'light', 'fog'):
            a, b = words(nat_kinds[k], name), words(orig_kinds[k], name)
            if not a or not b:
                out.append(f'  {name}: no block rows (native {len(a)} words, original {len(b)})')
                continue
            listed = 0
            for i, (x, y) in enumerate(zip(a, b)):
                if differ(x, y):
                    listed += 1
                    out.append(f'  {name}[{i}] (+0x{4 * i:X}): native {text(x)}; original {text(y)}')
            out.append(f'  {name}: {listed} of {len(a)} words differ')
    out.append('\nVSCommonBuffer by writer: native / original rows')
    for caller in sorted({r['caller'] for r in nat_vs} | {r['caller'] for r in orig_vs}):
        out.append(f"  writer {caller}: {sum(1 for r in nat_vs if r['caller'] == caller)} / {sum(1 for r in orig_vs if r['caller'] == caller)}")
    # Word by word per writer, only the writers both modes logged (the first step review's M1: the original's wrapper hook also
    # sees other passes' blocks that pass the near/far test; the native's sees the effects' buffers alone).
    for caller in sorted({r['caller'] for r in nat_vs} & {r['caller'] for r in orig_vs}):
        def floats(rows):
            rows = [r['f'] for r in rows if r['caller'] == caller]
            return [[v for v in (row[i] for row in rows) if v is not None] for i in range(len(rows[0]))] if rows else []
        listed = 0
        for i, (x, y) in enumerate(zip(floats(nat_vs), floats(orig_vs))):
            mx, my = (statistics.median(x) if x else None), (statistics.median(y) if y else None)
            if (mx is None) != (my is None) or (mx is not None and abs(mx - my) > max(1e-12, 0.01 * max(abs(mx), abs(my)))):
                listed += 1
                out.append(f'  {caller} vs[{i}] (+0x{4 * i:X}): native {mx}; original {my}')
        out.append(f'  writer {caller}: {listed} words differ')
    result = '\n'.join(out) + '\n'
    if len(sys.argv) > 3:
        with open(sys.argv[3], 'w', encoding='utf-8') as f:
            f.write(result)
    sys.stdout.write(result)


if __name__ == '__main__':
    main()
