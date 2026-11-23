# I-0005 static and runtime-analysis toolchain

Captured: 2026-09-19. Static analysis did not launch Dark Souls III. Runtime Pass A and reversible runtime Pass B later launched it under explicit authorization; its installation was not modified. Pass C preparation did not launch the game.

- Exact executable: `DarkSoulsIII.exe` 1.15.2.0, Steam BuildID `10167187`, SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Static base: canonical I-0002 Ghidra 12.1.3 project `DS3_1_15_2_10167187_I0002_FINAL`.
- Existing exporters: `I0002ExportSelected.java`, `I0002FindReferences.java`, and I-0003 bounded inventory/pointer scans.
- I-0005 scripts: `I0005ClockImports.java`, `I0005TemporalConstants.java`, `I0005InstructionScalarSearch.java`, `I0005DumpInstructionRange.java`, and `I0005TransientDisassemble.java`.
- All headless reads used `-noanalysis -readOnly`. The transient disassembly script runs inside a rolled-back transaction and does not modify the canonical Ghidra project.
- Instruction listings govern calling convention, offsets, scalar forwarding, and selector indexing. Decompiler output is supporting material only.
- Historical lead: `https://github.com/0dm/DS3DebugFPS` master commit `b9e5a8ca8390ed0fd545b33944103c53926b7720`, source read only; no binary or patch was executed.

## Runtime Pass A

- External injector and observation DLL: `runtime/pass-a/instrumentation/`.
- Hook library: MinHook v1.3.4 at commit `c3fcafdc10146beb5919319d0683e44e3c30d537`.
- Probe SHA-256: `8B089520DB5D07BEC8D4A5AB1B0FA0BE700B5BBDF96137FFA3FAA012C1A4C8EC`.
- Injector SHA-256: `B8CCCBD99300450C250D0D4CFC2C4530E8B7762E691CB8AAE141B549258524C1`.
- Analyzer: standard-library Python script `runtime/pass-a/analyze_pass_a.py`, SHA-256 `051A720331FCBFFBB17CF07E973206F39E99BD026C30597C056C3D749A444A1D` before documentation finalization.
- Raw timestamps use Windows `QueryPerformanceCounter`; session frequency was 10,000,000 ticks/s.
- The probe wrote only CSV/marker files under the repository evidence directory. No game file or timing state was written.
- Static follow-up for live callback addresses used the canonical Ghidra 12.1.3 project in `-noanalysis -readOnly` mode.

The noisy whole-program displacement scans were discarded after they failed to discriminate timing semantics. Curated exact references and selected functions are retained instead.

## Runtime Pass B

- Reversible external probe and injector: `runtime/pass-b/instrumentation/`.
- Hook library remained MinHook v1.3.4 at commit `c3fcafdc10146beb5919319d0683e44e3c30d537`.
- Intervention: central object `+0x354` float and `+0x358` byte only; original values logged and restored before hook removal.
- `force_present_zero` remained disabled in every session.
- Per-session analyzer: standard-library Python `runtime/pass-b/analyze_pass_b.py`.
- Cross-condition analyzer: standard-library Python `runtime/pass-b/compare_pass_b.py`.
- C++ sources were compiled with the pinned Visual Studio toolchain invoked by `runtime/pass-b/instrumentation/build.ps1`.
- Raw timestamps use QPC at 10,000,000 ticks/s.
- The probe wrote CSV and marker files only under `evidence/I-0005/runtime/pass-b/sessions/`.
- G-Sync/VRR was enabled according to the operator; no display scan-out instrumentation was used.
- Primary 60/90/120 consumer sessions loaded system module `C:\Windows\system32\nvspcap64.dll`; initial causal sessions and the detailed 120 callback session did not. No ReShade/NUE or DS3-directory D3D11/DXGI proxy was observed.

## Runtime Pass C preparation

- Separate semantic probe and injector: `runtime/pass-c/instrumentation/`.
- Probe SHA-256: `C3A9992B56B661D69621D23CAF4017919F5E6BA3D7A9E5D3305F8BBD09C890E3`.
- Injector SHA-256: `DC1FC6191461EB9CAE5BF52145EB8B9766AD605D64145F8F3084EB87CE729B52`.
- Build remains pinned to MinHook commit `c3fcafdc10146beb5919319d0683e44e3c30d537` and the Pass B MSVC environment.
- `verify_worldchrman.py` hashes the executable, maps PE file offsets to RVAs, and requires the unique instruction/global result `0x004C283A → 0x0477FDB8`.
- Semantic logging adds player position/facing, candidate animation ID/clock/duration/play-speed, HP/stamina, keyboard/XInput state, and marker endpoints. These field roles are not promoted until runtime qualification.
- The committed configuration is disabled. No Pass C session exists at this checkpoint.

## Runtime Pass C final-writer extension

- The disabled-by-default writer watch uses three per-thread, four-byte,
  write-only x64 debug-register breakpoints on the qualified transform
  coordinates and restores the original complete debug-register state.
- The standalone mechanism test and the writer-watch DLL were compiled with
  `/W4 /WX`; the synthetic test produced exactly three expected hits and exact
  restoration.
- The announced target-60 live session is
  `runtime/pass-c/sessions/20260919T144001Z-pid18536`. Its dedicated standard
  Node.js analyzer is `runtime/pass-c/analyze_write_watch.mjs`.
- Live bytes identified writer RVA `009DC580`. Follow-up exports and reference
  queries used the canonical Ghidra project with `-noanalysis -readOnly`; they
  are under `static/locomotion-final-writer/`.
- The committed configuration is again disabled after capture. No game file was
  changed, and the runtime timing state restored exactly to `30.0/0`.

## Offline final-writer algebra bridge

- New bounded exact-build exports cover `009D8340`, `009D9390`, `009DA810`,
  `009DC090`, `009E65B0`, `00DD9930`, `00DDA1C0`, `00DD9B50`, `00DDA1A0`,
  `00DDA750`, and direct exact-chain scalar-writer candidates.
- `I0005WriterFieldProvenance.java` supplied a broad reproducible offset lead
  inventory. Only manually revalidated exact owner chains are promoted into
  the curated candidate/scalar provenance CSVs.
- `writer-constants.csv` is a read-only byte dump. Instruction listings govern
  the mask, constant, branch, and producer conclusions; decompiler text is
  supporting material.
- No runtime probe was changed or run. The RVA `009DC559` mirror remains a
  design whose future implementation must preserve checkpoint-disabled
  defaults and exact restoration.
