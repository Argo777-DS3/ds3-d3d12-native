# I-0005 Pass C — sprint internal-stage observation design

## Outcome

The next acquisition is split into **two strictly ordered passes**. The first
pass discovers the exact instruction(s) that commit the qualified player world
position inside the already localized protected transition. The second pass is
allowed only after offline data-flow review has identified defensible input and
output vectors around that writer.

This is an observation design, not a correction. It does not authorize 240 FPS,
does not qualify roll, and does not assign root-motion, collision, or physics
names to unproven fields.

## Existing boundary and new static/runtime anchor

The accepted moving transition is:

```text
02344730 after, caller return RVA 00A1429C
  -> protected/runtime continuation
02344730 before, caller return RVA 00DE50EC
```

The qualified final position is the three-float region at player transform
`+0x80/+0x84/+0x88`. Static reconstruction confirms that `00DE50EC` is the
return after `00DE5040` directly calls `02344730(RDI+0x10, R14)`. The static
image around `00A1429C` still decodes as protected/transient bytes and cannot
support a semantic hook choice.

The existing matched logs provide one narrower runtime correlation anchor:

| Condition | `A1429C` context | moving `DE50EC` context | other `DE50EC` context | moving intervals |
|---|---|---|---|---:|
| target 60 | `0x0000009DDFCFF718` | same | `0x0000009DDFCFF648` (`-0xD0`) | 180 |
| target 165 | `0x0000007D7014F428` | same | `0x0000007D7014F358` (`-0xD0`) | 494 |

Within each session, the `self` values at both caller sites were stable. The
shared context identity is an observation/correlation key only; it is not a
high-level type or motion owner.

## Pass 1 — exact final-writer discovery at target 60

### Instrument

Extend the existing Pass C DLL with a disabled-by-default
`locomotion_write_watch` mode. It is valid only when deterministic sprint and
phase RCA modes are also enabled.

For the active trial/update thread only:

1. At `Hook2344730` return from caller `00A1429C`, require the shared-context
   identity for the current moving transition and snapshot the qualified final
   position.
2. Park that thread briefly inside the hook while the probe controller applies
   three local x64 data breakpoints to `transform+0x80`, `+0x84`, and `+0x88`,
   each configured for a four-byte write. This uses the three coordinate-sized
   watchpoints rather than guarding or scanning the containing page.
3. Return to the protected continuation. A vectored exception handler accepts
   only `EXCEPTION_SINGLE_STEP` events whose thread, boundary token, debug-hit
   bits, target address, and active trial all match.
4. For each accepted hit, record the post-instruction RIP, DR6 hit mask, full
   integer and SIMD context, arm-time position, current position, shared-context
   pointer, and bounded live instruction bytes around the reported RIP. Use
   `RtlLookupFunctionEntry` when available to record the containing runtime
   function range. Because an x64 data-breakpoint exception is reported after
   the accessing instruction, offline decoding must identify the immediately
   preceding store from the captured bytes; the exception RIP itself must not
   be mislabeled as the writer.
5. Keep the watch active until all changed coordinate lanes are accounted for,
   the next `02344730` entry from `00DE50EC` is observed, or a small hard event
   cap is reached. Then clear the debug registers before leaving the boundary.

The controller must preserve and restore the update thread's original debug
register state. Failure to arm, an unexpected exception/thread, a context
mismatch, or reaching the event cap aborts that trial rather than broadening the
capture.

### Why this is the minimum first instrument

- It watches the already qualified 12-byte final position, not an inferred
  movement structure.
- It is per-thread and write-only, avoiding page-wide read faults and blind heap
  scans.
- It produces the exact commit provenance needed before selecting any internal
  vector hook.
- It leaves one hardware-breakpoint slot unused and does not patch protected
  code or modify motion data.

### Pass-1 evidence and gate

Add `locomotion-write-watch.csv` plus one bounded runtime-byte artifact per
unique writer site. A target-60 deterministic sprint trial is sufficient for
discovery, but Pass 1 passes only if:

- every measured non-zero final displacement is covered by accepted watch hits;
- the writer site set is stable and lies inside the established
  `A1429C -> DE50EC` transition;
- post-hit coordinates reproduce the final transform sampled at the closing
  boundary;
- original debug registers and the central `60/1 -> 30/0` state restore exactly;
- no exception is accepted outside the gated update thread/window;
- the live bytes can be decoded and their immediate data source can be traced
  far enough to nominate a bounded applied-vector source.

If the last condition fails, stop with insufficient information. Do not run 165
and do not fall back to a whole-player or whole-heap differential scan.

## Offline bridge between passes

Decode the live writer bytes and trace the store source backward within the
bounded runtime function/continuation. Candidate vectors are promoted only when
instruction/data flow shows that they are:

1. consumed before a collision/application call or bounded solver region;
2. produced after that region and feed the final writer; or
3. the exact values committed to the final transform.

A bounded before/after snapshot of the already shared context may be used to
re-identify a data-flow-backed vector, but correlation alone does not assign a
semantic stage. If the trace exposes only the commit value, Pass 1 still
identifies the final writer but does not satisfy the gate for Pass 2.

## Pass 2 — three-stage matched vector capture

Only after the offline bridge identifies exact sites, add observation at those
sites and rerun the accepted deterministic sprint at target 60 and target 165.
For every broad update, record in one correlation row:

- input/request candidate vector before collision/application;
- output/applied candidate vector after collision/application;
- final committed transform delta;
- broad delta, update index, animation ID/progress, shared-context identity,
  writer RIP, and validity flags.

The analysis compares vector norm, direction, and norm per accumulated delta at
the three stages. The earliest stage whose target-165 / target-60 ratio departs
from the existing thresholds localizes the amplitude loss:

```text
request/root candidate -> applied/collision output -> final transform commit
```

Stage names remain `candidate` until the data flow and matched runtime relation
support them. A match between applied output and final delta can separate the
commit from upstream integration; it does not by itself identify the solver as
collision or physics.

## Explicit exclusions

- no gameplay correction or consumer compensation;
- no 240-FPS acquisition;
- no roll acquisition or inference from sprint;
- no Present patch, input substitution, game-directory write, or speculative
  field label;
- no repeat of the completed dispatcher pair unless required as the enclosing
  correlation/control for this new writer probe.

## Next checkpoint

Implement Pass 1 only, keep its configuration checked-in disabled, compile it,
and exercise its controller/CSV/restore paths synthetically. Immediately before
the first DS3 launch, tell Thomas and provide a short operator protocol. Thomas
remains responsible for the in-game position and `F4`, then held `Z+Space`,
inputs.

## Offline implementation status

Pass 1 is implemented in the Pass C probe behind
`locomotion_write_watch=0`. The probe refuses this mode unless deterministic
locomotion, phase RCA, the reversible timing override, and target 60 are all
enabled. It caps accepted hits at eight per boundary, stops on arm/restore
failure, records the full bounded context in `locomotion-write-watch.csv`, and
restores the target thread's complete original debug-register state.

The dedicated synthetic test compiled with `/W4 /WX` and passed: three
write-only watchpoints produced exactly three accepted exceptions and the
original debug-register state was restored exactly. The Pass C writer-watch DLL
also compiled with `/W4 /WX`. This is offline mechanism validation only; it does
not establish DS3 writer coverage, runtime overhead, stability, or any motion
stage identity.

## Pass-1 live result

The announced target-60 trial completed on 2026-09-19. All 180 moving boundary
intervals were covered by one exact writer at RVA `0x009DC580` inside
`DS3-1152-F-009DC4D0`: `MOVDQA [RBX+0x80], XMM6`. One thread, context,
transform, writer, runtime function, and byte window were observed; no overflow
or hook failure occurred, and timing state restored exactly to `30/0`.

Read-only static tracing shows that `009DC4D0` obtains a 16-byte candidate
through `RBX+0xB0` or `RBX+0xA8`, then nested `+0x40 -> +0x70`, blends it from
the current transform using `RBX+0x1C0`, and commits the result at `RBX+0x80`.
That establishes final-commit provenance but not a collision/application
boundary or requested/root-motion source. Therefore Pass 1's writer-identity
gate is satisfied while the offline bridge to Pass 2 remains unsatisfied.

See `PASS-C-SPRINT-FINAL-WRITER-RESULT.md`. Do not run the target-165 Pass 2
capture until a defensible pre/post application pair is identified.
