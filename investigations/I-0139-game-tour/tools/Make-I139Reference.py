# Make-I139Reference.py: builds investigations/I-0139-game-tour/REFERENCE.tsv, one row per tour run folder
# evidence/I-0066/artifacts/i139-*. Run from anywhere with: python -I Make-I139Reference.py   (stdlib only, read-only on evidence).
#
# Tranche mapping (INFERRED from the run prefix and the RESULT.md section names):
#   i139-v1*  = validation (the runner's first row, RESULT "The runner's validation", Run-I139Tour)
#   i139-b1*, i139-b2* = boss-runner trials (RESULT "first trial", "fog gate, phase and death", Run-I139Boss) -> "2-trial"
#   i139-v2*  = boss rows of verify-ui-overlay.txt (Run-I139Boss) -> "2-verify"
#   i139-t1*  = tranche 1 bonfire rows (Run-I139Tour); i139-t2* = tranche 2 bosses (Run-I139Boss)
#   i139-t3*  = tranche 3 cutscenes by id (Run-I139Cutscenes); i139-t4 = menus (Run-I139Menus) and Firelink NPCs (Run-I139Npc),
#   i139-t4b/c/d/p/q = Run-I139Npc rows; i139-t5* = tranche 5 credits (Run-I139Credits).
#   2026-11-15 evening (Run-I139Series6, Run-I139Dlc): i139-t2i = bosses with their flags; i139-t3f = Friede's intro by her event;
#   i139-t4k, i139-t4kb = the Fire Keeper; i139-t5d = the DLC entrances.
# Facts per run come from the run folder (baseline.json created_utc and module_sha256, which Run-StartupTrace writes for every
# Native and Reference run since 2026-10-07; controller.jsonl's exit row; restoration.json; the .png count; alphacast-args.json, the
# Run-AlphaCast arguments, from 2026-11-15) and from the runs-*.log files of evidence/I-0139/artifacts (run line "HH:mm:ss <run>",
# then "placed window=.. screen=.. rect=..", the exit JSON, "restoration ok: True"). build_folder: the evidence/I-0139/build-tourN
# whose game_bootstrap.dll has that SHA-256 (read, not inferred; « ? » when none matches). Reference runs load the module too.
import os, re, json, glob, hashlib, datetime

HERE = os.path.dirname(os.path.abspath(__file__))
W = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
ART = os.path.join(W, 'evidence', 'I-0066', 'artifacts')
LOGS = os.path.join(W, 'evidence', 'I-0139', 'artifacts')
BUILDS = os.path.join(W, 'evidence', 'I-0139')
OUT = os.path.join(W, 'investigations', 'I-0139-game-tour', 'REFERENCE.tsv')


def rd(p):
    try:
        with open(p, encoding='utf-8-sig', errors='replace') as f:
            return f.read()
    except OSError:
        return None


def tranche(run):
    m = re.match(r'i139-([a-z]+)(\d+)([a-z]*)-', run)
    k = m.group(1) + m.group(2) if m else ''
    if k == 'v1':
        return 'validation'
    if k in ('b1', 'b2'):
        return '2-trial'
    if k == 'v2':
        return '2-verify'
    if k and k[0] == 't':
        return k[1:]
    return '?'


builds = []
for d in sorted(glob.glob(os.path.join(BUILDS, 'build-tour*'))):
    dll = os.path.join(d, 'game_bootstrap.dll')
    if not os.path.isfile(dll):
        continue
    h = hashlib.sha256()
    with open(dll, 'rb') as f:
        for ch in iter(lambda: f.read(1 << 20), b''):
            h.update(ch)
    t = datetime.datetime.fromtimestamp(os.path.getmtime(dll), datetime.timezone.utc)
    builds.append((t, os.path.basename(d), h.hexdigest().upper()))
builds.sort()


def parse_utc(s):
    m = re.match(r'^(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d)(\.\d+)?Z$', s)
    if not m:
        return None
    return datetime.datetime.fromisoformat(m.group(1)).replace(tzinfo=datetime.timezone.utc)


runline = re.compile(r'^\d\d:\d\d:\d\d (i139-[a-z0-9-]+?)(?: \(\d+ NPCs\))?\s*$')
info = {}
for lp in sorted(glob.glob(os.path.join(LOGS, 'runs-*.log'))):
    name = os.path.basename(lp)
    cur = None
    for line in (rd(lp) or '').splitlines():
        m = runline.match(line)
        if m:
            cur = m.group(1)
            e = info.setdefault(cur, {'logs': [], 'last': {}})
            if name not in e['logs']:
                e['logs'].append(name)
            e['last'] = {}
            continue
        if cur is None:
            continue
        c = info[cur]['last']
        m = re.match(r'^placed window=\S+ screen=(\S+) rect=(\S+)', line)
        if m:
            c['screen'], c['rect'] = m.group(1), m.group(2)
        elif line.startswith('{"exit"'):
            try:
                c['exit'] = json.loads(line)
            except ValueError:
                pass
        elif line.startswith('restoration ok:'):
            c['restored'] = line.split(':', 1)[1].strip()

by_hash = {h: n for t, n, h in builds}
cols = ['run', 'tranche', 'mode', 'created_utc', 'module_sha256', 'build_folder', 'screen', 'window_rect', 'exit', 'faults',
        'crash_kind', 'restored', 'shots', 'args', 'log']
rows = []
for d in sorted(glob.glob(os.path.join(ART, 'i139-*'))):
    if not os.path.isdir(d):
        continue
    run = os.path.basename(d)
    mode = 'nat' if run.endswith('-nat') else 'orig' if run.endswith('-orig') else ''
    created, module = '', ''
    b = rd(os.path.join(d, 'baseline.json'))
    if b:
        try:
            bj = json.loads(b)
            created, module = bj.get('created_utc', ''), (bj.get('module_sha256') or '').upper()
        except ValueError:
            pass
    L = info.get(run, {'logs': [], 'last': {}})
    last = L['last']
    ex = last.get('exit', {})
    if not ex:
        for ln in reversed((rd(os.path.join(d, 'controller.jsonl')) or '').splitlines()):
            if '"exit"' in ln and '"faults"' in ln:
                try:
                    ex = json.loads(ln)
                    break
                except ValueError:
                    pass
    parts = []
    if last.get('restored') == 'True':
        parts.append('ok')
    if os.path.isfile(os.path.join(d, 'restoration.json')):
        parts.append('json')
    shots = sum(1 for f in os.listdir(d) if f.lower().endswith('.png'))
    args = ''
    a = rd(os.path.join(d, 'alphacast-args.json'))
    if a:
        try:
            args = json.dumps(json.loads(a), separators=(',', ':'), ensure_ascii=False)
        except ValueError:
            pass
    rows.append([run, tranche(run), mode, created, module, by_hash.get(module, '?' if module else ''), last.get('screen', ''),
                 last.get('rect', ''), str(ex.get('exit', '')), str(ex.get('faults', '')), str(ex.get('crash_kind', '')),
                 '+'.join(parts), str(shots), args, '+'.join(L['logs'])])

with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
    f.write('\t'.join(cols) + '\n')
    for r in rows:
        f.write('\t'.join(r) + '\n')
print(len(rows), 'rows ->', OUT)
