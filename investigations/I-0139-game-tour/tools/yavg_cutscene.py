"""I-0139 tranche 3: the mean luma (ffmpeg signalstats YAVG) of each cutscene shot, native against original: the OLED's 2560x1440
window's client (from 8,31), or on the ASUS (a whole 1920x1080 screen capture) the 1280x720 client at 8,31, the desktop left out
(the final review's L4); another capture size is refused, an ffmpeg failure reported (ERR, exit 1). Usage: python -I yavg_cutscene.py <nat-prefix> <orig-prefix> <label> [<label> ...].
JPEG screen captures: a level, not fine colour; the two runs' shots are the same seconds after the hand-off, not the same frame."""
import glob, os, re, subprocess, sys
ff = glob.glob(os.path.expandvars(r'%LOCALAPPDATA%\Microsoft\WinGet\Packages\Gyan.FFmpeg_*\ffmpeg-*\bin\ffmpeg.exe'))[0]
repo = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
shots = [2, 5, 10, 15, 20, 25, 30, 40, 50, 60, 75, 90]
errors = 0
def size(p):
    r = subprocess.run([ff, '-hide_banner', '-c:v', 'mjpeg', '-i', p], capture_output=True, text=True, creationflags=0x08000000)
    m = re.search(r', (\d{3,5})x(\d{3,5})', r.stderr); return (int(m.group(1)), int(m.group(2))) if m else None
def yavg(p):
    """The capture's client: the ASUS's whole 1920x1080 screen, the 1280x720 client at 8,31; the OLED's 2560x1440 capture, the
    window client from 8,31 (the frame's left columns and the title bar left out); any other size refused (the final review's M4:
    another window size needs its own client rectangle)."""
    global errors
    if not os.path.exists(p): return None
    wh = size(p)
    crops = {(1920, 1080): 'crop=1280:720:8:31', (2560, 1440): 'crop=iw-8:ih-31:8:31'}
    if wh not in crops:
        print(f'{p}: capture size {wh} not known', file=sys.stderr); errors += 1; return 'ERR'
    r = subprocess.run([ff, '-hide_banner', '-c:v', 'mjpeg', '-i', p, '-vf', crops[wh] + ',signalstats,metadata=print:key=lavfi.signalstats.YAVG', '-f', 'null', '-'],
                       capture_output=True, text=True, creationflags=0x08000000)
    m = re.search(r'YAVG=([0-9.]+)', r.stderr + r.stdout)
    if r.returncode or not m:
        print(f'{p}: ffmpeg exit {r.returncode}', file=sys.stderr); errors += 1; return 'ERR'
    return float(m.group(1))
nat, orig = sys.argv[1], sys.argv[2]
print('label\t' + '\t'.join(f's{s}' for s in shots) + '\tmedian_ratio')
for label in sys.argv[3:]:
    cells = []; ratios = []
    for s in shots:
        a = yavg(os.path.join(repo, 'evidence', 'I-0066', 'artifacts', f'{nat}-{label}-nat', f'cutscene-{s}.png'))
        b = yavg(os.path.join(repo, 'evidence', 'I-0066', 'artifacts', f'{orig}-{label}-orig', f'cutscene-{s}.png'))
        if a is None or b is None: cells.append('-'); continue
        if a == 'ERR' or b == 'ERR': cells.append('ERR'); continue
        cells.append(f'{a:.0f}/{b:.0f}')
        if b > 1: ratios.append(a / b)
    ratios.sort(); med = ratios[len(ratios) // 2] if ratios else None
    print(label + '\t' + '\t'.join(cells) + '\t' + (f'{med:.2f}' if med else '-'))
sys.exit(1 if errors else 0)
