# I-0132 R11h (R11-PLAN section 9 step 1): the automatic exposure's target of an area from the original's pictures: the mean, over
# the pixels between the 10th and the 90th percentile, of the luma of the encoded picture (Rec. 709 weights on R'G'B' / 255). Not quite
# the pass's statistic (auto_exposure_math.h: the kept bins' mean displayed grey of the luminance, without the HUD, the glare, the
# grading and the dither): the luma of encoded colours is at most the encoding of their luminance, so this reads low (R11h's final
# review); compare the native's own shots measured the same way against the original's before changing the target. Each picture is decoded by ffmpeg to raw
# rgb24 (no Python image module on this PC); the area's target is the mean of its pictures'.
# Usage: python -I Measure-AutoExposureTarget.py LABEL picture.png [picture.png ...]   (one line per picture, then "LABEL mean")
import subprocess, sys

def luma_histogram(path):
    probe = subprocess.run(['ffprobe', '-v', 'error', '-select_streams', 'v:0', '-show_entries', 'stream=width,height', '-of', 'csv=p=0', path],
                           capture_output=True, text=True, check=True).stdout.strip().split(',')
    width, height = int(probe[0]), int(probe[1])
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-'], capture_output=True, check=True).stdout
    if len(raw) != 3 * width * height:
        raise ValueError(f'{path}: {len(raw)} bytes for {width}x{height}')
    # Luma in 1/10 of a code value (0..2550) so the trimming keeps fractional weights.
    weights = (2126, 7152, 722)
    histogram = [0] * 2551
    r, g, b = raw[0::3], raw[1::3], raw[2::3]
    for pr, pg, pb in zip(r, g, b):
        histogram[(weights[0] * pr + weights[1] * pg + weights[2] * pb + 500) // 1000] += 1
    return histogram, width * height

def trimmed_mean(histogram, total, low=0.10, high=0.90):
    lo, hi = low * total, high * total
    acc = kept = weighted = 0.0
    for value, count in enumerate(histogram):
        a0, a1 = acc, acc + count
        w = max(0.0, min(a1, hi) - max(a0, lo))
        kept += w
        weighted += w * value
        acc = a1
    return weighted / kept / 2550.0 if kept else 0.0

def main():
    if len(sys.argv) < 3:
        print('usage: LABEL picture.png ...')
        return 2
    label, pictures = sys.argv[1], sys.argv[2:]
    values = []
    for path in pictures:
        histogram, total = luma_histogram(path)
        mean = sum(v * c for v, c in enumerate(histogram)) / total / 2550.0
        trimmed = trimmed_mean(histogram, total)
        values.append(trimmed)
        print(f'{path}\tmean {mean:.4f}\ttrimmed {trimmed:.4f}')
    print(f'{label} {sum(values) / len(values):.4f}')
    return 0

if __name__ == '__main__':
    sys.exit(main())
