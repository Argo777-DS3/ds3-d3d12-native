# I-0140 tranche 15 (the buffs and statuses on a small weapon): Make-I140SpellStrip's film strip cropped around the character
# (the game's client 1280x720 at (8,31), then the 9th argument's crop w:h:x:y inside it, scaled to half), native above the original.
# python -I Make-I140CropStrip.py <audit.log> <artifacts dir> <prefix> <family> <k> <out.png> [step_ms span_ms crop]
import os, re, subprocess, sys, tempfile
HIDDEN = 0x08000000  # CREATE_NO_WINDOW (D-0199)
log, art, prefix, family, k, out = sys.argv[1:7]; k = int(k)
step = int(sys.argv[7]) if len(sys.argv) > 7 else 400; span = int(sys.argv[8]) if len(sys.argv) > 8 else 4000
crop = sys.argv[9] if len(sys.argv) > 9 else '440:440:420:200'
w, h = (int(x) for x in crop.split(':')[:2])
runs, cur = {}, None
for line in open(log, encoding='utf-8', errors='replace'):
    m = re.match(r'^\d\d:\d\d:\d\d (\S+) \(\d+ spells\)', line)
    if m: cur = m.group(1); runs[cur] = {}; continue
    m = re.match(r'^cast (\d+) \S+ video_ms=(\d+)', line)
    if m and cur: runs[cur][int(m.group(1))] = int(m.group(2))
if not out.endswith('.png'): sys.exit('out must end in .png')
rows = []; tmp = tempfile.mkdtemp(prefix='i140-crop-')
try:
    for mode in ('nat', 'orig'):
        run = f'{prefix}-{family}-{mode}'; start = runs[run][k]
        n = span // step
        vf = f"crop=1280:720:8:31,crop={crop},fps={1000/step},scale={w//2}:{h//2},tile={n}x1"
        path = os.path.join(tmp, f'{mode}.png'); rows.append(path)   # its step review's L7: never beside the output
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-ss', f'{start/1000:.3f}', '-i', f'{art}/{run}/casts.mp4', '-t', f'{span/1000:.3f}', '-vf', vf, '-frames:v', '1', path], check=True, creationflags=HIDDEN)
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', rows[0], '-i', rows[1], '-filter_complex', 'vstack', out], check=True, creationflags=HIDDEN)
finally:
    for path in rows:
        if os.path.exists(path): os.remove(path)
    os.rmdir(tmp)
print(out)
