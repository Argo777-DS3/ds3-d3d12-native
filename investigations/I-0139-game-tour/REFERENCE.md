# I-0139 reference: the tour's runs, kept for a replay

Thomas, 2026-11-15: every test run of the tour becomes a reference, and a full tour is played again at the project's end. This page
says what each run set was (runner, rows, prefixes, screen, save, count) so that the replay can repeat it and be compared. The
per-run facts are in [REFERENCE.tsv](REFERENCE.tsv), made by `python -I tools/Make-I139Reference.py` from the run folders
`evidence/I-0066/artifacts/i139-*`, the logs `evidence/I-0139/artifacts/runs-*.log` and the module files of
`evidence/I-0139/build-tour*`. Facts come from the runners' headers, the logs and RESULT.md; anything inferred is marked INFERRED.

## What the TSV records and what it cannot

- The reader's run-name pattern takes several letters after the tranche's digits since 2026-11-15 (`t4kb`; before, one letter, so
  `i139-t4kb-keeper-orig` read « ? »); only the tranche column changed (the integrity review's L3).
- 433 run folders on 2026-11-16 00:40 (Paris): 233 native (`nat`) and 200 original (`orig`). One row per folder; regenerate the TSV
  after each series (`python -I tools/Make-I139Reference.py`) and read the counts from it.
- `module_sha256` is the run's own record: `baseline.json`'s `module_sha256`, which Run-StartupTrace writes for every Native and
  Reference run (the original renderer's runs load the module too, for the pacer, the world gate and the test tools). `build_folder`
  is the `evidence/I-0139/build-tourN` whose `game_bootstrap.dll` has that hash (read, not inferred; every run matches one). `args`:
  Run-AlphaCast's arguments (`alphacast-args.json`), from 2026-11-15 only; before it, the arguments are the runners' and the rows'
  below and the RESULT's.
- The build folders are not changed since: the SHA-256 of each `game_bootstrap.dll` equals the hash printed by its
  `build-tourN.log`. The build logs name no commit. The mapping to main commits below comes from RESULT.md.
- `screen` and `window_rect` come from the log line « placed window=... screen=... rect=... ». Windows renumbers the displays when
  the OLED is switched on or off, so `\\.\DISPLAY1` means the ASUS in some runs and the OLED in others. Read the rect: 0,0,1296,759
  is the ASUS (key `1920x1080@60`, the game window smaller than the screen); 0,0,2576,1460 is the OLED with a 2560x1440 window.
  A rect of 0,0,43,19 (two runs) is a window caught before it was resized.
- The night of 2026-11-14/15 mixed both screens: the cutscene redo and the Firelink and places NPC runs on the OLED, then the last
  places runs (`i139-t4q-*` and `i139-t4r-*`, 03:47-07:18 UTC) on the ASUS (reported as `\\.\DISPLAY2`). A replay must name one screen.
- `restored` is `ok` (the log line « restoration ok: True ») plus `json` (restoration.json present); every run has both.

## Common to every runner

- Save: Eta's zone save `C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2`, SHA-256
  `1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4`, copied in and continued (`-Continue`), the user's files
  restored by hash afterwards. Every runner names this one.
- Driver: `investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1`; native runs use `-BootstrapMode Native`, originals
  `-BootstrapMode Reference`; offline; one game process at a time.
- Screen argument `-Screen '1920x1080@60'` by default (the ASUS, by its key via `scripts/Get-Screens.ps1`). The Cutscenes, Npc and
  Credits runners take `-GraphicsLevels` for the OLED window (`ScreenMode=WINDOW;Resolution-WindowScreenWidth=2560;Resolution-WindowScreenHeight=1440`, Thomas 2026-11-14).

## Tranche 1: the bonfire rows (zones)

| Item | Value |
|---|---|
| Runner | `Run-I139Tour.ps1` (travel to each bonfire, look sweep `4,-300,2`, native then original) |
| Rows files | `tranche1-half1.txt` (28 rows), `tranche1-half2.txt` (27 rows); the last two rows (`runs-t1c.log`) INFERRED from a retry list given on the command line |
| Prefix | `i139-t1-` (labels `z01-...` to `z7x-...`) |
| Screen | ASUS, window rect 0,0,1296,759 (all 110 runs) |
| Build | `build-tour1`, main `8f415270` (RESULT); all 110 by their module hash |
| Logs | `runs-t1a.log`, `runs-t1b.log`, `runs-t1c.log` |
| Runs | 110 (55 native, 55 original), 2026-11-12 19:00 UTC to 2026-11-13 02:34 UTC |

Also with this runner: `i139-v1-z17-halfway-*` (2 runs, the runner's validation, `runs-v1.log`, one row on the command line,
`build-tour1`).

## Tranche 2: the bosses

| Item | Value |
|---|---|
| Runner | `Run-I139Boss.ps1` (travel to the bonfire, teleport to the fog gate, F4 invincible, steps, F6 the kill; native then original) |
| Rows files | `tranche2-night1.txt` and `tranche2-rows.txt` (25 rows) for `i139-t2-`; `tranche2-rest.txt`, `tranche2-rest2.txt`, `tranche2-rest3.txt`, `tranche2-rest4.txt` for `t2b`, `t2c`, `t2e` (INFERRED from the order of the logs, not stated in a log); `retry-unreached.txt` (4 rows) for `i139-t2f-`; `verify-ui-overlay.txt` (3 rows) for `i139-v2-` |
| Prefixes | `i139-t2-`, `t2b-`, `t2c-`, `t2e-`, `t2f-`, `v2-` (labels `bNN-name`) |
| Screen | ASUS, rect 0,0,1296,759 |
| Builds | `i139-t2`, `t2b`, `t2c`, `t2e`: `build-tour1` (by module hash); `t2f` and `v2`: `build-tour2` (main `63b113d9` merged, RESULT) |
| Logs | `runs-t2a.log`, `runs-t2b.log`, `runs-t2c.log`, `runs-t2d.log`, `runs-t2e.log`, `runs-t2f.log`, `runs-v2.log` |
| Runs | 64 (`i139-t2*`) + 3 (`i139-v2-*`, all native); 2026-11-13 02:38 UTC to 12:48 UTC |

Also with this runner: the boss-runner trials `i139-b1-b01-iudex-nat/orig` and `i139-b2-b01-iudex-nat` (3 runs, 2026-11-12 11:24 to
11:40 UTC, `runs-b1.log`, `runs-b2.log`, `build-tour1`).

## Tranche 3: the cutscenes by id

| Item | Value |
|---|---|
| Runner | `Run-I139Cutscenes.ps1` (travel to a bonfire of the scene's map, `-Cutscene <id>`, shots through the scene; native then original) |
| Rows files | `tranche3-all.txt` (59 scenes) for `t3c-`; `tranche3-redo.txt` (21 scenes) for `i139-t3m-`; `tranche3-redo2.txt` (3 scenes) for `i139-t3n-`; `i139-t3-` and `t3b-` were the witness rows of `tranche3-witnesses.md` (labels `c01-vordt`, `c30-ending`, `c31-friede`) |
| Prefixes | `i139-t3-`, `t3b-` (witness labels), `t3c-`, `t3m-`, `t3n-` (labels `cNNNNNNNN`) |
| Screen | `t3`, `t3b`, most of `t3c`: ASUS 0,0,1296,759; later `t3c` runs (25), `t3m` and `t3n`: OLED 0,0,2576,1460; two `t3c` runs have rect 0,0,43,19 |
| Builds | `t3` `build-tour3`; `t3b` `build-tour4`; `t3c` `build-tour5` (first six scenes, main `d1f0d340`) then `build-tour6` (main `e03ae881`) then `tour7` (main `a1a4138a`), `tour8` (`0b162a28`), `tour9` (`51df978b`), `tour10` (`d1fbc6f51`); `t3m` and `t3n`: `build-tour10` (each run's build in the TSV by its module hash) |
| Logs | `runs-t3a.log`, `runs-t3b.log`, `runs-t3c.log`, `runs-t3m.log` (also `t3n`) |
| Runs | 152 (`t3` 4, `t3b` 6, `t3c` 118, `t3m` 21, `t3n` 3); `t3m` and `t3n` are native only; 2026-11-13 15:19 UTC to 2026-11-14 21:30 UTC |

## Tranche 4: menus, the Fire Keeper, NPCs, places

| Item | Value |
|---|---|
| Runners | `Run-I139Menus.ps1` (options, menu and keeper tours, `-Tours`), `Run-I139Npc.ps1` (a place's NPC points, `-Lift`, `-Offset`) |
| Rows files | Menus: none (`-Tours options,menu,keeper`); Npc: `tranche4-firelink.txt` (2 rows `fl1`, `fl2`) for `i139-t4-fl1-nat` (the trial), `t4b-`, `t4c-`, `t4d-`; `tranche4-places.txt` (19 rows) for `t4p-` and `t4q-` (the row-to-prefix link is INFERRED from the order of the logs) |
| Prefixes | `i139-t4-` (menus: options, menu, keeper, plus the `fl1` trial), `t4b-`, `t4c-`, `t4d-`, `t4p-`, `t4q-` |
| Screen | Menus (6 runs): ASUS 0,0,1296,759. Firelink NPCs (`t4`-fl1, `t4b`, `t4c`, `t4d`) and `t4p`: OLED 0,0,2576,1460. `t4q` (28 runs) and `t4r` (10 runs, the five `t4p` places again, native and original): ASUS again, reported as DISPLAY2 |
| Builds | Menus: `build-tour3` (main `5fa7c0bd`, RESULT; all six by their module hash); NPC and places runs: `build-tour10` (main `d1fbc6f51`) |
| Logs | `runs-t4a.log` (menus), `runs-t4n.log` (Firelink NPCs), `runs-t4p.log` (places, `t4p` and `t4q`) |
| Runs | 60 (`t4` 7, `t4b` 4, `t4c` 4, `t4d` 2, `t4p` 5, `t4q` 28, `t4r` 10); 2026-11-13 16:27 UTC to 2026-11-15 07:14 UTC; `t4p` native only (its places again in `t4r`) |

## Tranche 5: the credits

| Item | Value |
|---|---|
| Runner | `Run-I139Credits.ps1` (forced flag 20, shots 10 to 600 s; native then original) |
| Rows file | none (`-Flag 20`) |
| Prefix | `i139-t5-flag20-nat` and `-orig` |
| Screen | OLED, rect 0,0,2576,1460 |
| Build | `build-tour10`, main `d1fbc6f51` (RESULT) |
| Log | `runs-t5.log` |
| Runs | 2 |

## The later sets of 2026-11-15

| Item | Value |
|---|---|
| Runners | `Run-I139Boss.ps1` (`t2g`, `t2h`: Wolnir, the Twin Princes, the Nameless King, the Gravetender, the Wyvern by their arrivals); `Run-I139Series6.ps1` (`t2i`: the Princes, the Nameless King, the Gravetender with `-WatchFlags`; `t3f`: Friede's intro by her event; `t4k`, `t4kb`: the Fire Keeper by `-TalkKeeper` on Run-AlphaCast's m45 save); `Run-I139Dlc.ps1` (`t5d`: the DLC entrances D01, D03, D04, D06); the final review's recipes: Series6 `-Part friede` (`t3g`) and `Run-I139Dlc -Only` (`t5e`: D06 with the goods given, D03 at two points) |
| Rows files | `tranche2-wolnir.txt`, `tranche2-princes.txt`, `tranche2-namelessking.txt`, `tranche2-gravetender.txt`, `tranche2-wyvern.txt`; the Series6 and Dlc rows are in the scripts themselves |
| Prefixes | `i139-t2g-`, `t2h-`, `t2i-`, `t3f-`, `t3g-`, `t4k-`, `t4kb-`, `t5d-`, `t5e-` |
| Screen | ASUS, rect 0,0,1296,759 |
| Builds | `t2g`, `t2h`: `build-tour11` (main `61b905572`); the others: `build-tour12` (native `a52d24e47`, the flag tool) |
| Logs | `runs-b3.log`, `runs-b4.log`, `runs-s6.log`, `runs-d.log` |
| Runs | 37 (`t2g` 4, `t2h` 6, `t2i` 6, `t3f` 2, `t3g` 2, `t4k` 2, `t4kb` 1, `t5d` 8, `t5e` 6); 2026-11-15 10:19 UTC to 23:25 UTC |

## Build folders and main commits

The build logs name no commit (they hold the compiler output and two SHA-256 lines, the first being `game_bootstrap.dll`). The commits
below are from RESULT.md only; `build-tour4` is the only folder whose main commit RESULT does not state.

| Folder | game_bootstrap.dll written (local) | SHA-256 (first 12) | Main commit (RESULT.md) |
|---|---|---|---|
| build-tour1 | 2026-11-12 11:44 | 114D4419FAA7 | 8f415270 |
| build-tour2 | 2026-11-13 12:05 | BE9C5B0FDF06 | 63b113d9 merged |
| build-tour3 | 2026-11-13 16:13 | E357390484A4 | 5fa7c0bd merged |
| build-tour4 | 2026-11-13 17:36 | 750749D371C8 | not stated |
| build-tour5 | 2026-11-13 18:34 | 8B4D6AE6D880 | d1f0d340 |
| build-tour6 | 2026-11-13 21:53 | 67867BED8467 | e03ae881 |
| build-tour7 | 2026-11-14 06:54 | 93ABC8B451F9 | a1a4138a |
| build-tour8 | 2026-11-14 09:27 | 4C1093B957D7 | 0b162a28 |
| build-tour9 | 2026-11-14 11:47 | 0F50D1168768 | 51df978b |
| build-tour10 | 2026-11-14 16:21 | E4822F0523E8 | d1fbc6f51 |
| build-tour11 | 2026-11-15 10:55 | AB9D987D2BDA | 61b905572 |
| build-tour12 | 2026-11-15 19:26 | F78D34A90D2D | branch `a52d24e47` (the flag tool) |

`evidence/I-0139/build-main-d1f0d340.log` is the launcher's module build of main `d1f0d340` (`game_bootstrap.dll` SHA-256 starts 91BFD756),
not a tour build.
