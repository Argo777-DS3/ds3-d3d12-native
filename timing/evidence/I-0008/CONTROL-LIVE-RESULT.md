# I-0008 target-165 control result

## Bounded conclusion

The announced three-second target-165 observation closes the discriminator for
the qualified stationary player path. In all 495 complete chains, generator
`010768C0` supplies the same float32 delta as central timing field `+0x268`,
control `01284D70` consumes that value in mode 1, its native result reproduces
the player `SprjChrTimeActModule` ring value bit-for-bit, and the delta is not
fixed `1/60`.

This refutes hidden fixed-60 substitution on the observed mode-1 path. It does
not establish every animation mode, transition, identity, actor or framerate,
and it does not justify an animation patch or a whole-C4 correctness claim.

## Capture identity and safety gates

- Exact build: Steam AppID 374320 / BuildID 10167187 / executable 1.15.2.0,
  SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Session: `control-live-20260920T120006Z-pid20676`, stationary offline player,
  temporary central timing `30/0 -> 165/1 -> 30/0`.
- Raw capture: 4,149 records, SHA-256
  `E9A605E173B300D694A951601AA4E48B8A6451211D8BF91DB55770F34FC2E242`.
- Analysis SHA-256:
  `F5C1C76F4BDB3938DCDCEBED97C5AB78438C7AFDA12A3B39BAA02827DFB6ED16`.
- Observer SHA-256:
  `79077965DCA877BBAF697B9191506881B276E27AC2B068207AD6A83BAEB12B76`.
- Breakpoint hits: 1,245 generator, 1,245 control entry, 1,245 control
  return and 741 ring-writer events. The analyzer retains 495 exact chains.
- No unrelated control identity, leading/internal unmatched ring, ambiguous
  ring or analyzer failure remains. The bounded capture gate passes.
- The observer reread and verified both the original timing pair and saved
  debug state after capture. A later process check confirms PID 20676 closed;
  no live instrument remains.

## Live dataflow

All 495 qualified chains share one generator, control, player and module
identity. Every control row has mode 1, flags 0, speed 1 and duration 2. The
actual generator XMM2 delta and control XMM1 delta equal central `+0x268`
bit-for-bit in every chain. Their frequencies are:

| float32 bits | approximate seconds | count |
|---|---:|---:|
| `0x3BC697FF` | 0.006060599815 | 492 |
| `0x3BC6DEBD` | 0.006069032941 | 3 |

Both values differ from float32 `1/60`. The 495 native control results replay
the exact formula from `01284D70` and match the subsequent ring values
bit-for-bit; the set includes one loop wrap. The 495 ring events span
2.9937565 seconds, or 165.010080145 Hz.

The observed causal chain is therefore:

`central +0x268 -> generator XMM2 -> control XMM1 -> 01284D70 mode-1 formula -> ring writer 009B4F8D`.

## Retained cross-target comparison

The earlier retained audit contains 7,371 stable ten-update ring-refresh
intervals at targets 60, 90, 120 and 165. At target 165, 3,429 of 3,476 match
the preceding ten variable deltas within `1e-5`. The remaining 47 intervals
all span ten updates and every one is closer to the measured ten-delta sum than
to `10 * (1/60)`; none favors fixed-60 cadence.

Those 47 rows form 16 clusters and span several animation identities and marker
regions, including movement, sprint, roll, collision and idle. The retained
format does not carry the live control identity or mode needed to assign their
transition/selection cause. They remain an explicit unresolved boundary, but
they are not evidence for hidden fixed-60 substitution.

## Epistemic result

- **CONFIRMED:** on the captured target-165 stationary-player mode-1 path, the
  actual native timing argument is the variable central delta and all 495
  outputs reach the qualified ring exactly.
- **REFUTED within that path:** a fixed `1/60` timing substitution.
- **SUPPORTED across retained target-165 rows:** the ten-update ring cadence is
  governed by accumulated variable deltas; all 47 residual rows favor that
  model over fixed 60.
- **UNKNOWN:** the precise transition, selection or alternate-mode cause of the
  47 residuals, and behavior outside the captured mode, identity, actor and
  target.

Canonical machine-readable evidence is in
`artifacts/control-live-20260920T120006Z-pid20676/`; `summary.json` joins the
live chain to the retained residual audit.
