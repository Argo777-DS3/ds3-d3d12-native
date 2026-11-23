# I-0006 writer-input mirror instrument

This exact-build instrument is derived from the validated Pass C timing,
stimulus, and restoration framework. Its live scope is reduced to the broad
update/delta anchor, Present cadence, the already qualified dispatcher pair
used only to arm and disarm, and one per-thread execute breakpoint at RVA
`009DC559`.

The probe writes sessions under `../sessions/`, never into the DS3 directory.
The capture schema and interpretation law are canonical in
`evidence/I-0005/runtime/pass-c/PASS-C-SPRINT-WRITER-ALGEBRA-BRIDGE.md`.
`evidence/I-0006/runtime/PROTOCOL.md` is the operator procedure.

Build with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1
```

Validate the execute breakpoint and exact debug-register restoration without
DS3 with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test_writer_mirror.ps1
```

The checked-in configuration is disabled. Use
`../Set-WriterMirrorTarget.ps1 -Target 60` or `-Target 165` only for the
announced, operator-authorized acquisition, then run
`../Reset-WriterMirrorConfig.ps1` after the session.
