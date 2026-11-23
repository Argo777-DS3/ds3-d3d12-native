# Spatial exchange control and crash audit

Session: `20260920T091359Z-pid4128`.

The target-60 control is structurally accepted. It contains 2,112 rows: 528
strict forward pairs and 528 strict reverse pairs, with no rejected pair and no
overflow. This establishes the four-site observation mechanism for that trial;
it does not establish a cross-rate result.

All 528 complete exchanges use the same strict order: forward entry, forward
write, reverse entry, reverse write. The descriptor-selected 48-byte block is
bit-identical at the forward and reverse entries in every exchange, while no
forward selected block equals its derived payload. Two selected addresses occur:

- one stable-owner slot appears in all 360 updates; its reverse write is a
  bit-exact no-op 236 times and changes only DWORD lanes 0 and 2 in the other
  124 exchanges;
- one shared slot appears in 168 updates across 14 owners; every reverse write
  is a bit-exact no-op within its exchange, while its value changes before the
  next occurrence.

This confirms a tightly ordered bidirectional handoff and distinguishes a
stable-owner slot from a transient shared slot. The single-rate data does not
name their gameplay semantics or establish how the values scale at 165 Hz.
For the stable slot, reverse-payload lanes 0 and 2 match the independently
sampled player `x` and `y` fields in all 360 updates within `1e-5` (maximum
absolute errors below `5e-7`); lane 1 does not match sampled `z`. Reverse-block
change and consecutive sampled player movement coincide in 123 updates, with
one changed-only and one movement-only boundary update. This supports spatial
position content in two lanes without assigning the complete block type.

All 360 stable-slot selected/current blocks also preserve a stronger internal
layout: lane3 is exactly1; lanes4–7 have unit norm, with lanes4/6 zero and
`2*atan2(lane5,lane7)` matching the independently sampled player angle in
360/360 updates; lanes8–10 are within `1e-5` of1 and lane11 is exactly1. This
supports three float4 groups consistent with translation/homogeneous,
unit-orientation and unit-scale components. The derived forward payload keeps
lane3, unit orientation and unit scale but has lane11=0 in360/360 rows and an
identity-like orientation that does not match player angle, so it remains a
distinct transform-like result rather than the current player transform.

For all 124 active stable-slot exchanges, derived translation and
selected-to-current world displacement activate together; the other236 rows
have neither. The relation
`current.translation - selected.translation ~= -rotate(selected.orientation,
derived.translation)` holds124/124 within a `2e-4` vector-error tolerance
(maximum `1.0911e-4`). Derived/world norm ratio has median1.00000125, and
derived translation norm per broad delta has median4.01140065. This binds the
derived block to the world displacement at the accepted60 boundary.

The raw incoming XMM1 lane 0 also separates the two exchange populations. The
stable slot receives the near-`1/60` value in both directions (360/360 each,
split only by two adjacent float encodings). The shared slot receives near
`0.5` on all 168 forward entries and near `1/60` on all 168 reverse entries.
Therefore XMM1 cannot be treated as one universal elapsed-time semantic at this
callback boundary; its interpretation is owner/direction dependent.

The same active instrument was switched to target 165 only after that control
passed. Trial 2 armed at QPC `54006471390` and never emitted `measure_end` or
`trial_end`. Thomas reported that the visible crash coincided with the
character stopping. The final serialized prefix begins a `02344730:before`
event from caller return RVA `00ECA94C`, 2.8892191 seconds after arm, with
animation ID `22100`, but contains only 20 of the expected 21 CSV columns.
It does not prove that logging returned or that the original function began.
Windows Error Reporting classifies the failure as `BEX64`, exception
`0xc0000005`, exception data `8` (invalid execute), with an unknown faulting
module/stack hash. The reported execute target `0x00007ff75fb0b120` lies outside
every module range captured at session start; that supports an invalid indirect
target or return but does not identify who corrupted or supplied it.

Before that prefix, the same session completed 830 pairs at this caller, 104
with animation ID `22100`; an independent accepted target-165 capture completed
another 494. The caller and animation context are therefore not exceptional.
Exact-build static export places `00ECA94C` after a direct call to `02344730`,
whose body contains eight indirect object-table calls. That shape is compatible
with an invalid indirect target, but the WER record lacks the instruction,
stack and registers needed to choose one or establish its source.

The durable global sequence has 2,879 missing slots after trial-2 start. Spatial
hits should dominate because this build retained them until an explicit stop or
control flush, but other CSV streams could also lose records after their last
periodic flush. The gap is an upper bound, not an exact recovery. Even the
upper bound stays below the 4,096 per-trial and 8,192 global spatial capacities,
so overflow remains unsupported.

The target-165 trial is rejected. There is no `COMPLETE` marker because the
process had already terminated, and no cross-rate comparison is valid. The
available evidence leaves the fault between phase-row logging and unobserved
dispatcher completion; it does not identify whether instrumentation, the game,
or another path supplied the invalid execute target.

Offline follow-up passed two consecutive four-site arms, 1,024 executions per
site per arm, RF resume, and exact debug-register restoration. Generic
re-arming is therefore not sufficient to reproduce the failure. The hardened
rebuild publishes completed spatial rows once per second and suppresses the
high-volume `02344730` phase log in spatial mode. It remains disabled and has
not been injected.

Canonical machine-readable result: [`crash-summary.json`](crash-summary.json).
Healthy/crash phase comparison:
[`crash-phase-boundary.json`](crash-phase-boundary.json).
Exact-build static review:
[`crash-boundary assessment`](../../../static/crash-boundary/ASSESSMENT.md).
Target-60 exchange topology: [`target60-dynamics.json`](target60-dynamics.json).
Player-position correlation:
[`target60-player-correlation.json`](target60-player-correlation.json).
Block-layout analysis: [`target60-block-layout.json`](target60-block-layout.json).
Raw evidence and hashes: [`session`](../../sessions/20260920T091359Z-pid4128/).
