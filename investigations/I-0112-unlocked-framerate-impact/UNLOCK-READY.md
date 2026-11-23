# The unlocked frame rate, method B: ready plan

Study thread, 2026-10-30 (Paris, evening), asked by the RT pilot for the unlock workstream (D-0240: a logic correct at any dt,
« Unlimited » included, after the modern cadence). Sources: [RESULT](RESULT.md) §1 and §4, [RECUR2](RECUR2.md), the site-by-site
reading [UNLOCK-SITES](UNLOCK-SITES.md) (a Sonnet subagent's, its key facts rechecked here), the decompiler pass of §6 (new,
[tools/I0112PcodeRecur.java](tools/I0112PcodeRecur.java)), the T0 reading of the limiter
([CADENCE-REFLEX-READY](../I-0111-dlss-reflex/CADENCE-REFLEX-READY.md) §8), `native/` (MinHook detours, the module's pins). Read-only;
no run. Labels: OBSERVED, INFERRED, GENERAL.

## 0. In short

1. **Every site is plain code**: each function decompiles and starts with an ordinary prologue, none behind an Arxan thunk
   (UNLOCK-SITES §0, OBSERVED by the subagent's entry reads), so the module's usual tool fits: **a MinHook detour at the entry**,
   calling the original, as `BeginFrame`, `EndFrame` and the pose updates are hooked in production. Three tiny leaves start with a
   RIP-relative load (MinHook relocates it). Mid-function patches (category D) are needed at five places only and are avoided
   where an entry method exists.
2. **One dt for all corrections**: `k = dt / dt60` with `dt` = the value the game step hands its systems (the tick's time object,
   `[[0x14489DD10]+0x268]`, the mean of the last three frame times, each at most 50 ms; CADENCE §8.2) and `dt60 = 1/60`, read
   once per frame in `BeginFrame` (the same thread). `k` is forced to exactly 1 when `|k − 1| < 1e-4`, so at 60 frames/s every
   corrected site is bit-for-bit the original (the timing project's rule, kept). Note: the dt selector `140DD4B60` gets the
   16-frame mean `+0x26C` instead (OBSERVED in `1408F2CA0`); nothing to change there.
3. **The laws** (general, valid for any dt; RESULT §0 point 4): a step `x += c` → `x += c·k`; a timer `t −= c` → `t −= c·k`; a
   decay `x *= r` → `x *= r^k`; a lerp gain `x += (y − x)·g` → `g' = 1 − (1 − g)^k`; a per-frame distance compared with a
   threshold → the distance divided by `k`; a constant dt argument `1/30` or `1/60` → that constant times `k` (the original PC game
   at 60 is the reference, so a `1/30` per call, twice real time at 60, stays twice real time: the law is relative).
4. **Order by visible risk**: locomotion, then the terrain response (TRTF's kind), turning, animation and transitions, camera and
   target tracking, physics (to settle first), effects and the eye adaptation, interface timers, the rest (§4).
5. **The decompiler pass finds TRTF's decay branch** (`1405E693E`, through a phi), which the instruction passes missed, and 70
   more functions to triage (§6): nothing in it changes the order, a few new candidates in the character code to read.

## 1. The methods (how a site is corrected from the module)

| Code | Method | When |
|---|---|---|
| A | **Entry detour with pre-bias**: before calling the original, move the field so that the original's own constant update lands on the corrected value (`x_pre = x_old + c·(1 − k)` for a step, `x_old·r^(k−1)` for a decay); after the call, if the field is still exactly `x_pre` (the update did not run), put `x_old` back. Preferred to a post-correction when the function reads or copies the field after updating it in the same call (UNLOCK-SITES: `1405E6580`, `1407F5260`, `1409B6D80`) | a field updated by a constant per call |
| A post | **Entry detour with post-correction**: snapshot, call, then `x = x_old + (x_new − x_old)·k` or `x_old·(x_new/x_old)^k` (guarded for `x_old = 0`) | when nothing in the call reads the new value |
| A' | **Scale an input around the call**: set a gain or coefficient field (or argument) to its corrected value, call, restore it | the recurrence's factor is a field or argument (`1405D0010`, `140828FE0`, `141180110`, `14050C7B0`) |
| B | **Replace a callee's dt argument**: a detour on the callee that receives the constant dt (a float in XMM1, or `[ptr+8]` of a `{vtable 0x1427457D8, float}` time object) multiplies it by `k` | a constant dt passed down (`1401FD120`, YEBIS `1416F27E0`, `140A4CDF0`) |
| C | **Redirect a constant**: only when the constant is read by that function alone (one case: `0x1429B5B88`, read only by `140EF2CE0`); the shared ones (1/30 at `0x143D7ED50`: 41 readers; 1/60 at `0x143D7ED3C`: 17; 30.0, 60.0, 0.2, 0.6, 0.8, 1.2) must never be redirected | |
| D | **Patch an instruction's operand** (the `disp32` of a `movss`/`mulss` that loads a constant, pointed at a module-owned float that holds `c·k`): only where A to C cannot express the law. It changes code bytes: the module's launch pins compare DS3's bytes and must be updated in the same change; the patch is applied once at start after the pins pass and never undone mid-session (a module float that changes each frame carries `k`) | five places (§3) |
| E | Not frame dependent as read: nothing to do, or a runtime read first | |

Re-entrancy: `1405E6580` and `1405D2280` both call `1405CA3F0`, `140D71920` calls `140D736D0`, `140C2A7B0` calls `140A4CDF0`
(which has other callers): one thread-local depth counter per family, the correction applied at the outermost call only.

## 2. Where `k` comes from, and the unlock itself

- **Read**: `BeginFrame` (`15_cpu_service_and_input.inc:342`) reads `[[image+0x489DD10]+0x268]` once and publishes `k` to the
  detours (a plain float; every site above is INFERRED to run on the engine thread's update, to confirm with the call counter of
  §4).
- **The rate**: `+0x354` (requested frames per second) and `+0x358` (enable), as `original-unlock.flag` writes them today
  (`15_cpu_service_and_input.inc:359-360`), set when the option changes. « Unlimited » = a high requested rate (360 measured,
  I-0088; the limit is the period `1/+0x354` as a float, so very high values are only bounded by the frame itself).
- **What DS3 does to dt by itself** (CADENCE §8.2, OBSERVED): each frame counts for at most 50 ms (`1/[+0x2C0]`, 20.0) and the
  logic's dt is the mean of three; under 20 frames/s the game slows, as the original.

## 3. The sites (from UNLOCK-SITES, condensed; « hook » = the entry detour)

| Site | What (OBSERVED) | dt inside | Method | Notes |
|---|---|---|---|---|
| `1405C72B0` (a) | locomotion: `sqrt(dist)·30 < 1` decides the branch (`mulss` at `1405C73D0`, 30.0 shared) | no (a time object built locally for a callee) | D: the `mulss` reads a module float `30/k` | LTF's site; the unit of `dist` INFERRED per frame |
| `1405C72B0` (b) | `[+0x25C] *= 0.8` (stop branch, `1405C73EE`) or `*= 1.2` (store `1405C7421`), clamps 0.5 and 1.0 | no | A post (decay law on whichever factor ran; the clamps stay) | RECUR2's site |
| `1405DC510` | gait class from `|v|·30` against 0.01, 2.0, 6.0, hysteresis 0.2 | `param_2` a vector, unit unproven | first a runtime read; then D (`1405DC55E`) or A' (scale the vector's x, z by `1/k`) | E if the vector is already a speed |
| `1409CF010` | a predicate `|v·1/30|² > bound²` | no | E (INFERRED: a fixed look-ahead on a velocity) | verify the unit at runtime |
| `1405E6580` | TRTF: `[+0x2DC] = 0.2·min(a,b) + x` or `[+0x378]·x`, clamp ±0.5 | **yes, XMM1** (OBSERVED here) | A pre-bias (additive) and decay law (`[+0x378]^k`) | `[+0x2DC]` is read by `1407D7180` in the same call: pre-bias; depth counter with `1405CA3F0` |
| `1405CA3F0`, `1405D2280` | the siblings: `*= 0.6`, `+= 0.2·min`, clamps | XMM1 from the caller | A, the same hook family | nested: outermost only |
| `1405D0010` | turning: `speed += rate·coeff[0]` without dt, then `angle += speed·dt` | yes (XMM1 dt, XMM3 rate) | A': `coeff[0]·k` around the call (the 5th argument, a stack pointer to `controller+0x3C`) | restore after; `coeff[1]` (the cap) untouched |
| `1409B6D80` | transform lerp `t += 0.1`, clamp 1, used in the same call | a time object pointer (`[p2+8]`) present, unused there | A pre-bias | |
| `1407F5260` | ramp `[+0x1E0] = min(x + 1/30, 1)`, copied into type-7 objects in the call | no | A pre-bias (or D `1407F53C7`) | the copies need the pre-bias |
| `1407CB390` | ramp `[+0x24] ±= [+0x28]/60`, 60 calls whatever the amplitude | no | A pre-bias, clamp edges checked | virtual |
| `140828FE0` | bone constraint `x += (y − x)·[+0x28]` | no | A': gain law on `[+0x28]` | per bone |
| `141180110` | bone weights with `[+0x70]`, `[+0x6C]` (and `+0x68`) | no | A': gain laws | large function, several branches |
| `14050C7B0` | tracker, mode 0: `x += (y − x)·[+0x20]` (mode 1 is dt-based) | time object | A' in mode 0 only | `[+0x20]` is a rate in mode 1 |
| `140D71920` (+`140D736D0`) | target/camera timer `[+0x240] −= 1/30`, refreshed when negative | no | A pre-bias at `140D71920` only | the nested one never hooked twice |
| `1402A5D60`, `1402AF510` | `[+0x2860] *= 0.55` when a distance exceeds `[+0x285C]`, then `−= [+0x3B30]` (a dt-like field) | `[+0x3B30]` INFERRED a dt | D (the `mulss`) or A with the condition re-evaluated outside | a clone pair; the condition is the difficulty |
| `1406DE400` | hold value `[+0x38] −= 1.5` per call | no | A post | its consumer not found |
| `1400A5AA0` → `1401FD120` | a child solver stepped with `dt = 1/30` (XMM1) | the callee's argument | B on `1401FD120` (its only callers are these two calls) | |
| `140245F90` | an 8-slot effect system's step 1/30 (`movss xmm15` at `140246240`), fades over 0.1 s | no | D | many fields: A impossible |
| YEBIS `1416F27E0` (and the stub `1416C9430`) | writes the post-effect time step `[ctx+0x694] = 1/60` (stack argument 5) | the callers hold none | B: rewrite stack argument 5 to the real dt | `1416E1680`'s smoothing law is already right (RESULT §4); the native's own YEBIS producers must pass the real dt too (to check in `native/world/yebis_*`) |
| `140C2A7B0` → `140A4CDF0` | a task evaluation with `dt = 1/60` | the time object | B on `140A4CDF0` under a flag set by a hook on `140C2A7B0` | `140A4CDF0` has other callers |
| `140741A30` | `movss xmm1, 1/30` into a UI virtual (`140741A6F`) | no | D, or E if the virtual is a one-shot | the virtual target not resolved |
| `140EF2CE0` | Steam and network poll timers `−= 0.016666` | no | C (`0x1429B5B88`) | its integer countdowns still count calls: harmless offline |
| `140663300`/`340`, `14072FEC0` | a 30-call cycle and a 4-call delay | no | E (INFERRED harmless), or an accumulator if a cycle length matters | name their subsystem first |
| `14087E8A0` | `[owner+0x1A4C] *= t` (t dt-aware) | yes | E unless `+0x1A4C` persists (runtime read), then A post with `t^k` | |
| `140DD4B60` | the dt selector: modes 1 and 2 give 1/30 and 1/60 | the real dt | A post if a mode ≠ 0 is ever set (no setter found: INFERRED E) | |
| `14100DDE0` | broadphase margin `*= 1.05` / `0.953` by step counts | no | E until the physics step rate is known | physics library |
| `14045A2E0`, `140DCE1E0`, `140B30240`, `140EDA940`, `140BB3E50`, `14235CDB0`, `140BDCBB0`, `140EF5590`, `140665970`, `140A608E0` | constant dt in one-shot, loading, wait or shutdown loops, or copied only | — | E (UNLOCK-SITES 2.2) | `140665970` to revisit if a held state calls it every frame |
| physics `141530300`, `141530D80`, `141668BB0` | character-proxy contexts `{1/60, 1/60}` handed to listener loops `141533750` (vt+8) and `141650F00` (vt+0x20) | no | **to read first**: if the listeners integrate with it, B on the two loops | Havok's stepping is the open physics question (RESULT §5.4) |
| physics `1414800F0`, `14149AA00` | constructor seeds `{0, 1/60, 1/60, 59.99}` and a world config with 1/30 | — | E (seeds) | who writes the step each frame: the same question |

## 4. Order of the corrections (by what a player sees first)

1. **Locomotion** (`1405C72B0` both sites; `1405DC510` after its runtime read; `1409CF010` checked): sprint, run and walk
   distances, stopping, gait changes.
2. **Terrain response** (the `1405E6580` family): slopes and stairs.
3. **Turning** (`1405D0010`).
4. **Animation and transitions** (`1409B6D80`, `1407F5260`, `1407CB390`, `140828FE0`, `141180110`, `14050C7B0`).
5. **Camera and target tracking** (`140D71920`, the `1402A5D60` pair).
6. **Physics**: read the proxy listeners and the world step first; then `1401FD120` (B), the rest as found.
7. **Effects and post** (`140245F90`, YEBIS `1416F27E0` and the native's own YEBIS time step).
8. **Interface** (`1406DE400`, `140741A30`, `140C2A7B0`).
9. **The rest** (`140EF2CE0`, the cycles, `14087E8A0`, `14100DDE0`).

One diagnostic run before the code (announced, the original at 60 and the native unlocked at 120): a call counter per site
(how many calls a frame: RESULT §5.1), the unit of `1405DC510`'s vector, whether `[owner+0x1A4C]` persists, and whether the
selector's mode is ever non-zero. It turns every INFERRED « once a frame » above into a fact.

## 5. Proving each correction

- **Unit controls** (no game): for each law, `k = 1` gives the original's bits; two calls at `k/2` equal one at `k` (exact for
  steps and timers, within rounding for powers); a timer that expires at call `n` at 60 expires at the same game time at 120.
- **Live, against the original at 60** (the sweep D-0240 names: 30, 60, 120, 240, Unlimited; the ASUS, the screen named, the
  logic's dt read from the cadence trace's `game_dt_us`):

| Mechanic | Measure | Pass |
|---|---|---|
| Sprint, run, walk | distance covered in 5 s of a fixed input on flat ground (`witness_player_position`) | within 2 % of the original at every rate |
| Stopping, gait | the gait class sequence (`1405DC510`'s output) and the stopping distance after releasing the stick | same sequence; distance within 2 % |
| Slopes and stairs | descent speed on a fixed slope and a staircase (Firelink's stairs), `[+0x2DC]` over time | the curve against time within 2 % |
| Turning | yaw rate at full stick, time to reach it | within 2 % |
| Roll and attacks | roll distance and duration; an attack's time to hit; stamina spent | within 2 %; same frame-of-hit in time |
| Animation blends | the ramp fields (`+0x1E0`, `+0x24`) against time | same time to complete |
| Camera | the follow lag after a fixed turn (time to settle) | within 5 % |
| Physics | a pushed crate's travel, a ragdoll's fall time | within 5 % |
| Effects and post | an effect fade (0.1 s), the eye adaptation time constant | within 5 % |
| Interface | a held button's hold time to trigger | same time |

The timing project validated sprint and roll this way at 165 and 238 frames/s (RESULT §3): its runners are the starting point,
not its code (D-0240).

## 6. The decompiler pass (TRTF's branch, and what else it finds)

[tools/I0112PcodeRecur.java](tools/I0112PcodeRecur.java): each listed function decompiled; every 4-byte store walked back through
its p-code (phis included, 24 operations) and kept when the flow reads the same address back, scales or offsets it by a read-only
constant or another field, and meets no float parameter, no call result and no writable global. Run headless on the mirror
`ghidra-mirror-i0068` at idle priority (2026-10-30 evening): I-0112's 314 functions plus every function of
`0x1405A0000-0x140610000` (the character movement code), **1,695 decompiled, 403 hits in 72 functions, 93 through a phi**
(the rows: `evidence/I-0112/pcode-recur.tsv`, with the function list `pcode-functions.txt`).

- **Validation**: it finds **TRTF's decay branch**: `1405E6580`'s store at `1405E693E` of `+0x2DC`, through a phi, with the 0.2
  constant and the field `+0x378` (the retention) as factors: both branches meet at one store, which is why the instruction passes
  saw only the additive one. It also finds `1405D2280`'s merged store (`1405D2557`), `1405CA3F0`, `1405C72B0`'s two stores,
  `1405D0010`, `1409B6D80`, `140828FE0`, `14050C7B0`, `140D71920`, `1402A5D60`.
- **False positives by construction**: a time object's dt read through its pointer (`[p+8]`) counts as « a field »: the hits
  whose factor is `param_2+0x8` or `lVar…+0x8` are dt-aware (`1405E08F0`, `1405E1150`, `14066F4C0`, `14099E290`, `140C8E440`,
  `1408C8590`, `14069E680`, `1409F9D30`); polynomial and matrix code (`142063FB0`: the series 1/6, 1/12, 1/20, 2.0, a sine or
  rotation expansion; `1404CB280`, `1404CB670`, `1404CC070`: products of a matrix's rows) is one-shot math.
- **New candidates to read** (INFERRED, not yet read): in the character code `1405C9DC0` (12 stores, `+0x200`/`+0x204` with 1.0),
  `1405CA570` (`+0x25C`, LTF's field, from another function), `1405CB200`, `1405D1B70` and `1405E57F0` (fields `+0x10`/`+0x2D0`,
  through phis), `1405C75F0`, `1405C7DB0`, `1405C2CD0`, `1405EA6A0` (with 2π: angles); elsewhere `1408651E0` (`+0x2920..+0x2928`
  with a factor `+0x2958`), `1406358D0` (10.0 and 60.0), `140F1DF00` (30.0), `140127240` (the dynamic quality scaler at a 60
  reference, already RESULT §1). The physics proxies (`141530300`, `141530D80`, `141668BB0`) appear too, with 2.0: the listener
  question above. The rest are known sites, the YEBIS library (`1416FED10`: luminance weights) or the `0x141C…` range (not named).
- Limits: the walk stops at 24 operations and at any call; a recurrence through memory written by a callee is not seen; the
  set of functions is a choice (the character code and the known sites), not the whole program.
