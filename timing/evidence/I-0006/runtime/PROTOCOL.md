# I-0006 operator protocol — final-writer input mirror

This protocol executes the bounded acquisition authorized by D-0009. The
capture schema, algebra, validity gates, and discrimination rule remain in the
canonical writer-algebra bridge; they are not restated here.

## Preconditions

- Exact I-0001 build and offline conditions.
- Active Windows 280-Hz display mode for target 165.
- `I0006WriterInputMirror.dll` and the analyzers pass their synthetic tests.
- No earlier Pass C DLL is loaded in the target process.
- The checked-in active Pass C configuration is disabled before and after the
  acquisition series.

## Strictly ordered acquisition

1. ChatGPT Work announces the required one-sentence discriminator and waits.
2. Thomas launches DS3 and places the character at the accepted fixed sprint
   anchor; ChatGPT Work prepares target 60 and injects the unchanged mirror.
3. After the ready signal, Thomas presses `F4`, then holds AZERTY `Z+Space`
   with neutral controller input through the one-accumulated-second warmup and
   three-accumulated-second measurement. He releases only after the automatic
   measurement end.
4. ChatGPT Work stops the probe, verifies `COMPLETE`, exact `60/1 -> 30/0`
   restoration, debug-register restoration, target cadence, one complete
   stimulus, and raw-capture integrity. Any failed gate stops the protocol.
5. DS3 is closed cleanly so the probe DLL is no longer loaded.
6. Steps 1–5 are repeated at target 165 with the same DLL hash and stimulus.
7. Only after both sessions pass their individual gates are bit-exact
   reconstruction and the ordered 60/165 comparison run.

## Stop conditions

Stop without interpretation on invalid source-chain reads, memory/register
mismatch, reconstruction mismatch, post-commit mismatch, overflow, unmatched
or duplicate mobile updates, failed cadence/stimulus, or restoration failure.
A changed source selection or inhibition frequency is reported first as the
categorical result; inhibited rows are excluded from numeric summaries.

The candidate source remains semantically neutral. Semantic Pass 2, roll,
correction, and 240 FPS remain closed.
