# I-0099 - Plantage à la fermeture (the exit heap fault study, read-only)

Capability: C3

Number: I-0099 given by the coordinator (2026-10-23 23:34 Paris), so that the stutter's tranche 2 keeps I-0097 or takes I-0098.

## Question

The heap fault `0xC0000374` raised at every quit (ntdll offset 0x118095): does our module cause or worsen it, or is it the
original game's own defect? Where is it raised, by which code, and what workstream would remove it?

## Scope

Included:

- The existing traces only: Windows' Application log (event 1000) and its archived error reports, the 27 minidumps kept by
  `exit-dump.flag` (I-0082, I-0085 to I-0097), the run journals, the decompilation exports.
- Bounded read-only Ghidra sessions on the mirror `ghidra-mirror-i0066`, outside the pilot's announced live tests.
- A workstream plan: the captures to make (vanilla and native, page heap), what each would show, and the fix's options.

Excluded: any change to `native/`, any build, any live run (a live capture is proposed in the plan, announced and coordinated
with the stutter pilot, never started by this study), any system setting (page heap, error-report keys), any merge into `main`.

## Inputs

- Claims: K-0464 (I-0082: the fault raised on a worker thread while the main thread ends the process).
- Results: I-0082 RESULT §5, I-0084 PITFALLS-AUDIT D2, I-0085 RESULT §7, I-0092 RESULT « The exit heap fault ».
- Artifacts: `evidence/I-0066/artifacts/*/crashes/startup-crash1.dmp`, `evidence/I-0066/artifacts/i92-13-original-quit`.

## Deliverable

`STUDY.md` (findings with their evidence and status, the plan); a copy in the project's files.

## The fix workstream (FINISH-LINE line 2, opened 2026-10-28)

Question: does DS3's own stop of its network thread, run where DS3 forgets it (FIX-PLAN §2), remove the exit heap fault without
changing anything else of the quit?

Included: the hook at `~DLNRD::SocketManager` (0x1419853A0, one new pin), native mode only, inert under
`no-exit-order-fix.flag`; its row `native_exit_network_stop`; the pilot's re-read of the FIX-PLAN's addresses (GhidraMCP,
read-only). Excluded: the live A/B of FIX-PLAN §4 and the handler's exit-phase special case (its step 4), both waiting for
evidence: no live run before a merge (D-0229); the start checks' close-box quits, the natural quits in Thomas's play and the runs he asks for are
the sample. Exit: the hook written, built with the unit tests, reviewed (step and final), merged; the RESULT says the fix is
not run.
