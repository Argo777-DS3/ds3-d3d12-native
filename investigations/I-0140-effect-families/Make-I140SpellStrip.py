# I-0140 (the spells' audit): a film strip of one cast in both renderers, from Run-I140Spells' log (the « cast k ... video_ms=N »
# lines of each run) and each run's casts.mp4: frames every -step ms from N to N+span, the game's client cropped, native above the
# original. The shots at fixed ms after the click miss effects whose cast lands 0.5 to 1 s later; the strip shows the whole cast.
# python -I Make-I140SpellStrip.py <audit.log> <artifacts dir> <prefix> <family> <k> <out.png> [step_ms span_ms]
import os, re, subprocess, sys
HIDDEN = 0x08000000  # CREATE_NO_WINDOW (D-0199)
log, art, prefix, family, k, out = sys.argv[1:7]; k = int(k)
step = int(sys.argv[7]) if len(sys.argv) > 7 else 300; span = int(sys.argv[8]) if len(sys.argv) > 8 else 3600
runs, cur = {}, None
for line in open(log, encoding='utf-8', errors='replace'):
    m = re.match(r'^\d\d:\d\d:\d\d (\S+) \(\d+ spells\)', line)
    if m: cur = m.group(1); runs[cur] = {}; continue
    m = re.match(r'^cast (\d+) \S+ video_ms=(\d+)', line)
    if m and cur: runs[cur][int(m.group(1))] = int(m.group(2))
if not out.endswith('.png'): sys.exit('out must end in .png')
rows = []
try:
    for mode in ('nat', 'orig'):
        run = f'{prefix}-{family}-{mode}'; start = runs[run][k]
        n = span // step
        vf = f"crop=1280:720:8:31,fps={1000/step},scale=256:144,tile={n}x1"
        path = out[:-4] + f'-{mode}.png'; rows.append(path)
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-ss', f'{start/1000:.3f}', '-i', f'{art}/{run}/casts.mp4', '-t', f'{span/1000:.3f}', '-vf', vf, '-frames:v', '1', path], check=True, creationflags=HIDDEN)
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', rows[0], '-i', rows[1], '-filter_complex', 'vstack', out], check=True, creationflags=HIDDEN)
finally:
    for path in rows:   # the two rows' intermediates, also on a failure (its review's L4)
        if os.path.exists(path): os.remove(path)
print(out)
