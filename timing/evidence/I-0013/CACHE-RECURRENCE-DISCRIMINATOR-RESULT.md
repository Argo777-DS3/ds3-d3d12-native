# I-0013 cache-recurrence discriminator result

## Outcome

The same-process target-60/target-240 discriminator completed with exact
timing restoration and no invalid sample. Thomas reported normal behavior at
60 and visible hopping at 240. The previously isolated vertical override is
inactive in every retained row, so its cadence-dependent recurrence is
refuted as the active cause of this reproduced symptom.

The reproduced high-rate symptom instead aligns with a large increase in
opposing vertical motion in the intermediate 48-byte cache. The sampled
movement source/root remains monotone in both trials. This localizes the next
boundary to cache production or consumption; it does not yet assign collision,
terrain or physics ownership.

## Acquisition and cleanup

Session: `20260921T043926Z-pid5160`.

| Target | Rows | Valid | Measured cadence | Operator observation |
|---|---:|---:|---:|---|
| 60 | 181 | 181 | 60.001762 Hz | normal |
| 240 | 720 | 720 | 239.770143 Hz | visible hopping |

Both captures have finite values and consecutive broad-update indices. The
observer used one hook at `00D57900`, no debug registers, no vectored exception
handler and no call into the protected exchange. `CONTROL` records successful
arming, saving, target change and restoration. DS3 was then closed. The exact
DLL and injector used live are retained in the session; their SHA-256 hashes
match the pre-run qualified build (`7AFB521A...CDDA` and
`30676E83...546C`).

## Refuted vertical-override candidate

At both rates, cache owner `+0x2EC` (override gate) and `+0x2F0` (branch flag)
are zero in every row:

| Target | Gate nonzero rows | Flag nonzero rows |
|---|---:|---:|
| 60 | 0 / 181 | 0 / 181 |
| 240 | 0 / 720 | 0 / 720 |

Consequently `005C8100` cannot have selected `+0x2E4` through the previously
reconstructed lane-7 override during these captures. The native recurrences
remain real exact-build dataflow and their continuous forms remain
mathematically valid, but neither is a justified patch for this symptom.

## Cache/source discriminator

The active route window is defined by nonzero planar progress. The source/root
vertical direction is inferred from its net signed displacement; an opposing
step is a per-update vertical step against that direction.

| Measure | 60 | 240 | 240 / 60 |
|---|---:|---:|---:|
| Active planar distance | 7.993284 | 9.116110 | — |
| Source opposing steps | 0 | 0 | — |
| Cache opposing-step count | 25 | 196 | — |
| Cache opposing steps / planar unit | 3.127626 | 21.500399 | 6.874x |
| Cache opposing distance / planar unit | 0.013623 | 0.074464 | 5.466x |
| Peak opposing cache step | 0.018501 | 0.044456 | 2.403x |

The source/root is bit-identical and has zero opposing step in both captures.
The cache changes in 73.89% of intervals at 60 and 78.72% at 240, so the
high-rate result is not explained by a broadly frozen cache. Source/cache
vertical-delta correlation falls from `0.618279` at 60 to `0.264453` at 240.

These measures are route-normalized because the traversals are not identical
in length. They establish a strong cadence-linked cache divergence in the same
trial where the operator saw hopping. Broad-update sampling cannot exclude an
intra-update source excursion that has already settled by the hook, and visual
observation remains a separate `OBSERVED` input. The result therefore
localizes a boundary; it does not yet prove which cache producer or consumer
causes the rendered motion.

## Next discriminator

`005C8100` obtains its base 48-byte transform from `009D97B0`, passes it through
`00D99C40` (which statically preserves translation), then through protected
`00D9A3C0` and an optional local transform before the cache writers copy it.
The smallest next measurement is a replacement one-hook observer at
`009D97B0`, filtered to the current player's movement object. Recording the
input position and returned base-transform translation determines whether the
opposing vertical motion already exists before the protected/downstream cache
operations. No correction is justified before that boundary is resolved and a
general time-based equation is identified.
