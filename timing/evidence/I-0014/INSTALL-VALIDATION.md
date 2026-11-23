# I-0014 reversible installation validation

Date: 2026-09-22

Dark Souls III remained closed throughout. ReShade 6.8.0.2155 was present and
no older known timing add-on conflicted with the target.

The hash-guarded manager completed this offline sequence:

1. install the bundled add-on;
2. verify installed SHA-256 equals bundled SHA-256;
3. uninstall the same hash-matched file;
4. verify the add-on is absent;
5. reinstall the bundled add-on for Thomas's manual test;
6. verify installed SHA-256 equals bundled SHA-256 again.

Final installed file:

- `C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DS3UnifiedTimingFixTest.addon64`
- 195,584 bytes
- SHA-256 `6341A19F0C2FBE5C9461BF960F1DC006C08E710CDBED82ED252D783DFF281904`

The SHA-256 values of existing `dxgi.dll`, `ReShade.ini` and
`ReShadePreset.ini` (where present) were unchanged across the cycle. No game
executable, game data or ReShade configuration file was edited. Removal remains
available only while the game is closed and only if the installed file still
matches the bundled artifact.
