# I-0140 tranche 16 (the weapon arts need their weapon's Strength and Dexterity; the game recomputes a stat written in memory,
# i140-e80): a NEW save copy, in the folder given (refused when it exists), whose character has the asked attributes; the source is
# only read. Every entry's MD5 is checked first (its step review's M1); the one USER_DATA00x slot holding the name (with its NUL
# after it; refused when two do, L2) is decrypted as Read-SaveSlot.py does, the attributes and the soul level (their sum less 89)
# written at the offsets Read-SaveSlot.py reads, the slot encrypted again with its own IV at its own length (L1) and its MD5 (over
# IV and body) renewed; the other entries are copied byte for byte. Not updated (its step review's M2): USER_DATA010's profile
# summary (the level shown in the load list) and its trailing image of the slot. Prints the new file's SHA-256.
# python -I New-StatsCopy.py <source DS30000.sl2> <name> <new folder> strength=99 dexterity=99 ...
import hashlib, json, os, struct, sys
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

KEY = bytes([0xFD, 0x46, 0x4D, 0x69, 0x5E, 0x69, 0xA3, 0x9A, 0x10, 0xE3, 0x19, 0xA7, 0xAC, 0xE8, 0xB7, 0xFA])
OFFSETS = {'vigor': -0x44, 'attunement': -0x40, 'endurance': -0x3C, 'strength': -0x38, 'dexterity': -0x34,
           'intelligence': -0x30, 'faith': -0x2C, 'luck': -0x28, 'vitality': -0x1C}

def entries(raw):
    for i in range(struct.unpack_from('<i', raw, 0xC)[0]):
        h = 0x40 + 0x20 * i
        size = struct.unpack_from('<q', raw, h + 8)[0]
        off, no = struct.unpack_from('<II', raw, h + 0x10)
        e = no
        while raw[e] or raw[e + 1]: e += 2
        yield raw[no:e].decode('utf-16le'), off, size

def main():
    src, name, folder = sys.argv[1:4]
    asked = {}
    for word in sys.argv[4:]:
        k, _, v = word.partition('=')
        if k not in OFFSETS or not v.isdigit() or not 1 <= int(v) <= 99: raise SystemExit(f'not an attribute word: {word}')
        asked[k] = int(v)
    if os.path.exists(folder): raise SystemExit(f'{folder} exists: a new folder only')
    raw = bytearray(open(src, 'rb').read())
    if raw[:4] != b'BND4': raise SystemExit('not a BND4 save')
    found = []
    for entry, off, size in entries(raw):   # pass 1: every MD5, the slots holding the name
        b = bytes(raw[off:off + size])
        if (size - 32) % 16: raise SystemExit(f'{entry}: its body is not whole AES blocks')
        if hashlib.md5(b[16:]).digest() != b[:16]: raise SystemExit(f'{entry}: its MD5 does not match')
        if not entry.startswith('USER_DATA00'): continue
        dec = Cipher(algorithms.AES(KEY), modes.CBC(b[16:32])).decryptor().update(b[32:])
        pat = name.encode('utf-16le') + b'\x00\x00'
        hits = []
        p = dec.find(pat)
        while p >= 0:
            hits.append(p); p = dec.find(pat, p + 1)
        if len(hits) > 1: raise SystemExit(f'{entry}: the name {len(hits)} times')
        if hits and hits[0] >= 0x70: found.append((entry, off, size, hits[0]))
    if len(found) != 1: raise SystemExit(f'{name}: in {len(found)} slots, one needed')
    entry, off, size, p = found[0]
    b = bytes(raw[off:off + size]); iv = b[16:32]
    dec = bytearray(Cipher(algorithms.AES(KEY), modes.CBC(iv)).decryptor().update(b[32:]))
    before = {k: struct.unpack_from('<i', dec, p + o)[0] for k, o in OFFSETS.items()}
    level = struct.unpack_from('<i', dec, p - 0x18)[0]
    if level != sum(before.values()) - 89: raise SystemExit(f'{entry}: level {level} is not the sum less 89 ({sum(before.values()) - 89}): offsets not trusted')
    after = dict(before); after.update(asked)
    for k, o in OFFSETS.items(): struct.pack_into('<i', dec, p + o, after[k])
    struct.pack_into('<i', dec, p - 0x18, sum(after.values()) - 89)
    body = iv + Cipher(algorithms.AES(KEY), modes.CBC(iv)).encryptor().update(bytes(dec))
    if len(body) + 16 != size: raise SystemExit(f'{entry}: the new body is {len(body) + 16} bytes, not {size}')
    raw[off:off + size] = hashlib.md5(body).digest() + body
    print(f'{entry}: {before} level {level} -> {after} level {sum(after.values()) - 89}')
    os.makedirs(folder)
    out = os.path.join(folder, 'DS30000.sl2')
    open(out, 'wb').write(bytes(raw))
    sha = hashlib.sha256(bytes(raw)).hexdigest().upper()
    json.dump({'source': src, 'source_sha256': hashlib.sha256(open(src, 'rb').read()).hexdigest().upper(), 'copy': out, 'sha256': sha,
               'changed': asked, 'by': 'I-0140 New-StatsCopy.py'}, open(os.path.join(folder, 'record.json'), 'w'), indent=2)
    print(out, sha)

if __name__ == '__main__':
    main()
