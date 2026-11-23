# I-0013 first-matrix cleanup

Date: 2026-09-21

Thomas followed the requested `Restore Native & Stop` then closed Dark Souls
III. The manager subsequently confirmed that no `DarkSoulsIII` process was
running and that the installed add-on exactly matched the validated bundled
SHA-256 `AF5C755A9840D71EF4682F1FCAB51954E3C4B431A98CCA77069DEE821967F691`.

All five external session files (`session-info.txt` and the four raw capture
CSVs) matched their preserved Evidence Store copies byte-for-byte by SHA-256.
The exact hash-matched add-on was then removed. A later preparation install for
a proposed speed repeat was immediately removed, without launching DS3, after
Thomas redirected the work to the terrain RCA. Final manager state:

- `GameRunning = False`
- `AddonInstalled = False`
- ReShade `6.8.0.2155` remains the unchanged host
- no executable or persistent game-data patch exists

Process exit removes all in-memory hooks and timing state independently of the
operator restore action. The UI action itself has no separate durable runtime
log, so this record retains Thomas's completion report rather than claiming an
instrumented button acknowledgement.
