# I-0140 tranche 14 (read-only): a character's stats and hands from a DS3 save copy, before a run.
# DS30000.sl2 is a BND4 of USER_DATA entries, each MD5(16) + IV(16) + AES-128-CBC with the public DS3 save key (as
# I-0066 Test-SaveSteamId.ps1). The slot holding the name (UTF-16) is read relative to the name (OBSERVED on Alpha's copies,
# 2026-11-16): -0x70 HP, -0x64 FP now, -0x60 its maximum (the all-spells copy's 181 and 181, tranche 13's « FP 181 of 181 »;
# Thomas's copies 117 and 91 of 326), -0x44 Vigor, Attunement, Endurance, Strength, Dexterity, Intelligence, Faith, Luck (int32
# each), -0x1C Vitality, -0x18 soul level (the nine less 89, a check); +0x1D4 the left hand's current slot, +0x1D8 the right's,
# +0x1EC the six hand slots L1 R1 L2 R2 L3 R3 (weapon ids; 110000 Fists). Writes nothing; prints the values.
# Usage: python -I Read-SaveSlot.py <DS30000.sl2> <name> [names.tsv: kind<TAB>id<TAB>name, for the weapons' names]
import struct, sys
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

KEY = bytes([0xFD, 0x46, 0x4D, 0x69, 0x5E, 0x69, 0xA3, 0x9A, 0x10, 0xE3, 0x19, 0xA7, 0xAC, 0xE8, 0xB7, 0xFA])
STATS = ['vigor', 'attunement', 'endurance', 'strength', 'dexterity', 'intelligence', 'faith', 'luck']


def entries(raw):
    if raw[:4] != b'BND4':
        raise SystemExit('not a BND4 save')
    for i in range(struct.unpack_from('<i', raw, 0xC)[0]):
        h = 0x40 + 0x20 * i
        size = struct.unpack_from('<q', raw, h + 8)[0]
        off, no = struct.unpack_from('<II', raw, h + 0x10)
        e = no
        while raw[e] or raw[e + 1]:
            e += 2
        b = raw[off:off + size]
        d = Cipher(algorithms.AES(KEY), modes.CBC(b[16:32])).decryptor()
        yield raw[no:e].decode('utf-16le'), d.update(b[32:]) + d.finalize()


def main():
    path, name = sys.argv[1], sys.argv[2]
    names = {}
    if len(sys.argv) > 3:
        for line in open(sys.argv[3], encoding='utf-8-sig'):
            cols = line.rstrip('\r\n').split('\t', 2)
            if len(cols) < 3 or not cols[1].isdigit():
                continue
            kind, i, n = cols
            if kind.startswith('武器名'):   # the weapon-name FMG
                names[int(i)] = n
    found = False
    for entry, dec in entries(open(path, 'rb').read()):
        if not entry.startswith('USER_DATA00'):
            continue
        p = dec.find(name.encode('utf-16le'))
        if p < 0x70:
            continue
        found = True
        i32 = lambda o: struct.unpack_from('<i', dec, p + o)[0]
        stats = {s: i32(-0x44 + 4 * k) for k, s in enumerate(STATS)}
        stats['vitality'] = i32(-0x1C)
        level = i32(-0x18)
        print(f'{entry} at {p:#x}: level {level} (sum less 89: {sum(stats.values()) - 89}), HP {i32(-0x70)}, FP {i32(-0x64)} of {i32(-0x60)}')
        print('  ' + ', '.join(f'{s} {v}' for s, v in stats.items()))
        slots = [i32(0x1EC + 4 * k) for k in range(6)]
        word = lambda v: f'{v} {names.get(v - v % 10000, "?")}'
        left, right = i32(0x1D4), i32(0x1D8)
        print(f'  left  (current {left + 1}): ' + ' | '.join(word(slots[2 * k]) for k in range(3)))
        print(f'  right (current {right + 1}): ' + ' | '.join(word(slots[2 * k + 1]) for k in range(3)))
    if not found:
        raise SystemExit(f'{name} not found in a USER_DATA00x slot')


if __name__ == '__main__':
    main()
