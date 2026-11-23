# Genuine startup CPU requests and visual reference

Historical technical record. Current execution and next action: [STATE](../../.project/STATE.md).
D-0156 supersedes the earlier continuation schedules; proof limits remain unchanged.

I-0066 cycles21-24, 2026-10-07. This closes the initial24-cycle tranche;
the same authorized startup/zone objective continues with budget32, used24.

## Original observations

Reference04 used the separate original D3D11 process and the reference observer.
Its real resource callback completed8 -> 9 with repository state4. A bounded
read of protected CPU entry506450 recorded loaded code, rather than treating
its encrypted installed bytes as instructions. Reference06 was an original
20-second run with no injected module; Read-LoadedCode.ps1 authenticated the
installed image and acquired512 bytes at4FB2B0 with read-only process access.
Disassemble-LoadedCode.py verifies acquired SHA256, derives a temporary minimal
PE for the actual MSVC dumpbin, preserves address-correct assembly and deletes
that temporary PE. Neither acquisition contains GPU inputs or resource payloads.

506450 returns the actual manager+18 result in RAX, despite the static
decompiler's void annotation. It allocates a real CPU request of sizeA0 with
heap479C258 and constructs it through4FB2B0. The loaded constructor uses the
common CPU resource base234A480, real vtable2798518 and a CPU pointer vector
at+80. Its allocator query, CPU flags and vector initialization contain no GPU
construction. Other request constructors are routed in
`artifacts/startup-resource-request-objects`; request functions and font source
are in `artifacts/startup-resource-continuation` under evidence/I-0066.

Reference05 used the existing repository driver, source native/world/drive.cpp,
under the [live protocol](../../evidence/I-0039/LIVE-PROTOCOL.md). E confirms the
title and dismisses the real previous-session warning. No saved game was loaded.
Retained captures show [Bandai Namco](../../evidence/I-0066/artifacts/i66-reference-startup-05/startup-02.png),
[FromSoftware](../../evidence/I-0066/artifacts/i66-reference-startup-04/fromsoftware.jpg),
[title](../../evidence/I-0066/artifacts/i66-reference-startup-05/startup-04.png),
[warning](../../evidence/I-0066/artifacts/i66-reference-startup-05/menu-02.png) and
[main menu](../../evidence/I-0066/artifacts/i66-reference-startup-05/menu-04.png).
The menu explicitly shows Offline. The driver executable SHA256 is
3C2B862982443C6537AD15940CB13625C3FA118D4B7FA157BFAA5D3F6F39762C.
Thomas now requests this screenshot/input method instead of Computer Use.

Reference05's automatic restoration guard reported a surviving game. After
game absence was verified, Restore-StartupTrace.ps1 -Baseline restored all
hashes; the error and recovery receipt are preserved. The runner now waits up
to two seconds for process enumeration to settle without touching another PID.
Reference06 automatic restoration passed. This does not establish the precise
cause of the reference05 transient.

## Native responsibilities to retain

The source D59E90 issues these actual CPU requests after genuine F0 state4:

| Entry RVA | Name / order | Original storage |
|---|---|---|
| 506320 | shader:/Shaderbdle.shaderbdlebnd | step+100 |
| 506450 | shader:/Shaderbdle.shaderbdlebnddebug | overwrites step+100 |
| 5052F0 | mtd:/AllMaterialBnd.mtdbnd | step+120 |
| 503E60 | D27D30 indices1,2,3,4,5,12,6,10, in that order | CPU vector step+148 |

These eight files are GPARAM, not texture packages. D27D30 reads the enabled
actual name table294F020 and formats gparam:/%s.gparam. Its real engine string
owns any allocated characters through allocator+68. Vector6120F0 grows the real
CPU vector, tailcalling611170: CPU allocator612510 and pointer copy60F3D0,
then frees previous storage with the real heap. Source/assembly in
`startup-cpu-vector`, `startup-cpu-vector-owner` and `startup-cpu-vector-leaves`
qualify this bounded storage responsibility. These functions use no GPU objects.

Keep exact original manager ownership and storage; request issuance alone
cannot certify native rendering readiness. Do not call D573B0, original GX
snow-texture transfers, or original debug-font GPU initialization2355720. Do
not set DrawStep+C8 or advance phase8 until actual native consumers exist.
The font CPU source names title:/Font/debugFont_ps.ppo and debugFont_vs.vpo are
known; their native program/geometry admission is unfinished.

## Continuing bounded plan

Cycles25-32: (1) preserve and validate actual CPU resource requests while
holding native render readiness; (2) identify the genuine startup primitive
producer and feed established native primitive/program/texture algorithms;
(3) validate actual logos/title/menu before the next zone-loading boundary.
Keep cursor loading and main-menu music in the startup obligations, as Thomas
explicitly added them. Their source/lifetime/audio behavior is not yet qualified.
No native logo/title/menu/cursor/music/zone success is claimed at this checkpoint.

## Continuation status

The initial24/32 plan above is historical. Current34/40: all eleven requests execute; native18 admitted all eight GPARAMs with the existing decoder and ran2,900 clear frames, fault/refusal/debug-errors0, closed1. Shader/effect/MTD GPU dependencies were replaced by owned CPU ingress. See [current RCA and lifetime qualification](CPU-BOUNDARY-RCA.md). The real game cursor has Thomas's native14 witness plus window resource101 source; actual native UI and menu audio remain open. Next: semantic startup primitive ingress before readiness/phase8 advancement.
