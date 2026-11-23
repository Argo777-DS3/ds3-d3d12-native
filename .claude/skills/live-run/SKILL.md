---
name: live-run
description: Prepare, announce, run and close a live Dark Souls III test on Thomas's PC (the one-minute start check after a merge, D-0229; otherwise on Thomas's request or when a plan he agreed to needs one). Use before launching the game.
---

# A live run

The rules are WORKFLOW « Live game rule »; the know-how is `.project/LIVE-RUN-NOTES.md`. In order:

1. **Free PC.** No `DarkSoulsIII` running and no other thread's run (thread list, `list_sessions`; the slot asked from the
   coordinator); builds and Ghidra may run during a run that does not measure, a measuring run has the PC entirely free
   (WORKFLOW); Thomas has not said « je joue ». Free disk space checked.
2. **Own build folder.** Build with `native/frame/Build-GameBootstrap.ps1 -Out <your folder>` (hidden, D-0199) and pass
   `-BuildFolder <your folder>` to the runner. Never write into `evidence/I-0066/artifacts/game-bootstrap-build` except the
   launcher's rebuild after a merge.
3. **Dry check.** The exact command line; its effect flags accepted by both groups of `Run-StartupTrace.ps1`'s `ValidatePattern`;
   a capture that writes where it should; a sampler whose start and end (`-EndMark`) fall inside the path; the test save and
   its SHA-256 (zone witnesses: Shebang, never Alpha unless from a copy for spells). Before a series of measurement runs, one
   short run proves that each measuring tool (PresentMon, the sampler, a counter) attaches and writes its data (Thomas,
   2026-10-27). A control run of a change covers 1280x720, 1920x1080 and one arbitrary window (`-GraphicsLevels`, Thomas,
   2026-10-27: one universal rule). A GPU capture: `pix-capture.flag` and F8 (or `Run-AlphaCast -CaptureAt`); `pixtool.exe` reads
   it, Windows developer mode being on since 2026-11-01 (Thomas; WORKFLOW « Tools »).
4. **Announce** in the thread at least 5 minutes ahead: read the PC clock just before
   (`[TimeZoneInfo]::ConvertTimeBySystemTimeZoneId([DateTime]::UtcNow,'Romance Standard Time')`), give « début HH h MM, fin vers
   HH h MM », whether it measures, the protocol in a few lines, the screen used (the ASUS by default, the OLED when justified).
5. **Run** from one process (`Run-LiveWitness.ps1`, `Invoke-ReplayRun.ps1`, `Run-AlphaCast.ps1`, the gate): offline, the save
   present at the start backed up and restored at the end by hash, nothing installed in the game folder. Do not edit any script
   the series calls while it runs. Every run checks its launch (`launch-check.json`).
6. **Close.** Check the restoration, then post « PC libre » with the outcome. A measurement taken while Ghidra, a build or a
   heavy agent ran is run again, not interpreted. Check `baseline.json` (`"measure": true`, `"debug_layer_disabled": true`)
   before interpreting.
7. Delete the run's disposable cache once the cycle that read it closes (D-0142); keep logs, tables and comparison pictures.
