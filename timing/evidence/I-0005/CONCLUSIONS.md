# I-0005 conclusions — runtime Pass B checkpoint

## Architecture

The evidence supports several distinct but coordinated temporal domains:

1. I-0004's optional QPC deadline is presentation-adjacent and remained inactive in every runtime capture.
2. A `0x368`-byte central timing object owns QPC phase stamps, current/history values, flags, and the `+0x354/+0x358` requested-FPS/enable pair.
3. A common update context distributes `context+0x08` through `00D57900 → 00023410 → 000499C0`.
4. The separate `00DD4B60` object selects dynamic, `1/30`, or `1/60` for a specialized branch.
5. Bounded downstream consumers can be scheduled per update while integrating elapsed state by the variable delta.

## Established after Pass B

- The central `+0x354/+0x358` pair is a causal multi-domain control point on the exact build.
- The pair alone produces measured 60/90/120-class update and Present cadence; no additional Present intervention is needed.
- `context+0x08` becomes approximately `1/60`, `1/90`, and `1/120` rather than remaining clamped to `1/60`.
- Present selection changes from stock `SyncInterval=1` to overwhelmingly `SyncInterval=0`; I-0004 manual QPC pacing remains inactive.
- The specialized selector stays in dynamic mode 0 and does not substitute fixed `1/30` or `1/60` in these runs.
- Central `+0x268` remains the nearest strong preceding relation to the broad delta across all targets; exact producer identity remains supported rather than confirmed.
- Both `000EFDA0` and `000F0D40` forward the callback delta to `002124A0`. The former was misread in Pass A and is now corrected.
- Active `000EFDA0 → 002124A0` and `0021CC20` elapsed state advances by the supplied delta at all three targets.
- Probe overhead remained stable: median distribution-chain observation latency was 5.6 microseconds and median Present call duration 53.5–54.2 microseconds.
- Every accepted run restored the exact original `30.0/0` central state, and no game file was modified.

## Smallest defensible intervention

The smallest evidence-backed cadence intervention is the atomic central pair `+0x354` requested FPS plus `+0x358` enable. A prototype needs exact-build gating, object-lifetime handling, provenance logging, drift monitoring, and exact restoration.

No I-0004 Present patch, blanket constant replacement, or consumer-specific compensation is justified by current evidence.

## Limits

- The protected central arithmetic remains unrecovered.
- Pass B does not separate the individual causal responsibilities of `+0x354` and `+0x358`; it validates their supported pair contract.
- G-Sync was enabled. Process-side Present cadence and arguments are established; physical scan-out and fixed-refresh behavior are not.
- The primary consumer-comparison sessions shared `nvspcap64.dll`; initial causal sessions did not. This limits an overlay-free claim but does not provide a differential explanation across the three matched targets.
- Human-repeated sentinels are not deterministic input traces.
- The sampled objects lack proven player, camera, animation, movement, or collision identities.
- Complete framerate independence, gameplay correctness, 30-FPS comparison, and distribution readiness are not established.

## Current checkpoint

Pass B is complete and requires no additional runtime rerun. Pass C is authorized and prepared as the semantic-validation continuation inside I-0005. Its instrumentation and protocol are stored under `runtime/pass-c/`; the first required capture is a 60-FPS discovery/control run that validates the candidate player/animation fields and measures probe overhead before any 90/120/165 comparison.

No Pass C runtime capture has occurred at this checkpoint. Complete framerate independence, semantic field identity, gameplay correctness above 120, and support for 165/240/280 or Unlimited remain unproven.

## Pass C final-writer checkpoint

The deterministic sprint work subsequently reproduced the 165 defect,
localized every sampled final-transform displacement to the protected
`A1429C -> DE50EC` transition, and completed the target-60 exact-writer pass.
RVA `009DC580` in `DS3-1152-F-009DC4D0` is the unique observed final commit:
`MOVDQA [RBX+0x80], XMM6`. It covered all 180 measured moving intervals, and
the captured `XMM6` lanes exactly matched the committed float32 transform.

Static data flow establishes a candidate loaded through
`RBX+0xA8/+0xB0 -> +0x40 -> +0x70`, a local blend controlled by `RBX+0x1C0`,
and the final commit. Further exact-build tracing closes the local law as
sequential packed float32 `X = T + s * (P - T)`, where the mask preserves C.xyz
and the constant forces P.w to `1.0`. `+0xB0` is selected whenever non-null;
otherwise `+0xA8` is the fallback. The scalar is consumed once and reset to
`1.0`; its direct exact-chain producers are bounded separately.

The exact inputs needed for a neutral discriminator are now established. One
pre-commit observation at RVA `009DC559` can capture current, candidate,
scalar, final, selected branch, nested validity, inhibition, and `MXCSR` within
the same writer invocation. No such 60/165 mirror has run, and no upstream
operation is assigned a requested/root-motion, collision, or physics role.
Semantic Pass 2, roll promotion, correction, and 240 remain closed.
