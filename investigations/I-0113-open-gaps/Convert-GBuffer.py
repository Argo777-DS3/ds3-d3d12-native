# I-0113 (the Handmaid chair): the native G-buffer witness files (world-pictures.flag and gbuffer-pictures.flag: world-N.rgba,
# -hdr.rgba16f, -gb0.rgba16f,
# -gb1/-gb2/-gb3.rgba8, -gb4.r11g11b10) of one picture to PNGs, and the mean of each channel inside a rectangle.
# python -I Convert-GBuffer.py <folder> <serial> <width> <height> [x0 y0 x1 y1] (the presented picture's extent; the rectangle in
# each file's own pixels: in a DLSS scaled mode the G-buffer's rectangle is not the presented picture's)
import os, struct, sys, zlib

def half(h):
    s = (h >> 15) & 1; e = (h >> 10) & 31; m = h & 1023
    if e == 0: v = m / 1024 * 2 ** -14
    elif e == 31: v = float('inf') if m == 0 else float('nan')
    else: v = (1 + m / 1024) * 2 ** (e - 15)
    return -v if s else v

def small(bits, mbits):   # unsigned float with a 5-bit exponent (R11G11B10)
    e = bits >> mbits; m = bits & ((1 << mbits) - 1)
    if e == 0: return m / (1 << mbits) * 2 ** -14
    if e == 31: return float('inf')
    return (1 + m / (1 << mbits)) * 2 ** (e - 15)

def read(path, w, h, kind):
    # Each file's own extent from its size (final review L7: with the DLSS scaled modes the G-buffer and the lit HDR are at the
    # render extent, the presented picture at the output extent; the aspect ratio is the same).
    b = open(path, 'rb').read(); px = []
    pixels = len(b) // (8 if kind == 'rgba16f' else 4); aspect = w / h
    for h in sorted(range(1, int((pixels / aspect) ** 0.5) + 3), key=lambda k: abs(k - (pixels / aspect) ** 0.5)):
        if pixels % h == 0 and abs(pixels // h / h - aspect) < 0.01: w = pixels // h; break
    else: raise ValueError(path + ': no extent of the picture aspect ratio')
    for y in range(h):
        row = []
        for x in range(w):
            if kind == 'rgba8': o = (y * w + x) * 4; row.append(tuple(c / 255 for c in b[o:o + 4]))
            elif kind == 'rgba16f': o = (y * w + x) * 8; row.append(tuple(half(v) for v in struct.unpack_from('<4H', b, o)))
            else:
                o = (y * w + x) * 4; v = struct.unpack_from('<I', b, o)[0]
                row.append((small(v & 2047, 6), small((v >> 11) & 2047, 6), small(v >> 22, 5), 1.0))
        px.append(row)
    return px

def png(path, px, f):
    h = len(px); w = len(px[0])
    raw = b''.join(b'\0' + bytes(max(0, min(255, int(f(c) * 255 + 0.5))) for p in row for c in p[:3]) for row in px)
    chunk = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))

folder, serial, w, h = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
rect = [int(v) for v in sys.argv[5:9]] if len(sys.argv) >= 9 else None
for suffix, kind, f in (('.rgba', 'rgba8', lambda c: c), ('-hdr.rgba16f', 'rgba16f', lambda c: c / (1 + c) if c == c else 1),
                        ('-gb0.rgba16f', 'rgba16f', lambda c: c * 0.5 + 0.5 if c == c else 1), ('-gb1.rgba8', 'rgba8', lambda c: c),
                        ('-gb2.rgba8', 'rgba8', lambda c: c), ('-gb3.rgba8', 'rgba8', lambda c: c),
                        ('-gb4.r11g11b10', 'r11', lambda c: c / (1 + c) if c == c else 1)):
    path = os.path.join(folder, 'world-' + serial + suffix)
    if not os.path.exists(path): print(suffix, 'absent'); continue
    px = read(path, w, h, kind)
    png(path + '.png', px, f)
    if rect:
        x0, y0, x1, y1 = rect; cells = [px[y][x] for y in range(y0, y1) for x in range(x0, x1)]
        mean = [sum(c[i] for c in cells) / len(cells) for i in range(4)]
        print(suffix, 'mean', ['%.4g' % m for m in mean])
