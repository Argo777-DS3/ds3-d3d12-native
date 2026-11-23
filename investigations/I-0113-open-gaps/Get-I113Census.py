# I-0113 tranche 31 (gap 16): the rows of effect-census.flag (effect_census, both modes; in the Reference with ffx-draw-observe.flag
# only) read from runs' journals. A row is one second of pass 0x2D's effect draws that held a draw blended otherwise than the
# default bank (8 natively, 0x10008 in the original), keyed "pass/bank/ps_dxbc/t0" (t0: copy, the frame copy; tex, another
# texture; none). Per journal: each row's UTC start and its non-default keys; then the totals over the rows by key, the default
# bank's last. Banks are compared as the native writes them: the original's 0x10000 + offset (the GX device table, BankIndexOf)
# taken back to the offset; an original bank below 0x10000 (the first bank table, [[G+0x4DD0]]) printed « g » + offset, never
# merged with the device table's; FFFFFFFF « unresolved », which the module never lets mark a window (the final review's L3).
# Usage: python -I Get-I113Census.py <journal.jsonl> [<more journals>]
import json
import sys
from collections import defaultdict


def rows(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"effect_census"' not in line:
                continue
            try:
                yield json.loads(line)
            except ValueError:
                pass


def bank_of(key, mode):
    bank = int(key.split('/')[1], 16)
    if mode == 'native':
        return '%X' % bank
    if bank == 0xFFFFFFFF:
        return 'unresolved'
    return '%X' % (bank - 0x10000) if bank >= 0x10000 else 'g%X' % bank


def plain(key, mode):
    return bank_of(key, mode) in ('8', 'unresolved')


def main(paths):
    for path in paths:
        totals = defaultdict(int)
        n = 0
        mode = 'native'
        print(path)
        for r in rows(path):
            n += 1
            mode = r.get('mode', 'native')
            odd = {k: v for k, v in r['uses'].items() if not plain(k, mode)}
            base = sum(v for k, v in r['uses'].items() if plain(k, mode))
            print('  row %3d %s %s ms default or unresolved bank draws %d; %s' % (r['row'], r['utc'], r['ms'], base,
                  ', '.join('%s %d' % kv for kv in sorted(odd.items(), key=lambda kv: -kv[1]))))
            for k, v in r['uses'].items():
                totals[k] += v
        print('  rows %d; totals by key (bank as compared: %s):' % (n, mode))
        for k, v in sorted(totals.items(), key=lambda kv: (plain(kv[0], mode), -kv[1])):
            print('    %-48s %-10s %8d' % (k, bank_of(k, mode), v))


if __name__ == '__main__':
    main(sys.argv[1:])
