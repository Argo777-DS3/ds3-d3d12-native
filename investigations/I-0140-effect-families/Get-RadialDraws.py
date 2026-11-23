# I-0140 (gap 16): reads the native_ffx_radial_draw rows (effect_draw.cpp, ffx-record) of a run journal: the 608 radial blur's
# draws (class 3D7E8C0), their counts, the first four vertices' first six floats and VS b0's first 32 words (two 4x4 matrices).
# For each row: whether the matrices are zero, whether the four vertices coincide, and the screen rectangle the first matrix
# would give the vertices' xyz (as rows, v * M, and as columns, M * v), in pixels of -Width x -Height. Prints numbers only.
# Usage: python -I Get-RadialDraws.py <journal> [width] [height]
import json, sys

def project(v, m, rows):
    x, y, z = v
    if rows:
        c = [x * m[0][j] + y * m[1][j] + z * m[2][j] + m[3][j] for j in range(4)]
    else:
        c = [m[j][0] * x + m[j][1] * y + m[j][2] * z + m[j][3] for j in range(4)]
    return c

def main():
    path = sys.argv[1]
    w = int(sys.argv[2]) if len(sys.argv) > 2 else 2560
    h = int(sys.argv[3]) if len(sys.argv) > 3 else 1440
    rows = []
    skipped = 0
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            at = line.find('{"event":"native_ffx_radial_draw"')
            if at < 0:
                continue
            try:
                rows.append(json.loads(line[at:].strip()))
            except json.JSONDecodeError:
                skipped += 1   # a non-finite float printed as nan or inf
    print(f'rows {len(rows)} skipped {skipped}')
    for r in rows:
        v = r['v']
        verts = [v[i * 6:i * 6 + 6] for i in range(4)]
        b0 = r['vs_b0']
        m0 = [b0[i * 4:i * 4 + 4] for i in range(4)]
        m1 = [b0[16 + i * 4:16 + i * 4 + 4] for i in range(4)]
        zero0 = all(x == 0 for x in b0[:16]); zero1 = all(x == 0 for x in b0[16:32])
        same = all(verts[i][:3] == verts[0][:3] for i in range(4))
        head = (f"n {r['n']} pass {r['pass']} topo {r['topology']} count {r['count']} start {r['start']} indexed {r['indexed']} "
                f"stride {r['stride']} b0 {r['vs_b0_bytes']} invalid {r['invalid']} m0_zero {zero0} m1_zero {zero1} verts_same {same}")
        print(head)
        for i, vv in enumerate(verts):
            print('  v%d ' % i + ' '.join('%.5g' % x for x in vv))
        print('  m0 ' + ' | '.join(' '.join('%.5g' % x for x in row) for row in m0))
        print('  m1 ' + ' | '.join(' '.join('%.5g' % x for x in row) for row in m1))
        if not zero0:
            for rows_mode in (True, False):
                xs, ys = [], []
                for vv in verts:
                    c = project(vv[:3], m0, rows_mode)
                    if c[3] == 0:
                        continue
                    xs.append((c[0] / c[3] * 0.5 + 0.5) * w); ys.append((0.5 - c[1] / c[3] * 0.5) * h)
                if xs:
                    print(f"  rect({'v*M' if rows_mode else 'M*v'}) x {min(xs):.1f}..{max(xs):.1f} y {min(ys):.1f}..{max(ys):.1f}")

if __name__ == '__main__':
    main()
