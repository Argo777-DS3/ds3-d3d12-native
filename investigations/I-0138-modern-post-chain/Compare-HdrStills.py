"""I-0138 tranche 5: compare two screen captures of the game's window on a screen without Windows' HDR, one taken while the
chain was the SDR one and one while it was a forced HDR10 chain.

The compositor shows an HDR10 picture on an SDR screen by decoding ST 2084, counting the screen's white as 80 nits and encoding
by sRGB's piecewise curve, clipping above that white (INFERRED from the captures: i138-t5a at a paper white of 80 nits gave a
ratio of 0.92, the sRGB encode against the screen's power 2.2; i138-t5c and -t5d at 200 nits gave pictures a third brighter).
So a capture of the HDR10 chain is taken back to our display-encoded value before it is compared:
    value = (80 x sRGB_decode(pixel) / paper white) ^ (1 / 2.2), a clipped pixel (255) left out.
Without this step a capture at a paper white of 200 nits reads as a brighter picture (the first reading of i138-t5c-cycle's
menus, wrong: RESULT « Tranche 5 », « The UI in place »).

    python -I Compare-HdrStills.py <sdr.png> <hdr.png> [--paper-white 200] [--origin 8,31] [--size 1280x720]
        [--box name=x0,x1,y0,y1 ...]

The two captures are full-screen PNGs (the pilot's); --origin is the window's client corner on the screen. Needs ffmpeg on
PATH to read the PNGs. Prints, for each box (client coordinates), the two means and the mean absolute difference, in 8-bit levels.
"""
import argparse, subprocess, sys


def read(path, origin, size):
    width, height = size
    out = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-vf', f'crop={width}:{height}:{origin[0]}:{origin[1]}',
                          '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-'], capture_output=True)
    if out.returncode or len(out.stdout) != width * height * 3:
        sys.exit(f'{path}: ffmpeg gave {len(out.stdout)} bytes ({out.stderr.decode(errors="replace").strip()})')
    return out.stdout


def srgb_decode(byte):
    v = byte / 255.0
    return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('sdr')
    parser.add_argument('hdr')
    parser.add_argument('--paper-white', type=float, default=200.0)
    parser.add_argument('--origin', default='8,31')
    parser.add_argument('--size', default='1280x720')
    parser.add_argument('--box', action='append', default=[])
    args = parser.parse_args()
    origin = tuple(int(v) for v in args.origin.split(','))
    size = tuple(int(v) for v in args.size.split('x'))
    width = size[0]
    boxes = {}
    for item in args.box or ['whole=0,%d,0,%d' % size]:
        name, values = item.split('=')
        boxes[name] = tuple(int(v) for v in values.split(','))
    sdr = read(args.sdr, origin, size)
    hdr = read(args.hdr, origin, size)
    back = [min(255.0, 255.0 * (80.0 * srgb_decode(p) / args.paper_white) ** (1.0 / 2.2)) for p in range(256)]
    print(f'{args.sdr} (SDR chain) against {args.hdr} (HDR10 chain, paper white {args.paper_white:.0f} nits, taken back to the encoded value)')
    for name, (x0, x1, y0, y1) in boxes.items():
        count = clipped = 0
        sum_sdr = sum_hdr = sum_difference = 0.0
        for y in range(y0, y1):
            row = y * width
            for x in range(x0, x1):
                i = (row + x) * 3
                for c in range(3):
                    pixel = hdr[i + c]
                    if pixel >= 255:
                        clipped += 1
                        continue
                    value = back[pixel]
                    sum_sdr += sdr[i + c]
                    sum_hdr += value
                    sum_difference += abs(sdr[i + c] - value)
                    count += 1
        if not count:
            print(f'  {name}: every value clipped')
            continue
        print(f'  {name}: SDR mean {sum_sdr / count:.2f}, HDR10 mean {sum_hdr / count:.2f}, mean absolute difference {sum_difference / count:.2f} '
              f'({count} values, {clipped} clipped left out)')


if __name__ == '__main__':
    main()
