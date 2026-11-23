# I-0005 runtime Pass B protocol — reversible cadence comparison

## Authorization and safety boundary

Pass B was authorized as an offline 60/90/120 comparison using external instrumentation and reversible in-memory writes only. No file in the Dark Souls III installation was written or replaced. No permanent patch, arbitrary constant edit, or additional Present intervention was used.

The tested executable was `DarkSoulsIII.exe` 1.15.2.0, Steam BuildID `10167187`, SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.

## Intervention

For each process, the probe waited for the central timing object at global `0x14489DD10`, recorded the exact original values, then wrote:

- target FPS as a float at central object `+0x354`;
- enable byte `1` at central object `+0x358`.

At capture stop it first disabled the override, restored the exact original float and byte, verified the restored values, disabled hooks, flushed logs, and emitted `COMPLETE`. All accepted sessions restored `30.0 / 0` successfully. `force_present_zero` remained `0` in every session.

## Conditions

Three short title/startup sessions first established that the pair alone could reach 60, 90, and 120 FPS without a second presentation intervention:

- `20260919T092903Z-pid18800` — 60 target;
- `20260919T093011Z-pid10632` — 90 target;
- `20260919T093108Z-pid25696` — 120 target.

A longer 120-FPS session with callback-vector serialization established live target coverage:

- `20260919T093215Z-pid13708` — 120 target, operator played for several minutes.

The primary matched consumer comparison used the same probe build and a short operator-repeated sentinel sequence: idle, movement into an obstacle, camera rotation, one or two attacks, then ordinary play.

- `20260919T094057Z-pid12600` — 60 target;
- `20260919T094304Z-pid19080` — 90 target;
- `20260919T094516Z-pid27388` — 120 target.

The input sequence was human-repeated rather than replayed from an exact trace. It supports activation coverage but is not a deterministic displacement, camera-angle, collision, or animation-duration measurement.

## Measurements

All primary sessions recorded:

- override apply/restore state;
- `00D57900 → 00023410 → 000499C0` call order and delta;
- central timing snapshots and the relation to `+0x268`;
- `00DD4B60` selector mode/input/output;
- I-0004 pacing state and `Present` arguments/timestamps;
- aggregate calls, activity, delta, elapsed increments, resets, output, and local coordinates for `000EFDA0`, `000F0D40`, and `0021CC20`.

The probe used QPC at 10,000,000 ticks/s. `analyze_pass_b.py` deterministically produces each session summary. `compare_pass_b.py` consumes the three primary summaries and produces the cross-condition CSV/JSON tables.

## Display context

Thomas reported that G-Sync was enabled during the comparisons. The evidence therefore establishes process-side `Present` cadence and arguments under a VRR/G-Sync environment. It does not directly measure physical scan-out or establish behavior on a fixed-refresh display without VRR.

Module inventories show `C:\Windows\system32\nvspcap64.dll` in all three primary consumer-comparison sessions, but not in the three initial causal sessions or the detailed 120 callback session. It is an external NVIDIA capture/overlay component, not a DS3-directory proxy. The primary comparison is therefore not described as overlay-free. Because it was common to all three primary conditions and the causal cadence/delta result was independently reproduced in the earlier sessions without it, it is retained as an external-validity limitation rather than treated as a differential explanation.
