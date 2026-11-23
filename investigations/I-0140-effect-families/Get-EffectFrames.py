"""I-0140 tranche 24 (D-0299: original and native compared at the same moment of the game).

Finds, in a run's film, the frames at an effect's own start plus chosen offsets: the module's ffx_create rows (24ED90, both modes:
ffx-record.flag natively, ffx-draw-observe.flag in the original) carry the QPC in ms; Run-AlphaCast's <film>.clock.json holds the QPC
and UTC read together and <film>.ffmpeg.txt gdigrab's first frame in seconds since 1970 (its « start: »). So a row's moment in the
film is unix + (ms - qpc_ms) / 1000 - start, to about one frame (gdigrab's clock and the frame grid).

Limits (its step review's L5): the creation is the CPU's moment; its picture reaches the screen some frames later (render, present,
the compositor, gdigrab's copy), not the same in the two renderers: measured once (tranche 24, one 1296x758 window at 60 fps, one
build; `evidence/I-0140/t24/lag-check.tsv`, each frame's own time), the HP bar's first frame after each of seven hits came 25 to
55 ms after the mapped creation natively and 10 to 28 ms in the original, 1 to 44 ms later natively (median 26), to one frame;
a repeated frame just before the cue in each film (original hit 3, native hit 5) sets the range's ends, about 15 to 35 ms
without them; each film's clock mapping (a constant a film, its error not measured) is in these values, and the bar is UI (the
effects' own picture not measured). The offsets are wall-clock ms after the creation, so where one mode runs slower the effect is younger at the same
offset: offsets well beyond a few frames, both modes' rates read. Native and original are paired by the creation's rank k among
that id's; --list gives each id's distinct caller chains with their counts, frames.tsv each creation's whole chain (the module's RVAs above the 24F0C4 wrapper, « 0 » a
frame outside the module), so a pair whose creations come from different code is seen and not compared (L4), within the chain's
depth: the module captures six frames, of which the native's own frames take some, so a native chain may stop above the frames
that tell two sources of the same path apart. The rows' `t` (the matrix's +0x30 row) is a direction on every row read, not a
place: not used.

  python -I Get-EffectFrames.py <run folder> --list                      the FXR ids created during the film, their chains, counts, first times
  python -I Get-EffectFrames.py <run folder> --fxr 12345 --at 0,200,400 --out <dir> [--film casts.mp4]

Writes <dir>/frames.tsv (fxr, k, create_ms, film_s, offset_ms, callers, file) and one PNG a frame, <fxr>-<k>-<offset>.png;
exits non-zero when no row or no frame (M2). Standard library; ffmpeg does the pixels.
"""
import argparse, glob, json, os, re, shutil, subprocess, sys

CAP = 6000   # the module's ffx_create rows a session (21_effects.inc, create_rows)


def ffmpeg_path():
    found = shutil.which('ffmpeg')
    if found:
        return found
    hits = glob.glob(os.path.expandvars(r'%LOCALAPPDATA%\Microsoft\WinGet\Packages\Gyan.FFmpeg_*\ffmpeg-*\bin\ffmpeg.exe'))
    if not hits:
        sys.exit('no ffmpeg')
    return hits[0]


def film_clock(run, film):
    stem = os.path.splitext(film)[0]
    with open(os.path.join(run, stem + '.clock.json'), encoding='utf-8-sig') as f:
        clock = json.load(f)
    with open(os.path.join(run, stem + '.ffmpeg.txt'), encoding='utf-8', errors='replace') as f:
        report = f.read()
    m = re.search(r'Input #0, gdigrab.*?start: ([0-9.]+)', report, re.S)
    if not m:
        sys.exit('no gdigrab start in the ffmpeg report')
    fps = re.search(r'Stream #0:0: Video: .*?, ([0-9.]+) fps', report)
    times = re.findall(r'time=(\d+):(\d+):([0-9.]+)', report)   # L1: the film's length, from the report's last progress line
    length = int(times[-1][0]) * 3600 + int(times[-1][1]) * 60 + float(times[-1][2]) if times else float('inf')
    return float(clock['qpc_ms']), float(clock['unix']), float(m.group(1)), float(fps.group(1)) if fps else 0.0, length


def creations(run):
    rows = []
    with open(os.path.join(run, 'startup.jsonl'), encoding='utf-8', errors='replace') as f:
        for line in f:
            if '"ffx_create"' not in line:
                continue
            try:
                row = json.loads(line)
            except ValueError:
                continue
            chain = list(row.get('callers') or [])
            if chain and chain[0] == '24F0C4':   # the hooked wrapper itself
                chain = chain[1:]
            rows.append((float(row['ms']), int(row['fxr']), '>'.join(chain) or '?'))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('run')
    ap.add_argument('--film', default='casts.mp4')
    ap.add_argument('--list', action='store_true')
    ap.add_argument('--fxr', default='')
    ap.add_argument('--at', default='0')
    ap.add_argument('--out', default='')
    a = ap.parse_args()
    if not a.list and not (a.fxr and a.out):
        sys.exit('--fxr and --out are needed (or --list)')
    offsets = [int(x) for x in a.at.split(',') if x]
    if any(off < 0 for off in offsets):
        sys.exit('offsets are ms after the creation, never negative')
    qpc_ms, unix, start, fps, length = film_clock(a.run, a.film)
    gap = start - unix   # ffmpeg's start-up: 65 and 51 ms in tranche 24's films; the mapping does not depend on it
    print(f'# gdigrab start {gap * 1000:.0f} ms after the clock pair', file=sys.stderr)
    if abs(gap) > 60:
        sys.exit(f'gdigrab start {start:.3f} lies {gap:.1f} s from the clock pair\'s UTC: not an epoch time')
    every = creations(a.run)
    if not every:
        sys.exit('no ffx_create row: ffx-record.flag (native) or ffx-draw-observe.flag (original) missing')
    rows = [(ms, fxr, unix + (ms - qpc_ms) / 1000.0 - start, chain) for ms, fxr, chain in every]
    before = sum(1 for r in rows if r[2] < 0)
    rows = [r for r in rows if 0 <= r[2] <= length]
    print(f'# {len(every)} ffx_create rows, {before} before the film, {len(rows)} inside it (film {length:.1f} s, {fps} fps)',
          file=sys.stderr)
    if len(every) >= CAP:
        print(f'# warning: {CAP} rows, the module\'s cap: later creations are not logged', file=sys.stderr)
    if a.list:
        seen = {}
        for ms, fxr, t, chain in rows:
            seen.setdefault(fxr, []).append((t, chain))
        for fxr, items in sorted(seen.items(), key=lambda x: x[1][0][0]):
            chains = {}
            for t, c in items:
                chains.setdefault(c, []).append(t)
            print(f'{fxr}\t{len(items)}\t' + '  '.join(f'{c} x{len(ts)} from {ts[0]:.2f} s' for c, ts in chains.items()))
        return
    want = {int(x) for x in a.fxr.split(',') if x}
    absent = want - {r[1] for r in rows}
    if absent:
        print(f'# no creation of {sorted(absent)} inside the film', file=sys.stderr)
    os.makedirs(a.out, exist_ok=True)
    ff = ffmpeg_path()
    rank, written, failed, late = {}, 0, 0, 0
    with open(os.path.join(a.out, 'frames.tsv'), 'w', encoding='utf-8', newline='\n') as tsv:
        tsv.write(f'# film {a.film}: gdigrab start {start:.6f}, clock qpc_ms {qpc_ms:.3f} at unix {unix:.6f}, {fps} fps, '
                  f'{length:.1f} s; offsets are wall-clock ms after the creation, its picture some frames later\n')
        tsv.write('fxr\tk\tcreate_ms\tfilm_s\toffset_ms\tcallers\tfile\n')
        for ms, fxr, t, chain in rows:
            if fxr not in want:
                continue
            k = rank[fxr] = rank.get(fxr, 0) + 1
            for off in offsets:
                name = f'{fxr}-{k}-{off}.png'
                at = t + off / 1000.0
                if at > length:
                    late += 1
                    continue
                target = os.path.join(a.out, name)
                if os.path.exists(target):
                    os.remove(target)
                subprocess.run([ff, '-v', 'error', '-y', '-ss', f'{at:.3f}', '-i', os.path.join(a.run, a.film), '-frames:v', '1',
                                target], check=False, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
                if os.path.exists(target):
                    written += 1
                    tsv.write(f'{fxr}\t{k}\t{ms:.3f}\t{at:.3f}\t{off}\t{chain}\t{name}\n')
                else:
                    failed += 1
                    print(f'# no frame for {name} at {at:.3f} s', file=sys.stderr)
    if failed or late:
        print(f'# {failed} frames not extracted, {late} past the film\'s end (earlier PNGs of other offsets in --out are kept; '
              f'frames.tsv lists this run\'s)', file=sys.stderr)
    if not written:
        sys.exit('no frame extracted')


if __name__ == '__main__':
    main()
