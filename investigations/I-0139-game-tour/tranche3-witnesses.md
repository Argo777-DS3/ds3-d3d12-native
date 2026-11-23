# I-0139 tranche 3: the YEBIS study's cutscene witnesses (first runs)

Run through [Run-I139Boss.ps1](Run-I139Boss.ps1) (one invocation each, its `-ExtraFlags` clearing the scene's « seen » flag before the
travel; the boss revived by `test-revive.txt`), natively then in the original, from sheets/cutscenes-events.md C01 and C31:

| Witness | Row | -ExtraFlags | -Steps | Why |
|---|---|---|---|---|
| C01 Vordt's intro `s30_00_0030` | `c01-vordt=323,280\|27.6 -25.0 20.2\|13000800` | `13000801 0` | `115,0;w30;w30;117,0;w40` | event 13005810: the scene unless 13000801; the trigger region 3002801 at 27.6 -27.6 20.2 (y raised to the arena floor of `c0000_0004`, -25.0) |
| C31 Friede's intro `s45_00_0000` | `c31-friede=923,412\|-244.7 27.5 -230.0\|14500800` | `14500801 0` | `115,0;69,200;w10;69,200;w30;w30;117,0;w40` | event 14505810: the prompt (text 9373) on Friede, part 4500803 at -246.7 27.3 -230.0; the point 2 m from it toward `c0000_0001` |

C30 (the ending scene `s41_00_0000`) waits for the Soul of Cinder's death (14101100) and its prompt: tranche 5.
