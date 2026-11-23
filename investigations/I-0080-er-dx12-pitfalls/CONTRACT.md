# I-0080 - DS3 DX12 Vague 2: ER study of the D3D12 pitfalls (audit preparation)

Capability: C3

## Question

For each classic D3D12 pitfall (pipelines compiled on the fly, barriers, CPU/GPU synchronisation and frames in flight,
video memory budget and leaks, descriptors and their CPU cost, data uploads, driver dependencies), how does Elden Ring 2.7.1
treat it, where does ER itself have a known defect, and where does our native backend stand today? The answer is the
starting point of the « audit des pièges DX12 » workstream (D-0195: copy ER's architecture, without its known defects).

## Scope

Included:

- Read-only static decompilation of ER 2.7.1 (headless Ghidra on the `er-mirror-*` copies, Sonnet 5.5 workers, Opus checks
  the sources a conclusion relies on), reuse of the earlier ER studies (study716, ER-DEEPENING-897, D3D12-DESIGN-REVIEW,
  I-0072, I-0073, I-0076, I-0077).
- A static description of the native code (`native/`, `main` at 92f7b9ab; the I-0079 branch noted where it changes a point).

Excluded:

- Any change to the native code, any live run (the study thread shares the PC with the I-0079 tests), any merge into `main`.
- The audit itself and its fixes (the next workstream).

## Inputs

- Canonical claims: K-0454 to K-0461; D-0157, D-0171, D-0187, D-0195.
- Artifacts and sources: `C:/Users/User/Desktop/ER Decompilation` (read-only), the ER Ghidra mirrors, the studies above.
- Tools: Ghidra 12.1.3 headless (`-readOnly -noanalysis`, below-normal priority, only between the I-0079 tests).

## Hypotheses

- H1: ER already treats most of these pitfalls (frame ring, fenced upload ring, delayed release, pipeline cache).
- H2: ER keeps some known defects (synchronous pipeline compilation on the recording thread, no budget awareness) that the
  native backend must not copy.

## Expected evidence

Per pitfall: the ER functions (absolute VAs, decompile or assembly line) marked as read in the decompilation, kept apart
from what is deduced; the native code's file and line.

## Exit conditions

- Success: one study document (ER-STUDY.md) covering every pitfall, decompiled facts and deductions kept apart, and the
  audit's leads.
- Refuted: n/a (a study).
- Insufficient information: a pitfall whose ER treatment is not found statically is recorded as not found, with what was
  searched.
- Stagnation: no new discriminating evidence after 3 cycles on one pitfall.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

No live-game launch, no irreversible change. Ghidra and the workers run only after the I-0079 thread's « PC libre » and are
stopped before each of its announced tests.
