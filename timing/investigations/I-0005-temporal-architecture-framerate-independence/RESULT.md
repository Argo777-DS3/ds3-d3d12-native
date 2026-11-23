# I-0005 result — 165 cadence passed; locomotion gate blocked

## Decision

**Static reconstruction, Passes A/B, and the Pass C cadence and currently instrumented bounded-sentinel gates are complete through 165; the 165 gameplay gate is blocked.** The first 165 request failed near 120 under the 119-Hz active display mode; the unchanged matched retry reached 165 after Windows was set to 280 Hz. Thomas then reported a reproducible locomotion-distance anomaly despite apparently normal animation speed. Treat that report as `OBSERVED` until quantified. Do not proceed to 240.

Dark Souls III was tested offline under explicit authorization. Pass A was observation-only. Pass B changed only central timing state in memory, restored the exact originals after every run, and did not modify the installation.

## Recovered architecture

```text
central 0x368-byte timing object at global 0x14489DD10
  | +0x354 requested FPS float / +0x358 enable byte
  | QPC phase stamps, current/history floats, flags
  | protected update entry DS3-1152-F-04E79363
  v
common update context +0x08
  -> DS3-1152-F-00D57900
  -> DS3-1152-F-00023410
  -> DS3-1152-F-000499C0
       -> virtual callbacks / explicit subsystem updates
       -> scaled accumulators

separate specialized branch:
central field +0x26C
  -> DS3-1152-F-008F2CA0
  -> DS3-1152-F-00DD4B60
       selects dynamic / 1/30 / 1/60

presentation branch:
000D5CB0 -> 000D5E30 -> 02139260 -> 0192CF00 -> Present
```

I-0004's QPC deadline remains a distinct presentation-pacing mechanism. It was inactive in Pass A and Pass B.

## Causal Pass B result

The only intervention was the central `+0x354/+0x358` pair. No Present patch and no fixed-constant replacement were used.

| Target | Updates | Update rate | Median delta | Presents | Present rate |
|---:|---:|---:|---:|---:|---:|
| 60 | 4,919 | 59.420 Hz | 0.0166666005 s | 4,919 | 59.420 Hz |
| 90 | 5,653 | 89.116 Hz | 0.0111111002 s | 5,653 | 89.116 Hz |
| 120 | 10,692 | 118.538 Hz | 0.00833330024 s | 10,692 | 118.538 Hz |

The pair therefore causally changes multiple domains: broad-update cadence, distributed delta, and requested DXGI presentation interval. It produces approximately `1/target` rather than leaving or clamping the broad delta at `1/60`.

At stock Pass A, Present used `SyncInterval=1`. Under the Pass B pair, 99.96–99.98% of active-window calls used `SyncInterval=0`; two startup/transition calls per session retained 1. `force_present_zero` was disabled throughout. G-Sync was enabled, so this is process-side Present behavior under VRR, not a direct scan-out measurement. The three primary matched runs also shared system NVIDIA capture module `nvspcap64.dll`; the initial causal runs did not, which bounds any overlay-free environmental claim without explaining the target-dependent differential.

Every session restored `target/1 → 30.0/0` successfully.

## Consumer result and corrected static interpretation

Calling-convention reinspection corrects the previous statement that `000EFDA0` ignored the delta. It changes `RCX` but leaves incoming `XMM1` intact for the immediate call to `002124A0`. Both `000EFDA0` and `000F0D40` therefore forward the delta to that local helper.

Pass B directly observed two bounded elapsed-state contracts:

- Active `000EFDA0 → 002124A0` calls were always accounted for by a delta-sized increment or reset at 60/90/120.
- Active `0021CC20` calls always incremented `+0x4F0` by the supplied delta: 541/541, 271/271, and 1,073/1,073.

`0021CC20` remained near 11 calls per broad update while its active elapsed state used the variable delta. This distinguishes per-update scheduling from delta-based time integration.

`000F0D40` was live, but its sampled helper duration fields remained zero; no active progress invariant is claimed. High-level owner names remain unknown.

## Selector and central relation

`00DD4B60` selected dynamic mode 0 and returned its input exactly on all 2,510 / 3,088 / 4,964 primary calls. Fixed `1/30` and `1/60` entries did not substitute cadence.

Central `+0x268` remained the strongest immediately preceding field relation, matching the following broad delta for 98.96% / 98.76% / 98.58% of aligned pairs. Original-producer identity remains `SUPPORTED`, not `CONFIRMED`, because the protected arithmetic is still unrecovered.

## Minimal intervention boundary

The smallest defensible current-build cadence intervention is the atomic pair:

- central object `+0x354`: requested FPS float;
- central object `+0x358`: enable byte.

A future prototype must verify the exact build, wait for the object lifetime, record original values, apply the pair coherently, monitor drift, and restore exactly. No separate Present patch, blanket `60 → 120` edit, or consumer-specific hotfix is currently justified.

This boundary controls cadence; it is not by itself proof of complete gameplay correctness.

## Remaining boundary

The operator completed short idle, movement/obstacle, camera, and attack sequences at all three targets. Because inputs were human-repeated and sampled objects lack proven semantic identities, those runs establish operability and activation but do not quantify animation duration, displacement, camera response, or collision invariance.

I-0005 therefore remains active. Pass C's 60-FPS discovery captured 10,494 semantic samples with a single stable non-null `WorldChrMan`/player/transform chain and 100% valid position and animation reads. Idle and camera-input markers left the player transform unchanged; movement, roll, attack, obstacle, and movement/physics markers produced coherent position and animation changes. The obstacle marker reached a 0.0056-unit last-second plateau.

The direct semantic-sampler window averaged 12.5661 us over 1,798 samples, with a 562.2-us maximum, while the overhead control achieved 59.95 updates and Presents per second. Matched 90 and 120 sessions then achieved 89.999090 and 119.969425 updates/s with 13.2600-us and 13.1911-us mean sampler windows. The first 165 request retained central `165/1` but stabilized at 119.989448 updates/s and 119.878341 Presents/s with a `1/120` delta under the 119-Hz active display mode. After Windows was set to 280 Hz, the unchanged 165 retry reached 164.821720 updates/s and 164.684749 Presents/s with a `1/165` median delta. All 37,774 semantic samples retained valid chains; idle and camera markers were exactly stable, the obstacle tail remained below 0.01 unit, and the animation clock retained the approximately `10 x delta` scale. The probe restored `165/1 -> 30/0` exactly. These are cadence and bounded-sentinel results only: the free/manual movement markers do not quantify cross-framerate locomotion. The observed 165 distance anomaly requires a deterministic comparison before any higher target or correction. A windowed display/driver/compositor ceiling remains supported without isolating a sole external owner. No final UI, permanent patch, consumer hotfix, or Unlimited mode is justified yet.

The deterministic locomotion instrument records pre/post broad-update world
transform, animation state, input, QPC, update count, and accumulated delta
inside a probe-timed steady-state window. The accepted target-60 baseline and
direct target-165 comparison established an approximately half-rate sprint
distance per accumulated delta at 165 while animation progression remained
normal. No current-build-qualified root-motion or pre/post-collision vector has
yet been identified, so those stages remain explicitly uninstrumented rather
than inferred.

The subsequent matched dispatcher RCA localized every sampled moving interval
to `02344730:after@00A1429C -> 02344730:before@00DE50EC`. The moving pair shares
the same runtime context pointer at target 60 and 165. Because the static bytes
around `00A1429C` remain protected/transient, the next checkpoint is now a
two-pass internal-stage design: target-60 final-writer discovery with three
per-thread write watchpoints on the qualified transform coordinates, followed
only after live-byte/data-flow qualification by a target-60/165 capture of
pre-application, applied, and committed vectors. See
`PASS-C-SPRINT-INTERNAL-STAGE-DESIGN.md`. No correction, roll promotion, or 240
work is authorized.

Pass 1 then completed in one announced target-60 sprint trial. Every one of 180
moving boundary intervals produced exactly one accepted hit at the same writer:
RVA `009DC580` in `DS3-1152-F-009DC4D0`, instruction
`MOVDQA [RBX+0x80], XMM6`. All hits shared one thread, boundary context,
transform, runtime function, and byte window; `XMM6` matched the committed
float32 lanes exactly, no overflow occurred, and timing state restored exactly
to `30/0`.

Read-only exact-build tracing shows that the writer function selects
`RBX+0xB0` or fallback `RBX+0xA8`, obtains a 16-byte candidate from nested
`+0x40 -> +0x70`, blends it from the current transform using scalar
`RBX+0x1C0`, and commits the result to `RBX+0x80`. This confirms the final
commit and a bounded upstream candidate, but it does not establish a
requested/root-motion source or collision/physics application boundary. The
offline bridge is now closed at the computational level: after masking and
forcing W to `1.0`, the exact sequential float32 law is
`X = T + s * (P - T)`. `+0xB0` has priority over fallback `+0xA8`; their
constructor/setup/update/teardown provenance and the bounded direct producers
of one-shot scalar `+0x1C0` are recorded separately with neutral labels.

One unchanged 60/165 mirror is designed at pre-commit RVA `009DC559`. A single
same-invocation event captures `T`, raw candidate `C`, scalar `s`, final
`XMM6`, source selection, nested validity, inhibition, and `MXCSR`; exact
reconstruction and the existing post-update snapshot gate every accepted row.
This acquisition has not been implemented or run. It is intended only to
discriminate the first categorical or numeric input whose 165/60 ratio explains
the observed approximately `0.5` final ratio. Semantic Pass 2, correction,
roll promotion, and 240 remain closed.

I-0005 reached its bounded `20/20` iteration budget at this checkpoint. Future
implementation or runtime work requires replan/extension from the clean SSOT
checkpoint; budget exhaustion is not a claim of technical completion.

## Evidence map

- [`Pass B result`](../../evidence/I-0005/runtime/pass-b/PASS-B-RESULT.md)
- [`Pass B protocol`](../../evidence/I-0005/runtime/pass-b/PASS-B-PROTOCOL.md)
- [`Pass B condition comparison`](../../evidence/I-0005/runtime/pass-b/comparison/condition-summary.csv)
- [`Pass B consumer comparison`](../../evidence/I-0005/runtime/pass-b/comparison/consumer-site-summary.csv)
- [`Pass A result`](../../evidence/I-0005/runtime/pass-a/PASS-A-RESULT.md)
- [`OBSERVATIONS.md`](../../evidence/I-0005/OBSERVATIONS.md)
- [`HYPOTHESES.md`](../../evidence/I-0005/HYPOTHESES.md)
- [`CONCLUSIONS.md`](../../evidence/I-0005/CONCLUSIONS.md)
- [`TOOLCHAIN.md`](../../evidence/I-0005/TOOLCHAIN.md)
- [`time-flow.csv`](../../evidence/I-0005/static/time-flow.csv)
- [`central-timing-state.csv`](../../evidence/I-0005/static/central-timing-state.csv)
- [`Pass C protocol`](../../evidence/I-0005/runtime/pass-c/PASS-C-PROTOCOL.md)
- [`Pass C probe`](../../evidence/I-0005/runtime/pass-c/instrumentation/README.md)
- [`WorldChrMan static verification`](../../evidence/I-0005/runtime/pass-c/worldchrman-static-verification.txt)
- [`Pass C 60 FPS control result`](../../evidence/I-0005/runtime/pass-c/PASS-C-CONTROL-RESULT.md)
- [`Pass C primary analysis`](../../evidence/I-0005/runtime/pass-c/analysis/20260919T110038Z-pid27016/analysis-summary.json)
- [`Pass C overhead analysis`](../../evidence/I-0005/runtime/pass-c/analysis/20260919T112723Z-pid11508/analysis-summary.json)
- [`Pass C 90 FPS result`](../../evidence/I-0005/runtime/pass-c/PASS-C-90-RESULT.md)
- [`Pass C comparison`](../../evidence/I-0005/runtime/pass-c/comparison/condition-summary.csv)
- [`Pass C 120 FPS result`](../../evidence/I-0005/runtime/pass-c/PASS-C-120-RESULT.md)
- [`Pass C 165 cadence RCA`](../../evidence/I-0005/runtime/pass-c/PASS-C-165-RCA.md)
- [`Pass C 165 matched result`](../../evidence/I-0005/runtime/pass-c/PASS-C-165-RESULT.md)
- [`Pass C 165 matched analysis`](../../evidence/I-0005/runtime/pass-c/analysis/20260919T121245Z-pid6348/analysis-summary.json)
- [`Pass C 165 locomotion RCA`](../../evidence/I-0005/runtime/pass-c/PASS-C-165-LOCOMOTION-RCA.md)
- [`Pass C sprint dispatcher RCA`](../../evidence/I-0005/runtime/pass-c/PASS-C-SPRINT-DISPATCHER-RCA-RESULT.md)
- [`Pass C sprint internal-stage design`](../../evidence/I-0005/runtime/pass-c/PASS-C-SPRINT-INTERNAL-STAGE-DESIGN.md)
- [`Pass C sprint final-writer result`](../../evidence/I-0005/runtime/pass-c/PASS-C-SPRINT-FINAL-WRITER-RESULT.md)
- [`Pass C sprint writer algebra bridge`](../../evidence/I-0005/runtime/pass-c/PASS-C-SPRINT-WRITER-ALGEBRA-BRIDGE.md)
- [`Final-writer algebra`](../../evidence/I-0005/static/locomotion-final-writer/writer-algebra.csv)
- [`Candidate provenance`](../../evidence/I-0005/static/locomotion-final-writer/writer-candidate-provenance.csv)
- [`Scalar provenance`](../../evidence/I-0005/static/locomotion-final-writer/writer-scalar-provenance.csv)
