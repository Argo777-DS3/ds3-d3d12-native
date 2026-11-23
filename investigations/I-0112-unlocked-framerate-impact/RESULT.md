# I-0112 - Le déblocage du framerate : ce qui dépend des 60 images/s: result

Status: complete for the study (2026-10-30), study thread, branch `claude/heuristic-bhaskara-faf567`: the frame-rate constant
census and its classes, the recurrences without dt, the near constants; its open points (§5) go to the unlock workstream. Labels: OBSERVED (read in data or a run), STATIC (read in DS3's code), INFERRED (a
reading not yet checked), GENERAL (public knowledge). Evidence: `evidence/I-0112/` (`const-refs.tsv`, `const-functions.tsv`,
`const-classes.tsv`, the decompiled functions in `ctx/full/`, the use lines in `ctx/contexts.txt`).

## 0. In short

1. **The engine already hands a real delta time to the logic** (timing project, K-0011, K-0014): `context+0x08` flows through
   `00D57900` → `00023410` → `000499C0` to the update callbacks; the requested rate is the central timing object's
   `+0x354`/`+0x358` (K-0019, raised natively for measurement by I-0088's `original-unlock.flag`, K-0470). Most systems read that
   dt. The defects come from the places that ignore it.
2. **The census** ([tools/I0112Const.java](tools/I0112Const.java), headless on the mirror): 334 uses of a frame-rate float (1/60,
   1/30, 1/120, 60, 30, 120) in 250 functions. Classified with [tools/I0112ConstContext.java](tools/I0112ConstContext.java) and a
   reading of each use (a Sonnet `ghidra-research` agent; the high-risk sites spot-checked at the source here):

   | Class | Functions | Risk above 60 frames/s |
   |---|---|---|
   | FIXED_STEP: a literal 1/30 or 1/60 advanced per call where a dt belongs | 31 | high |
   | PER_FRAME_SCALE: a per-frame quantity scaled by 30 without dt (the locomotion bug's kind) | 3 | high |
   | DT_CLAMP: a duration or step bounded at 1/30 | 6 | low |
   | RATE_SETTING: the timing object, a Havok-like world default, a dynamic quality scaler at a 60 reference | 3 | medium |
   | FRAMES_TO_SECONDS: frame counts converted at /30 or /60, time then advanced by the real dt | 44 | low |
   | UNRELATED: angles, distances, durations in seconds, parameter defaults | 121 | none |
   | UNKNOWN | 42 | to read |

3. **The census finds the locomotion bug** (`1405C72B0`, `30·distance < 1` without dt, LTF's site) and two siblings of the same
   kind (`1405DC510` a gait class from `30·speed`, `1409CF010` a vector scaled by 1/30 without dt). **It cannot find the terrain
   bug's kind** (TRTF, `1405E6580`: `state + 0.2·response` or `retention·state` per call, no frame constant at all): that class
   needs another search (§4).
4. **What this means for method B** (D-0240: a logic correct at any dt, « Unlimited » included): each FIXED_STEP and
   PER_FRAME_SCALE site takes the real dt in a general law (`x += c·dt/dt60`, `r^(dt/dt60)` for a decay, thresholds scaled by
   `dt/dt60`), valid for any dt, not a table of rates. These laws are the general ones every frame-independent engine uses; LTF and
   TRTF were two instances of them (studied, not copied, D-0240).

## 1. The high-risk sites (STATIC; each one's call frequency INFERRED from its shape)

**FIXED_STEP, a timer or ramp advanced by a constant per call** (if called once a frame, it runs twice as fast at 120):
- `140D71920`, `140D736D0` (INFERRED character or AI): a timer `-= 1/30` per call, refreshed when negative (checked at the
  source: `*(p+0x48) - _DAT_143d7ed50`).
- `1407F5260` (INFERRED animation or ragdoll blend): a 0..1 ramp `min(x + 1/30, 1)`, 30 calls to complete (checked at the source).
- `1407CB390` (INFERRED animation): progress `+= x28/60` per call, 60 calls to complete.
- `140245F90` (subsystem unknown, effects or decals INFERRED): a slot `+= 1/30` and a position scaled by 1/30 (0 when
  `DAT_14496237e`).
- `140EF2CE0` (INFERRED ambient timers): three timers `-= 0.016665` (`0x3C888723`, near 1/60 but not exact; found by reading,
  missed by the census), re-rolled in [60, 120] when spent.
- `1400A5AA0`: each child stepped with dt = 1/30 (`FUN_1401FD120`: `+0x64 += dt`).

**FIXED_STEP, a constant 1/30 or 1/60 passed as the delta time** (an `{vtable 0x1427457D8, float}` time object, as `140DD4B60`
fills with the real dt): `140665970` (INFERRED character or animation), `14045A2E0`, `140EDA940`, `140DCE1E0`, `140B30240`,
`140BB3E50`, `140BDCBB0`, `140EF5590`, `14235CDB0` (1/30); `140A608E0`, `140C2A7B0` (1/60); `140741A30` (a literal 1/30, not
confirmed as a dt). Their callees' subsystems are not named yet.

**The dt selector** (`140DD4B60`, K-0012): an object's dt is the dynamic one, or a fixed 1/30 or 1/60 by its `+0x40` mode
(`140DD4F90` sets 1/30; `140DD41C0`, `14099F940` default 1/30). The timing project saw mode 0 (dynamic) on every call it
observed (K-0017); which objects use the fixed modes is open.

**Physics (INFERRED Havok)**: `1414800F0` seeds a world's `{dt, dt, 59.99}` with 1/60 (an `hkStepInfo`-like default),
`14149AA00` a minimum step of 1/30 beside 200.0 (an `hkpWorldCinfo`-like default), `141530300`, `141530D80`, `141668BB0` hand a
`{1/60, 1/60}` pair to a listener loop. Whether Havok steps at a fixed 1/60 (sub-stepping, then frame-independent by design) or
at the frame's dt is the main open physics question: a fixed physics step is the norm in modern engines and stays correct at
any rate if the engine runs as many steps as the elapsed time asks (GENERAL).

**PER_FRAME_SCALE**: `1405C72B0` (the locomotion bug, confirmed), `1405DC510` (gait thresholds from `30·speed`, same family),
`1409CF010` (INFERRED root motion: a vector × 1/30 compared to a squared bound).

**Rendering, on the native's side too**: the post-effect library (`GPUPostEffect.cpp`, YEBIS) gets a 1/60 by `1416CC7D0`,
`1400E69A0`, `1416EBF80`, `141725EC0` through `FUN_1416C9430`/`FUN_1416F27E0` into `[_DAT_1448e5e38]+0x694` (consumer not
traced; INFERRED the eye adaptation or the motion blur's time step: at 120 frames/s it would adapt twice as fast). The native
runs YEBIS natively (W10): its own producers must check what they pass there. `140127240`, a dynamic quality scaler, assumes a
60 reference (`... / 60.0`). The native module itself writes no 1/60 (grep of `native/`: only the measurement flag's 360).

## 2. Low risk, recorded

- DT_CLAMP: `1402671F0`, `140267310`, `140267440` (a duration at least 1/30), `14116D940` (a fallback sampling step 1/30);
  `1402B4950`, `1402B6220` call them. No clamp of the real frame dt was found among the 250.
- FRAMES_TO_SECONDS: 44, among them dt-aware ones that normalise by 30 frames/s with the real dt (`140515790` a lerp factor
  `dt/(1/30)`, `14097BC20`, `1409F9D30`, `1409F9C30`, `14099E290`, `14050C7B0`, `14066FE40`, `1405CEA80`): safe as read.
  `1406665C0`, `1406664F0` stop an animation at `length - 1/60` (a one-frame margin, to check at high rates).

## 3. What the timing project already measured (`timing/`)

- Validated: sprint and roll travel (LTF, I-0007 to I-0012: 165 and about 238 frames/s), slopes and stairs (TRTF, I-0013,
  additive branch; the decay branch unvalidated), Hollows, the early dog and Iudex Gundyr played at high rate (I-0012, OBSERVED).
- Never validated (I-0009, I-0010): attack and jump, the roll's invincibility frames and damage, stamina, everything beyond
  movement (C4: physics, collision, camera, effects, input).
- GENERAL (thin, forum-level): the engine family's known high-rate defects are movement and jump (Dark Souls II's jump cancels
  itself at 120), grass swaying faster; nothing DS3-specific documented.
- GENERAL (Thomas's lead, 2026-10-30 09:01): Armored Core VI (2023, FromSoftware's later engine) is the studio's first game with a
  120 frames/s option on PC, as caps of 30, 60, 90 and 120, with no unlimited choice found in the reports (TheFPSReview, 80.lv,
  Steam discussions). A possible static reference for how FromSoftware made its logic run above 60 (its executable is not on
  this PC; not copied, D-0240); it does not reach Thomas's « Unlimited » target.

## 4. Second pass: recurrences without dt (the terrain bug's kind)

[tools/I0112Recur.java](tools/I0112Recur.java) (headless on the mirror) walks back from every scalar float store to a field
(17,320) and keeps those whose chain reads the same field back through a constant or another field, with no multiply by a
register: 224 hits; 166 once 58 additions of FLT_EPSILON are set aside, in 66 functions (`recur-candidates.tsv`). Validated: it
finds TRTF's site (`1405E6580` at `1405E693E`, `mul 0.2`) and the 1/30 timer `140D71920`. Each store read by a Sonnet
`ghidra-research` agent (`recur-classes.tsv`, its judgments (one class a function, applied to that function's stores), written by [tools/I0112RecurClasses.py](tools/I0112RecurClasses.py), whose scratchpad input is gone (recovered 2026-11-07 from a removed worktree, kept as the record), and the decompiled functions in `recur-dec/`):
138 ONE_SHOT (weighted accumulations, matrix and blend math, UI
layout), 6 RECURRENCE_DT, 3 UNKNOWN, and **19 stores in 15 functions RECURRENCE_NO_DT** (STATIC; subsystems INFERRED, the call
once a frame checked only for the first three):

| Function | Subsystem (INFERRED) | Update, as decompiled | Law for any dt |
|---|---|---|---|
| `1405E6580`, `1405CA3F0`, `1405D2280` | character, terrain response (TRTF's field `+0x2DC` and two siblings) | `x += 0.2·resp`, `x *= 0.6`, clamp ±0.5 | `+= 0.2·resp·dt/dt60`, `x *= 0.6^(dt/dt60)` |
| `1405D0010` | character turning | angular speed `+= k·a` without dt (the later step does use dt) | `+= k·a·dt/dt60` |
| `1409B6D80` | character transform lerp | `t += 0.1`, clamp 1 | `t += 0.1·dt/dt60` |
| `140828FE0` | animation, bone constraint (10 callers) | `x += (target - x)·g` (gain's origin not traced) | `g' = 1 - (1 - g)^(dt/dt60)` |
| `141180110` | animation, bone weight | `w += (1 - w)·g` | same |
| `1402A5D60`, `1402AF510` | camera (guess) | `x *= 0.55` while a distance exceeds a bound | `x *= 0.55^(dt/dt60)` |
| `140D71920`, `140D736D0` | camera or AI target (guess) | timers `-= 1/30` | `-= dt` |
| `14050C7B0` | a tracker, mode 0 | `x += (target - x)·g` (mode 1 uses dt) | `g'` as above |
| `1406DE400` | input or UI hold timer (guess) | `-= 1.5` per call | `-= 1.5·dt/dt60` |
| `14100DDE0` | physics, broadphase margin | `x *= 0.953` (low interest) | `0.953^(dt/dt60)` |
| `140EF2CE0` | network poll timers beside `SteamAPI_RunCallbacks` | `-= 1/60` (low interest) | `-= dt` |

- **A correct DS3 example exists**: `1416E1680` (the post-effect library) smooths with `g = 1 - base^(dt/0.1)` and clamps by a
  bound times dt, its dt being `+0x694`, the very field the engine fills with a **fixed 1/60** (§1, `1416C9430`/`1416F27E0`):
  the adaptation's law is right, its input is not; at 120 frames/s it would adapt twice as fast. The fix is to pass the real dt.
  The native's own YEBIS port: where its time step comes from is to check (not found by a search of `native/`).
- **UNKNOWN**: `14050D0C0`, `14050E9C0` (camera, a ramp `+= int·rate` whose int comes from `1407FE4A0`, encrypted at rest),
  `1412D2FB0`.
- Limits: only straight-line chains (a recurrence split by a jump, as TRTF's decay branch, is missed); integer counters and
  vector (packed) updates are not covered. A second pass covers them in part ([RECUR2](RECUR2.md), 2026-10-30): one new site
  in the locomotion function `1405C72B0`, one multiplier to read at runtime (`14087E8A0`), two frame-count cadences, no
  packed recurrence of game state; TRTF's decay branch is still missed.

## 5. Not reached (open)

1. **The call frequency** of each RECURRENCE_NO_DT and FIXED_STEP site (once a game update, or less): a light runtime counter in
   a later live run settles it.
2. **Near constants: done, nothing new** ([tools/I0112NearConst.java](tools/I0112NearConst.java), `near-const-refs-clean.tsv`): within 1 % of
   1/60, 1/30 or 1/120 only two values are referenced: `0x3C888723` (0.016666, the three network poll timers of `140EF2CE0`, §4)
   and 0.008332161 (204 uses in 173 functions: a minimax sine coefficient, 1/5! adjusted, UNRELATED). Integer frame counters (`++`
   per update used as time) are not searched yet.
3. **The 42 UNKNOWN** in `const-classes.tsv` (three that do not decompile, nine unmapped hits, eighteen of undetermined meaning).
4. **Havok's stepping** and the callees of the constant-dt group: name their subsystems.
5. **The YEBIS `+0x694` consumer** and what the native YEBIS producers pass.
6. **How high the requested rate goes** (`+0x354`): 360 works (I-0088); « Unlimited » by a high target or by our loop alone.

## 6. Proposed plan for the unlock workstream (after R1 and the cadence, D-0240)

1. The rate option (the two sliders, « Unlimited ») sets the requested rate; the modern loop presents (D-0240).
2. Per site of §1, a native correction by a general dt law, behind a flag, each with a unit control at dt = 1/60 bit-exact with
   the original (the timing project's discipline, kept as a method).
3. Live comparisons at 30, 60, 120, 240 and unlimited per mechanic, against the original at 60: movement (sprint, roll, jump,
   ladders, falls), combat (attack timing, invincibility frames, stamina, poise), physics objects and ragdolls, cloth, effects,
   AI and bosses, camera and lock-on, menus and interface animations, eye adaptation.
