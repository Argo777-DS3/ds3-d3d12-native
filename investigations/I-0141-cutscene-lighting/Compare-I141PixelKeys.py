# I-0141 tranche 17: the key each pixel takes, natively and in the original (pixel-keys.flag with light-observe.flag).
# Usage: python -I Compare-I141PixelKeys.py <native run folder> <original run folder> --arrival-key <light key a of the area travelled to>
#        [--settle S] [crop x,y,w,h in 1920x1080 client pixels ...]
#
# Original (reference_pixel_key, one row per 60-second QPC window and (model, the branch byte, the group's own key words, 137910's
# entry's at envData or none); tranche 17's run, 5105b1a75, also keyed envData, which follows the blend weights, and the group's
# address, a per-frame copy): the envData 5B3D0 wrote for the G-buffer rows (types 6 and 7), GBuffer3.w's value (K-0451), the branch
# byte [root+0x42A0], and the entry the composite reads at that index (null when envData is not below the entry count: « entry
# unread », never replaced by the group's own block).
# Native (native_pixel_key, one row per window and (model, group, key words 0 to 7, set_full), the first sample's set and blend
# weights written; the run's build also keyed the set): the set its envData takes. The set is the registry's slot of the group's own
# full block (NativeLightSetIndex, 05_selected_scene.inc), so the key its pixels take is that block by construction, except when
# the registry is full (set_full: set 0 not holding it; in a journal without the field, a set 0 row after the first
# native_light_sets_full line): such models are counted apart, not compared. native_pixel_key_gbuffer (every 300th collect, when
# the journal has it): the models with a G-buffer row, so the native's models are those whose pixels carry GBuffer3.w.
# native_pixel_key_list beside each world picture: that frame's scene's key list by set index and the render extent, so each
# G-buffer picture's GB3.w (its fourth byte) reads as a key (byte 0 also holds the pixels no G-buffer row wrote).
#
# Both modes are aligned on the game, not on the wall clock (D-0299): each mode's arrival is its first row whose own light key a's
# first word (the area) is --arrival-key (the travel's end, an event of the game's own run; natively among the models with a
# G-buffer row when the journal lists them). The witnesses write each state at its first sample in each 60-second QPC window, a
# window whose phase against the arrival differs between runs, so the comparison takes whole windows only: those starting at least
# --settle seconds (default 10) after the arrival and ending before the run's last row or its *_pixel_key_full row, the same number
# in both modes (the smaller), each mode's phase printed. This is the arrival's event, not a game-clock value read in both; the
# protocol's look sweep runs on the same delays after it in both. Per model drawn in both modes in those windows: the set of key
# blocks (words 0 to 7) its rows take.
import json, math, os, sys, collections

WINDOW_MS = 60000.0

def rows(path, events, bad):
    tags = ['"event":"%s"' % e for e in events]
    with open(path, encoding='utf-8', errors='replace') as f:
        for n, line in enumerate(f):
            if any(t in line for t in tags):
                try:
                    yield n, json.loads(line)
                except json.JSONDecodeError:
                    bad[0] += 1

def key8(k):
    return tuple(int(v) for v in k[:8])

def name(k):
    if k is None:
        return 'entry unread'
    if isinstance(k, str):
        return k
    return '{%d,%d|%d,%d}' % (k[0], k[1], k[4], k[5]) if k[0:2] == k[2:4] and k[4:6] == k[6:8] else str(k)

def arg(args, flag, default=None, cast=str):
    if flag in args:
        i = args.index(flag); v = cast(args[i + 1]); del args[i:i + 2]; return v
    return default

def main():
    args = sys.argv[1:]
    arrival_key = arg(args, '--arrival-key', None, int)
    settle = arg(args, '--settle', 10.0, float)
    if arrival_key is None:
        sys.exit('--arrival-key is required (the light key a of the area travelled to)')
    nat_dir, orig_dir = args[0], args[1]
    crops = [tuple(int(v) for v in c.split(',')) for c in args[2:]] or [(300, 120, 560, 300)]
    out = []
    say = out.append
    bad = [0]

    # The rows of both modes.
    oj = os.path.join(orig_dir, 'startup.jsonl'); nj = os.path.join(nat_dir, 'startup.jsonl')
    orig = [r for _, r in rows(oj, ['reference_pixel_key'], bad)]
    o_full = [r['qpc_ms'] for _, r in rows(oj, ['reference_pixel_key_full'], bad)]
    nat = []; lists = []; gbuffer = []; n_full = []; full_line = None
    for n, r in rows(nj, ['native_pixel_key', 'native_pixel_key_list', 'native_pixel_key_gbuffer', 'native_pixel_key_full',
                          'native_light_sets_full'], bad):
        e = r['event']
        if e == 'native_pixel_key': nat.append((n, r))
        elif e == 'native_pixel_key_list': lists.append(r)
        elif e == 'native_pixel_key_gbuffer': gbuffer.append(r)
        elif e == 'native_pixel_key_full': n_full.append(r['qpc_ms'])
        elif full_line is None: full_line = n

    # The original's sources over the whole run.
    branches = collections.Counter(r['branch_42a0'] for r in orig)
    unread = sum(1 for r in orig if r['entry'] is None)
    differs = [r for r in orig if r['entry'] is not None and key8(r['entry']) != key8(r['own'])]
    say('original: %d rows, types %s, branch [root+0x42A0] %s; entry unread: %d; entry at envData = own block: %d, not: %d; '
        'row cap: %s' % (len(orig), dict(collections.Counter(r['state_type'] for r in orig)), dict(branches), unread,
                         len(orig) - unread - len(differs), len(differs), ('at %.1f ms' % o_full[0]) if o_full else 'no'))
    by_model = collections.defaultdict(set)
    for r in differs:
        by_model[r['model']].add((r['env'], name(key8(r['own'])), name(key8(r['entry']))))
    for m in sorted(by_model)[:40]:
        say('  original entry differs from own: %s (env, own, entry) %s' % (m, sorted(by_model[m])[:4]))

    # The spans after each mode's arrival.
    def arrival(times_keys):
        hits = [t for t, k in times_keys if k == arrival_key]
        return min(hits) if hits else None
    ever_drawn = set().union(*(g['models'] for g in gbuffer)) if gbuffer else None
    if ever_drawn is not None and not ever_drawn:
        ever_drawn = None; say('native_pixel_key_gbuffer rows list no model: the native arrival and models not restricted')
    o_arr = arrival([(r['qpc_ms'], int(r['own'][0])) for r in orig])
    n_arr = arrival([(r['qpc_ms'], int(r['own'][0])) for _, r in nat if ever_drawn is None or r['model'] in ever_drawn])
    if o_arr is None or n_arr is None:
        print('\n'.join(out)); sys.exit('no row with the arrival key in %s' % ('the original' if o_arr is None else
            'the native' if ever_drawn is None else 'the native among the models with a G-buffer row'))
    o_end = min([max(r['qpc_ms'] for r in orig)] + o_full)
    n_end = min([max(r['qpc_ms'] for _, r in nat)] + n_full)
    def whole(arr, end):
        first = int(math.ceil((arr + settle * 1000.0) / WINDOW_MS)); last = int(end // WINDOW_MS)   # windows first .. last-1
        return list(range(first, last))
    o_w = whole(o_arr, o_end); n_w = whole(n_arr, n_end)
    count = min(len(o_w), len(n_w))
    if count < 1:
        print('\n'.join(out)); sys.exit('no whole window in both modes after the arrival and %.0f s' % settle)
    o_w = set(o_w[:count]); n_w = set(n_w[:count])
    say('arrival (first row under the area %d): original %.1f ms, native %.1f ms; %d whole window(s)%s, from %.1f to %.1f s after '
        'the arrival in the original and from %.1f to %.1f s in the native' % (arrival_key, o_arr, n_arr, count,
        ' (fewer than two: a state sampled once)' if count < 2 else '', (min(o_w) * WINDOW_MS - o_arr) / 1000.0,
        ((max(o_w) + 1) * WINDOW_MS - o_arr) / 1000.0, (min(n_w) * WINDOW_MS - n_arr) / 1000.0,
        ((max(n_w) + 1) * WINDOW_MS - n_arr) / 1000.0))
    win = lambda t: int(t // WINDOW_MS)

    o_states = collections.defaultdict(set)
    for r in orig:
        if win(r['qpc_ms']) in o_w:
            o_states[r['model']].add(key8(r['entry']) if r['entry'] is not None else None)
    drawn = None; samples = 0
    for g in gbuffer:
        if win(g['qpc_ms']) in n_w:
            drawn = (drawn or set()) | set(g['models']); samples += 1
    if drawn is None:
        restriction = ('not restricted: no native_pixel_key_gbuffer in this journal' if not gbuffer else
                       'not restricted: no native_pixel_key_gbuffer sample in the windows')
    else:
        restriction = 'restricted by %d native_pixel_key_gbuffer samples' % samples
    n_states = collections.defaultdict(set); full_models = set(); forward = set()
    for line, r in nat:
        if win(r['qpc_ms']) not in n_w:
            continue
        if drawn is not None and r['model'] not in drawn:
            forward.add(r['model']); continue
        fallback = r.get('set_full', full_line is not None and line > full_line and r['set'] == 0)
        if fallback:
            full_models.add(r['model']); continue
        n_states[r['model']].add(key8(r['own']))
    say('original: %d models in the windows; native: %d models (%s), %d models absent from every native_pixel_key_gbuffer sample '
        'in the windows left out, %d models with the registry\'s full fallback left out; unparsed rows: %d' % (len(o_states),
        len(set(n_states) - full_models), restriction, len(forward), len(full_models), bad[0]))

    common = sorted((set(o_states) & set(n_states)) - full_models)
    diffs = [m for m in common if o_states[m] != n_states[m]]
    say('models in both: %d; the same key blocks: %d, different: %d' % (len(common), len(common) - len(diffs), len(diffs)))
    for m in diffs[:60]:
        say('  %s: original %s; native %s' % (m, sorted(name(k) for k in o_states[m]), sorted(name(k) for k in n_states[m])))
    only_o = sorted(set(o_states) - set(n_states) - full_models); only_n = sorted(set(n_states) - set(o_states) - full_models)
    say('models only in the original: %d (%s)' % (len(only_o), ' '.join(only_o[:30])))
    say('models only natively: %d, by first letters %s (%s)' % (len(only_n),
        dict(collections.Counter(m[:3] for m in only_n).most_common(8)), ' '.join(only_n[:30])))

    # The native G-buffer pictures: GB3.w as a set index, through the list of the same serial and its logged extent.
    for l in lists:
        serial = l['serial']
        path = os.path.join(nat_dir, 'world-%d-gb3.rgba8' % serial)
        if not os.path.exists(path):
            say('picture %d: no gb3 file' % serial); continue
        data = open(path, 'rb').read()
        width, height = l['width'], l['height']
        if width * height * 4 != len(data):
            say('picture %d: %d bytes, not the logged %dx%d' % (serial, len(data), width, height)); continue
        keys = l['keys']
        def tally(x0, y0, w, h):
            x0 = max(0, min(x0, width)); y0 = max(0, min(y0, height))
            w = max(0, min(w, width - x0)); h = max(0, min(h, height - y0))
            c = collections.Counter()
            for y in range(y0, y0 + h):
                c.update(data[(y * width + x0) * 4:(y * width + x0 + w) * 4][3::4])
            return c
        def show(c):
            total = sum(c.values())
            if not total:
                return 'empty'
            cells = []
            for index, n in c.most_common(8):
                k = name(key8(keys[index])) if index < len(keys) else 'outside the list'
                cells.append('%d %s %.1f%%' % (index, k, 100.0 * n / total))
            return '; '.join(cells)
        say('picture %d (%dx%d, %d keys listed): whole %s' % (serial, width, height, len(keys), show(tally(0, 0, width, height))))
        sx = width / 1920.0; sy = height / 1080.0
        for (x, y, w, h) in crops:
            say('  crop %d,%d %dx%d: %s' % (x, y, w, h, show(tally(int(x * sx), int(y * sy), int(w * sx), int(h * sy)))))
    print('\n'.join(out))

if __name__ == '__main__':
    main()
