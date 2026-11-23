# I-0005 Pass C — 60 FPS discovery/control result

## Decision

**PASS for the 60 FPS discovery/control gate.** The exact-build player, transform,
position/facing, animation, and XInput chains behaved coherently throughout the
bounded marker sequence. Requested cadence was reached, the semantic sampler's
direct cost is bounded, and both successful interventions restored the exact
original `30.0/0` timing state before hook shutdown.

This result qualifies the fields for matched Pass C comparison. It does not prove
gameplay invariance above 60 FPS, identify every animation ID, measure physical
scan-out, or promote a camera transform that was not captured.

## Primary 60 FPS discovery session

Session: `20260919T110038Z-pid27016`

- Capture span: 174.8824325 s.
- Broad updates: 10,494; measured rate 60.000309 Hz; median delta
  `0.0166666005` s.
- Presents: 10,512; measured rate 59.999905 Hz; 10,494 interval-0 and 18
  transition/startup interval-1 calls.
- Position and animation chains were valid in 100% of semantic samples.
- `WorldChrMan`, player, and transform each retained one non-null pointer value.
- Controller presence was observed in 100% of samples.
- Raw volume was 26,636,779 bytes, or 152,312.49 bytes/s.
- The timing override applied `30/0 -> 60/1` and restored `60/1 -> 30/0`, both
  with success `1`.

| Marker | Duration | Net displacement | Last-1-s displacement | Interpretation |
|---|---:|---:|---:|---|
| idle | 5.467 s | 0 | 0 | position/facing stable |
| sprint | 9.767 s | 39.518 | 0.069 | sustained movement then stop |
| roll | 8.100 s | 16.589 | 0 | movement and animation transitions observed |
| representative animation | 6.650 s | 7.934 | 1.362 | multiple coherent animation IDs/clocks observed |
| collision/obstacle | 7.467 s | 3.219 | 0.0056 | approach followed by position plateau |
| camera rotation | 8.850 s | 0 | 0 | camera input did not move or turn the player |
| movement/physics | 13.483 s | 5.633 | 0.713 | player transform responded during the segment |

Animation validity remained 100% in every marker. Animation IDs, clocks,
durations, and transitions changed during roll, attack, movement, and collision,
while the play-speed field remained `1`. This supports the candidate semantic
roles for comparison without assigning high-level names to individual IDs.

## Direct sampler-cost control

Session: `20260919T112723Z-pid11508`

The probe was extended to time the complete semantic-sample window: player and
input reads, log synchronization, and the semantic CSV write.

- Samples: 1,798 over 29.98 s at 59.95 updates/s and 59.95 Presents/s.
- Mean sampler window: 12.5661 us, approximately 0.0754% of a 60 FPS frame.
- Maximum observed sampler window: 562.2 us, approximately 3.37% of a 60 FPS
  frame.
- Raw output rate: 152,846 bytes/s.
- Position and animation chains were valid in 100% of samples.
- Exact restoration `60/1 -> 30/0` succeeded before completion.

The cost result is a direct in-process QPC measurement for this sampler window.
It does not isolate every hook's total CPU cost, but achieved cadence and the
small measured window satisfy the bounded discovery gate.

## Excluded attempts retained for provenance

- `20260919T104934Z-pid17512`: operator aborted after a controller problem;
  no markers and no valid gameplay pointer chain. Process exit destroyed the
  memory state, but no ordered restore record exists. Not used for conclusions.
- `20260919T110920Z-pid27016`: same-process overhead reinjection encountered the
  already-signaled named stop event; zero semantic samples and no override.
  Not used for conclusions.

All raw files are retained unchanged. Derived metrics and SHA-256 values are in
`analysis/<session>/analysis-summary.json`; marker metrics are in
`marker-summary.csv`.

## Gate and next action

The 60 FPS semantic-field and overhead prerequisites are satisfied. The primary
discovery session is the 60 FPS comparison baseline. The next permitted condition
is the same bounded sequence at 90 FPS. Conditions 120 and 165 remain ordered
behind it; 240 and 280 remain gated on stability of all lower conditions.
