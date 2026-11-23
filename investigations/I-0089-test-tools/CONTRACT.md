# I-0089 - Outils de test : boss ranimés et invincibilité

Capability: C3 (Wave 2, test tooling; not part of the production target, D-0084)

## Question

Can Thomas, in a test version only, bring back a boss he has already defeated in his end-game save (Shebang), or all of them,
and be invincible with a key (F4), without touching his own saves and without anything in the play version?

## Scope

Included:

- The 26 boss-defeated event flags of the game, read statically from the game's own scripts (`common.emevd` event 970 and the
  boss template 20005500 of the map scripts).
- A test mode of the native module behind `test-tools.flag` (placed only by the test launcher): F4 invincibility, the revival
  of the chosen bosses through the game's own event-flag setter `1404C5E20` (as the existing `witness-flag.txt`), F5 to revive
  again.
- A separate launcher `tools/Launch-DS3-DX12-Test.ps1` (and its two double-click wrappers) with the saves as a sandbox,
  restored by hash when the game exits; the play launcher is unchanged.

Excluded:

- Editing the `.sl2` file (encrypted, layout not established here); the flag is cleared in memory on a test copy of the run.
- Online play of any kind; a damage hook (the invincibility refills HP each game-thread step: a single hit of at least the
  maximum HP still kills, recorded as a limit).

## Acceptance

- The module builds, unit tests pass; the play launcher's folder never receives `test-tools.flag`.
- Live (after the PC is free, announced, offline, saves restored by hash): the F4 toggle refills HP under damage and gives the
  two beeps and the title; the F5 / launcher revive brings back at least one boss in its arena after a bonfire rest; the keys
  clash with no game binding.
- Opus step and final reviews applied; replayed path gate PASS before any merge (the change touches `native/`).
