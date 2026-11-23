# I-0105: reads the output of I0105Str.java (143D20580:4160) as the 65 built-in material parameter definitions (0x40-byte rows:
# name pointer, kind, animation selector, vector index, scalar offset, ..., transform flag at +0x3C). Run with py -I.
import re, struct, sys
t = open(sys.argv[1], encoding="utf-8").read().splitlines()
rows, names, sec = {}, {}, None
for l in t:
    if l.startswith("  -- 8-byte"): sec = "p"; continue
    if l.startswith("  -- UTF"): sec = "u"; continue
    m = re.match(r"  \+([0-9A-F]{3}) ((?:[0-9a-f]{2} ){16})$", l)
    if m and sec is None: rows[int(m.group(1), 16)] = bytes.fromhex(m.group(2).replace(" ", ""))
    m = re.match(r"  \+([0-9A-F]{3}) -> (\S+) wide=(.*?) ascii", l)
    if m and sec == "p": names[int(m.group(1), 16)] = m.group(3)
blob = b"".join(rows[k] for k in sorted(rows))
for i in range(len(blob) // 0x40):
    r = blob[i * 0x40:(i + 1) * 0x40]
    kind, sel, vec, scal = struct.unpack_from("<4I", r, 8)
    print(i, names.get(i * 0x40, "?"), "kind", kind, "sel", sel, "vec", vec, "scalar", scal, "xform", r[0x3C], "tail", r[0x18:0x3C].hex())
