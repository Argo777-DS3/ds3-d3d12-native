# Investigation result

## Conclusion

Steps 1 and 2 done (2026-11-16, `claude/recursing-wiles-ccfaee`, D-0295), measured on this PC (Ryzen 7 5800X3D, 16 threads):

- **Step 1, the build in parallel by Ninja:** a complete build with its unit tests takes 431 s against 909 s serial (its 443
  commands 163 s against about 10 min), and its 442 built files (binaries, objects, shader headers; besides 12 SDK hard links and 3
  script-written headers the folder also holds) **equal the serial build's byte for byte** (build-b: the serial build at `ded10c18d`, the Ninja build at `4e14f0420`, which differs from it only by the PDBs' removal
  before a build; the PDBs not compared; the serial path the branch's, its `/Z7` objects' command lines now those of Ninja's). The
  head `2e1fd43a6` (main `af6963426` merged) built complete with its tests: passed, 432 s (« The builds before the merge »).
- **Step 2, a thread's object cache (`-Cache`), every timing without the unit tests (`-NoControls`), the final series (`147bd0d5c`, E playing on the ASUS beside it):**
  nothing changed 9 s (the module relinked, its PDB removed, and `dumpbin`), one `.cpp` 19 s (3 commands), one shader 17 s (6), `scene_pass.h`
  (29 of the 114 units) 180 s (31), a `.inc` of `game_bootstrap` 151 s (3: its one unit takes about 2 min 10 s); everything 191 s.
  **The cache's own check failed once:** six of the seven published modules have the complete rebuild's hash (`59CEE6A2…`); the
  `.inc` step's (`C1E364F5…`) and its one recompiled object, `game_bootstrap.obj`, differed, cause unknown; three more
  compilations of that unit (its `.cpp` touched, not the `.inc`) gave the complete rebuild's object each time (« The final
  series »). A cache build with the unit tests passed once (401 s, « The builds before the merge »).
- Left for later (the study's step 3): `game_bootstrap.cpp`'s single unit (30,500 lines, about 2 min alone) bounds a fix in a `.inc`
  or a header it includes; the unit tests (about 4 min) still run one after another; a `-NoControls` cache build deletes the test
  objects (`cleandead`), so the next build with the tests recompiles them (77 units; not measured).

## The plan of the checks

1. `-Serial -KeepObjects` into a folder of this thread, its binaries, objects and generated headers hashed; then the Ninja build
   (complete) into the same folder: every hash equal (`/Brepro`; the same folder because the module names its PDB's path), the unit
   tests passing, both times measured.
2. `-Cache` into a cache of this thread with `-NoControls`: a first build (everything runs), then a source touched, then a header
   touched: the commands rerun, the time, and the module equal to a complete Ninja build of the same cache folder.

## Step 1: the step review

The `step-reviewer` on `88a274fc6..2624304b2`: no High.

- M1 (the records stated the byte-for-byte equality before any build measured it): TOOLS now says « must equal » and that the
  equality is measured before the merge; the branch is not merged before the checks above.
- L1 (any `call` line skipped): only the vcvars line is skipped; another `call` fails the build.
- L2 (`cmd /s /c` reads `%` and AutoRun unlike a batch file): `cmd /d /s /c`, and a command with `%` fails the build.
- L3 (a link's PDB not declared): the `/PDB:` path is the link's second output, in the written-by-this-build check.

## Step 2: the step review

The `step-reviewer` on `2624304b2..9b9dc9ff5`: one High, three Medium, five Low.

- H1 (`-Cache` could name the launcher's folder, which carries a build marker): the cache's path is resolved and refused when it is
  the launcher's.
- M1 (Ninja's header lists leave out Visual Studio's and the Windows SDK's headers; the tools come from PATH): a toolchain stamp,
  rewritten by `build-ninja.cmd` only when it changes, is an input of every command (its content since the re-review: below).
- M2 (the shaders' third-party includes were no inputs): a third-party stamp (MinHook's and NRD's commits, SHaRC's headers, the
  archives' hashes) is an input of every command.
- M3 (a re-extracted archive keeps the zip's old dates): the extracted files are dated now.
- L1 (a `-NoControls` cache build published an earlier build's test binaries): only this graph's outputs and the files this run
  wrote are published.
- L2 (outputs of removed commands stayed in the cache): `ninja -t cleandead` before each cache build.
- L3 (the header-list check failed on `./` or `//` spellings): a session's `-Out` and `-Cache` are made absolute once, and both
  sides of the check go through `GetFullPath`.
- L4 (`-Cache` = `-Out` through the evidence junction passed): a probe file written in `-Out` must not appear in the cache.
- L5: a lock file held for the build (one build at a time in a cache); with `-Cache`, `-Out` must be new or empty (one folder per
  version); the link's PDB already declared (step 1's L3). Left: the archives hashed three times a run and Streamline's DLLs twice
  (seconds), and one `.hlsli` edit reruns every shader (time, not correctness).

## The fixes' re-review

The `step-reviewer` on `9b9dc9ff5..65228a9fb`: no High, two Medium, nine Low.

- M1 (Ninja's header lists are relative to its working folder: a cache built from another checkout's folder missed a header edit):
  every Ninja call runs with `-C` the build's own folder.
- M2 (a Windows SDK servicing update in place kept its folder name): the toolchain stamp is written by `toolchain-stamp.ps1` in vcvars'
  environment: each tool's path, size and date, and every file of the Windows SDK's include and x64 library folders and of Visual
  Studio's by size and date (5808 lines, 2.3 s on this PC).
- L1 (`where` passes when one tool of five is found): each tool is looked up alone, a missing one or a missing folder fails.
- L2, L6, L7 (the lock and the markers): a cache carries `ds3-cache.marker`, required by `-Cache` and refused as any `-Out`; `-Out` is
  claimed (its marker) at the start; the lock is released when the script ends or fails (a `trap`).
- L3 (build files naming the cache published): only the graph's outputs, this run's `.txt` and `.raw` results and the marker.
- L4 (Streamline's headers tracked by date only): hashed into the third-party stamp.
- L5 (a header the script no longer writes stays in the cache): before a cache build, a `.h`, `.inc`, `.hlsli` or `.rsp` neither
  written this run nor any command's output is removed.
- L8 (the launcher's folder refused by its name only): also by identity (a probe file) against the launcher's folder.
- L9: the RESULT's count of hashings corrected above.

## The third review

The `step-reviewer` on `65228a9fb..3f5612296`: no High, no Medium, seven Low.

- L1 (claiming `-Out` was not atomic): the marker is created with `CreateNew` after the cache's lock is taken.
- L2 (a cache inside the launcher's folder or inside `-Out` passed): refused by their full paths.
- L3 (the trap could read a caller's `$cacheLock`): set to `$null` at the top. Left: a run stopped (Ctrl+C) inside a long-lived
  PowerShell keeps the lock until that session ends; builds run as `pwsh -Command` processes.
- L4 (only the tools' executables in the stamp): every file of the tools' folders (`c1xx.dll`, `c2.dll`, `dxcompiler.dll`...); 6122
  lines.
- L5 (comments on publishing): reworded.
- L6 (the published module names the cache's PDB): left, written in TOOLS; a cache build is compared with a complete build of the
  same cache folder.
- L7 (Ninja's working folder recorded in the `/Z7` objects): see « The first comparison ».

## The first comparison (build-a, 13:21 to 13:41)

- `-Serial -KeepObjects` (`3f5612296`): passed with its tests, 921 s (E's build ran beside it until 13:35: not a measure).
- The Ninja build into the same folder (`0cee11075`): every command ran in 160 s from the script's start to the end of the link and
  `dumpbin`, then the build stopped at the header-list check: `sample_symbolize.cpp` includes only system headers, which Ninja leaves
  out, so its list is rightly empty. The check now asks a non-empty list only of a unit with a quoted include (190 of 191).
- Hashes (`.dll`, `.exe`, `.obj`, generated headers, 457 files): 176 differ, `game_bootstrap.dll` (same size), the two `/DEBUG`
  tests and 173 objects; the 18 equal objects are the C units without debug information. A `/Z7` object records its compiler's
  command line and working folder: the Ninja lines carried `/showIncludes` and ran in the build folder, the serial ones did not.
  Both paths now run the same lines (`/showIncludes` and `VSLANG` in `build.cmd` too, the serial path dropping the include notes
  from its output) in the build folder (`ded10c18d`); the comparison is redone in build-b.

## The second comparison (build-b, 13:42 to 14:14)

- `-Serial -KeepObjects` (`ded10c18d`) into the new folder: passed with its tests, 909 s.
- The Ninja build into the same folder (`ded10c18d`): passed with its tests, 425 s (the commands 169 s, the tests the rest); 454
  of 457 files equal. The three that differed, the module and the two tests linked with `/DEBUG`, differed by 66 bytes in the
  module: the PE time, the four debug entries' times, the PDB's GUID and the `/Brepro` hash, every one of them following the PDB,
  which the second link had updated in place (69.4 MB against 66.5 MB). Code, data, imports and relocations equal.
- A complete build now starts from no PDB (`4e14f0420`). The Ninja build again into build-b: passed with its tests, 431 s (the
  commands 163 s); all 457 files equal to the serial build's (442 built ones, `hashes-b-serial.json` against `hashes-b-ninja2.json`),
  `game_bootstrap.dll` `0DFCA1CD…99F6` both. The two earlier differences (66 bytes; 176 files) were read from files since overwritten.
- The CONTRACT's « the tests whose binary changed » was corrected in `cbc06a076`: the tests stay as they are.

## The cache series (14:14 to 14:49)

[tools/Run-CacheSeries.ps1](tools/Run-CacheSeries.ps1): `-Cache evidence/I-0142/cache -NoControls`, each build into a new `-Out`,
a file's date touched between builds, then the cache's log removed and everything rebuilt; `evidence/I-0142/cache-series.txt` and
`cache-*.log`.

Counts below are as the script printed them then, each one too high by the module's PDB (fixed: 289 commands, `.cpp` 3...).

- First series (`4e14f0420`): first 163 s (290), nothing 5 s (0), one `.cpp` 15 s (4), `scene_pass.h` 145 s (32), the
  `.inc` 128 s (4). The shader (`rt_shadow.hlsl`) failed the check after the build (« Ninja finds work left after the build: ... [1/3] rt_scene.obj [2/3] game_bootstrap.dll », the
  series' console output, not kept: its log stops at the shaders). Ninja
  records a compilation's headers relative to its working folder, while the graph named the generated shader headers by their
  absolute paths: two nodes for one file, so the includer was not rebuilt in the build that rewrote its header. The check caught
  it; paths inside the build folder are now written relative to it (`2644fd92c`).
- Second series (`2644fd92c`): every build reran all 290 commands; `cleandead` deleted them first, the log still holding the earlier
  absolute spellings. A cache whose log another form of the graph wrote now starts from no log (`730e809e2`); the objects after
  its complete rebuild equal those before it (114, 0 differing).
- Third series (`730e809e2`): first 182 s, nothing 5 s (0), `.cpp` 16 s (4), shader 12 s (7), `scene_pass.h` 135 s (32),
  complete rebuild 157 s; `cleandead` 0 files each time; 114 objects, 0 differing after the complete rebuild. But the published
  modules had five hashes for one source tree (the final review's M1): each in-place update of the cache's PDB shifted the module's
  identity fields (the PE time, the debug entries' times, the PDB's GUID, the `/Brepro` hash). Every build, a cache's too, now starts
  from no PDB (`54c70fd40`).

## The final series (15:18 to 15:39)

[tools/Run-CacheSeries.ps1](tools/Run-CacheSeries.ps1) `-Tag final` on `147bd0d5c` (`evidence/I-0142/cache-series-final.txt`,
`cache-final-*.log`; the published folders removed after their module's hash was read), E playing on the ASUS without a measure:

| Step | Time | Commands | Module |
|---|---|---|---|
| first | 196 s | 289 | `59CEE6A2` |
| nothing | 9 s | 2 (the module relinked, its PDB removed; `dumpbin`) | `59CEE6A2` |
| `backend.cpp` | 19 s | 3 | `59CEE6A2` |
| `rt_shadow.hlsl` | 17 s | 6 | `59CEE6A2` |
| `scene_pass.h` | 180 s | 31 | `59CEE6A2` |
| `17_render_step.inc` | 151 s | 3 | `C1E364F5` |
| complete (log removed) | 191 s | 289 | `59CEE6A2` |

One object differed between the `.inc` step and the complete rebuild, with the same sources and dates. [tools/Test-UnitRepeat.ps1](tools/Test-UnitRepeat.ps1)
then compiled `game_bootstrap.cpp` three times in the same cache (`unit-repeat.txt`, 137 to 144 s each): the object `DE13206F…` and
the module `59CEE6A2…` each time, the complete rebuild's. The one difference is not explained (OBSERVED once, not reproduced). If
it is a compilation of that unit that does not always give the same bytes, complete builds are concerned too: the serial-against-
Ninja equality rests on one pair of builds (build-b). Not inferred further.

## The builds before the merge (16:22 to 16:36)

On the head `2e1fd43a6` (main `af6963426` merged, its script now 444 commands), E playing on the ASUS without a measure:

- Complete, with the tests, into `evidence/I-0142/build-final-2e1fd43a6`: passed, 178 PASS lines, 432 s (the commands 162 s);
  `build-complete-2e1fd43a6.log`.
- Through the cache (`cache-final`, last built at `147bd0d5c` without the tests), with the tests, into `cache-final-out-tests`:
  passed, 178 PASS lines, 401 s, 202 of 444 commands (the tests' units and links, deleted by the earlier `-NoControls` builds'
  `cleandead`, and the units main changed); 77 test executables and this run's 174 results published; `build-cache-tests-2e1fd43a6.log`.

## The merge

Merged into `main` `4b69136c3` (2026-11-16 17:27). The launcher rebuilt from it, its first build in Ninja: passed, 178 PASS lines,
472 s (444 commands, 190 s; `evidence/I-0142/build-launcher-4b69136c3.log`). Start check `start-4b69136c3`: passed, restored. The
disk rotation: this investigation's build folders (`build-a`, `build-b`, `build-final-*`, `cache`, `cache-final`,
`cache-final-out-tests`) removed at its close (D-0142), their logs, hashes and reports kept.

## Reviews at the merge

The `final-reviewer` on `main...56d8a5b05` (main `2b15cb17a` merged in): no High, two Medium, eight Low.

- M1 (a cache build's module followed the cache's PDB history: five hashes for one tree): every build starts from no PDB, a cache's
  too (`54c70fd40`); the final series above (six of seven modules equal, the seventh's object not reproduced in three more compilations).
- M2 (the shaders' includes tracked by name, a new include would not rerun them): each programme's includes are read at graph time,
  followed through the including file's folder and the command's `-I` folders, and declared as its inputs.
- L1 (the containment checks by string only): the folders compared by their real paths through every junction or link of their
  ancestors (`Resolve-RealPath`); a cache under the launcher's folder spelled by the store's path is refused (tried at its arguments'
  check). Left: a `subst` drive or a volume mount point.
- L2 (outputs counted as commands; « a header most units include »): commands counted; the RESULT's numbers corrected.
- L3 (« 457 files equal » counted SDK links and unchanged headers): 442 built files, stated.
- L4 (timings beside other builds; the `.inc` step measured before the node fix; the tool's paths): the final series with the `.inc`
  step and the tool with parameters; its conditions stated. Not remeasured on an idle PC.
- L5 (a PDB that cannot be removed kept silently): the build fails.
- L6 (a relative launcher `-Out`): made absolute, fully qualified.
- L7 (optimisation): left, stated in the conclusion (the test objects after a `-NoControls` cache build; `game_bootstrap.obj` starting
  at 30 s of a complete build; the stamps without a cache, 2 to 3 s).
- L8 (records): the CONTRACT's exclusion amended; the step-2 review's stamp line corrected; the final tree's build cited. STATE's
  « Pilot branch » row names the pilots' branches, which this study thread is not (resume-thread, step 6).

The `step-reviewer` on the fixes (`56d8a5b05..147bd0d5c`): no High, one Medium, seven Low.

- M1 (the include walk redone for every shader, about 3 s a build): memoised per run; the graph ready 3 s after the start.
- L1 (every shader still depending on every repository `.hlsli`): only its real includes and NRD's configuration (26 of 171 include
  `rt_material.hlsli` or `rt_lights.hlsli`; checked on the generated graph).
- L2 (an include under a conditional the compiler skips would fail; two `-I` spellings): an include found nowhere is skipped (one the
  compiler takes fails the compilation itself); `-I"x"` and `/I "x"` read.
- L3 (an empty input reached the walk): the input check first.
- L4 (`IsPathRooted` accepted `C:dir`): `IsPathFullyQualified`.
- L5 (mount points, shares, `subst` in `Resolve-RealPath`): left (none on this PC).
- L6 (the series tool over an earlier series): it refuses an existing tag.
- L7 (the series could not show a skipped command): each step's command count checked against a minimum.
- These last fixes (`cb3f3cdaa`) change the graph's shader inputs and the walk, not the commands: checked on the generated graph only;
  the launcher's build at the merge is their first build.

The `integrity-reviewer` on `main...a716f0876` with this RESULT: no High (no check weakened, no special case, the figures matching
their files), two Medium, four Low, all on the records.

- M1 (the cache's failed check only in the RESULT, and a « not concerned » that was reasoning): the failure stated in the conclusion,
  TOOLS, STATE and the index; the reasoning removed; the repeat's touched file named.
- M2 (the cache's timings without saying « without the tests »; no cache build with the tests; « about 30 s » not measured): stated;
  the cache build with the tests is the next build (before the merge); the estimate removed.
- L1 (« the final tree » not the head): the last build with the tests and the head's state stated.
- L2 (the comparison over two commits): both named, with what differs between them.
- L3 (the serial reference itself changed): stated in the conclusion.
- L4 (claims from overwritten files or a console line): said where each comes from.

## Evidence

- The study: [STUDY.md](STUDY.md).
- `evidence/I-0142`: `build-final-56d8a5b05.log` (the final tree: passed, 178 PASS lines), `cache-series-final.txt`,
  `cache-final-*.log`, `unit-repeat.txt`, `unit-repeat-*.log`.
- `evidence/I-0142`: `build-a-*.log`, `build-b-*.log` (the comparisons), `hashes-b-serial.json`, `hashes-b-ninja.json`,
  `hashes-b-ninja2.json` ([tools/Save-BuildHashes.ps1](tools/Save-BuildHashes.ps1)), `cache-series.txt`, `cache-*.log`; the build
  folders `build-a`, `build-b` and `cache` are disposable (D-0142), the `cache-out-*` folders already removed.

## Artifacts produced or modified

- `native/frame/Build-GameBootstrap.ps1`; `.project/TOOLS.md`, `.project/WORKFLOW.md` (one row each); D-0295.
