# I-0084 - Audit des pièges DX12

Capability: C3 (Wave 2, stability, D-0200 fourth item)

## Question

For each classic D3D12 pitfall (pipelines compiled on the fly, barriers missing or superfluous, CPU/GPU synchronisation and
frames in flight, the video memory budget and leaks, descriptors, uploads at the wrong moment, driver and vendor
dependencies), where does the native backend stand today against Elden Ring and ER's known defects, which measurable check
shows it, and which pitfalls can be removed now without a redesign that belongs to a later wave-2 workstream?

## Scope

Included:

- The leads A1 to G3 of [I-0080 ER-STUDY](../I-0080-er-dx12-pitfalls/ER-STUDY.md) section 8, each with its current state at
  `main` 68b392d4 (re-read in the code, not copied from the study), ER's treatment and defect, a measurable check, and a verdict:
  solved (with its evidence), fixed here, measured and kept (why), or deferred to a named later workstream (why).
- Fixes small enough to land in this workstream and that remove a pitfall rather than add a path beside it (WORKFLOW
  « Optimise, do not stack »): the video memory budget read and acted on (D1), the leak counters the night endurance test needs
  (D2), full GPU drains left in play (C1), start-time feature checks with a message (G2), resource names for DRED and captures
  (G3), and the others the inventory shows to be bounded.
- Live checks under D-0116/D-0117 at Firelink and witness zones: GPU-based validation (B1), a cold pipeline cache (A1), a run
  under an artificially limited budget (D1), the sampler's view of the waits (C3), a WARP start (G2/G3); the replayed path gate
  before the merge (D-0202).
- The exit heap fault 0xC0000374 (lead D2): its kept dump `i82-w1-dred` read for the faulting stack.

Excluded:

- Redesigns already placed elsewhere in the ROADMAP: bindless binding and a new root signature (E2), a COPY queue for creation
  and streaming uploads (F1), aliasing of the transients by the placement planner (D3), compiling a missed pipeline off the
  frame path (A1's second half; it conflicts with D-0197 « no pop-in » unless the draw waits, to be designed with the
  precompile screen). The audit records their state, check and measurement, not their implementation.
- AMD and Intel hardware runs (no such card on Thomas's PC; WARP stands in, testers before a public release).
- The night endurance test itself (D-0200 fifth item; this workstream gives it its counters).

## Inputs

- I-0080 ER-STUDY (ER decompiled pitfalls, the leads), I-0082 RESULT (minidumps, DRED, the debug layer opt-in, K-0464,
  D-0201), I-0083 RESULT (the gate, K-0465, D-0202), I-0079 and I-0081 (stutter, streamed memory pools, K-0462, K-0463),
  I-0072 (pipeline cache), D-0171 (frame graph barriers), D-0187 (two frames in flight), D-0197 (no pop-in).
- ER: the decompilation readings of I-0080 (ER 2.7.1, read-only); this audit relies on them only where it states ER's
  treatment, and re-checks an address only if a design depends on it.
- Tools: `native/frame/Build-GameBootstrap.ps1`, clangd, `Run-StartupTrace.ps1`, the I-0079/I-0081 readers,
  `scripts/Invoke-ReplayGate.ps1`, `native/frame/Get-SampleProfile.ps1`.

## Hypotheses

- H1: no D3D12 validation error or warning is reachable on the Firelink path and the witness zones under GPU-based
  validation (the frame graph covers every hazard; the raw barriers outside it are correct).
- H2: the video memory is not budget-aware today, and a budget reaction on what is kept (spare heaps, staging, upload
  chunks) keeps the local usage under an imposed budget without a stand-in or a pop-in.
- H3: full GPU drains still happen in play outside resize and close (gpu_audit.drains > 0 on a zone walk).

## Expected evidence

Per lead: the code site (file:line at the audit's base), the counter or log row that measures it, a live run where the lead
needs one (run name under `evidence/I-0066/artifacts/`), and the before/after numbers for a fix (measure mode, no debug layer,
the same save and path).

## Exit conditions

- Success: every lead A1-G3 has a verdict with its evidence in `PITFALLS-AUDIT.md`; the bounded fixes landed with their checks;
  step reviews and the final Opus review applied; the replayed path gate passes on the final tree.
- Refuted: a hypothesis contradicted by its run (recorded as such, e.g. validation errors found and then fixed or listed).
- Insufficient information: a check cannot run on this PC (AMD/Intel, a real TDR); recorded with the stand-in used.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0116/D-0117 (offline, protected files restored by hash, nothing installed in the game folder), each announced
in the thread at least 5 minutes before with the PC clock, one live test at a time, Eta's or another non-Alpha Shebang test save;
measurement runs without the debug layer. A threshold rise of the gate stays Thomas's decision.
