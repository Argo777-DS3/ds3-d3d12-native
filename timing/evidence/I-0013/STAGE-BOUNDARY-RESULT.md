# I-0013 broad-update stage-boundary result

## Outcome

The safe one-hook replacement completed target 60 and target 240 in one
process, then restored the original timing pair exactly. Thomas reported that
the behavior was normal. No crash, invalid row, update gap, pointer-identity
failure or overflow occurred.

This result does not reproduce the visible hopping from the first matrix and
does not yet identify a collision or terrain owner. It does refine the engine
map and removes one apparent downstream boundary.

## Acquisition

Session: `20260921T033747Z-pid1624`.

| Target | Rows | Valid | Measured cadence | Active planar distance |
|---|---:|---:|---:|---:|
| 60 | 181 | 181 | 60.000168 Hz | 10.286522 |
| 240 | 719 | 719 | 239.647031 Hz | 8.898957 |

The target-240 cadence is the process cadence actually measured during the
capture. Thomas notes that Windows was configured for 280 Hz with G-Sync; that
display setting is context, not a substitute for the measured update rate.

The observer used exactly one hook at `00D57900`, no debug registers, no
vectored exception handler and no protected-exchange call. `CONTROL` records
successful ready, both capture saves, the 60-to-240 transition and restoration.
The exact live DLL and injector are retained with the session because the
offline source was subsequently corrected to label the cache matrix translation
lanes explicitly.

## Dynamic engine-map refinement

For every retained row at both rates:

- `WorldChrMan -> list+0x28` final transform and
  `PlayerIns -> modules+0x68` movement state are the same pointer;
- their sampled XYZ fields are bit-identical (181/181 at 60 and 719/719 at
  240);
- therefore the earlier “movement source versus final root” distinction
  collapses at this sampled field. It is one object/view, not two dynamic
  stages.

The direct `PlayerIns+0x50 -> +0x120` 48-byte cache is a 3x4 matrix-like layout.
Its translation occupies lanes 3/7/11, not lanes 0/1/2. All 12 lanes were
preserved bit-for-bit, so the initial column-label error caused no evidence
loss and the corrected analyzer decodes the raw block.

## Cache/source comparison

Within the active route window, cache-translation vertical minus source/root
vertical has RMS `0.084606` at 60 and `0.088441` at 240, a ratio of
`1.045333`. The corresponding raw distributions overlap substantially; this
does not show a large high-rate amplification at the sampled cache boundary.

The routes are not extent-matched: the 240 traversal covers 8.898957 planar
units versus 10.286522 at 60, about 13.49% less. Full normalized-route profile
correlations are therefore not accepted as an equivalence test. Monotonic
nearest-route matching finds only 27 strict pairs within 0.02 planar units. In
those pairs, source vertical differs by RMS `0.000339`, while the cache/source
gap difference has RMS `0.033164`. The small source difference supports common
terrain geometry at those matched locations; the sparse matches and phaseful
cache gap prevent a stronger cross-rate claim.

## Bounded conclusion

The safe observer resolves the crash problem and establishes that the sampled
“source” and “final root” are the same live object. It does not localize the
reported hopping to the cache: cache/source gap amplitude is close at the two
rates, while its route profile is not sufficiently matched to prove temporal
equivalence. Thomas's normal observation makes this a non-reproducing trial,
not a refutation of the earlier repeated high-rate symptom.

The next useful work is offline: reconstruct the cache matrix production and
its relationship to contact/ground-response state around `005C8100` and the
`005C6900/005C6F10/005CAA20` writers. A later dynamic test is justified only if
that work identifies a smaller safe field or a mathematical recurrence. No
correction is proposed by this result.
