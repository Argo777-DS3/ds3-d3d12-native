# I-0012 independent functional test

Date: 2026-09-21

Evidence status: `OBSERVED` operator report, with ReShade lifecycle corroboration.
This was intentionally not an instrumented actor comparison.

## Operator observations

- The tool has no effect in Dark Souls III exclusive fullscreen.
- The controls function in standard windowed mode. The game offers no native
  borderless mode in the tested configuration.
- The bounded visual panel — Hollow, early-game dog and Iudex Gundyr — passed
  Thomas's functional/gameplay assessment with the correction enabled.
- A separate issue was observed on slopes and stairs: the player intermittently
  hops or jitters vertically while traversing terrain.

The actor result is a bounded visual pass only. Cadence, actor transforms,
animation boundaries, path distance and recovery durations were not captured,
so this evidence does not promote non-player timing invariance to `SUPPORTED`
or `CONFIRMED` and does not close C4.

## ReShade lifecycle corroboration

The preserved ReShade log records:

- loading `DS3TimingValidation.addon64` and registering `DS3 Timing Validation`
  against ReShade API 20;
- an exclusive-fullscreen transition followed by swap-chain resize and runtime
  destruction/recreation;
- the return to windowed mode with another runtime recreation;
- normal add-on unregistration and ReShade shutdown.

No add-on-specific error appears in the log. The fullscreen observation is
therefore a functional limitation, not evidence that the add-on failed its
initial load. The log does not expose the core's internal timing requests, so
it cannot establish the exact cause of the fullscreen limitation.

## Provenance and cleanup

- Original mutable game-directory log: 32,940 bytes, SHA-256
  `25C409A370F0BDF708E7BA5408531CCA399BC658D2A2C1A4967FA855499AA49C`.
- Repository copy: line-ending-normalized text, SHA-256
  `7E9A0B126D6D736839A52F27C410B4E3309E8A106BC4316FDBFE241568199BBF`.
- The installed add-on matched the validated build SHA-256
  `325C57AAD1A93C20C0D44E1F5F3B91E8C7ABB9A181FF6108B4421EEE15218910`.
- Dark Souls III was closed before cleanup. The hash-guarded manager removed
  the single `.addon64` file and then reported `AddonInstalled=False`.
- Process exit unregistered the add-on and removed the in-memory ReShade hooks.
- ReShade retained its own UI/configuration state in `ReShade.ini`; this is not
  a cadence or executable patch and was not rewritten without an exact original
  byte copy.

## Success boundary and successor

Thomas explicitly accepted I-0012 as successful at this bounded visual/tool
level and redirected the project to the newly observed terrain-collision issue.
The successor RCA must begin with the discriminating matrix:

1. 60 FPS — correction OFF (native reference);
2. 240 FPS — correction OFF;
3. 240 FPS — correction ON;
4. 60 FPS — correction ON when inexpensive, as a non-regression control.

The three primary cells distinguish a high-frequency issue independent of the
locomotion correction from a correction-induced side effect or a branch the
correction already covers. They do not yet identify collision ownership.
