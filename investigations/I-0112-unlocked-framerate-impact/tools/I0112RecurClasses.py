import csv, re
from collections import Counter

SP = 'C:/Users/User/AppData/Local/Temp/claude/C--Users-User-Desktop-DS3-Decompilation--claude-worktrees-heuristic-bhaskara-faf567/3efa0a4e-0c38-4edc-8486-201684e67e38/scratchpad/'
txt = open(SP + 'dec/contexts.txt', encoding='utf-8').read().split('\n')
lines = [l for l in txt if l.strip() != '']
exprs = {}
fn = None
cur = None
for i, l in enumerate(lines):
    if l.startswith('=== '):
        fn = l.split()[1]
    elif l.startswith('-- use'):
        cur = re.search(r'st@([0-9a-f]+)', l).group(1)
    elif l.startswith('>>'):
        e = l[2:].strip()
        j = i + 1
        while not e.rstrip().endswith(';') and j < len(lines) and not lines[j].startswith(('--', '===')) and j < i + 4:
            e += ' ' + lines[j].strip()
            j += 1
        exprs[(fn, cur)] = e

NO = 'RECURRENCE_NO_DT'
DT = 'RECURRENCE_DT'
ONE = 'ONE_SHOT'
UNK = 'UNKNOWN'
F = {}


def s(f, cls, sub, law=''):
    F[f] = (cls, sub, law)


s('1401130f0', ONE, 'rendering (post-process kernel/weight table rebuilt when flag [p+0x34] set; guess: blur/bloom taps)')
s('14022e5c0', ONE, 'rendering/environment params (blend of a param row into a stack copy of the base params built by caller 14022d620 via FUN_14022e0f0; weight = row[5]); NOT persistent')
s('140230610', DT, 'unknown (accumulates the delta of an input: s += (cur-prev)*k then prev=cur; independent of call count)')
for f in ('1402a5d60', '1402af510'):
    s(f, NO, 'camera (guess: two ~3.1 KB clones, blend of two parameter sets by distance; [+0x2860] is a blend timer decremented by [+0x3b30], probably the frame delta, unverified)', 'x *= 0.55^(dt/dt60) while the condition holds (or make the cut a one-shot)')
s('1402b2e30', DT, 'camera shake/sway (guess): [p+0x30] += (obj[+0xa8]-[p+0x10]) * speed [p+0x80]; first factor is a difference of two clock values, so a time delta')
s('140361b70', ONE, 'math (matrix multiply, 14 callers)')
s('14050c7b0', NO, 'unknown tracker class (vtable, temp Time wrapper); mode 0 = exponential smoothing cur += (target-cur)*gain [+0x20]; mode 1 = rate*Time step (dt present). The store is the join of both', 'mode 0 only: g_new = 1-(1-g)^(dt/dt60)')
s('14050d0c0', UNK, 'camera? ([p+0xa8] ramp 0..1 += (float)int * rate [p+0xac], clamp [0,1], drives a lerp of [+0xa0]/[+0xa4]). The int comes from FUN_1407fe4a0, which is encrypted code (undecodable statically): a +-1 direction would make it NO_DT, a tick count DT', 'if the int is a direction: += int*rate*dt/dt60')
s('14050e9c0', UNK, 'camera? (same ramp as 14050d0c0 on [p+0x138], int from the same encrypted FUN_1407fe4a0, rate [p+0x140], clamp [lo,1]); param_2 (float, dt-like) multiplies other terms of this function but NOT this ramp', 'if the int is a direction: += int*rate*dt/dt60')
s('140515790', DT, 'camera (large update; fVar100 = dt arrives in XMM1, fVar125 = dt/(1/30)); [p+0x314] += dt*[p+0x324]*weight')
s('1405ca3f0', NO, 'character movement: ground/terrain response term [p+0x2dc] (same field as 1405e6580; clamped to [-0.5,0.5] in the sibling); here x*=0.6 when no hit, or x = clamp + 0.2*resp', 'x = 0.6^(dt/dt60)*x ; x += 0.2*resp*dt/dt60')
s('1405d0010', NO, 'character movement (turn; angle wrapped on 2*pi, 0x40490fdb=pi): angular speed [p+0x10] += param_4*param_5[0] each call with no dt, clamped to param_4*param_5[1]; dt (param_2, from caller 1405cfc10) enters only in the later angle step speed*param_2. Param roles inferred (hypothese)', '[p+0x10] += param_4*param_5[0]*dt/dt60 (clamp unchanged)')
s('1405d2280', NO, 'character movement: sibling of 1405e6580 (several probes, then [p+0x2dc] = x*0.6 when no hit, else min-probe*0.2 + x), clamped to [-0.5,0.5]', 'x = 0.6^(dt/dt60)*x ; x += 0.2*resp*dt/dt60')
s('1405e6580', NO, 'character movement: known terrain bug, [p+0x2dc] += 0.2*response (or x*0.6), clamp [-0.5,0.5]', 'x += 0.2*resp*dt/dt60 ; x = 0.6^(dt/dt60)*x')
s('1406de400', NO, 'input/UI (guess): counter [p+0x38] -= 1.5 per call while the same id [p+0x40]==[p+0x44] persists, reset to 0 on change (hold/repeat timer?)', 'x -= 1.5*dt/dt60')
s('1406e0a80', ONE, 'UI (rect interpolated by fractions [p+0x58],[p+0x5c] on a rect passed in each call)')
s('14078ea30', ONE, 'event counter: obj[+0x2c] += obj[+0x30]+1.0 once per event (called by 1408f24c0)')
s('140828fe0', NO, 'animation (skeleton pose, bone index [p+0x10], quaternion twist): [p+0x1c] += ([p+0x18]-[p+0x1c])*[p+0x28]; gain [+0x28] set elsewhere (not checked), could already be dt aware upstream', 'g_new = 1-(1-g)^(dt/dt60)')
s('140834710', ONE, 'animation (morph/shape blend: weights x delta vectors accumulated into a freshly zeroed buffer)')
s('140834e40', ONE, 'animation (morph/shape blend, same pattern)')
s('1409b6d80', NO, 'character animation/transform (attach-like: lerp factor [p+0x60] += 0.1 per call while condition (flag byte 0x9cc&1) holds, clamp 1.0; consumed as t in a position/rotation lerp)', '+= 0.1*dt/dt60')
s('1409f9d30', DT, 'unknown (a wrapper gives dt/(1/30) at +8: *p = k*(dt*30)*0.6*(*p) or *p += param_4*0.2*(dt*30)); dt enters but linearly, not as a power')
s('140d71920', NO, 'camera follow/history (guess: ring buffer of positions, retarget timer [p+0x48] -= 1/30 per call, refilled by FUN_140d71de0 when < 0)', 'x -= dt (seconds) instead of 1/30 per call')
s('140d736d0', NO, 'camera follow (same object as 140d71920): timer [p+0x240] -= 1/30 per call; < 0 triggers FUN_140d74970', 'x -= dt')
s('140ef2ce0', NO, 'network/online (calls SteamAPI_RunCallbacks; randomised poll timers in seconds, each -= 1/60 per call)', 'x -= dt')
s('14100dde0', NO, 'physics (broadphase-like: per-object margin factor [p+0x6c] *= 1.05 if far out, *= 0.953 if well inside; clamp 1.0). Called from the collision pass FUN_140fc9a00', 'x = 1.05^(dt/dt60)*x (and 0.953^...)')
for f in ('141071430', '141072c80', '141074140', '1410749a0', '1410bfc20'):
    s(f, ONE, 'animation blending (weighted accumulate of a source pose/track into the output buffer; weight is a blend weight, not dt)')
s('1410ccfb0', ONE, 'math/animation (record built from a template then scaled per element)')
s('141180110', NO, 'animation (per-bone weight array at [p+0x88], stride 0x30: w += (1.0-w)*[p+0x70] when condition bVar6; vtable method)', 'g_new = 1-(1-g)^(dt/dt60)')
s('1412b2790', ONE, 'math (sparse matrix-vector accumulate)')
s('1412b2960', ONE, 'math (axpy)')
s('1412d2fb0', UNK, 'unknown (recursive weighted accumulation into cached tree nodes: node[param_3] += (1-param_5)*child[param_4]; param_3 capped to 11)')
s('141475c20', ONE, 'physics/collision query (radius [param_4+0x10] *= 20 inside a query struct; cannot be persistent across frames)')
s('14155fbc0', ONE, 'math (normalisation by rsqrt)')
s('14168ecb0', ONE, 'animation (pose accumulation weighted by 1/count)')
s('1416d11e0', ONE, 'rendering/UI (copy then offset by ratio terms)')
s('1416d9800', ONE, 'math (normalisation)')
s('1416e1680', DT, 'character animation (IK-like offset array, stride 0x48): v += (target-v)*g with g = 1 - base^([p+0x694]/0.1), clamped to +-bound*[p+0x694]; [p+0x694] is the frame delta. A correct DS3 example of the rate-independent law')
s('1416fed10', ONE, 'math')
s('14172c980', ONE, 'math')
s('141731540', ONE, 'rendering (post-process setup globals _DAT_1448e6c00/08)')
s('1417cd730', ONE, 'UI text layout (pen position advance)')
s('141832830', ONE, 'UI text layout (glyph advance)')
s('141a15360', ONE, 'UI animation (keyframe track sampled at the time passed in param_9)')
for f in ('141ba100e', '141bd0300', '141bd0460', '141c376d0', '141c3ae20', '141c65530', '141c7aca0', '141c8bd20', '141c911c0'):
    s(f, ONE, 'math (affine matrix / point transform composition)')
s('141c07ce0', ONE, 'UI (rect offset)')
s('141c27500', ONE, 'UI (rect scaled by 20 * clamp)')
for f in ('141c3b410', '141c3c1d0', '141c3c610'):
    s(f, ONE, 'blend node (out = out + (src-out)*weight [p+0x20]); output parameter filled by the caller, no persistent state seen; LOW confidence (would be a smoother if [p+0x20] is a rate and out persists)')
s('14200f0f0', ONE, 'UI/animation (key array time-reversal t -> T - t)')
s('1420f23d0', ONE, 'rendering (4 coefficient arrays scaled by a factor, caller 14209a9f0)')
s('1422f5d20', ONE, 'AI/navigation (guess: path cost halved once per query)')

rows = list(csv.reader(open('C:/Users/User/Desktop/DS3_evidences/I-0112/recur-candidates.tsv', encoding='utf-8'), delimiter='\t'))[1:]
out = []
cnt = Counter()
fcnt = {}
miss = set()
for r in rows:
    f, st = r[1], r[3]
    if f not in F:
        miss.add(f)
        continue
    cls, sub, law = F[f]
    e = exprs.get((f, st), '') or '(see function; store %s)' % st
    out.append((f, st, cls, sub, e, law))
    cnt[cls] += 1
    fcnt.setdefault(f, cls)
print('missing', miss, 'stores', len(out), dict(cnt), 'functions', dict(Counter(fcnt.values())), len(F))
with open('C:/Users/User/Desktop/DS3_evidences/I-0112/recur-classes.tsv', 'w', encoding='utf-8', newline='') as fh:
    w = csv.writer(fh, delimiter='\t', quoting=csv.QUOTE_ALL)
    w.writerow(['function', 'store_addr', 'class', 'subsystem', 'expression_as_decompiled', 'law'])
    for o in out:
        w.writerow(o)
for f, c in fcnt.items():
    if c in (NO, UNK):
        print(c, f, [o[1] for o in out if o[0] == f], [o[4][:100] for o in out if o[0] == f][:3])
