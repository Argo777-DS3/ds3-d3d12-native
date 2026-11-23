"""I-0139: the mean luma (ffmpeg signalstats YAVG) of each tour picture's game client (1280x720 at 8,31), native against original.
Usage: python -I yavg.py <prefix> <rows-file> [n-rows]. JPEG screen captures: a level, not fine colour."""
import glob, os, re, subprocess, sys
ff = glob.glob(os.path.expandvars(r'%LOCALAPPDATA%\Microsoft\WinGet\Packages\Gyan.FFmpeg_*\ffmpeg-*\bin\ffmpeg.exe'))[0]
repo = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
names = ['arrive', 'sweep-1-1', 'sweep-2-1', 'sweep-3-1', 'sweep-4-1']
def yavg(p):
    r = subprocess.run([ff, '-hide_banner', '-c:v', 'mjpeg', '-i', p, '-vf', 'crop=1280:720:8:31,signalstats,metadata=print:key=lavfi.signalstats.YAVG', '-f', 'null', '-'],
                       capture_output=True, text=True, creationflags=0x08000000)
    m = re.search(r'YAVG=([0-9.]+)', r.stderr + r.stdout); return float(m.group(1)) if m else None
rows = [l.split('=')[0] for l in open(sys.argv[2], encoding='utf-8') if l.strip()]
if len(sys.argv) > 3: rows = rows[:int(sys.argv[3])]
print('row\t' + '\t'.join(names) + '\tmean_ratio')
for row in rows:
    out = []; ratios = []
    for n in names:
        a = os.path.join(repo, 'evidence/I-0066/artifacts', f'{sys.argv[1]}-{row}-nat', n + '.png')
        b = os.path.join(repo, 'evidence/I-0066/artifacts', f'{sys.argv[1]}-{row}-orig', n + '.png')
        ya = yavg(a) if os.path.exists(a) else None; yb = yavg(b) if os.path.exists(b) else None
        out.append(f'{ya:.0f}/{yb:.0f}' if ya is not None and yb is not None else 'n/a')
        if ya and yb: ratios.append(ya / yb)
    print(row + '\t' + '\t'.join(out) + '\t' + (f'{sum(ratios)/len(ratios):.2f}' if ratios else 'n/a'))
