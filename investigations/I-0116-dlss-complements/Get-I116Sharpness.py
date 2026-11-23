# I-0116 tranche 2 (2026-11-01): the presented pictures' high-frequency energy of two runs on one path (`world-N.rgba`,
# world-pictures.flag, RGBA8 at the presentation extent, the render size at DLAA), to tell whether DLSS read a reactive mask forced to 1 (reactive-full.flag):
# a model that reads it takes the current, jittered colour everywhere, so the pictures' pixel-to-pixel differences grow.
# Usage: python -I Get-I116Sharpness.py <run folder A> <run folder B> <width> <height>
import os
import sys


def energy(path, width, height):
    data = open(path, 'rb').read()
    if len(data) != width * height * 4:
        raise SystemExit(f'{path}: {len(data)} bytes, not {width}x{height} RGBA8')
    luma = [0.0] * (width * height)
    for i in range(width * height):
        r, g, b = data[4 * i], data[4 * i + 1], data[4 * i + 2]
        luma[i] = 0.2126 * r + 0.7152 * g + 0.0722 * b
    total = 0.0
    count = 0
    for y in range(1, height - 1):
        row = y * width
        for x in range(1, width - 1):
            c = luma[row + x]
            lap = 4 * c - luma[row + x - 1] - luma[row + x + 1] - luma[row + x - width] - luma[row + x + width]
            total += abs(lap)
            count += 1
    return total / count


a, b, width, height = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
names = sorted(n for n in os.listdir(a) if n.startswith('world-') and n.endswith('.rgba') and n[6:-5].isdigit())
for name in names:
    if not os.path.exists(os.path.join(b, name)):
        continue
    ea, eb = energy(os.path.join(a, name), width, height), energy(os.path.join(b, name), width, height)
    print(f'{name}\t{ea:.3f}\t{eb:.3f}\t{eb / ea:.3f}')
