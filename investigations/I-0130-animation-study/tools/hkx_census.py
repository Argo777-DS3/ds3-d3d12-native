"""I-0130: decode every animation of a folder with hkx_read and report what the curves are made of.

Usage: python -I hkx_census.py <folder> [<folder> ...]
Prints counts only (frames, tracks, static / spline shares, the rotation quantisation, the decode checks).
"""
import collections
import glob
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hkx_read as H  # noqa: E402


def main(folders):
    files = []
    for f in folders:
        files += [p for p in glob.glob(os.path.join(f, '**', '*.hkx'), recursive=True)
                  if not os.path.basename(p).lower().startswith('skeleton')]
    agg = collections.Counter(); fails = []; frames = []; ntt = collections.Counter(); bpf = []
    worst = 0.0; edge = 0.0; multi = 0; rm = 0; degrees = collections.Counter(); samples = 0
    for f in files:
        try:
            pf = H.Packfile(open(f, 'rb').read())
            names = tuple(sorted(set(n for _, _, n in pf.objects)))
            agg[('classes', names)] += 1
            for sec, off, name in pf.objects:
                if name == 'hkaDefaultAnimatedReferenceFrame':
                    rm += 1
                if name != 'hkaSplineCompressedAnimation':
                    continue
                hdr, blocks, st = H.read_spline_anim(pf, sec, off)
                frames.append(hdr['frames']); ntt[hdr['transform_tracks']] += 1
                bpf.append(hdr['data_bytes'] / max(1, hdr['frames']))
                multi += hdr['blocks'] > 1
                for k, v in st.items():
                    for kk, vv in v.items():
                        agg[(k, kk)] += vv
                m = hdr['max_frames_per_block']
                for b in blocks:
                    for pos, rot, scl in b:
                        for tr in (pos, rot, scl):
                            if tr[0] == 'spline':
                                degrees[tr[1]] += 1
                for t in range(hdr['transform_tracks']):
                    for fr in range(hdr['frames']):
                        _, q, _ = H.sample(hdr, blocks, t, fr); samples += 1
                        worst = max(worst, abs(math.sqrt(sum(c * c for c in q)) - 1))
                    for b in range(len(blocks) - 1):      # the same frame read from the end of a block and the start of the next
                        rot = blocks[b][t][1]
                        if rot[0] == 'spline':
                            qa = rot[3](float(m - 1)); _, qb, _ = H.sample(hdr, blocks, t, (b + 1) * (m - 1))
                            d = min(sum((a - c) ** 2 for a, c in zip(qa, qb)), sum((a + c) ** 2 for a, c in zip(qa, qb))) ** .5
                            edge = max(edge, d)
        except Exception as e:  # noqa: BLE001  (a census: report, do not stop)
            fails.append((os.path.basename(f), repr(e)[:100]))
    frames.sort(); bpf.sort()
    print('files', len(files), 'decoded', len(frames), 'failed', len(fails), fails[:5])
    print('frames min / median / max', frames[0], frames[len(frames) // 2], frames[-1], '; multi-block', multi,
          '; root motion (hkaDefaultAnimatedReferenceFrame)', rm)
    print('transform tracks', ntt.most_common(6), '; spline degrees', dict(degrees))
    print('bytes a frame, median', round(bpf[len(bpf) // 2]), '; samples', samples,
          '; worst |quaternion norm - 1|', round(worst, 4), '; worst block-edge jump', round(edge, 4))
    for k, v in sorted(agg.items(), key=lambda x: str(x)):
        print(' ', k, v)


if __name__ == '__main__':
    main(sys.argv[1:])
