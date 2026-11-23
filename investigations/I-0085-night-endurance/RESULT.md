# I-0085 result: the night endurance test

Complete (2026-10-22). The first long run found a real leak that the 15-minute gate had passed: the process's private memory
rose 151 MB at every zone visit (7.4 to 15.4 GB in 94 minutes) and every zone's late frames rose with it. Its cause was in the
native module: about 1,100 model layouts per zone load, never retired. With layouts retired with their material, the same
94-minute loop rises 2.7 MB a visit, with no drift and no fault. Two small slopes remain, recorded below. The gate now has a
memory leg, and a short endurance runs every night Thomas does not play (D-0204).

## Observations

1. **Tooling.** The native controller's bound and `Run-StartupTrace.ps1` go up to 7200 s. [Run-Endurance.ps1](Run-Endurance.ps1)
   runs the trace, an outside memory sampler (`memory.csv`), the title sequence and
   [Pilot-EnduranceTour.ps1](Pilot-EnduranceTour.ps1). The tour checks every state: the notices by Pilot-TitleContinue's pixel
   test, and the world and the title from the journal's `native_present_pacing` rows. It recovered the one quit menu its clicks
   missed (i85-long-2 visit 21: the clicks landed in System > Game Options). Unlike I-0081's tour (every other zone missed), no
   load was missed in the 125 visits in measure mode. [Get-EnduranceSummary.ps1](Get-EnduranceSummary.ps1) splits a journal into world and title
   stays, writes `visits.csv` and `summary.json`, and fits per-visit slopes.
2. **The leak** (`i85-long-2`, build-1 = `main` 95aec11c plus the bound; five Shebang witness zones, Alpha untouched, 54 loads of
   54, 94 minutes, measure mode).
   - The title's private memory went from 7,400 to 15,351 MB, +151.5 MB a visit, peak 16.3 GB.
   - Local video memory stayed level: +0.4 MB a visit, peak 2.1 GB against a 15.3 GB budget, no window over it.
   - Late frames per zone, frames over 25 ms per 1,000, first visit to last: Beta m31 1.0 to 6.3, Delta m50 1.7 to 5.8,
     Epsilon m45 0.7 to 251, Gamma m37 0.7 to 8.3 (60.0 to 52.9 frames/s), Zeta m51 1.4 to 2.9.
   - No fault.
3. **Cause, read in the code and the journal.**
   - The journal has 24,179 `native_model_layout_cpu` rows after 22 visits, about 1,100 a load, and no layout retirement.
   - `BuildModelLayout` keys a layout by its `NativeModelMaterial` object. Every FLVER load makes new materials.
   - The layout holds its material and its programmes: interfaces, reads, block plans, caches and bytecode copies.
   - Its engine prefix is never retired. In the original, the resolver's cache owns it, keyed by the shared MTD shader. The
     native path deliberately skips that cache (1B0040/1AFB50).
4. **Fix** (build-2, `18_resource_admission.inc`, `01_state.inc`, `09_model_admission.inc`).
   - A material records the layouts built for it. `DestroyModelMaterial` retires them: its FLVER resource's meshes go with it.
   - The scene records that still hold a retired layout keep it alive until they are rebuilt.
   - Its 0x538-byte prefix goes to a spare list. The next build reuses it, only when it still reads as ours: our table, the
     variants and the vector nil.
   - A material retired between the admission's two lookups is a soft miss, not a refusal.
   - A `native_model_registry` row every 600 frames.
   - `i85-fix-1` (30 minutes): the layouts return to 502-520 at every title stay, and their prefixes are reused (up to 1,133
     spares).
5. **The fixed build** (`i85-long-fix`, the same loop, 54 loads of 54, 93 minutes).
   - The title's private memory went from 7,396 to 7,630 MB, +2.7 MB a visit, peak 9.1 GB.
   - Each position's title level rose 6-25 MB from the second loop to the third, then a few MB a loop.
   - Late frames stay at their first visits' level in every zone. One period of 20 minutes (visits 32-42, about 02:35-02:55)
     was slower in all zones (p99 up to 19.5-23.6 ms, Gamma at 56.7-57.9 frames/s), then recovered. It came from the PC, not a
     trend.
   - No fault; the protected files restored by hash.
6. **Remaining slopes** (fixed build, small and linear): the CPU descriptors in use at the title +2.3 a visit (1,128 to 1,272),
   and the local video memory in a zone +3 MB a loop of five (title +0.5 MB a visit). That is about two resources of a few
   hundred KB per visit not retired. 144 descriptors and about 33 MB after 94 minutes: not a risk for a play session. Recorded
   for the nightly endurance to watch.
7. **Exit heap fault 0xC0000374.** Dumped at each native close with `exit-dump.flag` (i85-long-2, i85-fix-1, i85-long-fix). The
   faulting thread was already gone (report "thread not opened"), so there is no stack, as in I-0084. Pre-existing in the
   original (WAVE1-CLOSURE); not a native leak.
8. **GPU-based validation at the witness zones** (`i85-gbv-1`, build-2, the debug layer with GPU-based validation).
   - Undead Settlement (m31) ran 492 s in the world at about 11 frames/s, an enemy's attack included, with no error and no
     warning. Any error or warning ends the native run; the game was closed with the close box and its files restored.
   - Under validation, the zone's load outlasted the tour's 480 s world wait. The tour's recovery then pressed the title keys in
     the world. The run was stopped at 03:39, so the other four zones are not validated yet.
   - A tour under validation needs a world wait of 15 minutes or more.

## Conclusion

H1 was refuted, then fixed. The private memory leaked 151 MB a visit through the model layouts. With them retired it rises
2.7 MB a visit, mostly a first-loop warm-up. H2 holds, with two small slopes recorded: the streamed heaps, staging, pipelines
and layouts return to their level, while the CPU descriptors rise +2.3 a visit and the local memory +0.5 MB. H3 held on the
fixed build. On the old build the drift followed the leak. H4 holds for the faults: there was none in 216 minutes of zone
travels. Under GPU-based validation it holds at Undead Settlement only, where 8 minutes of play gave no error; the other four
witness zones are still to run.

## Uncertainty

- The engine is assumed never to retire a layout prefix itself (DestroyModelLayout never ran in 125 visits; a call on a spare is
  now tolerated) and its resolver heap to live as long as the process (a reused spare is checked to still read as ours).
- The remaining descriptor and local-memory slopes have no owner yet.
- One PC, one save, standing stays with camera turns: no walking, fights or deaths. A longer play session can reach other
  resources.

## Reviews

Step review of the tooling (Opus 5.5): three Medium applied:
- `-GpuValidation` broke the title step's timeout bound.
- A title stay included the next zone's loading screen.
- The zones were named by order, not by frame.

Eight Low applied or recorded. Review of the native fix (Opus 5.5): no High. One Medium applied: a material retired between
the admission's two lookups could refuse the run once its prefix served a new layout. Two Medium hardened: a spare is reused
only when it still reads as ours, and the destructions happen after the lock.

Final review (Opus 5.5, the whole branch): no High. Three Medium, applied:
- The gate's memory leg now measures every title stay over the same window, 10 to 22 s after the quit. The last stay had been
  measured longer, past the close.
- A crash in the leg (the game gone before its close, `close.txt`) fails the gate.
- The investigation records were finished.

Low, applied:
- The spare prefixes of another heap are passed over.
- The leg is named in the gate's record, with its module checked and its read errors made inconclusive.
- `-MemoryRun` needs `-Evaluate`.
- The leg's tour deadline is widened.

Low, recorded: a prefix the engine itself retired after its reuse would lose its new layout. The engine never retired one in
125 visits; see Uncertainty.

The replayed path gate (D-0202) with its new memory leg then ran on the final tree: `i85-final` PASS (2026-10-22 04:11).
- The mean of the two measure runs: frames over 34 ms 13.5, over 25 ms 92.5, held 49.5, game steps over 34 ms 2, against the thresholds 22, 105, 62 and 6.
- The validation run completed without a fault; the standstill pictures were 0.37 and 0.50 from the reference.
- The memory leg (`i85-final-z1`, Beta m31 four times): 3 MB a visit, against the threshold of 40.

## Artifacts

- Runs under `evidence/I-0066/artifacts/i85-*`: journals, `tour.txt`, `memory.csv`, `visits.csv`, `summary.json`, screenshots,
  exit dumps.
- Builds `evidence/I-0085/artifacts/build-1` (before the fix) and `build-2` (the fix).
- The scripts in this folder; the gate's memory leg in `scripts/Invoke-ReplayGate.ps1`.
