# I-0013 aggregator-output discriminator result

## Outcome

The current-player `005C8100` output is the first measured boundary containing
the reproduced high-rate vertical oscillation. Across all 901 accepted calls,
its translation is bit-exactly:

`final.x = source.x`, `final.y = source.y`,
`final.z = float32(base.z + carried +0x250)`.

The optional local-transform predicate is active in every row with modes
`+0x254 = 0` and `+0x258 = 0`; the vertical override remains inactive. The
preceding cache is exactly the prior call's final 48-byte output in every one
of 899 possible comparisons. Thus neither the later writer nor consumer creates
the oscillation: the cache transports it. On this sampled path, the protected
operation contributes no unresolved vertical residual after the exact carried
addition.

This establishes the exact operation that exposes the symptom but not the two
dynamic contributions summed by `005C75F0` to produce carried `+0x250`.

## Acquisition and cleanup

Session: `20260921T061259Z-pid23660`.

| Target | Rows | Valid | Calls/s | Active rows | Operator observation |
|---|---:|---:|---:|---:|---|
| 60 | 181 | 181 | 60.000200 | 142 | normal |
| 240 | 720 | 720 | 239.840215 | 576 | same visible hopping |

Only caller return `005CAA80` was active for the current player. Every sequence,
pointer, finite-value, QPC and reconstructed-predicate gate passes. `CONTROL`
records successful target change, both saves and exact timing/hook restoration
with zero invalid row. Thomas then closed DS3. Exact live binaries are retained.

## Exact boundary measurements

| Measure | 60 | 240 | 240 / 60 |
|---|---:|---:|---:|
| Active planar distance | 9.239476 | 9.029312 | — |
| Source opposing steps | 0 | 0 | — |
| Final opposing steps / planar unit | 3.246937 | 27.576853 | 8.493x |
| Final opposing distance / planar unit | 0.022026 | 0.054977 | 2.496x |
| Carried opposing steps / planar unit | 9.199656 | 44.410916 | 4.827x |
| Carried opposing distance / planar unit | 0.105837 | 0.205581 | 1.942x |
| Exact `source + carried` vertical rows | 181 / 181 | 720 / 720 | — |
| Exact prior-output cache links | 180 / 180 | 719 / 719 | — |

The carried scalar has more opposing variation than final world height because
the monotone source partly cancels it. Its high-rate amplification is therefore
already present before cache transport and is not an aliasing artifact created
by the cache.

## Static bridge

Read-only exact-build decompilation agrees with the live equation. `005C75F0`
sums two dynamic float contributions into `owner+0x24C`, copies that float to
`owner+0x250`, then clears `+0x24C`. With live modes `0/0`, `009F8E10` builds a
local vertical translation from `+0x250`; `0017CE0` composes it into the base
transform. The live bit-exact equation validates this bounded algebra without
assigning original source names or collision semantics.

## Remaining causal boundary

The unresolved source is now upstream of the addition: one contribution is
read through the object at `owner+0x8` and its virtual slot `+0x178`; the other
is returned by the owner's virtual slot `+0xE0`. Their concrete runtime targets,
individual values and time laws are not captured. The smallest next step is a
one-hook `005C75F0` target/provenance observer, followed by decompilation of the
resolved concrete callees. No filter, clamp or FPS-specific compensation is
justified.
