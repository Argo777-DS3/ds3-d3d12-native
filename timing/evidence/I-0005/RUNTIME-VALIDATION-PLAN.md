# I-0005 runtime validation gate

This plan records the staged live experiment required because the central producer passes through a protected/flattened control-flow boundary that the current Ghidra project cannot reconstruct reliably. Pass A and reversible Pass B were separately authorized and completed on 2026-09-19.

## Hypotheses to discriminate

1. The float at the update context `+0x08` is the principal elapsed-time value distributed by `DS3-1152-F-000499C0`.
2. Central timing fields `+0x264`, `+0x268`, `+0x26C`, and the histories beginning at `+0x27C`/`+0x2C4` contain related raw, clamped, quantized, or domain-specific deltas.
3. `+0x354` and `+0x358` control a cadence target/override on this exact build, but changing them may affect presentation, engine update, or both.
4. Major consumers do not necessarily share one temporal contract; animation, camera, movement/gameplay, and physics must be identified from observed callbacks before any modification.

## Pass A — observation only — completed

Run at the stock 60 FPS behavior without changing timing state.

- Log QPC, thread id, and call sequence at `DS3-1152-F-00D57900`, `DS3-1152-F-00023410`, `DS3-1152-F-000499C0`, `DS3-1152-F-00D5E30`, and `DS3-1152-F-0192CF00`.
- At `00D57900`, log the context pointer and float at `context+0x08`.
- At the protected timing update boundary, snapshot only the bounded central object fields listed in `static/central-timing-state.csv`; do not patch them.
- At `00DD4B60`, log selector `object+0x40`, input value, and output `object+0x38`.
- At `000499C0`, record direct/indirect target addresses and the forwarded float so recurring major consumers can be clustered without inventing class names.
- Correlate update counts with Present counts and I-0004 manual-pacing activation.

Pass A succeeded: the log demonstrates stable ordering, cadence, field relationships, and re-identifiable consumer addresses. See `runtime/pass-a/PASS-A-RESULT.md`. No patch was applied.

## Pass B — reversible cadence comparison — completed

Only after Pass A succeeds, change the already identified central override fields in memory (not on disk) for controlled 60, 90, and 120 FPS runs. Repeat the same scene and input trace offline.

Measure:

- Presents, broad-update calls, and candidate consumer calls per second;
- values and distributions of context `+0x08`, central timing fields, selector input/output, and I-0004 pacing state;
- elapsed wall time for the same scripted input interval;
- animation phase/duration, player displacement, camera response, and physics/collision outcomes at matched wall-clock checkpoints;
- fixed-step counts, repeated/zero/clamped deltas, and divergence between render and simulation cadence.

The experiment must not infer correctness from a higher Present rate alone. A modification candidate is acceptable only if durations, distances, animation phase, collision outcomes, and behavior remain invariant within a predeclared tolerance.

Pass B established the causal cadence/delta/presentation reach of the central pair and two bounded delta-integrating consumers. See `runtime/pass-b/PASS-B-RESULT.md` and `runtime/pass-b/PASS-B-PROTOCOL.md`.

The repeated gameplay sentinels were human-operated rather than an exact input replay, and sampled objects do not yet have defensible high-level identities. Therefore animation, displacement, camera, collision, and physics invariants remain the next separately defined validation gate; they were not silently promoted to PASS from operability alone.

## Files and game-directory impact

- No existing game file should be replaced or edited.
- Instrumentation source, binary, configuration, and logs should live under a new repository investigation/evidence directory.
- If injection is required, use a reversible external launcher or debugger attachment declared before the run; do not deploy a proxy DLL into the DS3 installation.
- Run offline to avoid exposing instrumentation to online services.
