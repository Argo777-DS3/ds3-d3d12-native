# I-0113 tranche 30 (gap 16): the rows of effect-scan-draws.flag (native_effect_draw_scan, native_effect_draw_scan_window) read
# from a run's journal. A row is a scan point whose negative pixels grew: the statement before it (kind 1: a 0x29 drawing statement,
# its programme and states; 2: the 0x29 rows drawn before statement `key`; 0: the block's baseline; 3: after the 0x29 pass; tranche
# 31: 4 a 0x2D drawing statement, 5 after the 0x2D pass) wrote
# `pixels - before` pixels holding a negative channel. Grouped by what wrote them: the rows, the pixels added, the largest
# addition, the channels, the most negative values, the frames.
# Usage: python -I Get-I113DrawScan.py <journal.jsonl> [<more journals>]
import json
import sys
from collections import defaultdict


UNPARSED = [0]


def rows(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"native_effect_draw_scan' not in line:
                continue
            try:
                yield json.loads(line)
            except ValueError:
                UNPARSED[0] += 1


def main(paths):
    for path in paths:
        groups = defaultdict(lambda: {'rows': 0, 'added': 0, 'largest': 0, 'neg': [0, 0, 0, 0], 'peak': [0.0, 0.0, 0.0],
                                      'serials': set(), 'box': None, 'nan': 0})
        windows = []
        for r in rows(path):
            if r['event'] == 'native_effect_draw_scan_window':
                windows.append(r)
                continue
            k = r['kind']
            who = ('baseline',) if k == 0 else ('after-0x29',) if k == 3 else ('after-0x2D',) if k == 5 else \
                ('rows-before', r['key']) if k == 2 else \
                ('draw-%X' % r['pass'], r['ps'], r.get('ps_hash', ''), r['vs'], r['blend'], r['depth'], r['raster'], r['op'], r['textures'], r['targets'])
            g = groups[who]
            added = r['pixels'] - r['before']
            g['rows'] += 1
            g['added'] += added
            g['largest'] = max(g['largest'], added)
            g['nan'] = max(g['nan'], r['nan'])
            for c in range(4):
                g['neg'][c] = max(g['neg'][c], r['neg'][c])
            for c in range(3):
                g['peak'][c] = max(g['peak'][c], r['peak'][c] if r['peak'][c] is not None else float('inf'))
            g['serials'].add(r['serial'])
            b = r['box']
            g['box'] = b if g['box'] is None else [min(g['box'][0], b[0]), min(g['box'][1], b[1]), max(g['box'][2], b[2]), max(g['box'][3], b[3])]
        print(path, '(unparsed rows: %d)' % UNPARSED[0])
        UNPARSED[0] = 0
        for w in windows[-1:]:
            print('  window', {k: w.get(k) for k in ('serial', 'frames', 'points', 'flagged', 'logged', 'skipped', 'capped')})
        for who, g in sorted(groups.items(), key=lambda kv: -kv[1]['added']):
            print('  %-90s rows %4d added %8d largest %7d neg(rgba max) %s peak(rgb) %s frames %d box %s nan %d' % (
                ' '.join(str(x) for x in who), g['rows'], g['added'], g['largest'], g['neg'],
                ['%.3g' % p for p in g['peak']], len(g['serials']), g['box'], g['nan']))


if __name__ == '__main__':
    main(sys.argv[1:])
