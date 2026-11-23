# Security audit of the mod's own code (2026-11-23)

Asked by Thomas through the coordinator (2026-11-23 01:44 Paris): a defensive audit of **our** code only (the module, its
controller, the launchers, the scripts, the repository), for what could harm a player who installs the mod. Not the original
game, its servers or its anti-cheat. Read only at first; then fixed on Thomas's word (« Fixes » below).

**Scope read**: `native/frame/game_bootstrap_ctl.cpp` and `game_bootstrap_shared.h` (the injector and its shared section),
`native/frame/game_bootstrap/22_setup.inc` (start, flags, loaded libraries), `01_state.inc` (module folder, settings file),
`04_updates_and_test_tools.inc` (test and witness tools), `16b_rekindled_overlay.inc` (the page's picture and music),
`native/renderer/backend.cpp` (system DXGI, the pipeline list), `streamline_host.cpp`, `rekindled_settings.h`, the content parsers
(`texture_container.cpp`, `dds_resource.cpp`, `gparam.cpp`, `acquire.cpp`), `crash_dump.cpp`, `original_image.h`; `tools/*.ps1`
and `*.cmd`; `scripts/Clear-DiskCache.ps1`, `Move-SaveBackups.ps1`, `tools/Backup-Repo.ps1`, the build's pins
(`Build-GameBootstrap.ps1`); the tracked tree and the Git history for secrets, personal data and binaries. **Not read**: the
render passes and most hooks line by line (their game reads go through the guarded `Read`, `acquire.cpp:13-16`), the HLSL, the
Python and replay scripts. No dynamic test (no fuzzing, no sanitizer run). Each point below is OBSERVED in the code unless marked
INFERRED or UNVERIFIED.

## High

**H1. Nothing keeps a player offline.** The play launcher starts the game through Steam as usual
(`tools/Launch-DS3-DX12.ps1:149-154`) and the module hooks and patches hundreds of the game's functions; FromSoftware's online
client is present and running in the process (`native/frame/game_bootstrap/15_cpu_service_and_input.inc:160-170`). The test
launcher's warning not to go online is a source comment and its USAGE (`tools/Launch-DS3-DX12-Test.ps1:17`,
`investigations/I-0089-test-tools/USAGE.md:47`), never printed; the play launcher prints one since 493cc14ff; no code of ours
blocks the sign-in. The « offline » notices seen at the title in the project's
runs are not explained by anything found in our code (UNVERIFIED: their cause, and whether a player's game would connect). If it
does connect, a modified client online risks a penalty on the player's account (INFERRED) and exposes the hooked process to other
players' network data. Fix to discuss: refuse or cut the network sign-in in the module (a hook that keeps the game in its own
offline mode), and say it in the launcher; check the actual behaviour in an announced live run first.

## Medium

**M1. The witness tools run in every build, during play.** `witness-teleport.txt`, `witness-phantom.txt`, `witness-flag.txt`,
`witness-flag-watch.txt` and `witness-cutscene.txt` beside the module are polled every 15 character-manager calls
(paths `04_updates_and_test_tools.inc:718`, `:848`, `:865`, `:883`, `:917`; read at `:834`, `:853`, `:867`, `:887`, `:923`; called from `:1609`), in both modes, with no gate
(the F-key test tools have one: `test-tools.flag`, `:1494`). They write the player's position, phantom id and any event flag
(the game's setter called at `:870`), all saved by the game. The play launcher refuses them only once, at start
(`tools/Launch-DS3-DX12.ps1:59`); a file arriving in `%LOCALAPPDATA%\DS3-DX12` during play (a test script aimed at the wrong
folder, a copied test folder) changes the save silently. Fix: gate them like the test tools (a flag read once at start, refused by
the play launcher), or better a build profile for players without them (see « Idea »).

**M2. The crash dumps carry the process data.** Corrected after the first version of this report: the dump callback already keeps
the data segments of `game_bootstrap.dll` alone (`crash_dump.cpp:11-17`, not every module's as first written). It also wrote
`MiniDumpWithProcessThreadData` (the PEB and TEBs: the environment, the Windows user name, paths) and
`MiniDumpWithIndirectlyReferencedMemory` (memory the stacks point to) (`crash_dump.cpp:31-32`); a player who sends a dump for
help sends those (INFERRED: no dump inspected), and the controller's message pointed at the folder without a word
(`game_bootstrap_ctl.cpp:72-74`). **Fixed** (493cc14ff): a play session's dump has no process data (`WriteDump`'s
`with_process_data`, false when the controller is unbounded), the runners' keep it; the message says the dump holds part of the
game's memory. The referenced memory stays (the stacks' values are what a crash needs).

**M3, lowered to Low after checking: the repository.** This checkout has no remote. The lightened copy on GitHub was checked
on 2026-11-23: private, one commit (« Copie de main fa858ad84 », 2026-11-10), no
`decompiled/`, the Windows account written `C:\Users\User`, no binary, the GitHub no-reply e-mail. Left: the main repository
itself must never be pushed as it is (`decompiled/`, 7 402 Ghidra exports some read from decrypted code; a 215 MB history with
319 `.exe`, 111 `.dll`, 140 `.bin`; about 300 files naming the account), and each refresh of the GitHub copy has to redo its
cleaning. No secret was found anywhere (no key, token or password pattern; commit e-mails `.invalid`; no SteamID).

## Low

**L1. Libraries resolved by the standard search order.** `LoadLibraryW(L"mfplat.dll")` and `L"mfreadwrite.dll"`
(`16b_rekindled_overlay.inc:393`), the delay-loaded Media Foundation and the imports `d3d12.dll`, `D3DCOMPILER_47.dll`,
`dbghelp.dll`, `dwmapi.dll`, `WINMM.dll` (the build's `imports.txt`) are searched from the game's folder first, where players
often drop proxies (ReShade, mod loaders); such a proxy is then used by our module. DXGI is already taken from System32 on purpose
(`renderer/backend.cpp:36-60`, the build refuses a DXGI import, `Build-GameBootstrap.ps1:877`). Fix: `/DEPENDENTLOADFLAG:0x800` on
the link line (`Build-GameBootstrap.ps1:491`) and `LoadLibraryExW(..., LOAD_LIBRARY_SEARCH_SYSTEM32)` for the two explicit loads.

**L2. Signatures checked once, then the path loaded.** `sl.interposer.dll` is checked NVIDIA-signed then loaded by path with
`LOAD_WITH_ALTERED_SEARCH_PATH` (`renderer/streamline_host.cpp:104-107`): a swap between the two is possible (TOCTOU).
`D3D12\D3D12Core.dll` beside the module is hash- and signature-checked at build time only (`Build-GameBootstrap.ps1:893-903`),
loaded at run time without a check (`20_startup_phases.inc:271`). Both sit in the module's own folder, as trusted as the module
itself. Fix: open the file without write sharing for the check and the load, or check the loaded image.

**L3. The pipeline list feeds the driver.** `native-pipelines.v3.psc` in the play folder carries root signatures and shader
bytecode compiled at start (`renderer/backend.cpp:1357-1405`); its checksum is not a MAC. Its reader is bounded
(`backend.cpp:1218-1226`). Writing it needs the same access as replacing the DLL. Fix (optional): key the list to the module's hash.

**L4. The shared section has the default DACL.** `Local\DS3I66Bootstrap-<pid>` (`game_bootstrap_shared.h:32-34`,
`game_bootstrap_ctl.cpp:100`): the module takes its journal path from it (`22_setup.inc:131-132`), the controller reads
`crash.reason` and `crash.dump` back through NUL-terminated calls (`game_bootstrap_ctl.cpp:62`, `:66`). Another process of the same
user could redirect the journal or make the controller read past the strings; same user, so no privilege is gained. Fix: an
explicit DACL (the controller and the game only) and `wcsnlen`/`strnlen` bounds.

**L5. The launcher's diagnostic flags are a deny list.** `-DiagnosticFlags` refuses `test-*` and a few names
(`tools/Launch-DS3-DX12.ps1:15`) but lets through dev switches such as `pix-capture` (PIX loaded, about 1 GB a capture,
`22_setup.inc:190`), `pin-census` or `no-black-window`. Fix: an allow list of the diagnostics meant for play.

**L6. The test launcher with `-Shebang -Keep` replaces the player's save and keeps it replaced.** No restoration marker is
written with `-Keep` (`tools/Launch-DS3-DX12-Test.ps1:120`), the Shebang save is installed over `DS30000.sl2` (`:135-139`); the
backup stays in `%USERPROFILE%\.ds3-test-backups\<date>` (never pruned without `restored-ok.txt`, `:122`) but nothing tells the
player. Fix: refuse `-Keep` with `-Shebang`/`-SaveCopy`, or print the backup's path.

## Fixes (Thomas, 2026-11-23 01:48: « corrige le et nettoie le », commit 493cc14ff)

- **M1 fixed**: the controller writes its bound in the shared section (`bounded_seconds`, `control_version` 5); the five witness
  readers act only when it is not 0 (`WitnessToolsAllowed`, `01_state.inc`), so only the runners' bounded runs (1 to 7200 s)
  read them, never a session of the play or test launcher (both unbounded). No runner changes: they all pass a bound.
- **M2 fixed** (above).
- **L1 fixed**: `/DEPENDENTLOADFLAG:0x800` on the module's link line; Media Foundation by `LoadLibraryExW(...,
  LOAD_LIBRARY_SEARCH_SYSTEM32)`.
- **L2 fixed for Streamline**: `sl.interposer.dll` held open without write or delete sharing from its signature check to its load.
  `D3D12Core.dll` keeps its build-time check only.
- **L4 fixed in part**: the controller's reads of `crash.reason` and `crash.dump` bounded; the section's DACL unchanged (same user).
- **L5 fixed**: `-DiagnosticFlags` accepts `*-observe` names only (still never `test-*` or `unlock-*`).
- **L6 fixed**: the test launcher warns when `-Keep` leaves a test save active and says where the player's own copy is.
- **H1 in part**: the play launcher tells the player to launch offline. The block in the module needs to know what the game does
  online first: an announced live run.
- **L3** unchanged (low value; it would change the cache's behaviour).

**Step reviews.** The first (f148eed8f..493cc14ff): no High or Medium, three Low, fixed in 40370dad5: the module keeps the
bound once in `Setup` before any hook (`bounded_run`) instead of reading the section again; `crash_dump.h` no longer says a play
dump is free of the user's name (it drops the environment block; the name stays in the module paths, the kept data segments and
the memory the stacks point to); the interposer's path, not only its file, could be swapped through a junction above it. Four
attempts at that last point (a file-id comparison, the final path given to the check, the load and the plugin folder, then a
mapped-name comparison: 40370dad5, 6aaff121a, 1b92704bc, 09add0bb7) each drew a Medium or a High from the next review (a path
check left apart from the load; Win32 normalisation naming another folder; a fallback chosen by whoever holds the junction; hard
links of one interposer across the build folders, D-0260, giving a later folder the first link's mapped name, so DLSS lost).
No RCA was opened (WORKFLOW asks for one after two failed fixes); the point was closed by scope instead (1130d15b7): whoever can
write the module's folder or a junction above it can replace the unsigned `game_bootstrap.dll` itself, so the code keeps
493cc14ff's hold of the file and says what is out of reach. Observation: L5 also refuses the non-`-observe` names the play
launcher took before (`world-pictures`, `admission-marks`, `pix-capture`; `investigations/I-0102-code-cleanup/RESULT.md:45`);
they stay open to the runners and the test launcher. `game_bootstrap_ctl.exe` (in the play folder) has no `/DEPENDENTLOADFLAG`.

**Reviews at the merge.** Final review (final-reviewer, main...7242a0593): one High, one Medium, eleven Low.
- High: Windows Defender quarantines the controller built from 40370dad5 on (`Trojan:Win32/Wacatac.B!ml`, SHA-256 `5E3A2FC9...`,
  02:28 and 06:00), not main's nor 493cc14ff's (`CDE2332C...`). Answered (cdd6a3a18): the controller's source is back to
  493cc14ff's (its dumps take the bound from the section it wrote itself; the module keeps its own copy of the bound). A
  machine-learning detection may still hit a later controller: Thomas reporting the file to Microsoft remains the lasting fix.
- Medium: no run has yet used this branch's module. Owed after the merge, beyond the start check (which covers DLSS from the
  launcher's build folder only): a play-launcher start with a `witness-flag-watch.txt` placed in the play folder after the
  launcher's check (expected: no `witness_*` row, `dlss_ready` true), and a runner's teleport from a second build folder (expected:
  `witness_teleport` applied, `dlss_ready` true).
- Low, fixed in the docs: H1's paragraph; the RCA wording; the private copy's name removed from this report; the skill briefs the
  report's findings and « Left » after the code, as the agent's severities; the play launcher's comments on witness files and
  `*-observe` names. Low, left (« Left »): the rest.

Integrity review (integrity-reviewer, main...d5fb9ea9a): no High, two Medium, three Low, all in the records; no test, check or
refusal weakened. Answered here: the owed runs are in « Left »; M1, M2 and L2 are fixed in the code and not yet run in a play
session (the start check is a bounded run, so it exercises the runner's side only); « false positive » is INFERRED, pending
Microsoft's verdict, and « not flagged » rests on one ML pass on build-1's copy (02:09), before Thomas's allow (by threat ID,
machine-wide, in place by 06:10). Left: the step reviews' counts have no stored evidence beyond the commits.

## Informational

- Fixed-size `sprintf_s` rows (`char row[160]` and the like, throughout the module): an over-long value ends the process through
  the CRT's invalid-parameter handler, never a memory overwrite.
- `tools/Backup-Repo.ps1:40` mirrors (`/MIR`): a deletion in the source reaches the backup at the next run; it is a copy, not a
  history.
- `.mcp.json` points GhidraMCP at `http://127.0.0.1:8080/` without authentication: any local process can drive Thomas's Ghidra
  (dev machine only).

## What is already sound

The game is identified by SHA-256 in the launchers (`tools/Launch-DS3-DX12.ps1:41`) and by size and PE words in the injector
(`original_image.h:22-48`); every hooked function's first bytes are pinned before any hook (`22_setup.inc:139-162`); the container
and texture parsers bound every offset and size and throw on any mismatch (`content/texture_container.cpp:10-35`,
`content/dds_resource.cpp:9-14`, `content/gparam.cpp:10-15`, 128 MB and 64 MB caps); the settings file is read up to 64 KiB, known
keys only, every value range-checked (`01_state.inc:827-829`, `renderer/rekindled_settings.h:118-176`) and written whole then moved
(`02b_rekindled_graphics.inc:40-47`); game memory is read through `ReadProcessMemory` (`content/acquire.cpp:7-16`); code pages
made writable are restored (`frame/effect_system.cpp:277-287`); Streamline's over-the-air updates are off
(`streamline_host.cpp`, its preferences); no network, process-spawning or registry-writing code in the module; the third-party sources and
binaries the build uses are pinned by hash or commit (`Build-GameBootstrap.ps1:139-155`, `:893-903`); the play launcher backs the
saves up before each start (`tools/Launch-DS3-DX12.ps1:142-146`); the disk scripts are dry runs by default, skip reparse points
and move saves only after a hash match (`scripts/Clear-DiskCache.ps1:52`, `scripts/Move-SaveBackups.ps1:33-38`).

## Idea (one, outside the plan)

A **player build profile**: one preprocessor switch in `Build-GameBootstrap.ps1` that compiles out every test, witness and
capture tool (the `test-*` and `witness-*` readers, PIX, RenderDoc, the shader dumps) for the launcher's module, so M1 and L5
disappear by construction rather than by the launcher's checks; the runners' builds keep them.

## Left

After the merge (c728e936c, 2026-11-23 06:57-07:00, offline, restored by hash): the start check passed with `native_streamline`
`dlss_ready` true (`evidence/I-0066/artifacts/start-c728e936c`); a runner's teleport from a second build folder (`sec-tele-b6`,
build-6) gave `witness_teleport` applied and `dlss_ready` true. Still owed, to the H1 thread (it runs the play launcher anyway):
a play-launcher start with a `witness-flag-watch.txt` placed in the play folder after the launcher's check (expected: no
`witness_*` row, `dlss_ready` true); until then M1, M2 and L2 are not run in a play session. H1's block in the module (after an announced live run showing what the
game does online); L3; L4's DACL; the idea above. Out of
reach, as the unsigned module is: whoever writes the module's folder or a junction above it (the interposer's path, its plugins).
A play dump still holds the user's name (module paths, kept data; a scrub would be its own work). Windows Defender flags
`game_bootstrap_ctl.exe` as `Trojan:Win32/Wacatac.B!ml` (2026-11-23 02:28 and 06:00; a machine-learning false positive on an
injector, INFERRED, no Microsoft verdict) on the controller built from 40370dad5 on; the merged controller is 493cc14ff's, not
flagged on its one pass (build-1, 02:09); Thomas allowed the threat ID machine-wide (to remove once Microsoft clears the file,
which Thomas reports). From the final review, left:
the docs that say witness files work in any session but the play launcher's (`WORKFLOW.md:264`, Thomas's to change;
`TOOLS.md:27`; `Run-StartupTrace.ps1:165`); `with_process_data` defaults to true and no test shows a dump without the PEB; an
`-Original` play crash gets no privacy note; `locomotion-observe` and `jump-observe` change behaviour although named `-observe`;
`test-*.flag` files are read at the first schedule call, not in Setup; the module copies the journal path from the section again
at crash time; a section version mismatch is refused without a journal; the report's line numbers drift with later changes.
