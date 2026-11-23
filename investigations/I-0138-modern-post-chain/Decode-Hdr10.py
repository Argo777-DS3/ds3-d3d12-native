"""I-0138 tranche 5: read an HDR10 back buffer written by the module (`<stem>.r10g10b10a2`: R10G10B10A2_UNORM texels, little-endian
32-bit words, ST 2084 codes over Rec. 2020, rows top to bottom) and, when the run also wrote the 8-bit picture the output pass
read (`<stem>-display.rgba`, world-pictures.flag), check the pass: every texel of that picture taken forward as the pass does (the
power 2.2, Rec. 709 to Rec. 2020, the paper white, ST 2084) against the back buffer's 10-bit codes.

    python -I Decode-Hdr10.py <picture.r10g10b10a2> <width> <height> --paper-white 80 [--peak 180] [--sdr <picture-display.rgba>] [--png <out.png>]
    python -I Decode-Hdr10.py <picture.r10g10b10a2> <width> <height> --paper-white 200 --peak 1000 --scene <picture-scene.rgba16f> --ui <picture-ui.rgba>

The second form is a float frame's (step 2): the float picture (R16G16B16A16_FLOAT, display-encoded) the output pass read, taken
forward as it does (the power 2.2, the roll-off; the game's brightness option at its identity). --ui is for the runs made while
the UI was drawn on a layer (i138-t5b, i138-t5c: premultiplied colour, coverage in alpha; scene x (1 - coverage) + UI first);
since the UI is drawn in place the float picture holds it and --ui is left out. Then the power 2.2, the roll-off
of the largest channel to the peak, Rec. 2020, the paper white, the clamp, ST 2084): every code must be within one and a half of
the model's (the pass dithers by one code) and the mean difference under a tenth of a code. It also prints how much of the picture
is above the paper white.

Prints the codes' range, the nits reached, and with --sdr the count of codes that differ from the model's by more than one (exit 1
when any does: half a code of rounding and the card's pow). Beside it, for information, the picture decoded back to 8-bit SDR
(ST 2084 to nits, the paper white, Rec. 2020 to Rec. 709, the power 1/2.2) against the 8-bit picture: not a criterion, since the
weakest channel of a saturated colour is not held to an 8-bit level by 10-bit codes over a wider gamut (the first witness,
i138-t5a-f80-m40-taa: 99 % of the values equal, 1 % a level off, 187 of 2.76 million two to seven levels off, every code within
one of the model's). --png writes the decoded SDR view. Standard library only (a picture's distinct colours are decoded once each).
"""
import argparse, math, struct, sys, zlib

M1 = 2610.0 / 16384.0
M2 = 2523.0 / 4096.0 * 128.0
C1 = 3424.0 / 4096.0
C2 = 2413.0 / 4096.0 * 32.0
C3 = 2392.0 / 4096.0 * 32.0
# The inverse of ITU-R BT.2087's Rec. 709 to Rec. 2020 matrix (hdr_output.h holds the forward one).
FORWARD = ((0.6274, 0.3293, 0.0433), (0.0691, 0.9195, 0.0114), (0.0164, 0.0880, 0.8956))


def invert(m):
    (a, b, c), (d, e, f), (g, h, i) = m
    det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g)
    return (((e * i - f * h) / det, (c * h - b * i) / det, (b * f - c * e) / det),
            ((f * g - d * i) / det, (a * i - c * g) / det, (c * d - a * f) / det),
            ((d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det))


BACK = invert(FORWARD)


def pq_encode(nits):
    y = max(min(nits / 10000.0, 1.0), 0.0) ** M1
    return ((C1 + C2 * y) / (1.0 + C3 * y)) ** M2


def pq_decode(signal):
    e = max(min(signal, 1.0), 0.0) ** (1.0 / M2)
    n = max(e - C1, 0.0)
    return 10000.0 * (n / (C2 - C3 * e)) ** (1.0 / M1)


def write_png(path, width, height, rgb):
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw += rgb[y * width * 3:(y + 1) * width * 3]

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xFFFFFFFF)
    with open(path, 'wb') as out:
        out.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) +
                  chunk(b'IDAT', zlib.compress(bytes(raw), 6)) + chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('picture')
    parser.add_argument('width', type=int)
    parser.add_argument('height', type=int)
    parser.add_argument('--paper-white', type=float, required=True)
    parser.add_argument('--peak', type=float, default=10000.0, help='the peak the pass clamps to (nits)')
    parser.add_argument('--sdr')
    parser.add_argument('--scene')
    parser.add_argument('--ui')
    parser.add_argument('--png')
    args = parser.parse_args()
    data = open(args.picture, 'rb').read()
    count = args.width * args.height
    if len(data) != count * 4:
        sys.exit(f'{args.picture}: {len(data)} bytes, {count * 4} expected for {args.width}x{args.height}')
    words = struct.unpack(f'<{count}I', data)
    nits_of = [pq_decode(code / 1023.0) for code in range(1024)]
    lo = [1023] * 3
    hi = [0] * 3
    alpha = set()
    sdr = bytearray(count * 3)
    paper = args.paper_white
    cache = {}
    for index, word in enumerate(words):
        key = word & 0x3FFFFFFF
        value = cache.get(key)
        if value is None:
            codes = (word & 1023, (word >> 10) & 1023, (word >> 20) & 1023)
            linear2020 = [nits_of[c] / paper for c in codes]
            value = bytes(min(255, max(0, int(round(255.0 * max(sum(BACK[r][k] * linear2020[k] for k in range(3)), 0.0) ** (1.0 / 2.2)))))
                          for r in range(3))
            cache[key] = value
        sdr[index * 3:index * 3 + 3] = value
        alpha.add(word >> 30)
    for word in cache:
        for c in range(3):
            code = (word >> (10 * c)) & 1023
            lo[c] = min(lo[c], code)
            hi[c] = max(hi[c], code)
    print(f'{args.picture}: {args.width}x{args.height}, {len(cache)} distinct colours, alpha codes {sorted(alpha)}')
    print(f'  codes R {lo[0]}..{hi[0]}  G {lo[1]}..{hi[1]}  B {lo[2]}..{hi[2]}; brightest channel {nits_of[max(hi)]:.1f} nits (paper white {paper:.0f})')
    if args.png:
        write_png(args.png, args.width, args.height, sdr)
        print(f'  SDR view written: {args.png}')
    failed = False
    if args.sdr:
        reference = open(args.sdr, 'rb').read()
        if len(reference) != count * 4:
            sys.exit(f'{args.sdr}: {len(reference)} bytes, {count * 4} expected')
        histogram = {}
        codes = {}
        linear_of = [(v / 255.0) ** 2.2 for v in range(256)]
        forward = {}
        worst = 0.0
        for index in range(count):
            for c in range(3):
                d = abs(sdr[index * 3 + c] - reference[index * 4 + c])
                histogram[d] = histogram.get(d, 0) + 1
            key = reference[index * 4:index * 4 + 3]
            expected = forward.get(key)
            if expected is None:
                linear = [linear_of[key[k]] for k in range(3)]
                expected = tuple(1023.0 * pq_encode(min(sum(FORWARD[r][k] * linear[k] for k in range(3)) * paper, args.peak)) for r in range(3))
                forward[key] = expected
            word = words[index]
            for c in range(3):
                d = abs(((word >> (10 * c)) & 1023) - expected[c])
                worst = max(worst, d)
                bucket = 0 if d <= 0.5 else 1 if d <= 1.0 else 2
                codes[bucket] = codes.get(bucket, 0) + 1
        total = count * 3
        over = codes.get(2, 0)
        print(f'  against {args.sdr}, the model of the pass: {codes.get(0, 0)} codes within half a code, {codes.get(1, 0)} within one, {over} beyond; worst {worst:.3f}')
        print(f'  {"FAIL" if over else "PASS"}: {over} of {total} codes differ from the model by more than one')
        print('  decoded back to 8 bits (information): ' + ', '.join(f'{d} level{"s" if d != 1 else ""}: {n} ({100.0 * n / total:.3f} %)' for d, n in sorted(histogram.items())))
        failed = over > 0
    if args.scene:
        scene = open(args.scene, 'rb').read()
        layer = open(args.ui, 'rb').read() if args.ui else bytes(count * 4)
        if len(scene) != count * 8 or len(layer) != count * 4:
            sys.exit(f'{args.scene} or {args.ui}: sizes {len(scene)} and {len(layer)}, {count * 8} and {count * 4} expected')
        halves = struct.unpack(f'<{count * 4}e', scene)
        top = args.peak / paper
        start = max(1.0, 0.5 * top)
        worst = 0.0
        total_difference = 0.0
        beyond = 0
        above = 0
        brightest = 0.0
        memo = {}
        for index in range(count):
            key = (halves[index * 4], halves[index * 4 + 1], halves[index * 4 + 2], layer[index * 4:index * 4 + 4])
            expected = memo.get(key)
            if expected is None:
                a = key[3][3] / 255.0
                linear = [max(key[k] * (1.0 - a) + key[3][k] / 255.0, 0.0) ** 2.2 for k in range(3)]
                m = max(linear)
                if m > start and top > start:
                    scale = (start + (top - start) * (1.0 - math.exp(-(m - start) / (top - start)))) / m
                    linear = [v * scale for v in linear]
                expected = tuple(1023.0 * pq_encode(min(sum(FORWARD[r][k] * linear[k] for k in range(3)) * paper, args.peak)) for r in range(3)) + (max(linear),)
                memo[key] = expected
            word = words[index]
            if expected[3] > 1.0:
                above += 1
            brightest = max(brightest, expected[3])
            for c in range(3):
                d = ((word >> (10 * c)) & 1023) - expected[c]
                total_difference += d
                if abs(d) > worst:
                    worst = abs(d)
                if abs(d) > 1.5:
                    beyond += 1
        mean = total_difference / (count * 3)
        print(f'  against the float picture{" and the UI layer" if args.ui else ""}, the model of the pass: worst {worst:.3f} codes, mean {mean:+.4f}, {beyond} beyond one and a half')
        print(f'  {100.0 * above / count:.2f} % of the picture above the paper white, the brightest value {brightest:.2f} times it ({brightest * paper:.0f} nits before the clamp to {args.peak:.0f})')
        scene_failed = beyond > 0 or abs(mean) > 0.1
        print(f'  {"FAIL" if scene_failed else "PASS"}: the scene frame against the model')
        failed = failed or scene_failed
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
