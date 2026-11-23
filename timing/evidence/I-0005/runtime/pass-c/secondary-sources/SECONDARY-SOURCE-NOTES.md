# Secondary historical/source leads for Pass C

These sources are not canonical evidence for the current executable. They supply pointer/field hypotheses that must be revalidated on the C1 build.

## DS3DebugFPS

- Repository: `https://github.com/0dm/DS3DebugFPS`
- Inspected commit: `b9e5a8ca8390ed0fd545b33944103c53926b7720`
- Inspected file: `src/exe/main.c`
- File SHA-256: `04A071D2C263636086976149F7CE14801A970B00289801DC4EC88FC1110876E4`
- Historical lead: dereference executable-relative `0x489DD10`, then write target float at `+0x354` and enable byte at `+0x358`.
- Current-build disposition: independently CONFIRMED by Pass B; the external source is corroboration, not the proof basis.

## The Grand Archives DS3 Cheat Table

- Repository: `https://github.com/The-Grand-Archives/Dark-Souls-III-CT-TGA`
- Inspected commit: `6f6a1e0f726f313b43814c6fa939cf60f9d8b05b`
- Inspected file: `DS3_The-Grand-Archives.CT`
- File SHA-256: `6960CB9CBD52C50A7AE4E3AB1183AC9470675DE5DE3EF4261C8FACF5F942F536`
- Repository requirement states DS3 App ver. 1.15.2.
- Historical leads used for observation only:
  - `WorldChrMan -> +0x40 -> +0x28`, coordinates at `+0x80/+0x84/+0x88` and angle at `+0x74`;
  - local player at `WorldChrMan +0x80`, module aggregate at player `+0x1F90`;
  - current animation identity through module `+0x80`, ID at `+0xC8`;
  - animation clock through module `+0x10`, current/max at `+0x24/+0x2C`;
  - play-speed candidate through module `+0x28`, field `+0xA58`;
  - HP/stamina candidates through module `+0x18`, fields `+0xD8/+0xF0`.

## Current-build static revalidation

The table's historical `WorldChrMan` AOB family was searched in the exact C1 executable. At instruction RVA `0x004C283A`, bytes `48 8B 1D 77 D5 2B 04 ...` resolve the RIP-relative singleton global to RVA `0x0477FDB8`. This exact global identity is current-build OBSERVED static evidence. The downstream field semantics remain HYPOTHESIS until the 60-FPS discovery capture.
