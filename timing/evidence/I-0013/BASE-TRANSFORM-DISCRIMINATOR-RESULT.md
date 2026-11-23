# I-0013 base-transform discriminator result

## Outcome

The exact current-player `005C8100 -> 009D97B0` boundary is not the source of
the reproduced high-rate vertical oscillation. At both 60 and 239.940 Hz, every
active returned base transform has a lane-7 translation bit-exactly equal to
the statically reconstructed movement-position input, and both have zero
opposing vertical steps. Thomas reported visible hopping at 240.

The cache value present immediately before the same calls retains the earlier
high-rate divergence. Its opposing vertical distance per planar unit is 5.423
times larger at 240 and its event count per unit is 9.579 times larger. This is
the second independent 60/high-rate session with a nearly identical
route-normalized distance amplification (5.466x in cycle 14).

The cause is therefore downstream of base-transform construction: inside the
remaining `005C8100` operations, the subsequent cache write, or a cache
consumer. This result does not yet choose among those stages or assign
collision/terrain ownership.

## Acquisition and cleanup

Session: `20260921T054031Z-pid27432`.

| Target | Rows | Valid | Measured calls/s | Active rows | Operator observation |
|---|---:|---:|---:|---:|---|
| 60 | 181 | 181 | 60.002266 | 104 | no abnormal behavior reported |
| 240 | 721 | 721 | 239.940159 | 538 | visible hopping |

Every retained row has the exact expected caller return RVA `005C812C`, the
same current-player identity, finite data, consecutive sequence numbers and a
monotone QPC interval. Movement position is unchanged across every observed
`009D97B0` call. `CONTROL` records successful ready, both arms/saves, target
change and exact restoration, with zero invalid row. The hook was removed and
DS3 was closed. The exact live DLL and injector are retained with the session.

## Boundary measurements

| Measure | 60 | 240 | 240 / 60 |
|---|---:|---:|---:|
| Active planar distance | 6.749636 | 8.381403 | — |
| Source opposing steps | 0 | 0 | — |
| Base-transform opposing steps | 0 | 0 | — |
| Base lane-7 exact reconstruction | 104 / 104 | 538 / 538 | — |
| Prior-cache opposing steps / planar unit | 2.814967 | 26.964459 | 9.579x |
| Prior-cache opposing distance / planar unit | 0.014882 | 0.080701 | 5.423x |
| Peak prior-cache opposing step | 0.016579 | 0.027519 | 1.660x |

The “prior cache” is the cache contents at `009D97B0` entry. It is not the
final output of that same `005C8100` invocation. It nevertheless establishes
that the oscillatory state persists across invocations while the newly built
base transform remains exact and monotone.

## Targeted downstream reconstruction

Read-only Ghidra work refines the remaining direct path:

1. `009D97B0` returns the now-measured exact base transform.
2. `00D99C40` can adjust orientation but statically preserves translation
   lanes 3/7/11.
3. protected `00D9A3C0` can modify the output; its exact operation remains
   unavailable in the disk image.
4. the `+0x2EC` vertical override follows, but cycle 14 proved it inactive.
5. `009F8FE0(owner+0x24C)` gates an optional local transform from fields
   `owner+0x250`, `+0x254` and `+0x258`; `009F8E10` builds that transform and
   `0017CE0` composes it with the current output.

`005C75F0` accumulates two returned scalar contributions at `owner+0x24C`,
moves their sum into `owner+0x250`, then clears `+0x24C`. Thus the optional
transform consumes a carried value from the preceding producer invocation.
`009F8D80` initializes the four-field block to zero. `005C6D30`, called through
`004757B0`, writes `owner+0x258` and the neighboring selector `+0x2D8`.

This carried state is a justified candidate boundary, not a causal recurrence
or a semantic contact label. Its live activation and relationship to the final
output remain unmeasured.

## Next discriminator

Use one replacement hook at `005C8100`, filtered to the current cache owner.
Record its returned 48-byte output plus `+0x250/+0x254/+0x258`, `+0x2EC` and
the three known caller sites. If the returned output already oscillates, the
cache writer/consumer is cleared and the predicate distinguishes the protected
operation from the optional local transform. If it remains monotone, the
writer or consumer becomes the selected side. No correction is justified yet.
