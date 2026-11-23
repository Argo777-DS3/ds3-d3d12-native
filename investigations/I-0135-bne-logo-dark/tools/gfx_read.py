"""I-0135: list a Scaleform GFX movie's tags (DS3's menu pages), read-only.

Usage: python -I gfx_read.py <file.gfx>

Prints the header (stage rectangle in pixels, frame rate and count) and every tag, nested sprites indented: the background
colour (SetBackgroundColor), each shape's bounds and fill styles (solid RGBA, gradient, bitmap with its character id), the
placements (PlaceObject2/3: depth, character, matrix, colour transform), Scaleform's external images (DefineExternalImage and
DefineExternalImage2: export name and file name), imports, frame labels, exports and the strings of action tags. Unknown tags
are named by code and length. The file is read only; nothing is written.

Limits (I-0135 step review): only each shape's initial fill style array is listed (styles a shape record introduces later,
and line styles, are not), so « no white fill » cannot be concluded from this listing alone; PlaceObject3's class name is read
only with HasClassName (the specification also names HasImage with HasCharacter, which DS3's exporter does not write: read so,
05_901_logo_bne.gfx's sprite 4 placed a wrong character); its blend mode is read only without a filter list; the GFX external
image tags follow the community layout (their unread bytes are printed: a wrong layout shows as leftovers).
"""
import struct
import sys
import zlib

NAMES = {0: "End", 1: "ShowFrame", 2: "DefineShape", 9: "SetBackgroundColor", 11: "DefineText", 12: "DoAction",
         22: "DefineShape2", 26: "PlaceObject2", 28: "RemoveObject2", 32: "DefineShape3", 33: "DefineText2",
         37: "DefineEditText", 39: "DefineSprite", 43: "FrameLabel", 56: "ExportAssets", 57: "ImportAssets",
         59: "DoInitAction", 69: "FileAttributes", 70: "PlaceObject3", 71: "ImportAssets2", 75: "DefineFont3",
         76: "SymbolClass", 77: "Metadata", 82: "DoABC", 83: "DefineShape4", 86: "DefineSceneAndFrameLabelData",
         1000: "GFX_ExporterInfo", 1001: "GFX_DefineExternalImage", 1002: "GFX_FontTextureInfo",
         1003: "GFX_DefineExternalGradient", 1004: "GFX_DefineGradientMap", 1005: "GFX_DefineCompactedFont",
         1006: "GFX_DefineExternalSound", 1007: "GFX_DefineExternalStreamSound", 1008: "GFX_DefineSubImage",
         1009: "GFX_DefineExternalImage2", 6: "DefineBits", 20: "DefineBitsLossless", 21: "DefineBitsJPEG2",
         35: "DefineBitsJPEG3", 36: "DefineBitsLossless2", 90: "DefineBitsJPEG4", 8: "JPEGTables"}


class Bits:
    def __init__(self, data, pos):
        self.data, self.byte, self.bit = data, pos, 0

    def u(self, n):
        v = 0
        for _ in range(n):
            v = (v << 1) | ((self.data[self.byte] >> (7 - self.bit)) & 1)
            self.bit += 1
            if self.bit == 8:
                self.bit, self.byte = 0, self.byte + 1
        return v

    def s(self, n):
        v = self.u(n)
        return v - (1 << n) if n and v & (1 << (n - 1)) else v

    def align(self):
        if self.bit:
            self.bit, self.byte = 0, self.byte + 1
        return self.byte


def rect(data, pos):
    b = Bits(data, pos)
    n = b.u(5)
    vals = [b.s(n) / 20.0 for _ in range(4)]  # xmin, xmax, ymin, ymax in pixels
    return vals, b.align()


def matrix(data, pos):
    b = Bits(data, pos)
    sx = sy = 1.0
    r0 = r1 = 0.0
    if b.u(1):
        n = b.u(5)
        sx, sy = b.s(n) / 65536.0, b.s(n) / 65536.0
    if b.u(1):
        n = b.u(5)
        r0, r1 = b.s(n) / 65536.0, b.s(n) / 65536.0
    n = b.u(5)
    tx, ty = b.s(n) / 20.0, b.s(n) / 20.0
    return (round(sx, 4), round(r0, 4), round(r1, 4), round(sy, 4), tx, ty), b.align()


def cxform(data, pos, alpha=True):
    b = Bits(data, pos)
    has_add, has_mul = b.u(1), b.u(1)
    n = b.u(4)
    k = 4 if alpha else 3
    mul = [b.s(n) / 256.0 for _ in range(k)] if has_mul else None
    add = [b.s(n) for _ in range(k)] if has_add else None
    return {"mul": mul, "add": add}, b.align()


def cstring(data, pos):
    end = data.index(0, pos)
    return data[pos:end].decode("latin-1"), end + 1


def pstring(data, pos):
    n = data[pos]
    return data[pos + 1:pos + 1 + n].decode("latin-1"), pos + 1 + n


def gradient(data, pos, rgba, focal):
    b = data[pos]
    count = b & 0x0F
    pos += 1
    stops = []
    for _ in range(count):
        ratio = data[pos]
        col = tuple(data[pos + 1:pos + (5 if rgba else 4)])
        pos += 5 if rgba else 4
        stops.append((ratio, col))
    if focal:
        pos += 2
    return {"spread_interp": b >> 4, "stops": stops}, pos


def fill_styles(data, pos, shape_tag):
    rgba = shape_tag in (32, 83)
    count = data[pos]
    pos += 1
    if count == 0xFF and shape_tag != 2:
        count = struct.unpack_from("<H", data, pos)[0]
        pos += 2
    out = []
    for _ in range(count):
        kind = data[pos]
        pos += 1
        if kind == 0x00:
            col = tuple(data[pos:pos + (4 if rgba else 3)])
            pos += 4 if rgba else 3
            out.append({"solid": col})
        elif kind in (0x10, 0x12, 0x13):
            m, pos = matrix(data, pos)
            g, pos = gradient(data, pos, rgba, kind == 0x13)
            out.append({"gradient": hex(kind), "matrix": m, **g})
        elif 0x40 <= kind <= 0x43:
            bid = struct.unpack_from("<H", data, pos)[0]
            m, pos = matrix(data, pos + 2)
            out.append({"bitmap": bid, "kind": hex(kind), "matrix": m})
        else:
            out.append({"unknown_fill": hex(kind)})
            break
    return out


def tags(data, pos, end, depth=0):
    pad = "  " * depth
    while pos < end:
        if pos + 2 > end:
            print(f"{pad}(truncated tag header at {pos})")
            return
        head = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        code, length = head >> 6, head & 0x3F
        if length == 0x3F:
            length = struct.unpack_from("<I", data, pos)[0]
            pos += 4
        body, nxt = pos, pos + length
        name = NAMES.get(code, f"tag{code}")
        line = f"{pad}{name} ({code}) len {length}"
        try:
            if code == 9:
                line += f" rgb {tuple(data[body:body + 3])}"
            elif code in (2, 22, 32, 83):
                sid = struct.unpack_from("<H", data, body)[0]
                r, p = rect(data, body + 2)
                if code == 83:
                    _, p = rect(data, p)
                    p += 1
                line += f" id {sid} bounds {r} fills {fill_styles(data, p, code)}"
            elif code in (26, 70):
                flags = data[body]
                p = body + 1
                flags2 = 0
                if code == 70:
                    flags2 = data[p]
                    p += 1
                dep = struct.unpack_from("<H", data, p)[0]
                p += 2
                info = {"depth": dep, "move": bool(flags & 1)}
                if code == 70 and flags2 & 0x08:   # DS3's GFX exporter writes no class name for HasImage with HasCharacter
                    info["class"], p = cstring(data, p)
                if flags & 0x02:
                    info["char"] = struct.unpack_from("<H", data, p)[0]
                    p += 2
                if flags & 0x04:
                    info["matrix"], p = matrix(data, p)
                if flags & 0x08:
                    info["cxform"], p = cxform(data, p)
                if flags & 0x10:
                    info["ratio"] = struct.unpack_from("<H", data, p)[0]
                    p += 2
                if flags & 0x20:
                    info["name"], p = cstring(data, p)
                if flags & 0x40:
                    info["clip_depth"] = struct.unpack_from("<H", data, p)[0]
                    p += 2
                if code == 70:
                    info["flags2"] = hex(flags2)
                    if flags2 & 0x02 and not flags2 & 0x01:
                        info["blend_mode"] = data[p]
                line += f" {info}"
            elif code == 28:
                line += f" depth {struct.unpack_from('<H', data, body)[0]}"
            elif code == 39:
                sid, frames = struct.unpack_from("<HH", data, body)
                print(f"{line} id {sid} frames {frames}")
                tags(data, body + 4, nxt, depth + 1)
                pos = nxt
                continue
            elif code == 43:
                line += f" '{cstring(data, body)[0]}'"
            elif code in (56, 76):
                n = struct.unpack_from("<H", data, body)[0]
                p = body + 2
                items = []
                for _ in range(n):
                    cid = struct.unpack_from("<H", data, p)[0]
                    s, p = cstring(data, p + 2)
                    items.append((cid, s))
                line += f" {items}"
            elif code in (57, 71):
                url, p = cstring(data, body)
                if code == 71:
                    p += 2
                n = struct.unpack_from("<H", data, p)[0]
                p += 2
                items = []
                for _ in range(n):
                    cid = struct.unpack_from("<H", data, p)[0]
                    s, p = cstring(data, p + 2)
                    items.append((cid, s))
                line += f" url '{url}' {items}"
            elif code == 1001:
                cid, fmt, w, h = struct.unpack_from("<HHHH", data, body)
                export, p = pstring(data, body + 8)
                fname, p = pstring(data, p)
                line += f" id {cid} format {fmt} size {w}x{h} export '{export}' file '{fname}'"
                if p < nxt:
                    line += f" leftover {data[p:nxt].hex()}"
            elif code == 1009:
                cid, fmt, w, h = struct.unpack_from("<IHHH", data, body)
                export, p = pstring(data, body + 10)
                fname, p = pstring(data, p)
                line += f" id {cid} format {fmt} size {w}x{h} export '{export}' file '{fname}'"
                if p < nxt:
                    line += f" leftover {data[p:nxt].hex()}"
            elif code in (12, 59, 82, 1000, 77):
                raw = data[body:nxt]
                words = [w.decode("latin-1") for w in raw.split(b"\0") if len(w) >= 3 and all(32 <= c < 127 for c in w)]
                line += f" strings {words[:40]}"
            elif code not in (0, 1):
                line += f" hex {data[body:min(nxt, body + 32)].hex()}"
        except (IndexError, struct.error, ValueError) as error:
            line += f" (unparsed: {error})"
        print(line)
        pos = nxt
        if code == 0 and depth:
            break


def main(path):
    raw = open(path, "rb").read()
    sig, version, length = raw[:3], raw[3], struct.unpack_from("<I", raw, 4)[0]
    data = raw[:8] + (zlib.decompress(raw[8:]) if sig in (b"CFX", b"CWS") else raw[8:])
    r, p = rect(data, 8)
    rate, frames = struct.unpack_from("<HH", data, p)
    print(f"{sig.decode()} v{version} length {length} (file {len(raw)}) stage {r} rate {rate / 256.0} frames {frames}")
    tags(data, p + 4, len(data))


if __name__ == "__main__":
    main(sys.argv[1])
