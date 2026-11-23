# I-0140 tranche 16 (the weapon arts, 118 casts a renderer: a number to choose which strips to read first): for each cast of a
# family in both runs of Run-I140Spells' log, the mean luma (ffmpeg signalstats YAVG) of a crop around the character, 10 frames a
# second over -span ms from the click; prints one TSV row a cast: k, word, the native and original curves' peak, their mean, and the
# peak's time. A row whose two peaks differ by more than 10 % is marked « differs » (a lead to read, not a judgement).
# python -I Get-I140CastLuma.py <audit.log> <artifacts dir> <prefix> <family> <words file or -> [span_ms crop]
import re, subprocess, sys
HIDDEN = 0x08000000  # CREATE_NO_WINDOW (D-0199)
log, art, prefix, family, words = sys.argv[1:6]
span = int(sys.argv[6]) if len(sys.argv) > 6 else 4000
crop = sys.argv[7] if len(sys.argv) > 7 else '640:480:320:150'
runs, cur = {}, None
for line in open(log, encoding='utf-8', errors='replace'):
    m = re.match(r'^\d\d:\d\d:\d\d (\S+) \(\d+ spells\)', line)
    if m: cur = m.group(1); runs[cur] = {}; continue
    m = re.match(r'^cast (\d+) \S+ video_ms=(\d+)', line)
    if m and cur: runs[cur][int(m.group(1))] = int(m.group(2))
names = [w.strip() for w in open(words, encoding='utf-8')] if words != '-' else []
def curve(run, k):
    start = runs[run][k]
    out = subprocess.run(['ffmpeg', '-ss', f'{start/1000:.3f}', '-i', f'{art}/{run}/casts.mp4', '-t', f'{span/1000:.3f}', '-vf',
                          f'fps=10,crop=1280:720:8:31,crop={crop},signalstats,metadata=print:key=lavfi.signalstats.YAVG', '-f', 'null', '-'],
                         capture_output=True, text=True, creationflags=HIDDEN).stderr
    return [float(v) for v in re.findall(r'YAVG=([0-9.]+)', out)]
nat, orig = f'{prefix}-{family}-nat', f'{prefix}-{family}-orig'
print('k\tword\tnat_peak\torig_peak\tnat_mean\torig_mean\tnat_peak_s\torig_peak_s\tmark')
for k in sorted(set(runs.get(nat, {})) & set(runs.get(orig, {}))):
    a, b = curve(nat, k), curve(orig, k)
    if not a or not b: print(f'{k}\t\tno frames'); continue
    pa, pb = max(a), max(b)
    mark = 'differs' if abs(pa - pb) > 0.10 * max(pb, 1) else ''
    w = names[k - 1] if k - 1 < len(names) else ''
    print(f'{k}\t{w}\t{pa:.1f}\t{pb:.1f}\t{sum(a)/len(a):.1f}\t{sum(b)/len(b):.1f}\t{a.index(pa)/10:.1f}\t{b.index(pb)/10:.1f}\t{mark}')
