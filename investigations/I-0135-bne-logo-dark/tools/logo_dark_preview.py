"""I-0135: the Bandai Namco logo made for a black background, offline, the rule the native applies (preview only).

Usage: python -I logo_dark_preview.py <MENU_BNE_Logo.dds> <out_dir>

Reads the game's DXT1 image (extracted read-only, outside Git) and writes, in <out_dir> (outside Git, never published):
  bne_dark_on_black.png   the copy composed on black (what the page shows at full opacity);
  bne_dark_straight.rgba  the copy itself, straight alpha, raw RGBA8 rows (what native/frame/dark_logo.h must produce);
  bne_dark_mask.png       the classification (black: background, made transparent; grey: colour-to-alpha band;
                          white: kept as drawn);
and prints the counts. The rule (coordinator, 2026-11-12 05:19):
  1. white = min(R, G, B) >= 245; the non-white pixels are labelled (4-connected); the blob is the largest component.
  2. each white component is kept (the letters BANDAI NAMCO) only if it does not touch the image's border and touches the
     blob; every other white component (the outer background, the counters of « Entertainment » and of the trade mark) is
     background.
  3. colour-to-alpha from white (alpha = max(255 - c) / 255, colour unmixed) on the background, on the non-blob components
     that touch the background (the red letters: identity on their saturated core) and on the blob's pixels within 3 pixels
     of the background; the blob's interior, the kept white letters and their counters (non-blob components enclosed by a
     kept letter) are never touched.
"""
import struct
import sys
import zlib

WHITE, BAND = 245, 3


def rgb565(c):
    return ((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31


def decode_bc1(dds):
    assert dds[:4] == b"DDS " and dds[84:88] == b"DXT1", "a DXT1 DDS is expected"
    h, w = struct.unpack_from("<II", dds, 12)
    px = [(0, 0, 0, 0)] * (w * h)
    pos = 128
    for by in range(h // 4):
        for bx in range(w // 4):
            c0, c1, bits = struct.unpack_from("<HHI", dds, pos)
            pos += 8
            a, b = rgb565(c0), rgb565(c1)
            if c0 > c1:
                pal = [a + (255,), b + (255,), tuple((2 * x + y) // 3 for x, y in zip(a, b)) + (255,),
                       tuple((x + 2 * y) // 3 for x, y in zip(a, b)) + (255,)]
            else:
                pal = [a + (255,), b + (255,), tuple((x + y) // 2 for x, y in zip(a, b)) + (255,), (0, 0, 0, 0)]
            for i in range(16):
                px[(by * 4 + i // 4) * w + bx * 4 + i % 4] = pal[(bits >> (2 * i)) & 3]
    return w, h, px


def label(w, h, member):
    lab = [0] * (w * h)
    sizes = [0]
    for start in range(w * h):
        if not member[start] or lab[start]:
            continue
        n = len(sizes)
        sizes.append(0)
        lab[start] = n
        stack = [start]
        while stack:
            i = stack.pop()
            sizes[n] += 1
            x, y = i % w, i // w
            for j, ok in ((i - 1, x > 0), (i + 1, x < w - 1), (i - w, y > 0), (i + w, y < h - 1)):
                if ok and member[j] and not lab[j]:
                    lab[j] = n
                    stack.append(j)
    return lab, sizes


def dilate(w, h, mask, r):
    rows = [False] * (w * h)
    for y in range(h):
        acc = [0]
        for x in range(w):
            acc.append(acc[-1] + mask[y * w + x])
        for x in range(w):
            rows[y * w + x] = acc[min(w, x + r + 1)] - acc[max(0, x - r)] > 0
    out = [False] * (w * h)
    for x in range(w):
        acc = [0]
        for y in range(h):
            acc.append(acc[-1] + rows[y * w + x])
        for y in range(h):
            out[y * w + x] = acc[min(h, y + r + 1)] - acc[max(0, y - r)] > 0
    return out


def colour_to_alpha(p):
    r, g, b = p[:3]
    a = max(255 - r, 255 - g, 255 - b)
    if not a:
        return (0, 0, 0, 0)
    return tuple(max(0, 255 - ((255 - c) * 255 + a // 2) // a) for c in (r, g, b)) + (a,)   # as native/frame/dark_logo.h


def png(path, w, h, rows):
    raw = b"".join(b"\0" + bytes(row) for row in rows)
    chunk = lambda t, d: struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def main(src, out_dir):
    w, h, px = decode_bc1(open(src, "rb").read())
    white = [min(p[:3]) >= WHITE for p in px]
    dark_lab, dark_sizes = label(w, h, [not v for v in white])
    blob = max(range(1, len(dark_sizes)), key=lambda n: dark_sizes[n])
    white_lab, white_sizes = label(w, h, white)
    border, near_blob = set(), set()
    for i, n in enumerate(white_lab):
        if not n:
            continue
        x, y = i % w, i // w
        if x in (0, w - 1) or y in (0, h - 1):
            border.add(n)
        for j, ok in ((i - 1, x > 0), (i + 1, x < w - 1), (i - w, y > 0), (i + w, y < h - 1)):
            if ok and dark_lab[j] == blob:
                near_blob.add(n)
    kept = {n for n in range(1, len(white_sizes)) if n not in border and n in near_blob}
    background = [bool(n) and n not in kept for n in white_lab]
    outside = set()   # the non-blob components touching the background (the red letters), not the counters of the kept letters
    for i, n in enumerate(dark_lab):
        if n and n != blob and n not in outside:
            x, y = i % w, i // w
            if any(ok and background[j] for j, ok in ((i - 1, x > 0), (i + 1, x < w - 1), (i - w, y > 0), (i + w, y < h - 1))):
                outside.add(n)
    near = dilate(w, h, background, BAND)
    band = [background[i] or dark_lab[i] in outside or (dark_lab[i] == blob and near[i]) for i in range(w * h)]
    out = [colour_to_alpha(px[i]) if band[i] else px[i] for i in range(w * h)]
    print(f"{w}x{h}: {len(dark_sizes) - 1} non-white components (blob {dark_sizes[blob]} px), {len(white_sizes) - 1} white "
          f"components: {len(kept)} kept ({sum(white_sizes[n] for n in kept)} px), background {sum(background)} px, "
          f"colour-to-alpha band {sum(band) - sum(background)} px; {len(outside)} non-blob components outside, "
          f"{len(dark_sizes) - 2 - len(outside)} enclosed (the counters of the kept letters, untouched)")
    for n in sorted(range(1, len(white_sizes)), key=lambda n: -white_sizes[n])[:40]:
        xs = [i % w for i, v in enumerate(white_lab) if v == n]
        ys = [i // w for i, v in enumerate(white_lab) if v == n]
        print(f"  white {n}: {white_sizes[n]} px x {min(xs)}-{max(xs)} y {min(ys)}-{max(ys)} border {n in border} "
              f"blob {n in near_blob} -> {'kept' if n in kept else 'background'}")
    open(f"{out_dir}/bne_dark_straight.rgba", "wb").write(bytes(c for p in out for c in p))
    png(f"{out_dir}/bne_dark_on_black.png", w, h,
        [[c for p in out[y * w:(y + 1) * w] for c in (p[0] * p[3] // 255, p[1] * p[3] // 255, p[2] * p[3] // 255)] for y in range(h)])
    png(f"{out_dir}/bne_dark_mask.png", w, h,
        [[c for i in range(y * w, (y + 1) * w) for c in ((0,) * 3 if background[i] else (128,) * 3 if band[i] else (255,) * 3)]
         for y in range(h)])


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
