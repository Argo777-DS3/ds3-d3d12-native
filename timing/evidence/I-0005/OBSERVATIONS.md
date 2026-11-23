# I-0005 observations — static, runtime Pass A, and runtime Pass B

All observations apply to `DarkSoulsIII.exe` 1.15.2.0, Steam BuildID `10167187`, SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`. Static work did not launch the game. Runtime Pass A later launched it offline under authorized observation-only instrumentation. Runtime Pass B used separately authorized reversible in-memory writes. The installation was not modified.

## Runtime Pass A provenance

- Session `20260919T083240Z-pid12832` captured 239.092487 seconds from the first through last broad update.
- All eight hooks reported `MH_OK`; the `COMPLETE` marker confirms a clean stop and flush.
- The installation contained 89 files totaling 26,649,084,250 bytes and no checked local D3D11/DXGI proxy or graphics mod.
- Process module snapshots contained Windows `dxgi.dll`/`d3d11.dll`, Steam's standard overlay, and the declared probe. No ReShade, NUE, RTSS/RivaTuner, Special K, Game Bar, or NVIDIA Overlay module was present.

## Live broad update and central relation

- `00D57900`, `00023410`, and `000499C0` were each observed 14,222 times in complete ordered triples; all triples forwarded exactly the same scalar.
- The scalar median was `0.0166666005` seconds; p99 was `0.016669192048`; maximum was `0.0490661301`.
- The immediately preceding central `after` snapshot field `+0x268` matched the next broad delta within `1e-7` in 14,067/14,221 aligned pairs. Median ratio was 1.0 and Pearson correlation was 0.7969.
- Other sampled current/history fields frequently equaled `1/60` but had weaker correlations or were constant. Equality under a nearly constant stock workload is not treated as source proof.

## Live presentation and selector

- The active window contained 14,221 Presents; all used `SyncInterval=1` and returned success. Median interval was 0.0166659 seconds.
- Every corresponding `00D5E30` observation had `requested=1`, `active=0`, `mode=60`, and returned `1`. The manual QPC wait branch was inactive.
- `00DD4B60` ran 8,644 times on one object. Selector index was always `0`, and output equaled dynamic input within `1e-7` on every call.

## Live callback targets

- The first virtual callback vector at `000499C0` was nonempty on 9,242 update rows and contained only two targets across the capture.
- `000EFDA0`: 871,628 calls across 344 object addresses. A later calling-convention reinspection corrected the initial interpretation: at `0x1400EFDB6` the function changes only `RCX`, leaves incoming `XMM1` intact, and calls `002124A0`; it therefore forwards the delta before its transform/matrix work.
- `000F0D40`: 53,932 calls across 14 object addresses. Static recovery shows the forwarded delta retained and passed to `002124A0`.
- `002124A0` accumulates its input, derives a normalized/interpolated value over a selected interval/range, and writes local state. Its high-level owner is not established.
- The direct `0017BE20 → 0021CC20` path from `000499C0` has elapsed/duration/progress arithmetic. Pass A did not instrument its internal activation flag, so a live active transition is not claimed.

## Central timing object

- `DS3-1152-F-00ED8E50` and `DS3-1152-F-00EDAAE0` allocate `0x368` bytes, invoke `DS3-1152-F-00EED360`, and store the pointer at global `0x14489DD10`.
- `00EED360` seeds QPC timestamps at `+0x20..+0x58`, initializes several current/history values to IEEE-754 `1/30`, writes `30.0` at `+0x354`, and clears the byte at `+0x358`.
- `00EEE100`, `00EEE130`, `00EEE160`, `00EEE190`, `00EEE1C0`, and `00EEE1F0` take QPC samples into six phase fields. Their callers cluster around `00D56780`, `00D56930`, and `00D56A90`.
- Readers consume at least `+0x268`, `+0x26C`, and `+0x2BC`. The minimum offset map is in `static/central-timing-state.csv`.

## Protected producer boundary

- Both allocation paths call the thunk at `0x140EED810`, whose destination is `DS3-1152-F-04E79363`.
- That entry manipulates the stack and transfers control by `XCHG`/`RET` into fragments at `0x144F382D4` and other distant addresses. A transaction-rolled-back transient disassembly found mixed fragments and opaque continuations, not a defensible single function body.
- Ghidra decompilation of nearby apparent functions produces bad-data/overlap warnings. Those heuristics are not promoted to facts.
- Therefore the arithmetic that updates the live delta fields cannot be reconstructed reliably with the present static method. This is a concrete tool/protection boundary, not evidence that no producer exists.

## Broad update scalar

- `DS3-1152-F-00D5B140` forwards the same context object to several update branches.
- `DS3-1152-F-00D57900` loads a float from `context+0x08` into `XMM1` and calls `DS3-1152-F-00023410` without altering that value.
- `00023410` preserves `XMM1`, calls a profiling accumulator, then passes the same float to `DS3-1152-F-000499C0`.
- `000499C0` passes the float to a vector of virtual update callbacks and explicit subsystem routines, then uses it to advance four scaled accumulators at `+0x750..+0x75C`.
- This proves a broadly distributed update scalar. Static evidence does not yet prove its units, producer, runtime cadence, or that every important subsystem uses it.

## A distinct fixed/dynamic selector

- `DS3-1152-F-008F2CA0` reads central timing field `+0x26C` and passes it as a wrapped value to `DS3-1152-F-00DD4B60`.
- `00DD4B60` constructs three 16-byte stack entries containing the dynamic input, `1/30`, and `1/60`. It loads index `object+0x40`, scales it by 16, and writes the selected float to `object+0x38`.
- `DS3-1152-F-00889BC0` returns `object+0x38` in the same small wrapper type; nearby consumers gate specialized object/resource calls on the value being positive.
- This is a proven selectable timing mode in a specialized subsystem. No static flow connects its output to the broad `000499C0` update distributor, so it is not treated as the engine's global timestep.

## Meaningful `1/60` uses

- I-0004's `/60` is presentation pacing and remains separate.
- `00DD4B60` contains a genuine dynamic/`1/30`/`1/60` selector.
- `DS3-1152-F-0052FBF0` clamps a resource/timeline position against an end value minus `1/60`; `006664F0`/`006665C0` form a related step/seek branch. This is a local boundary behavior, not a time producer.
- Other raw matches occur in constructors, configuration, geometry, and unrelated code. The hypothesis that constant search alone locates the 60 FPS limiter is refuted.

## Historical lead, not canonical proof

The external project `0dm/DS3DebugFPS` at master commit `b9e5a8ca8390ed0fd545b33944103c53926b7720` writes a requested FPS float at a discovered object's `+0x354` and an enable byte at `+0x358`. Those offsets exactly match the current-build constructor defaults. This independently supports the timing-controller interpretation and identifies a useful runtime discriminator. It does not prove which current-build domains are affected, nor gameplay correctness above 60 FPS.

Source: `https://github.com/0dm/DS3DebugFPS`, `src/dll/dllmain.c` lines 118–148, retrieved 2026-09-19.

## Runtime Pass B causal comparison

- The primary reversible sessions were `20260919T094057Z-pid12600` (60), `20260919T094304Z-pid19080` (90), and `20260919T094516Z-pid27388` (120).
- Each session recorded `30.0/0 → target/1` at central `+0x354/+0x358`, then restored exactly to `30.0/0` with `success=1`.
- No game-directory file was changed. `force_present_zero` was zero in every run.
- Measured update rates were 59.420, 89.116, and 118.538 Hz; median deltas were `0.0166666005`, `0.0111111002`, and `0.00833330024` seconds.
- Update and Present counts were exactly equal within each primary active window: 4,919, 5,653, and 10,692.
- After the transition, 99.96–99.98% of active-window Presents used `SyncInterval=0`; two startup/transition calls per session retained `SyncInterval=1`.
- I-0004 manual pacing remained inactive in all primary sessions.
- `00DD4B60` selected index 0 and returned its dynamic input exactly on all 2,510, 3,088, and 4,964 observed calls.
- Central `+0x268` matched the following broad delta within `1e-7` for 98.96%, 98.76%, and 98.58% of aligned pairs.
- G-Sync was enabled according to the operator. Present evidence is therefore process-side behavior under VRR, not a direct physical scan-out measurement.
- `nvspcap64.dll` from Windows system32 was loaded in all three primary consumer-comparison sessions, but absent from the three initial causal sessions and the detailed 120 callback session. It is recorded as an external NVIDIA capture/overlay component, not a game-directory proxy.

## Runtime Pass B bounded consumers

- `000EFDA0 → 002124A0` active samples were fully accounted for as either delta-sized elapsed increments or resets: 37,261 + 1,373 at 60, 50,672 + 1,245 at 90, and 75,112 + 1,387 at 120.
- `0021CC20` incremented elapsed field `+0x4F0` by the supplied delta on every observed active call: 541/541 at 60, 271/271 at 90, and 1,073/1,073 at 120.
- `0021CC20` calls per broad update were 11.080, 11.173, and 10.971. Its scheduling follows update count while its active elapsed field follows the variable delta.
- `000F0D40 → 002124A0` remained live, but sampled duration fields were zero, so no active progress invariant is asserted for that owner.
- Operator-repeated idle, obstacle, camera, and attack sentinels established operability/activation only. They were not exact input replays and do not prove high-level movement, camera, collision, or animation invariance.

## Pass C exact final-writer observation

- Session `20260919T144001Z-pid18536` ran the announced target-60 sprint writer
  watch. The trial covered updates 1116 through 1296 and contained 180 moving
  boundary intervals.
- All 180 accepted write hits shared one update thread, one shared boundary
  context, one transform, one writer address, one runtime function, and one live
  byte window. There were no overflow hits.
- The unique writer is RVA `0x009DC580` in runtime function range
  `0x009DC4D0..0x009DC5BA`. The live instruction bytes decode as
  `MOVDQA [RBX+0x80], XMM6`.
- Each hit's `XMM6` bits exactly reproduce the four committed float32 transform
  lanes. Each arm snapshot equals the enclosing `A1429C` after snapshot and
  each hit position equals the following `DE50EC` before snapshot.
- Exact-build static listing of `DS3-1152-F-009DC4D0` selects `RBX+0xB0` or
  fallback `RBX+0xA8`; `DS3-1152-F-00DDA140` copies a 16-byte candidate from
  selected object `+0x40 -> +0x70`. The function then masks/adds constants,
  blends from current `RBX+0x80` by scalar `RBX+0x1C0`, copies the old
  transform to `RBX+0x90`, and conditionally commits `XMM6` at `RBX+0x80`.
- Static calls at RVAs `0x007EA190` and `0x007EA4B0` are single `RET`
  instructions in this build. They do not expose a solver boundary.
- The central timing override restored exactly from `60/1` to `30/0`; the
  session completed without a `FAILED` marker.

## Offline final-writer algebra bridge

- No game process was launched for this bridge. The canonical Ghidra 12.1.3
  project was queried with `-noanalysis -readOnly`.
- Constant bytes at `1439F2720` are three all-one lanes followed by a zero
  lane. Constant bytes at `1439F2500` decode to `(0,0,0,1)` as float32.
- RVAs `009DC542..009DC556` therefore compute sequential packed float32
  `X = T + s * (((C & M) + H) - T)`; for XYZ this is `T+s*(C-T)` modulo the
  explicit instruction-by-instruction rounding environment.
- `+0xB0` has direct priority. `+0xA8` is read only when `+0xB0` is null. When
  both are null, or when selected `+0x40` is null, the local candidate is not
  written by the observed function path.
- `009DA810` always attempts to build `+0xA8`; it attempts `+0xB0` only when a
  neutral query is nonzero or either of two float32 pairs differs. Both use the
  same outer/nested constructor family, which initializes nested `+0x70` from
  three input-record floats and W zero.
- `+0x1C0` is initialized to and reset to `1.0`. Eight bounded direct
  exact-chain functions write the same non-default scalar to `+0x1B8` and
  `+0x1C0` before the final writer consumes it.
- RVA `009DC559` is the unique designed observation point at which current
  `T`, local candidate `C`, broadcast scalar, final `XMM6`, source pointers,
  inhibition byte, and `MXCSR` coexist before reset and commit. This is a
  design result only; no 60/165 acquisition occurred.

## Pass C preparation checkpoint — no runtime capture yet

- The exact C1 executable was rescanned using the historical `WorldChrMan` instruction family. One match at instruction RVA `0x004C283A` resolves to singleton global RVA `0x0477FDB8`; the executable hash matched K-0001.
- The Grand Archives 1.15.2 table provides candidate player transform and animation chains. These are recorded as secondary-source hypotheses, not current-build semantic facts.
- A separate Pass C probe compiles successfully with the pinned Pass B toolchain. It retains the reversible `+0x354/+0x358` intervention and exact restoration, and adds sampled player position/facing, animation ID/clock/duration/play-speed, HP/stamina, keyboard/XInput state, and F5–F12 semantic markers.
- The checked-in Pass C configuration has `enable_timing_override=0`; no DS3 launch or Pass C capture occurred during preparation.
- Planned first runtime step is a 60-FPS discovery/control capture that must validate the candidate field meanings and quantify probe overhead before matched high-framerate comparison.
