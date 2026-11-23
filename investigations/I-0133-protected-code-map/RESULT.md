# I-0133 result: the encrypted and obfuscated parts of DarkSoulsIII.exe

Study thread, 2026-11-11 00:36-01:00 (Paris). Static only: the file `game/DarkSoulsIII.exe` 1.15.2 (SHA-256 `EF5E07C5…B69498C`),
the decrypted-code capture of 2026-10-09 already in the evidence store, headless Ghidra 12.1.4 on `ghidra-mirror-i0066`
(read-only). No game launch. Statuses: OBSERVED (measured here), INFERRED (reasoned from what was measured), GENERAL (the
behaviour of this product family, not measured on DS3).

## Conclusion

1. **Where.** The protection adds three code sections to the game's own: a second `.text` (9.0 MB), `.bind` (0.2 MB, which
   holds the program's entry point) and `.interpr` (49 KB). Inside the game's main `.text` (39.1 MB), **15,602 of its 222,418
   functions (7.0 %) have their body encrypted at rest: 3.06 MB, 7.8 % of the game's code** (OBSERVED). 495 more functions
   (0.19 MB) carry small runtime patches of the protection (an instruction and a jump, `rdtsc` checks), and 1,216 functions
   start with a jump into the protection's `.text` (an entry guard; 931 of them have a plain body behind it).
2. **The encrypted bodies are already readable.** The capture of 2026-10-09 (`i66-reference-code-935`, 240 s from the title
   into play) saw **98.3 % of the encrypted bytes decrypted** (OBSERVED: the high-entropy regions of the file against the
   capture's overlay; decrypted bytes have the entropy of ordinary code, 6.09 bits a byte, against 7.86 on disk). Laid over
   the current 12.1.4 mirror with I-0066's `I0066Overlay936.java`, three functions no one had read decompile cleanly tonight
   without launching the game (below). So encryption costs a step, not a wall.
3. **Which systems.** By the classes and callers of each function (method and limits below): characters and gameplay about
   780 KB, the game's built-in debug tools about 775 KB, the world and events 165 KB, rendering and effects 157 KB, menus 150
   KB, input 137 KB, engine core 119 KB, files 96 KB, utilities 89 KB, Havok 34 KB, online 15 KB; about 610 KB left
   unattributed (INFERRED, the attribution's confidence varies by row). A quarter of the encrypted code is debug code the
   retail game does not show; the gameplay share (characters, player control, damage, AI behaviours, camera) is the one future
   gameplay work would meet.
4. **What it really blocks.** Not reading: the overlay route reads 98 % of it today. What stays hard is the protection's own
   obfuscated code (9 MB, control flow scrambled, no decompiler output worth reading), which matters only where game code was
   moved into it (the frame limiter's entry, some relocation stubs). What stays forbidden is changing it: patching an
   encrypted body or a guard risks the protection's reaction (I-0114's fault jumping 1 GB off the image, INFERRED), and the
   project's hooks already avoid protected entries or pin their decrypted bytes. For the project's way of rebuilding systems
   (the native module replacing the engine's work, not a recompiled executable), the protection blocks nothing that has been
   needed so far.
5. **Recommendation:** keep the protection untouched and the game offline; make the overlay the standard way to read an
   encrypted function (one line in the `ghidra-research` brief and WORKFLOW « Tools »); build a full decrypted Ghidra project
   only when a workstream needs many encrypted functions at once (the gameplay work after Elden Ring is the likely one); no new
   capture until a function falls in the missing 1.7 %.

## 1. Where: the sections (OBSERVED, PE headers and entropy, `tools/pe_protect_scan.py`, `tools/scan2.py`)

| Section | RVA | Size | Executable | Entropy (bits/byte) | Role |
|---|---|---|---|---|---|
| `.text` (the game's) | `0x1000` | 39.1 MB | yes | 6.78 | the engine's code; 3.06 MB of it encrypted at rest |
| `.interpr` | `0x271B000` | 49 KB | yes | 6.45 | plain code, 4 Ghidra functions; name and size suggest the protection's interpreter (INFERRED) |
| `.rdata`, `.data`, `.pdata`, `_RDATA`, `.rsrc`, `.reloc`, `.idata` | | 40 MB | no | | ordinary data |
| `.text` (second) | `0x4DBD000` | 9.0 MB | yes | 7.67 | the protection's code: 1,023 Ghidra functions; half its 4 KB windows above 7.2 (4.6 MB near-random), the rest obfuscated code |
| `.bind` | `0x56C1000` | 0.2 MB | yes | 7.96 | holds the entry point (`0x1456C1310`); near-random but unchanged in memory during the 935 run |

- The capture compared all four executable sections in memory with the relocated file every 2-5 s for 240 s and recorded
  differences **only in the game's `.text`** (7,769 records, all periodic). So the second `.text` and `.bind` do not decrypt
  in place; their near-random half is data the guards read or code decoded elsewhere (UNKNOWN which).
- In the game's `.text`, 888 of 10,009 windows of 4 KB (3.47 MB) and, at 256 bytes, 2,029 regions (2.98 MB) are
  high-entropy; the capture's 3.49 MB of differing bytes cover 98.3 % of those regions, and 83.9 % of the differing bytes
  lie inside them (the rest are the small patches of point 1 and region edges).
- The encrypted regions are spread over the whole section (34 of its 40 MB hold some); the largest single
  regions are 132 KB at `0x1419A6900`, 77 KB at `0x1419CDD00` and 51 KB at `0x14185D100`.

## 2. How much: functions (OBSERVED, `.pdata` crossed with the capture, `tools/pdata.py`)

| Class | Functions | Bytes |
|---|---|---|
| plain | 206,321 | 31.1 MB |
| encrypted body (at least half the body differs in memory) | 15,602 | 3.06 MB |
| partly changed (small runtime patches) | 495 | 0.19 MB |
| start with a jump into the protection's `.text` | 1,216 (931 plain, 253 partly changed, 32 encrypted behind it) | |

- The 1,216 entry jumps: 20 functions are only the jump (5-8 bytes, the body moved into the protection's section, as
  `0x140B3E1D0`, MENUS-MAP); the others keep a body behind the guard, as `0x1408DF920` (LEGACY-ROOT-INVENTORY-903).
- The small patches (1,305 runs of 16 bytes or less) are an instruction followed by a `jmp`, `push rbp; lea rbp,[rip+x];
  xchg [rsp],rbp; ret` (an obfuscated jump) or `rdtsc` (timing checks): the protection's own stitches, none at a function
  entry (OBSERVED in the bytes; their role INFERRED).
- Ghidra's database follows the file: 4,061 of the capture's 7,169 runs lie outside any Ghidra function (encrypted bytes are
  not disassembled), and the functions that do contain runs often stop at a stub. That is why callers went missing in
  MENUS-MAP and why `.pdata`, not Ghidra, gives the boundaries here.

## 3. Which systems (INFERRED, `tools/attrib2.py`)

Method, on the decrypted image (file plus overlay): a function's own class when a vtable holds it (RTTI of 10,700 vtables,
2,169 encrypted functions); else the names registry `decompiled/GHIDRA-NAMES.tsv` (24); else its callers' and callees'
classes or names, three rounds, common helpers (more than 40 callers) skipped (3,535); else the nearest labelled function
within 2 KB, the same compilation unit by the linker's layout (5,469, the weakest); strings referenced by the body break ties.
Then keywords map the labels to systems. Per-function rows: `evidence/I-0133/artifacts/scan/attrib2.tsv`.

| System | Encrypted functions | Bytes | of which by nearest neighbour | Examples |
|---|---|---|---|---|
| characters and gameplay | 2,544 | 780 KB | 207 KB | `ChrIns`, `PlayerIns`, `PlayerCtrl`, `EnemyIns`, `SprjChrDamageModule`, AI behaviours, the follow camera's constructor, item upgrade names |
| debug tools | 4,489 | 775 KB | 363 KB | the debug menu's items and tweakers, `WorldChrManDbg`, Havok's debug display, the Lua and EzState debuggers, map-walk keys |
| world, maps, events | 471 | 165 KB | 53 KB | stage and event code, MSB objects, cutscene (Remo) data |
| rendering and effects | 560 | 157 KB | 42 KB | `GXFfxSceneCtrl`, `SprjSfxImp`, the light lists, `STEP_Initialize_forRenderingSystem` |
| menus and UI | 401 | 150 KB | 41 KB | `MenuMan`'s constructor parts, NewMenuSystem's gates, equipment dialogs, `CSWindowImp` |
| input | 490 | 137 KB | 42 KB | pad steps, manipulators |
| engine core | 501 | 119 KB | 45 KB | steps and tasks, the file-system aliases (`system:/`, `game:/`), language start-up |
| files and resources | 338 | 96 KB | 32 KB | loaders, the encrypted binder utilities |
| utilities (CRT, std) | 417 | 89 KB | 36 KB | |
| Havok physics | 121 | 34 KB | 27 KB | |
| online | 55 | 15 KB | 10 KB | notifications, login jobs; three bodies cite `CSCheatDetection.h` |
| unattributed | 5,215 | 610 KB | | small functions with no class, caller or neighbour found |

The functions the repository had already met encrypted fit this map: the menu gates and the menu-button test (MENUS-MAP),
the Options page's preset logic (§77.5), the fade gate and its easing (FADE-PLATES-NATIVE), the region and language getters
(FONTS-LANGUAGES-NATIVE), the SFX manager's methods (LEGACY-ROOT-INVENTORY-903), the camera ramp `0x1405E2810` (I-0112,
I-0114), the animation graph's update and the HavokScript bindings (I-0130), the material solver slot `0x1400A7E40`.
Seven encrypted functions are named by address in `native/` (four of the effect scene control's, `SprjSfxImp`'s
constructor, the rendering system's initialise step, the menu input gate): the module reaches them only after the game has
decrypted them (INFERRED from the 935 capture, where they were decrypted from the first passes).

## 4. The read route, checked tonight (OBSERVED)

`I0066Overlay936.java` (evidence, I-0066 cycle 936) on `ghidra-mirror-i0066` under 12.1.4, in memory, read-only: the 7,169
runs applied, three functions decompiled (`evidence/I-0133/artifacts/overlay-route-test/`):

- **`0x140ED1080`** (I-0076's « encrypted launch-option getter ») returns the configuration key
  **`SPRJ.WIN64.isFakeFullScreen`** (UTF-16 at `0x1429ACE50`, read through `FUN_14179ADC0` as `MenuMan.NewSystem` is), or
  false when `FUN_140ED1440` is true. So the retail source of CSWindowImp `+0x40` (DS3's borderless mode) is that key. I-0076's
  open point is answered; the native `borderless.flag` stays the switch.
- **`0x140EF61A0`** is **not** « window style and placement » (DISPLAY-SWAPCHAIN-NATIVE's INFERRED role, and its name in the
  registry, `ApplyWindowStyleAndPlacement`): when the global `0x144964480` is set (the debug menu, LEGACY-ROOT-INVENTORY-903)
  it registers `WINDOW` items under a debug-menu node; otherwise it does nothing. The name is to be corrected at the next
  names pass (writer token, Thomas saves, D-0269).
- **`0x14075EBD0`** (9 KB, called by `MenuMan`'s constructor) decompiles to 876 lines: it builds the « MENU MAN » debug-menu
  pages (`FrpgMenuSys List`, flags).

## 5. What it blocks

| Need | Blocked? | Evidence |
|---|---|---|
| Reading an encrypted function (static decompilation) | **no** for 98.3 % of the bytes: the overlay route, no run; the missing 1.7 % (52 KB, region edges) and functions decrypted only later in play would need a new capture | §1, §4 |
| Reading the protection's own code (second `.text`, `.bind`) | **yes in practice**: obfuscated, partly near-random, unchanged in memory; only matters where game code was moved into it (20 functions; the frame limiter's entry `0x140EED810`) | §2; CADENCE-STUDY |
| Ghidra's cross-references and function bounds around stubs | **partly**: callers in undisassembled code; worked around with `.pdata`, raw rel32 scans (`I0067Map.java` query `e:`) and linear listings | MENUS-MAP; §2 |
| Hooking an engine function | **only at protected entries**: a guarded or encrypted entry is not hooked (D-0219's cadence work, CADENCE-REFLEX-READY); the module's start pins accept two functions only in their decrypted form; 4 of about 370 native launches refused at start by the pins (`prefix_refused`, cause not established here) | STATE; CADENCE-STUDY |
| Patching protected code, removing the protection | **yes, and not to do**: the guards check code while the game runs (GENERAL); I-0114's faults with the protection's thunk on the stack (INFERRED its reaction); a stripped executable would be a modified game binary, out of the project's rules and risky online (GENERAL) | I-0114 RESULT |
| Rebuilding a system natively (the project's way) | **no so far**: every replacement read its original first (decrypted when needed: the SFX manager, the menu gates, the camera); the module runs inside the protected game without touching it | I-0066, I-0067, I-0114 |
| Recompiling the whole executable from decompiled code | the protection is not the obstacle; the size is (222,418 functions, 39 MB of code, no symbols); the 7 % encrypted are readable through the overlay | §2 |

## 6. Recommendation

1. Keep the protection and the executable untouched, the runs offline (D-0116), the hooks on plain entries or pinned on
   their decrypted bytes, as now.
2. Make the overlay the standard route for any encrypted function: one line in WORKFLOW « Tools » and in the `ghidra-research`
   brief (`I0066Overlay936.java` with `overlay935/overlay.bin`, on a 12.1.4 mirror, read-only). Copying the script and
   `code_overlay.py` from the evidence store into `investigations/I-0133-protected-code-map/tools/` would put them under
   version control (they are the project's code, not the game's). Both edits are rule changes: Thomas's yes in the thread
   first (relayed approvals do not count for WORKFLOW).
3. When a workstream needs many encrypted functions (the gameplay work after Elden Ring: 780 KB of character code), build once
   a decrypted analysis copy: the overlay written into a copy of the executable kept with the Ghidra mirrors (never run,
   never in Git or the cloud), imported and analysed in its own 12.1.4 project (a few hours of CPU, run hidden), the names of
   `GHIDRA-NAMES.tsv` replayed. Not worth it before.
4. No new capture now. One would be needed only if a function falls in the missing 1.7 % or is decrypted later than the first
   minutes (a reference run with the code capture, which I-0102 removed from the module: it would come back from commit
   `45c51173`).
5. Correct `0x140EF61A0`'s role in DISPLAY-SWAPCHAIN-NATIVE (done in this study) and its Ghidra name at the next names pass.

Thomas agreed (2026-11-11 01:14, « D'accord »): D-0272; point 2's rule lines are written (WORKFLOW « Tools », the
`ghidra-research` brief) and the overlay scripts `I0066Overlay936.java` and `code_overlay.py` copied into `tools/`.

## Uncertainty / unresolved questions

- The attribution is INFERRED and uneven: 2,169 functions by their own vtable (solid), 3,535 by callers and callees (good), 5,469
  by the nearest neighbour (weak, a compilation unit can mix classes); 5,215 unattributed. The keyword map can misfile a label
  (a debug helper named after the class it inspects is counted with that class).
- « Encrypted body » means at least half the body differed in memory during the 935 run; a function decrypted only later in
  play, or re-encrypted before the first pass, would be missed (the high-entropy check bounds that to the 1.7 % above).
- What the near-random half of the protection's `.text` and `.bind` holds is UNKNOWN (not decrypted in place in 240 s).
- The product's name (Arxan) is the repository's earlier usage, not established here.

## Artifacts produced or modified

- `tools/pe_protect_scan.py` (sections, entropy; its overlay table names both `.text` sections `.text`, `scan2.py` separates
  them), `tools/scan2.py`, `tools/pdata.py`, `tools/attrib2.py` (Python, standard library), `tools/I0133ProtectMap.java`.
- `evidence/I-0133/artifacts/scan/` (summaries, per-region and per-function tables), `gh935/` (Ghidra blocks and runs per
  function), `overlay-route-test/` (the three decompiled functions; game code, local only).
- DISPLAY-SWAPCHAIN-NATIVE and I-0076 RESULT: pointers to §4.
