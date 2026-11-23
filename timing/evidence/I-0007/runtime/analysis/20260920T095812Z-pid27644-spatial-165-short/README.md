# Short target-165 spatial prefix

Session: `20260920T095812Z-pid27644`.

The hardened observer ran in a fresh exact-build process with target165 and a
1.5-second requested measurement window. Thomas held straight movement beyond
the planned automatic end. The observer reached its 4,096-hit per-trial limit
after about 1.16 seconds and stopped before `trial_end`; this trial is rejected
for accepted cross-rate comparison and will not be repeated.

Shutdown nevertheless completed normally. `COMPLETE` records disabled hooks,
restored debug registers and flushed logs. The timing transition restored exact
`165/1 -> 30/0`, and the checked-in configuration is disabled. No crash occurred.

## Complete prefix

All 4,096 persisted rows precede the overflow attempt. They form 1,025 complete
forward pairs and 1,023 complete reverse pairs with no identity, pointer,
elapsed-lane or ordering rejection. The common 1,023 exchanges all use strict
forward-entry/write then reverse-entry/write order and preserve the selected
48-byte block between entries.

This prefix covers 192 broad updates. It is not source-state matched to the
accepted target60 control: the prefix has six selected slots and 15 owners,
while the control has two selected slots and 15 owners. Each target165 update
contains 20 or 24 recorded events, compared with the target60 control's smaller
and different topology. Overflow and this topology mismatch independently block
an accepted cross-rate exchange comparison.

The dominant stable-owner slot appears in all 192 updates. Its reverse payload
lanes 0 and 2 match the independently sampled player x/y fields in 192/192 rows
within `1e-5`; lane 1 again does not match sampled z. The prefix therefore
reproduces the bounded spatial correlation without qualifying the whole trial.

The same stable-slot layout invariants hold in all 192 prefix exchanges.
Selected and reverse-current blocks have lane3=1, unit-norm lanes4–7,
unit lanes8–10 and lane11=1. Their `2*atan2(lane5,lane7)` equals the sampled
player angle in192/192 updates; reverse-current lanes0/2 equal sampled player
x/y in192/192. The derived forward payload instead has lane11=0 in192/192 and
an identity-like orientation unrelated to player angle. This reproduces the
bounded three-float4 transform structure, while overflow still blocks accepted
cross-rate equivalence and lane1 remains semantically unresolved.

The local-to-world relation also repeats row by row. Derived translation and
selected-to-current displacement are jointly active in65 exchanges and jointly
inactive in127; there are no one-sided rows. Negating the derived translation
after rotation by the selected quaternion predicts world displacement in65/65
active rows within `2e-4` (maximum error `6.59e-5`). Median derived norm per
broad delta is4.00818600, versus4.01140065 in the accepted60 control, a
target165/target60 ratio0.99919862. This is retained as row-level engine mapping,
not an accepted cross-rate trajectory or gameplay pass.

## Raw XMM1 lane-0 regimes

Joining each entry row to its exact broad-update delta gives only two regimes:

| Capture | Entry rows | current delta | 30 × current delta | other |
|---|---:|---:|---:|---:|
| accepted target60 control | 1,056 | 888 | 168 | 0 |
| rejected target165 prefix | 2,048 | 1,983 | 65 | 0 |

Every reverse entry matches current delta in both captures. The `30 × delta`
regime occurs only at forward entries. At target60 this is the previously
observed near-0.5 value; at target165 it is near 0.181818. This supports a
scaled-delta interpretation of the two numeric regimes rather than unrelated
constants. Because target165 overflowed and its selected-slot topology differs,
the result is retained as `OBSERVED` row-level engine mapping, not accepted
cross-rate equivalence or a gameplay-cause claim.

## Runtime owner identity

After the observer had stopped and restored all temporary state, a read-only
RTTI inspection of the still-live process resolved all 15 captured owners as
`NS_SPRJ::SprjBehaviorCharacterProxyDriver`. Their `owner+0x90` referents were
one `NS_SPRJ::PlayerIns` and 14 `NS_SPRJ::EnemyIns`; every reachable
`player+0x1F90 -> modules+0x28` object was an
`NS_SPRJ::SprjChrBehaviorModule`.

The PlayerIns-backed owner and four EnemyIns-backed owners use only the
unscaled-delta regime in this prefix. All ten owners that receive `30 * delta`
on forward entry are EnemyIns-backed and occur for only six or seven captured
forward entries, while the five unscaled-only owners persist across all 192
updates. EnemyIns identity therefore bounds the scaled population but does not
select the regime by itself. This instance-level map does not assign a gameplay
role or prove that the same population remains present in another scene.

## Evidence

- [`spatial-exchange-summary.json`](spatial-exchange-summary.json)
- [`spatial-dynamics.json`](spatial-dynamics.json)
- [`target165-prefix-elapsed-regimes.json`](target165-prefix-elapsed-regimes.json)
- [`target165-prefix-player-correlation.json`](target165-prefix-player-correlation.json)
- [`target165-prefix-block-layout.json`](target165-prefix-block-layout.json)
- [`owner-regime-summary.json`](owner-regime-summary.json)
- [`live-owner-rtti.json`](live-owner-rtti.json)
- [`observer-live-build-validation.json`](observer-live-build-validation.json)
- [accepted target60 elapsed regimes](../20260920T091359Z-pid4128-spatial-crash/target60-elapsed-regimes.json)
- [raw session and hashes](../../sessions/20260920T095812Z-pid27644/)
