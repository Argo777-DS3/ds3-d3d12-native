# I-0005 runtime Pass B — reversible 60/90/120 comparison

## Decision

**Pass B succeeded.** On the exact C1 build, the central `+0x354/+0x358` pair is a causal multi-domain control point: changing only that pair changes the broad-update cadence, the distributed delta, and the requested DXGI presentation mode. The pair alone reached measured 60/90/120-class behavior; no separate Present override and no I-0004 manual QPC wait were needed.

This is not yet proof that every animation, movement, camera, collision, or gameplay system is framerate-independent. It establishes the smallest current-build intervention boundary and verifies two bounded time-driven consumers.

## Primary comparison

| Target | Active span | Updates | Update rate | Median delta | Presents | Present rate | Median Present interval |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 60 | 82.777 s | 4,919 | 59.420 Hz | 0.0166666005 s | 4,919 | 59.420 Hz | 0.0166666 s |
| 90 | 63.435 s | 5,653 | 89.116 Hz | 0.0111111002 s | 5,653 | 89.116 Hz | 0.01111155 s |
| 120 | 90.202 s | 10,692 | 118.538 Hz | 0.00833330024 s | 10,692 | 118.538 Hz | 0.0083314 s |

The span rates include loading/stalls and reached 98.8–99.0% of the targets. Median intervals and deltas are the stronger steady-cadence discriminator. Update and Present counts were exactly equal in each primary session.

Every accepted session logged a successful `30.0/0 → target/1` transition and an exact successful `target/1 → 30.0/0` restoration.

## CONFIRMED — the broad delta follows the selected target

The median `context+0x08` delta became:

- approximately `1/60` at target 60;
- approximately `1/90` at target 90;
- approximately `1/120` at target 120.

Every ordered `00D57900 → 00023410 → 000499C0` triple retained the same scalar. The median observation latency remained 5.6 microseconds in all three primary sessions.

This is causal evidence for the pair as a central cadence/delta control. It does not assign separate causal roles to `+0x354` and `+0x358`, because Pass B deliberately treated them as the already-supported requested-FPS/enable pair.

## CONFIRMED — presentation changes without an additional patch

`force_present_zero` remained disabled. After the transition, 99.96–99.98% of active-window Presents used `SyncInterval=0`; the two initial `Present(1, …)` calls per session occurred around startup/transition. I-0004 pacing remained `active=0`, and no manual QPC wait was observed.

At stock Pass A, the same path used `Present(1, …)` throughout. At Pass B target 60, the engine kept an approximately `1/60` delta while changing the requested presentation interval to zero. Therefore the pair causally reaches both engine cadence selection and presentation mode, while the manual I-0004 wait remains a distinct inactive mechanism.

G-Sync was enabled according to the operator. These results describe process-side Present behavior under VRR; they do not measure physical display scan-out.

The three primary consumer-comparison module inventories also contain the system NVIDIA capture/overlay module `nvspcap64.dll`; the three initial causal 60/90/120 sessions and the detailed 120 callback session do not. The primary conditions share this module, while the central-pair cadence/delta result is reproduced without it. It therefore limits an overlay-free environmental claim but does not explain the target-dependent differential.

## SUPPORTED — central `+0x268` remains the nearest observed delta relation

The preceding `+0x268` snapshot matched the next broad delta within `1e-7` for 98.96%, 98.76%, and 98.58% of aligned 60/90/120 pairs respectively. The relationship therefore survives cadence changes and is stronger than stock-only correlation, but protected arithmetic still prevents claiming `+0x268` as the original producer rather than a current/history copy.

## CONFIRMED — the specialized selector stayed dynamic

`00DD4B60` used selector index `0` in all 2,510, 3,088, and 4,964 observed calls at 60, 90, and 120. Output equaled dynamic input within `1e-7` on every call. Neither fixed `1/30` nor fixed `1/60` mode was selected.

The selector is therefore not the mechanism that held or substituted cadence in these tests.

## CONFIRMED — bounded consumers advance by delta, while invocation count follows updates

Static reinspection corrects the Pass A interpretation of `DS3-1152-F-000EFDA0`: at `0x1400EFDB6` it adjusts only `RCX`, leaves incoming `XMM1` intact, and immediately calls `DS3-1152-F-002124A0`. It therefore forwards the callback delta. `000F0D40` also forwards the delta to the same helper.

Direct aggregate hooks then established:

- `000EFDA0 → 002124A0`: for every active sampled call at all three targets, the helper either incremented elapsed state by the supplied delta within `1e-5` or performed a reset. Counts were 37,261 matches + 1,373 resets at 60; 50,672 + 1,245 at 90; and 75,112 + 1,387 at 120.
- `0021CC20`: every observed active call incremented `+0x4F0` by the supplied delta within `1e-5`: 541/541 at 60, 271/271 at 90, and 1,073/1,073 at 120.
- `000F0D40 → 002124A0` was live, but the sampled helper duration fields were zero throughout these three sequences, so no active progress invariant is claimed for this owner.

`0021CC20` calls per broad update stayed near 11.1 across targets (11.080, 11.173, 10.971). Thus invocation count grows with update count, while its active elapsed state grows by the per-update delta. The same delta-based accounting is proven for active `000EFDA0` helper state. This distinguishes update-count scheduling from wall-time-like local integration without inventing high-level subsystem names.

## OBSERVED — representative operator sentinels

The operator completed idle, movement/obstacle, camera, and attack sequences at each target without the probe or game failing. Because inputs were human-repeated and the sampled objects lack proven player/camera/collision identities, this is operability/activation evidence only. It is not a quantitative proof that movement distance, camera response, collision, or animation duration are invariant.

## Five Pass B answers

1. **What does `+0x354/+0x358` control?** The pair causally controls multiple domains: selected update cadence, distributed delta, and presentation interval selection. It does not activate the manual I-0004 QPC wait.
2. **What happens to `context+0x08`?** It becomes approximately `1/90` and `1/120` at those targets, rather than staying at or being clamped to `1/60`.
3. **Which consumers follow wall time or update count?** The sampled functions are invoked per broad update; active `002124A0` and `0021CC20` elapsed state advances by delta, not by a fixed amount per update. High-level ownership remains unknown.
4. **Which invariants remain stable or diverge?** The local elapsed-by-delta contracts remain exact at all targets, and no selector substitution occurs. High-level animation/movement/camera/collision invariants remain unproven rather than classified stable or divergent.
5. **What is the smallest defensible intervention boundary?** The exact-build central `+0x354` requested-FPS float plus `+0x358` enable byte, treated atomically with exact restoration. No additional Present patch or blanket constant replacement is currently justified.

## Intervention boundary and remaining gate

The next non-blind implementation may wrap only this pair, with exact-build verification, object-lifetime detection, before/after logging, reversible application, and exact restoration. That would be a cadence-control prototype, not yet a complete framerate-independence patch.

Before distribution or a correctness claim, the project still needs deterministic, semantically identified invariants for representative animation, movement, camera, and collision/physics behavior. Pass B does not justify consumer hotfixes or edits to fixed `1/30`/`1/60` constants.

## Evidence map

- Protocol: `PASS-B-PROTOCOL.md`
- Per-session analyzer: `analyze_pass_b.py`
- Cross-condition analyzer: `compare_pass_b.py`
- Primary comparison: `comparison/condition-summary.csv`
- Consumer comparison: `comparison/consumer-site-summary.csv`
- Primary raw sessions: `sessions/20260919T094057Z-pid12600/`, `sessions/20260919T094304Z-pid19080/`, `sessions/20260919T094516Z-pid27388/`
- Detailed 120 callback session: `sessions/20260919T093215Z-pid13708/`
- Reversible instrumentation source: `instrumentation/`
- Exact static callback listings: `../pass-a/static-callback-targets/`
