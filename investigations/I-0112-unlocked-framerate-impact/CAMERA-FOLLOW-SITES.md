# I-0114: the follow camera's laws and the 240 sprint's sideways drift (static study)

Study thread, 2026-10-31 (Paris, night), for the RT pilot's I-0114 (the 240 sprint's heading drifting sideways, its RESULT
« Locomotion at 240 » and « Series 6 »). Read-only: GhidraMCP on DarkSoulsIII.exe 1.15.2, byte scans of `game/DarkSoulsIII.exe`
(RTTI, call and RIP-relative references), the I-0112 p-code recurrence table (`evidence/I-0112/pcode-recur.tsv`) and
the timing project's `DS3-1152-F-008FA050` export. No run, no build. Filed under I-0112 (UNLOCK-SITES' companion) because
I-0114 is not on `main` yet; the I-0114 pilot may move it.

## Answer in short

**Update (01:50): the pilot's test ruled out §2 for this drift; the next candidates are in §5.**

The input's « forward » is the follow camera's yaw, and that yaw has one law that clearly depends on the frame rate: the
**camera's automatic wall recovery** (it turns the camera away from a wall it is about to touch). There an angular step
accumulates `dt` once per frame and is then applied once per frame: at 240 it ramps up four times faster, and it decays four
times faster once nothing is hit (§2). That would steer a straight sprint sideways near walls, and only in the frames where the
casts hit. **Decisive test**: run the same 240 path, and a 60 reference, with that option off (the byte `[14455BA10] = 0`, §2).
If the drift disappears, §2's correction applies. If it stays, look next at the focus smoothing (§3 A), whose Euler law
`30·dt·g` drifts from 60 by a gain-dependent amount. The other per-frame laws of the camera move the pitch, the distance or the
field of view, not the yaw (§3 B-D).

## 1. Where the camera is updated

- The game step's camera call (`FUN_1408FA050`, timing export) calls the **protected thunk `145187E57`** with `CameraMan`
  (`[step+0x68]`), the player and a context; its body sits behind the anti-tamper code and was not read. Then the same caller runs
  `140510240` (an FOV/blend copy from `CameraMan+0x68` or `+0x78`) and `140510330`.
- The classes (RTTI, `NS_SPRJ`): `CameraMan` (vtable `14279C7C0`), `ChrCam` (`14279D400`), **`ChrExFollowCam`**
  (`14279DAD0`), `ChrAimCam` (`14279CA00`), `SprjPersCam`, `FallDieCam`, `BallistaAimCam`. `140510330` reads
  `[CameraMan+0x60]+0x190` (the follow camera's distance) when `CameraMan+0x84 == 0` (the plain follow mode).
- **`FUN_140515790` is the follow camera's update** (4,158 decompiled lines). It takes `dt` in XMM1 and forms
  `k30 = dt / (1/30)` (`143D7ED50`), which is 0.5 at 60. Fields read in this study: `+0x140` pitch, clamped to
  `[+0x1FC, +0x200]`; `+0x144` yaw, wrapped to ±π; `+0x90` the smoothed focus point; `+0xD0` the focus; `+0xE0` the camera
  position; `+0x164` the « recentre » weight after a stick input (decays by `dt / [+0x1EC]`); `+0x190` the distance; `+0x2B0`
  lock-on.
- In plain follow (no lock-on, no stick), the yaw and pitch are **recomputed every frame from the direction camera
  position → focus** (`14050FDF0` and `14050FD30` on `+0xD0 - +0xE0`, about line 1806). The camera position follows the focus's
  displacement times `lerp([+0x1B4], 1, [+0x164])` and is then pulled back to its distance. That lag, a « towed trailer », scales
  with the displacement, not with time: frame-rate independent to first order.

## 2. The candidate: automatic wall recovery (`+0x304`, `+0x314`), lines 2974-3383

- **Gate**: the global byte `[14455BA10]` (read at `140517DC8`, `140517FEF`, `1405182C8`). Its only writer is `1405105A5`
  (`mov [14455BA10], al` from `[rdx+0x15]`, an options structure; INFERRED: the game option « Camera auto wall recovery », on
  by default in DS3). `+0x304` is cleared while the stick, lock-on or another mode drives the camera.
- **Casts**: two collision casts (filter 0x2F, `FUN_1407E7340`, or `1407E76B0` when `+0x3D5`) on either side of the camera,
  built from `+0x370`, `+0x368` and `+0x36C`. Their hit depths go to `+0x374` and `+0x378`.
- **The angular step `v = [+0x314]`, once per call**:
  1. On a hit on one side: `v += dt · [+0x324] · s`, or `v -= ...` on the other side; `s` is a 0-1 ramp of the hit depth, or a
     blend toward `[+0x334]` past `[+0x340]` (`140518728`, `140518796`, `140518837`, `1405188C4`). A step of the opposite
     sign is reset to 0 first (`140518701`).
  2. Then, when `+0x304` is set and `v != 0`: `v = clamp(c · v, ±dt · [+0x354])`, with `c = 1` on a frame with a hit and
     `c = [+0x360]` on a frame without one (load `14051890E`, multiply `14051893B`, the clamp `14051891A` / `14051892B`, store
     `140518966`). `|v| <= 0.0175` (one degree, `143D7ED40`) is snapped to 0 (`140518989`).
  3. The camera position is rotated about the focus by `v` (`140512D50`). If `140813D60()`, the yaw then gets
     **`[+0x144] -= v`** (`140518A6F` / `140518A77`) and the position is placed back at its distance.
- **Why it depends on the rate**: `v` is an angle applied **per frame**, but it grows by `r · dt` per frame
  (`r = [+0x324] · s`). After a contact of `T` seconds, `v = r · T` and the yaw rate is `v / dt = r · T / dt`: four times
  larger at 240 than at 60 for the same `T` (the ramp's slope in rad/s² is four times larger). The clamp `dt · max` caps the yaw rate at `[+0x354]` rad/s at every rate (that part is right). The decay
  `v *= [+0x360]` runs per frame, so four times more often at 240. A camera sliding along a wall therefore turns more sharply at
  240 at the start of each contact, and stops sooner after it. That bends the « forward » the sprint follows: a sideways drift
  that appears only near geometry, which fits a drift seen in some sprints and not in the walk (INFERRED).
- **Correction (the law at 60, at any rate)**: keep `v60`, the step per 1/60 s, with `k = dt · 60`:
  - the increment is unchanged: `v60 += dt · [+0x324] · s` (it is per second already);
  - the clamp becomes `±[+0x354] / 60`;
  - the decay becomes `v60 *= [+0x360]^k`;
  - the yaw step and the rotation of the position use `v60 · k`.
  
  Practically: pass `dt' = dt / k` (that is 1/60) to the clamp only, raise `[+0x360]` to the power `k`, and scale the
  applied angle (`140518A6F` and the `140512D50` rotation built from `+0x314`) by `k`. `140518966` stores the clamped value,
  which is the place to keep `v60`. A unit test: a constant contact of 0.5 s gives the same yaw at 60 and 240 within 1 %.

## 3. The other per-frame laws of `140515790` (for completeness)

| | Field (store) | Law | Frame-rate dependence | Weighs on the sprint's heading? |
|---|---|---|---|---|
| A | `+0x90..+0x9C` focus smoothing (line 1578) | `p += k30 · [+0x1A8] · (focus - p)`; `[+0x1A8]` itself ramps toward `[+0x1AC]` by `dt` (`140518BAF`) and is reset to `[+0x1B0]` on a wall-recovery frame | Euler: per second `(1 - g/2)^60` at 60, `(1 - g/8)^240` at 240; the gap is small for small `g` (about 4 % of the rate at `g = 0.2`) and large for `g` near 1 (30 %) | **yes, indirectly**: the yaw is the direction camera → focus, and a lag of the focus behind a turning character turns the yaw. Second candidate. Correction: `1 - (1 - g/2)^k` in place of `k30 · g` |
| B | `+0x140` pitch toward a target (line 2468, `[+0x288]`); pitch damping (line 2579, `[+0x1B8]`) | Euler `k30 · g` | as A | no (pitch) |
| C | lock-on: yaw and pitch `+= k30 · [+0x28C] · wrap(target - angle)` (lines 1995, 2174); offset `+0x2E0 += k30 · [+0x300] · (...)` (`140517486`) | Euler `k30 · g` | as A | only with a lock-on (not on the replayed path) |
| D | `+0x194 += 0.1 · (+0x198 - +0x194)` (`140515BBC`, `143D7ED70` = 0.1); `+0x198`, `+0x1FC`, `+0x294`, `+0x174`, `+0x50` lerped by `[+0x3D0]` toward the camera parameter row (`140515A9A`, `140515ACB`, `140515AF0`, `140515B90`) | per call, no dt | RECURRENCE_NO_DT if called once a frame: `1 - (1 - c)^k` | no (distance, pitch limits, field of view) |

The dt-proportional rates (`+0x2B4`, `+0x134`, `+0x130`, `+0x138`, `+0x164`, `+0x168` timers; the stick's yaw and pitch
`dt · rate`) are right as read. I-0112 RECUR2 listed `140515790` as « a dt-aware lerp, safe as read »: true of its timers,
not of its Euler smoothing (A-C) nor of the wall recovery (§2).

## 4. What would decide it (the pilot's runs)

1. **Wall recovery off** at 60 and 240 (byte `[14455BA10] = 0` after the options load, or the in-game option). If the 240 path
   then follows 60's, §2 is the cause.
2. Otherwise, a per-step row with the follow camera's `+0x144` (yaw), `+0x314`, `+0x304` and `+0x90` and the player's heading:
   whichever leads the drift (the camera's yaw or the character's facing) names the site. The camera object is
   `[CameraMan+0x60]` (INFERRED from `140510330`); `CameraMan` is `[step+0x68]` in `FUN_1408FA050`.

## 5. After the test (2026-10-31, 01:50)

- **§2 is ruled out** (the pilot's run, relayed by the coordinator): with the wall recovery off, the 240 path drifts the same
  way and the sprint ends at the same point. §2's law is still frame-rate dependent (a correction for later), but it is not
  this drift.
- **The next candidates**, between the input and the character's direction, for the pilot's per-frame log (camera yaw,
  character yaw):
  1. **The body twist** (`14050C7B0` in mode 0, `[+0x1C] += ([+0x18] - [+0x1C]) · [+0x20]` per call, no dt). `1405CFF00`
     writes `[tracker+0x1C] × 57.2958` (`143D7EFA0`) to the Havok behaviour variable **`TwistLowerRootAngle`** (name at
     `1427CCA00`). `1405F2680` does the same for `TwistUpperRootAngle` and `TwistMasterAngle` (`1427D1210`), with the gain
     `[+0x1D8]`. A lower-root twist turns the legs and thus the root motion's direction, so this can move the character
     sideways without changing its facing. The pilot's mode-0 correction (`0dff0891`) was in the wall-recovery test's build
     (`build-8`) and the drift stayed: either the twist is not the cause, or the behaviour graph filters the variable per frame
     itself.
     - **Where to read it without a hook**: `tracker = [[[[PlayerCtrl+0x348]+0x40]+0x30]+0x0]`. PlayerCtrl's slot 26 is
       `1405E58D0` (vtable `1427CF078`); at `1405E5FDA` it loads `rcx = [this+0x348]` and calls `1405C22F0` →
       `1405CFE60` (`[A+0x40]`) → `1405CFF00` (`[X+0x30]`).
     - In the tracker: `+0x1C` is the value (radians), `+0x18` the target, `+0x20` the call's gain (from `holder+0xC` when it
       is ≥ 0, else `holder+0x8`; `holder+0xC` is reset to -1 after each call), and `+0x58` the mode.
     - Check: `[X+0x8]` is PlayerCtrl and `[PlayerCtrl+0x8]` is the ChrIns. PlayerCtrl is `[ChrIns+0x50]` (INFERRED).
       EnemyCtrl (vtable `1427CCE08`, slot 26 `1405D1C60`) makes the same call.
     - The upper and master twists belong to the class `WalkTwist` (vtable `1427D1158`, slot 7 `1405F2680`); the instance's
       owner was not found.
  2. **The focus smoothing** (§3 A): the camera's yaw is the direction camera → focus.
  3. **The character proxy** with a fixed listener dt of 1/60 (`141530300`, `141530D80`, `141668BB0`, UNLOCK-SITES « physics »):
     contact and sliding resolved per call. Less likely if the walk passes the same walls without drifting.
- **How the log tells them apart**: the camera's yaw leaving 60's names (2); equal yaws with the velocity off the facing
  name (1); a gap that starts only at a wall contact names (3).

## 6. The turning controller (after the pilot's series 9, 2026-10-31, 02:40)

The pilot's series 9 (OBSERVED): the lower twist stays at 0 for the whole path (candidate §5.1 out). At 60 the character's
facing equals the camera's yaw on every leg. At 240 it settles 0.04-0.05 rad beside it and stays there, while the camera, which
follows the character, keeps turning. The facing does not converge. Static reading of the controller:

- **The chain.** `1405D0010` has one caller, `1405CFC10(X, dt, target, rate, ...)`, and `X = [[PlayerCtrl+0x348]+0x40]`. The
  helper `M = [PlayerCtrl+0x348]` has `M+0x20 = PlayerCtrl`, and `X+0x30` is the twist's holder (§5).
- **`1405CFC10`'s three callers.**
  - `1405C2760`, a method of `M`: the plain turn.
  - `1405C29B0`, a method of `M`: rate from `FUN_1405C2570`, a branch gated by `1405C6020` and the bounds
    `[M+0x6C]`/`[M+0x70]` (INFERRED: lock-on or a facing mode).
  - `1405D3BC0`: the enemies' variant.
- **The state** (`param_6 = X+0x10`, rewritten each call): the rotation (yaw at `X+0x14`), the angular speed `X+0x20`, the last
  `|error|` `X+0x24` and the sign `X+0x28`. Then the coefficients `X+0x3C..0x44`, copied each call from
  `[module[0]+0x1A8/0x1AC/0x1B0]` (with `module = [ChrIns+0x1F90]`) while `X+0x80 == 0`, and the enable byte `X+0x48`. With
  the byte at 0, the target is copied into the state and the speed is zeroed.
- **rate** (`1405C2760`, `1405C24E0`): `[M+0x28]`, replaced by `[module[0x17]+0x16C]` if ≥ 0, then by `[module[0]+0x1A0]` if
  ≥ 0, times π/180. A rate in degrees per second, from the animation (INFERRED TAE values, -1 when absent).
- **The target** (`1405C25F0`): the current facing (`1405C5460`) plus the wanted delta (`1405C2400`), the delta clamped to
  `rate · dt` (`1405C24E0` through `14034DE90` on the time object). In `1405C29B0`'s branch, the delta is zeroed when
  `[ChrIns+0x1070]` is set and `|delta| > [module[0]+0x1A4]` degrees. No other step copies the facing onto the target
  (`1405CFB90` copies the rotation into the state when the turn is bypassed).
- **The law** (`1405D0010`):
  1. `err = wrap(target.yaw - X.yaw)`. If the sign changed, `speed = 0`.
  2. `speed = min(speed + rate · c0, rate · c1)`, once per call; the pilot's correction makes the increment `rate · c0 · k`.
  3. `step = speed · dt`; if `|err| <= step`, snap to the target and set `speed = 0`.
  4. The step is applied only if `|err| <= X+0x24` (the error did not grow) or `|err| >= rate · c2`. Otherwise `out[9] = 1`
     and `speed = 0`.
- **Why it freezes at 240** (INFERRED, the pilot's log of `out[5]`, `out[9]` and the step will tell). After a reset, the first
  step is `rate · c0 · k · dt` (∝ dt²), while the camera, following the character, grows the error by `ω · dt` (∝ dt) a frame.
  The growth test passes if `ω <= rate · c0 · k`: `rate · c0` at 60, `rate · c0 / 4` at 240. Below `rate · c2` each failure
  zeroes the speed again, so the residual stays. Without the ×k correction the threshold equals 60's, but the drift was already
  there in series 5: either another source, or the bound `rate · c2` also matters. Read `c0`, `c2` and `rate` at the entry.
- **Proposed correction**: keep the ×k ramp, but judge the error's growth over 1/60 s. Either compare with the error kept from at
  least 1/60 s earlier (update `X+0x24` every `1/k` calls), or, equivalent to first order, test
  `|err| <= prev + previous step · (1/k - 1)`. Both are bit for bit the original at 60 (k = 1). Unit test: a 0.045 rad error
  under the bound, with a target turning at `ω` between `rate · c0 / 4` and `rate · c0`, converges at 60 and at 240.

## 7. The wanted delta (after series 10, 2026-10-31, 03:20)

Series 10 (OBSERVED by the pilot) refutes §6's freeze. On every player call, `out[9] = 0` and `out[8] = 0` (23 exceptions);
`rate` is 12.566 rad/s, `c0 = 1` and `c2 = 0`, at 60 as at 240. The facing always reaches its target; **the target differs**.
At 60 the facing follows the camera's yaw within a leg. At 240 it stays fixed for a leg, then moves the other way, and in a
long sprint it stays at π - 0.045 while the yaw climbs from -0.035 to +0.33. Static reading of the target's source:

- **PlayerCtrl slot 26** (`1405E58D0`). When `FUN_14088CA90(chr)` is true (the player), it calls
  `1405C21B0(M, out, time, param_3+0x10)`. `param_3+0x10..0x1C` is the wanted delta (angles). `1405C2760` then calls
  `1405C25F0`: target = current facing (`[[ChrIns+0x1F90]+0x68]+0x70`, the pilot's « physics +0x74 » for the yaw) + delta,
  with the delta clamped to `rate · dt`. When the byte `param_3+0x30` is set, the target is facing + delta directly, without the
  turn.
- **The PadManipulator** (vtable `1427CE4E8`) updates in `1405DF000`. For the player it runs `1405E1B80(dt)`, `1405E1150(dt)`,
  `1405E1940` and `1405E08F0(dt)`.
  - `1405E1B80`: without lock-on (`[PlayerCtrl+0x2C1] == 0`) it calls **`1405E2810(manip, dt, PlayerCtrl, input)`**, passing
    `[ChrIns+0x1080..0x108C]` (the lock-on target's position, written by `1408651E0`). With lock-on it calls `1405E2020`.
    **`1405E2810` is encrypted at rest** in the executable (its bytes `73 D2 90 B1 3B DB EE B3 ...` are not code; Ghidra cannot
    decompile it). It takes `dt` and most likely computes the wanted delta (INFERRED: into `[manip+0x20..0x2C]`, which
    `1405E1150` adds to the current rotation).
  - `1405E1150`: the world target `[manip+0x20]` + the character's rotation (`1408893A0`), latched into `[manip+0x2F0]`
    while the timer `[manip+0x3F0]` is under its bound (a dt latch).
  - `1405E08F0` → `1405E34D0`: the stick's history in a ring of N frames (`[manip+0x34C]` / `[+0x39C]`, with N `[144569CF8]`
    and the index `[manip+0x3EC]` incremented every frame). A frame-counted term, but for flick detection (steps, rolls), not
    for the facing.
- **Confirmed**: PlayerCtrl is `[ChrIns+0x50]`. In `1405E1B80`, `[[manip+0x2D8]+0x50]` has `+8` equal to the ChrIns and
  `+0x2C1` equal to the lock-on flag.
- **What would decide it**: either dump `1405E2810`'s decrypted bytes from the running game (about 0x900 bytes after its first
  call in free movement) for a disassembly, or log the delta `param_3+0x10..0x1C` at PlayerCtrl slot 26's entry beside the
  camera's yaw and the facing, every frame, at 60 and 240.

## 8. `1405E2810` decrypted (2026-10-31, 03:40)

The pilot read the decrypted bytes in a running game (run `i114-loaded-f60`, 2,560 bytes, three identical reads). Decompiled
with [I0114LoadedDecompile.java](tools/I0114LoadedDecompile.java) on the read-only mirror:
[decompiled/DS3-1152-F-005E2810.loadedi114f60.c](../../decompiled/DS3-1152-F-005E2810.loadedi114f60.c), with its listing. The
body ends at `1405E2C22`, inside the acquisition, so 2,560 bytes are enough.

- **No dt and no per-frame term.** `param_2` (dt) is not read. The function is a pure map from the stick, the manipulator's
  reference rotation `[manip+0x90..0x9C]` and the current facing:
  - `[manip+0x80]` gets the raw stick pair; `[manip+0x320]` / `[+0x328]` get the negated stick.
  - Free path (`[ChrIns+0x1070] == 0` and `param_6 == 0`): `[manip+0x40] = [manip+0x90]`, and **the wanted delta is
    `[manip+0x20..0x2C] = wrap([manip+0x90..0x9C] - facing)`**, with the facing from `1405C5460`.
  - Lock or target path: the delta points the facing at the target position `param_5` (`[ChrIns+0x1080]`).
  - In both paths the world move vector `[manip+0x10..0x18]` (copied to `+0xA0`) is the stick rotated by the yaw
    `[manip+0x94]` (`FUN_14006C730`, the sine and cosine).
- **Who writes `[manip+0x90]`**: `FUN_140510290(CameraMan, chr)` writes `(pitch, yaw, 0, 0)` taken from CameraMan's forward
  vector `+0x30..0x38` (`atan2`) into `FUN_14088A320(chr)+0x90`. `FUN_14088A320` returns the manipulator,
  `[[ChrIns+0x50]+0x318]` or else `[[ChrIns+0x50]+0x110]`. It is called once, at `140510DD6`, in the plain follow branch of
  CameraMan's update: after the follow camera's update and the copy of its view into CameraMan. That code sits behind the
  protected entry `140510440` (`jmp 145187E57`) and is not a Ghidra function. No other static writer of `+0x90` / `+0x94`
  was found in `1405A0000..140620000`, `140500000..140520000`, `1407F0000..140810000` or `140860000..1408A0000` (other
  classes' `+0x90` aside).
- **So**: the facing's target is the camera's current view yaw, copied in once per camera update, and the drift needs that copy
  or its consumer to lag or freeze at 240. Neither `1405E2810` nor `140510290` integrates anything. What remains to know is
  **how often each one runs** and in what order:
  - the camera update (`1408FA050` → the protected `145187E57`);
  - the manipulator's update (`1405DF000` → `1405E1B80` → `1405E2810`);
  - PlayerCtrl's turn (slot 26).
  
  If the manipulator or the camera runs at a fixed step while PlayerCtrl runs every frame, the frames between two updates
  reuse a delta taken against an older facing, or one reset to 0 (`1405CC460` clears `+0x20`).
- **What to log** (per frame, 60 and 240): `[manip+0x90..0x94]`, `[manip+0x20..0x2C]`, CameraMan `+0x30..0x38` and the follow
  camera's `+0x144`, the facing, and a count of calls of `140510290` (plain code, hookable) per game step. `1405E2810` is
  encrypted at rest: count it through its caller `1405E1B80` rather than at its entry.

## 9. The view against the yaw: a per-frame dead zone (after series 11, 2026-10-31, 04:00)

Series 11 (OBSERVED by the pilot):
- `140510290` and `1405E1B80` run once per game step at both rates.
- The facing follows CameraMan's forward within 0.001 rad, and the wanted delta is ≈ 0.
- **CameraMan's forward and the follow camera's yaw `+0x144` separate at 240.** The mean gap is 0.0015 rad at 60 and
  0.050-0.085 rad at 240 (maximum 0.015 against 0.17-0.20), in walk and sprint alike.

Static reading (lines of the full decompile of `140515790`):

- **The view is not the yaw.** CameraMan copies the follow camera's view matrix `+0x10..0x4F`. At the end of `140515790`
  (lines 3398-3778), the forward row `+0x30 = normalize(focus +0xC0 - P)`. The final position P is also stored at `+0x40`,
  and at **`+0x100` for the next frame**. P = focus + normalize(Pprev + w·(Porient - Pprev) - focus)·distance, where:
  - Pprev = `[+0x100]`;
  - Porient = `[+0xF0]`, the position wanted by the orientation `+0x140/+0x144`, moved every frame by the focus's displacement
    (lines 1776-1786);
  - w = (`[+0x1BC]`, `[+0x1C8]`, `[+0x1BC]`), or (`[+0x1D8]`, `[+0x1DC]`) under lock or aim. `[+0x1BC]` ramps toward
    `[+0x1C0]` by dt and restarts from `[+0x1C4]` on a wall-recovery frame.
- **The horizontal dead zone** (lines 3524-3554). With g the angle, seen from the focus, between Pprev and Porient:
  - g below a1 = `[+0x1CC]` (`[+0x1E0]` under lock; loaded into xmm9 at `140518CA5`): **wx = wz = 0**, the position does not
    follow horizontally at all;
  - between a1 and a2 = `[+0x1D0]` (`[+0x1E4]`, kept ≥ a1): wx and wz are scaled by t = (cos g - cos a1)/(cos a2 - cos a1);
  - beyond a2: full weight.
  
  Then w += `[+0x130]`·(1 - w), where `+0x130` is the recentre timer after a stick input. The step w·(Porient - Pprev) is
  applied **once per frame, without dt**, and the dead zone is judged on a frame-to-frame gap. While g stays under a1, the view
  does not turn horizontally although `+0x144` does, so the gap view/yaw is bounded by a1. How often the gap passes a1, and the
  catch-up law, both depend on the frame rate.
- **The writers of `+0x144`.**
  - Line 1825, automatic mode (no stick, no lock, `+0x2B2 == 0`): `+0x140/+0x144` = direction (focus `+0xD0` - position
    `+0xE0`), every frame.
  - Lines 2263-2282, on a stick input: direction (`+0xD0` - `+0x100`).
  - Lines 2374 and 2431: the stick's rotation, dt × rate.
  - Line 2080: lock.
  - Line 3330: wall recovery.
  
  In a walk without the stick, `+0x144` follows the lazy position `+0xE0` (moved by the focus's displacement ×
  `lerp([+0x1B4], 1, [+0x164])`, line 1758, then pulled back to its distance). There is no separate « behind the character »
  term.
- **The focus smoothing** (§3 A, `+0x90`, Euler) enters the focus `+0xB0/+0xC0/+0xD0`, so it enters both the yaw (through
  `+0xD0`) and the view (through `+0xC0`).
- **What would decide it** (per frame, 60 and 240): `[+0x1CC]`, `[+0x1D0]`, `[+0x1BC]`, `[+0x1C8]`, `[+0x130]`, `+0xF0`,
  `+0x100`, `+0xC0`, and g (or cos g) with the factor t applied. If g stays under a1 at 240 while `+0x144` turns, it is the dead
  zone.
- **The correction then**: the catch-up as `1 - (1 - w)^k`, and the dead zone judged on the gap accumulated over 1/60 s
  rather than frame to frame.

## 10. The per-frame chain of the follow camera (after series 12, 2026-10-31, 04:30)

Series 12 (OBSERVED by the pilot): a1 = 0.1745 rad (10°), a2 = 0.3491 rad (20°), w_h = 0.100, w_v = 0.030, and the recentre
timer `+0x130` stays at 0, at both rates. g (seen from the focus `+0xC0`, between `+0xF0` and `+0x100`) averages 0.0016 at
60 (at most 0.016, never a1), and 0.053-0.079 at 240 (a1 reached on 65 of 1,168 rows). The dead zone holds the view, so the
facing, while `+0x144` turns. The cause is upstream: why F0 leaves the view position frame to frame at 240.

The chain, in `140515790`'s order, in automatic mode (no stick, no lock):

0. Origin shift (lines 1269-1361): `+0x40` and `+0x90..+0x10C` (so E0, F0 and `+0x100`) += `[+0x110..0x11C]`·dt. Dt-scaled
   and equal for all.
1. **Focus base** `+0x90` (line 1578): `+0x90 += k30·[+0x1A8]·(target - +0x90)` (Euler; `[+0x1A8]` ramps toward `[+0x1AC]`
   by dt and restarts from `[+0x1B0]` on a wall-recovery frame).
2. The focus `+0xB0` is `+0x90` plus the offsets `+0x170..+0x178` along the character's axes (lines 1690-1729). Then
   `+0xC0 = +0xD0 = +0xB0` (lines 1744-1749).
3. **Trailers** (lines 1751-1786): with W = `lerp([+0x1B4], 1, [+0x164])` and Δ this frame's displacement of `+0x90`,
   E0 += Δ·W and F0 += Δ·W. Linear in displacement.
4. **Automatic orientation** (lines 1806-1827, branch `+0x2B2 == 0`): (`+0x140`, `+0x144`) = direction(`+0xD0` - E0).
5. **Pitch damping** (lines 2572-2583): `+0x140 = +0x160 + wrap(+0x140 - +0x160)·[+0x1B8]·k30`. It multiplies one frame's
   change by 30·dt·g, so it is frame-rate dependent, but it acts on the pitch only.
6. E0 is rebuilt from (pitch, yaw) at its distance from `+0xD0` (lines 2585-2663).
7. **Distance**: `[+0x190]` is recomputed from the focus's speed when `[+0x180] > 0`, then
   E0 = `+0xC0` + normalize(E0 - `+0xC0`)·`[+0x190]` (lines 2664-2725).
8. Collision of E0 (lines 2740-2813), only with the wall recovery off.
9. **F0** = `+0xC0` + normalize(E0 - `+0xC0`)·`[+0x194]` (lines 2814-2831): F0 has E0's yaw, so `+0x144`'s, every frame.
10. Wall recovery (lines 2974-3383), with the option on.
11. **View P** (lines 3398-3655), §9: P = `+0xC0` + normalize(Pprev + w·(F0 - Pprev) - `+0xC0`)·d.
    - w has the dead zone, applied once per frame without dt.
    - d = lerp(|F0 - C0|, |P' - C0|) by `[+0x1D4]`, also per frame without dt (line 3602).
    - Then `+0x100 = +0x40 = P`.

Under the dead zone (always at 60), P is a pure trailer: its horizontal direction from the focus is Pprev's, which stays put in
the world while the focus moves. E0 and F0 form a trailer moved by W·Δ and re-projected at `[+0x190]`. Both laws are
geometric and frame-rate independent to first order; the per-frame terms (pitch, the vertical weight, the distance blend) do
not touch the horizontal yaw except through the 3D re-projection. **To decide**: log E0, D0, C0, F0, `+0x100`, `+0x164`,
`+0x1B4`, `[+0x1A8]` and Δ`+0x90` per frame, with the yaws (seen from the focus) of E0 just after step 3 and of Pprev:
- W differing between the rates names `+0x164`;
- an equal W with diverging yaws names the Euler focus (through Δ and `+0xC0`).

## 11. The view position is a pure trailer at both rates (after series 13, 2026-10-31, 05:00)

Series 13 (OBSERVED by the pilot):
- W = 0.3 at both rates (`[+0x1B4]` = 0.3; `[+0x164]` is 0 while walking and 1 after an input), and the focus gain
  `[+0x1A8]` = 0.1 at both rates.
- Within a leg toward the camera (43-47 s), seen from the focus: at 60, E0 goes from -0.011 to -0.027 and P follows 0.004-0.007
  behind; at 240, E0 goes from -0.010 to -0.033 while P stays at -0.005.

The study thread's reading of the same rows (`evidence/I-0066/artifacts/i114-trail-*/startup.jsonl`, `unlock_follow_view`,
one row every 15 frames):
- |ΔP| ≈ |ΔC| at both rates: P moves with the focus.
- The focus's lateral motion against the axis focus → P:
  - at 60, -0.0005 to -0.0024 per row between 45 and 46.25 s, while P's yaw slides from -0.0068 to -0.0222;
  - at 240, 0.0000 to 0.0002 per row, while P's yaw stays at 0.0172 to four decimals.

  That is a trailer with its distance re-imposed every frame: when the focus moves along the axis, P backs off along the same
  axis and its yaw does not change.

Re-read of lines 3398-3655 against the pilot's three leads:
- **(a)** g is the **accumulated** gap between the current F0 and the last view `+0x100` (after the origin shift), not a
  per-frame increment (§9's wording corrected here).
- **(b)** No other horizontal path moves P under the dead zone.
  - The distance blend by `[+0x1D4]` and the vertical weight 0.03 change only length and height: normalizing to d keeps the
    horizontal direction.
  - `[+0x130]` is 0.
  - The origin shift `+0x110`·dt comes from WorldChrMan`+0xA0`, which CameraMan copies and clears at `1405106C0`. Only
    `14089B560` writes it, when the character stands on a moving object (that object's velocity), so it is ≈ 0 at Firelink.
- **(c)** P restarts only from `+0x100`, written at line 3872 alone (plus the origin shift).

**Interpretation** (INFERRED): at 240 the loop locks. The facing is the view, the character runs exactly along the view's axis,
the trailer P is pushed straight and its yaw never changes, so the view and the facing stay fixed while E0 (`+0x144`) diverges.
E0 is a trailer pushed by a focus moving toward it, which is unstable (a jackknife whose gap grows with the distance run). At
60, a small lateral motion of the focus turns P toward E0, and the view and the facing follow.

**The open question**: where the lateral motion at 60 comes from and why it is missing at 240.
- **The character's velocity off its facing at 60**: compare Δ`at` with phys70's yaw in `unlock_player_position`, at both
  rates (root motion, or a turn applied one frame late).
- **The facing one frame behind the view**: the camera → manipulator → turn order within the step; one frame is four times
  more angle at 60 than at 240 while the view turns.
- **A lateral focus offset**: `+0x170..0x178` along the character's axes (lines 1640-1699), turned with the facing.

## 12. The order within a game step (2026-10-31, 05:30)

The pilot's working hypothesis (series 13 rows, OBSERVED): at 60 the facing trails the view by 0.0003-0.0005 rad while the view
turns, and the character's velocity is 0.001-0.002 rad off the axis; at 240, facing - view is about 1e-5. Both trailers are
unstable when the character walks toward the camera, and the facing's lag behind the view, counted in steps, starts P's swing.
The test in progress is `view-delay-n.flag` (n = 3 and 6 at 240).

Static facts on the order:
- **The frame table** (ENGINE-STEP-THREAD-MAP §2): PadStep (row 5), **MoveMapStep 0x1B (row 16)**, ChrIns_PreBehavior 0x10
  (23), HavokBehavior (25), ChrIns_PrePhysics 0x15 (27), the Havok world (about 35), ChrIns_PostPhysics 0x3B (50),
  ChrIns_UpdateDraw (56), **CameraStep 0x43 (59)**, DrawStep (79).
- **The camera update** `1408FA050` has no direct caller. The only jump to it is from `1408F99B0` (at `1408F9AC2`), which has no
  direct call, jump or raw pointer in the executable: it is entered through protected code, so its step is not proved.
- **The evidence for MoveMapStep**: `1408F99B0`/`8FA050` lie in MoveMapStep's code range (`8F2xxx-8FCxxx`), and their
  `param_1` has the MoveMapStep object's fields `+0x58`, `+0x60` and `+0x68` (CameraMan). MoveMapStep's InGame update
  `1408F2CA0` also uses `param_1+0x60`, and calls a protected thunk `144F8111C(_DAT_144769240, dt)`.
- **INFERRED order in step N**: the camera, inside MoveMapStep (row 16), computes the view from the character's position at
  the end of step N-1 and writes `manip+0x90`. Then, in the ChrIns rows (23-27), the manipulator reads it and the turn runs, and
  the physics applies the move. So the facing's target has no lag on the view, while the view lags the character's position by
  one step. If the camera ran in CameraStep (row 59) instead, the target would lag the view by one step.
- **What would decide it**: a global counter stamped at the entry of `140510290`, `1405E1B80` and PlayerCtrl slot 26
  (`1405E58D0`) within one step.
- **The delay to aim for**: 16.7 ms is 4 steps at 240, so with a native lag of 1 step, 3 steps are added; with no native lag,
  4 are added.

## 13. The fixed-step camera: what to save, the side effects, the copy chain (after series 15, 2026-10-31, 06:00)

Series 15 (OBSERVED by the pilot): with `camera-step-4.flag`, `140515790` runs on one call in four with dt × 4, the others
returning at once. Both 240 runs then end at (269.97, -55.63, 611.65), 0.3 m apart and within 2 m of 60, with normal sprints
(20-23 m). **The drift is in the follow camera's per-call laws.** The pilot's fix: camera logic on a fixed 1/60 step (an
authoritative state S60), with the display view extrapolated each frame, and the facing's target taken from S60's view.

Static facts for that fix:
- **The object**: 0x440 bytes, 16-aligned, allocated in ChrCam's constructor `14050F980` (`FUN_141777A00(0x440, 0x10)`, then
  the constructor `1405121F0`), stored at ChrCam`+0x60`.
  - The ChrCam (vtable `14279D400`; the « CameraMan » of the earlier sections) also owns `+0x68` ChrAimCam (0x1C0 bytes),
    `+0x70` (0xB0) and `+0x78` (0xE0).
  - The follow camera's vtable is `14279DAD0`, its destructor `1405130F0`. The highest field `140515790` touches is `+0x434`.
  - **`+0x3D8` is an embedded CSCamFadeSlot** (destructor `FUN_140989510(this+0x3D8)`), spanning `[0x3D8, 0x430)`.
- **Side effects outside `this`**: no direct store (every store in the 4,158-line decompile is to `this` or the stack). The
  callees are math, getters and read-only collision queries (`1407E7340`, `1407E76B0`, `1407E8600`). The exception is the fade
  slot, both blocks gated by the byte `+0x3D5`:
  - `FUN_140989810(this+0x3D8, hit object)` (lines 2912-2930, also needs `param_4` ≠ 0) may allocate an instance on the heap
    through the slot's vt`+0x30`;
  - the slot's vt`+0x18` (`140303810`, lines 2965-2971) advances the instances once per call with dt (INFERRED: they write
    the fade on their targets).
- **For a save and restore**:
  - save `[0x10, 0x3D8)` and `[0x430, 0x440)`, **never** the slot's `[0x3D8, 0x430)` (list and heap pointers);
  - for display calls pass `param_4 = 0` (no casts: no E0 collision, no wall recovery, no fade registration) and clear
    `+0x3D5` for the call (no slot update), so the fade advances only on the 1/60 tick.
  - The cost: the extrapolated view ignores the camera's collision between two ticks.
- **The copy chain** (ChrCam update, behind the protected entry `140510440`, follow branch):
  1. `140515790` at `140510DC7`;
  2. **`1405103E0(ChrCam, follow)`** at `140510DD3` copies `+0x50..0x5C` and the matrix `+0x10..0x4F` (through `1404095F0`);
  3. **`140510290`** at `140510DDE` writes `manip+0x90` from ChrCam`+0x30`;
  4. `1408FA050` then copies ChrCam's `+0x50..0x5C` and matrix into **camera slot 1, `FUN_14050D6D0(1)` =
     `[[_DAT_144755E98]+0x10]`**, the camera the game hands to the renderer (INFERRED: the render-side reader was not read).
  
  So an extrapolated follow view reaches the renderer through the normal copies. The facing's target needs `140510290` (plain
  code) to read S60's `+0x30` row on display frames.
- `140515790` also has callers in ChrCam's other mode branches (the blends `14050E9C0` / `1405101D0`, around
  `140510D83-DA8`), so a hook belongs at its entry, not only at `140510DC7`.

## Open

- The protected `145187E57` (CameraMan's update) was not read: whether it calls `140515790` once per game step, and with which
  dt (INFERRED: the frame's dt, as the other camera code). Its other work (mode switches, the aim camera) is not covered here.
- The gains `[+0x1A8]`, `[+0x288]`, `[+0x28C]`, `[+0x324]`, `[+0x354]`, `[+0x360]` come from the camera parameter row (`LockCamParam`
  INFERRED) and were not read: they set how large each dependence is.
