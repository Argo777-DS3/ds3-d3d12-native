# I-0142 - The native build in parallel (Ninja), then incremental

Capability: C3 (the tooling every workstream's fix waits on)

## Question

Can `native/frame/Build-GameBootstrap.ps1` run its compilations in parallel, with a module byte-identical to the serial build's,
and then rebuild only what a fix touches?

## Scope

Included (Thomas, 2026-11-16 12:41 Paris, « Vas y » after the [study](STUDY.md)):

- Step 1: the script writes its commands as a Ninja graph and runs them in parallel (`-Jobs`); `-Serial` keeps the old
  `build.cmd` path. Every edge runs (no incremental build), the unit tests as before.
- Step 2 (Thomas's « Je confirme », 12:55, D-0295, since it relaxes D-0260): objects kept in a per-thread cache (`-Cache`), MSVC's
  dependencies (`/showIncludes`), generated headers written only when they change; the unit tests stay as they are (a fix's
  iteration builds with `-NoControls`, the build before a commit with them).

Excluded: splitting `game_bootstrap.cpp`'s single unit (the study's step 3), precompiled headers, any change to what is compiled
(sources, tests, the flags that shape the code), the launcher's folder outside a merge. Amended at the merge: `/showIncludes`, `VSLANG`
and the build folder as the working folder changed every `/Z7` object's recorded command line and folder, not its code (RESULT).

## Inputs

- The study: [STUDY.md](STUDY.md) (timings of 2026-11-16's builds from their files' times).
- Tools: Ninja 1.13.2, shipped with Visual Studio 18 Insiders (`Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe`).

## Exit conditions

- Success (step 1): on one commit and one output folder, the serial build and the Ninja build give the same SHA-256 for every
  `.dll`, `.exe` and generated header, the unit tests pass, and the Ninja build's time is measured.
- Refuted: a hash differs for a reason that cannot be removed without changing the module.
- Budget exhausted: replan under `.project/WORKFLOW.md`.

## Safety and human interaction

No live run. Builds only in this thread's own `-Out` folder, at the slot the coordinator gives (another thread's run first).
