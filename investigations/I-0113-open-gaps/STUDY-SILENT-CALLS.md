# The engine calls that do nothing natively, without a word (study thread S, static, 2026-11-07)

Thomas (2026-11-07, relayed by the coordinator): an audit, given to S, of the engine calls that natively do nothing and say
nothing, after gap 22 (I-0113 RESULT « Tranche 22 »: the Irithyll barrier's jumps, an effect draw of pass 0x2D). The cause E
found that morning (tranche 23, branch `claude/cranky-wu-bcf0b0`, OBSERVED there): the original copies the rectangle of each
distortion quad into the frame copy (`FUN_140260440`) before drawing it, the native once before the pass. The copy function
found no target natively and returned, with no error and no row. It was missing from [STUDY-NOT-YET-NATIVE](STUDY-NOT-YET-NATIVE.md)
and from [I-0103 CENSUS](../I-0103-effects-census/CENSUS.md). The fixes stay with E and R; S changes no code. **STATIC** (GhidraMCP
on Thomas's GUI, headless Ghidra on the mirrors `ghidra-mirror-i0066` and `-i0068`, the native tree at `main` `080ba94d`), plus the
run journals already taken, then one native run with `ffx-record.flag` (§4).

**Method.** Five Sonnet sweeps, each re-read where a conclusion rests on it: (1) the 75 GX context wrappers 141920200..141921C00
and the writers of the context's state cache; (2) the gates of the draw context's 29 slots (vtable `143D65688`), `299E40` and its
constructor `25FB80`; (3) the gates of the 17 builders and renderers the native runs (`allowed[]` in `effect_draw.cpp`); (4) every
instruction reading the GX root's device members `[[144961B50]+0x4DD0]`, `+0x4DD8`, `+0x4E10`, `+0x4E18`; (5) the 120 newest native
journals and the native code's silent branches. Their tables and scripts are in `evidence/I-0113/silent-audit/` (SHA256SUMS.txt).
The two copy functions (`260440`, `262A70`) were decompiled and read by S itself.

## 0. Answer first

- **Why such calls are silent.** The native effect context (I-0104) hands engine code a zeroed GX inner context (`effect_draw.cpp`:
  only `+0` the trap object, `+0x258`/`+0x4D8` one viewport) and native tokens for the frame copy and the depth (`+0x08` zero).
  The GX wrappers themselves cache nothing (sweep 1: none writes or filters on the context): the state cache (`+0x4F8..+0x530` the
  render targets, `+0x538` the depth view, `+0x59D` their count) is written by the engine's callers after a wrapper call
  (`1400151A0`, `14000AA90`, `140005F00` and eight more). No caller binds a target on the native inner context, so **every engine
  read of the bound target finds none**. A call that reaches an unhooked wrapper traps and is logged by default (`native_ffx_trap`,
  none in the 120 newest native journals, sweep 5, as E found in 60); a call that reads the cache first and stops never reaches a trap.
- **In the effect path, statically, two functions read that cache, and both are a per-rectangle scene copy**: `260440` (slot 26)
  and `262A70` (slot 20). Sweep 3 found no other read of the inner context in a condition in the 17 builders and renderers, and no
  branch on a GX wrapper's return. Readers of `+0x59D` in the whole image (sweep 1's displacement list): those two, the engine's
  set-target helpers, `206FC0` (the cache's « get targets », called by `206850` and `1ADF7C0`, outside the effect path) and the
  state save/restore pair `1ADEED0`/`1ADF7C0`. So **the class is small and enumerated**: table below, rows 1 and 2.
- **The « ~150 device-absent branches »** (`01_state.inc` comment, STUDY-NOT-YET-NATIVE §3, GX-DEPENDENCIES A2) are not 150
  branches: 249 instructions in 159 functions read `[GXRS+0x4DD0]`/`+0x4DD8`/`+0x4E10`, and **only 18 functions test** the member
  (sweep 4). About 70 dereference it untested (they would fault if reached, not stay silent: a behaviour risk, R2's kind) and 73
  pass it on. The 18 tested ones are covered by native replacements or have no native consumer (§2).
- **What stays silent by default and can hide a missing family:** the skipped renderer classes and entity kinds, the model
  particles' unreadable inputs, counted only with `ffx-record.flag` (row 3). None of the 120 newest native journals ran with it.

## 1. The ranked list

Rank: how likely a visible defect is, and how much of the game it touches. « Visible » is INFERRED unless it says OBSERVED.

| # | Call (where) | What the original does | What the native does | Effect visible, probably | Status |
|---|---|---|---|---|---|
| 1 | `FUN_140260440`, draw context slot 26 (+0xD0), called by `396F30` (FXR 607, E04 distortion, at `140399C77`) and `39E900` (608, E05 radial blur, at `1403A02C0`) | copies the bound target's rectangle around the quad (±16 px, clamped) into the frame copy `[[ctx+0xA70]+8]` with `141920280` (CopySubresourceRegion), when the formats match, before the quad | `[[ctx+0x20]+0x59D]`/`+0x4F8` zero: returns; the copy token's `+0x08` is zero too (a second closed gate); the builder draws anyway | **OBSERVED (E, tranche 23): gap 22**, the Irithyll barrier and the Firelink fire jump; the radial blur (Fire Orb's casts, 608) samples the same stale copy: INFERRED, the same kind of jump where its quads overlap other effects of the pass | E fixes it (the CopyRect statement, a hook of 260440); E's check should include a 608 cast |
| 2 | `FUN_140262A70`, slot 20 (+0xA0), the 8th appearance setup, called at least by the trails 606 (`3709B0`) and 10012 (`2891A0`) (their setup slots, `effect_draw.cpp`'s comment, I-0105) | when the trail's FXR property at resource `+0x280` (read through the curve table at `[1449327A0]`) is above 0, the same rectangle copy as row 1 (`141920280` at `140263214`), around the trail's bounds | same two gates closed: returns before the copy; the trail draws | trails with a distortion (a heat haze along a swing, a projectile's or a spell's trail) refract a copy made before the pass, without the effects drawn earlier in it: jumps like gap 22, or those effects absent from the refraction (INFERRED, not seen; which FXRs set the property: not read) | **new**; told to E for its fix (one hook for both, or a sibling) |
| 3 | `effect_draw.cpp` Execute: commands of a renderer class outside `allowed[]`, an entity that is not a node draw item, a model particle with unreadable arguments or bones | runs them | skips them; `skipped_renderer_<class>`, `skipped_entity_kind`, `model_*` counted **only with `ffx-record.flag`** | the light shafts 10003 (class `3D65A88`, m30, m33, m38: known, CENSUS), any class the witness zones never showed (the DLC's data, a spell's), invisible in an ordinary run | E: one default row per new skipped class or kind (rate-limited, as `native_ffx_trap`) would make the next one loud; the run of §4 found none at Firelink, the High Wall and four pyromancies |
| 4 | `ClearRtv` hook (`141920200`, ClearRenderTargetView) on the native context | clears a target | nothing, not recorded (`effect_draw.cpp:220`) | none today: its callers in the effect code are the light shafts' `271B40` (a skipped class) and GX passes outside the effect commands (`2ABDF0`, `2AC2C0`, `2ACAC0`, `2AD3A0`, `2AD560`, reached from `2AA6D0`/`2AB540`, not from a command). Visible if 10003 is ever allowed | note for whoever allows 10003 |
| 5 | `2A7E70` (GPU particles' draw) reads `[[GXRS+0x4DD0]+0x2C0]` (cbCubeEnvSH1) | the scene's environment SH | a native 48-byte buffer of zeros, only during Execute (`effect_draw.cpp:439`) | GPU particles lit without the environment (STUDY-NOT-YET-NATIVE row 8, known) | known |
| 6 | Other pipelines' exposure: `87FB0`, `88D90`, `89140` read `[[[GXRS+0x4DD8]+0xCB8]+0x21B0]` only when `+0x4DD8` exists (and bit 10 of the pipeline's `+0x50`) | the auto exposure | the constant at `143D7EE10`; the native hook answers the native exposure for the native pass context's pipeline block only (`NativePipelineExposure`, `21_effects.inc`) | an effect or a programme scaled by another pipeline's exposure (a menu or portrait view's) would be 1.0: whether any runs natively is not read | TO CHECK, low |
| 7 | Logged, but never studied for what they hide (journals, sweep 5) | | `native_asm_part_missing_binding` and `_missing_inputs` (13,609 rows each, 109 of 120 journals), `native_offscreen_hair_refused` (1,140, 19 journals), `native_disp_refused` (1,000, 5), `native_selected_object_unsupported` (872, 109), `native_object_skipped` (36, `nonfinite_placement`) | read by E (I-0113 RESULT « Tranche 24 », « Tranche 25 »): the part pair, parts with no geometry loaded; the unsupported classes `CSSgEzDraw` (debug primitives) and `GXVisbTester` (the occlusion queries the native omits), RTTI; the disp refusals the map pieces (SprjModelDispEntity) the game's own tests refuse; the hair refusals 63CB0's masks in offscreen views; the skipped objects non-finite placements | INFERRED benign, the hair refusals TO CHECK on a portrait against the original |

The silent native branches sweep 5 found elsewhere (the cloth upload's `status=1` on a closed stream, `NativeDynamicMeshBufferSet`'s
null owner, `NativeMoviePublish`'s missing view, the releases of effect objects, the snapshot light producers' blends « not
transcribed ») are transient, teardown or offline paths; none names a visible family (the list is in the evidence folder).

## 2. The engine's device-absent branches (sweep 4)

The 18 functions that test `[GXRS+0x4DD0]`, `+0x4DD8` or `+0x4E10` (VERIFIED in the decompile), and what covers them natively:

| Functions | Skipped without the member | Natively |
|---|---|---|
| `1AC840` | the instance's probe light blend | `NativeObjectProbeLightBlend` (D1, GX-DEPENDENCIES) |
| `87FB0`, `88D90`, `89140` | the pipeline's auto exposure | row 6 |
| `14002C320`, `1401BB810`, `1401BCE90` | a GPU pass body | the native renderer's passes |
| `1401010A0`, `14019E090`, `1401B93C0`; `1401B16A0`, `140022A80` | GPU handle creation; pool and teardown releases | no consumer natively |
| `140017E30`, `17E60`, `17EA0` | option bytes written into the device's settings `[[+0x4DD8]+0xCB8]+0x2271/0x2272` | the native reads the options itself (I-0067 Q4); `17EA0` is not an option: the game camera's focus distance (I-0141 tranche 9, detoured natively since) |
| `140023100`, `140023410` | the device's reset and update calls | the native device |
| `140264230` (slot 27), fragments `140260767`, `1402649A2` | `+0x4E10` null: t3 = the context's `+0xA68` (the depth token) instead of the override's view | the native binds its depth (t3) |

`+0x4E18` is a settings byte, not a device member (`213A0` writes 0x101, `25830` registers it). `[GXRS+0x4E10]` is set and cleared
by two undisassembled fragments (`14010C44A`, `14010C46E`): an override object, null natively. The untested readers (70
dereferences, 73 forwards) are listed with their callers in `gxrs-device-sites.tsv`; they are a fault risk (R2), not silent no-ops.

## 3. The GX context layer (I-0104, D-0190): what S sees, nothing changed

Thomas worries about the « GX context » layer. What the sweeps show, as facts for him and the pilots:

- **The layer is the engine's own GX device context, emulated** (as Elden Ring's GX context sits over D3D12, I-0104
  ER-EFFECTS-STUDY): the hooked wrappers record state into the native draw record, nothing D3D11 is called. That part is sound:
  the 66 real wrappers are thin forwarders with no logic of their own (sweep 1), so hooking them loses nothing.
- **Its blind spot is the context's readable state.** The original's callers keep a cache in the context (targets, depth view,
  viewports, scissors, the bound states) and some engine code reads it back. The native fills only the viewport. Every
  read-back of anything else returns « nothing bound ». In the effect path that is rows 1 and 2 and nothing else (statically);
  outside it, `206FC0` and the save/restore pair are not on the native context.
- **The tokens are opaque on purpose**, but `+0x08` (a GX texture's D3D11 resource) is zero: code that reads a texture's
  resource through `+0x08` stops. Rows 1 and 2 again; no other effect reader of a token's `+0x08` was found (sweeps 2 and 3 list
  the token reads: `+0x48` the view, the width and height slots).
- **Two ways to close the blind spot**, for E to choose (not S's design): (a) per reader, the native statement the reader stands
  for (E's CopyRect for 260440, its sibling for 262A70), which keeps the layer as it is; (b) a truthful cache: Execute writes the
  HDR target's and the depth's tokens at `+0x4F8`/`+0x538` and `+0x59D` = 1, and the copy token's `+0x08`, so the engine's own
  code finds a target, and its CopySubresourceRegion then reaches the wrapper `141920280`, unhooked today (it would trap and log).
  (a) is what E is writing; (b) would catch a reader the static sweeps missed. Neither changes D-0190.
- **The traps stay the right alarm for unhooked calls**; the skipped classes (row 3) are the one silent skip of the layer that is
  cheap to make loud.

## 4. What a run would add

The traps are already logged by default and none occurred (E). A 10-minute native run with `ffx-record.flag` (Alpha's every-spell
copy casting through its catalysts, then a second zone) gives what no journal of the last 120 holds: the skipped classes and
entity kinds by spell and zone (row 3), the commands run by class (601 `3D7E4A0`, never witnessed, included), and the model
particles' unreadable inputs. Rows 1 and 2 cannot be counted without new code (a counter in E's hook would do it).

**The run, `s-silent-rec-1`** (2026-11-07 10:45-10:51, the ASUS, native, `ffx-record.flag`, the launcher's module of `main` `080ba94d` copied to `evidence/I-0113/silent-audit/build`; `alpha-allspells-pyro` loaded at Firelink, travelled to the High Wall of Lothric, eight casts of Seething Chaos, Flame Fan, Fire Orb and Profaned Flame; no fault, the save restored by hash; read in `evidence/I-0113/silent-audit/s-silent-rec-1.md`). OBSERVED over 16,800 frames in 28 `native_ffx_execute` rows: **no skipped renderer class, no skipped entity kind, no trap**, no `update_unowned`, no ring or arena refusal, no dropped draw; the classes run were 603 (`3D7E5C0`, 778,151 commands), the GPU particles (`3D65878`, 72,012), 604 (`3D7E680`, 21,751), 607 (`3D7E800`, 21,652), 10012 (`3D660B8`, 5,854), 608 (`3D7E8C0`, 1,205), E03 605 (`3D65F88`, 547), 606 (`3D7E380`, 300) and 602 (`3D7E740`, 140); 600 and 601 did not run there. About 52 classic draws a frame (877,558 in all), 2 of them distortion quads (pass 0x2D, GX 4), so 260440 is reached about twice a frame at these places. So row 3 is empty at Firelink, the High Wall and these four pyromancies; the zones and spells not run (miracles, sorceries, the DLC) stay unknown. Logged, not silent: `native_asm_part_missing_binding` and `_inputs` 249 rows each, `native_selected_object_unsupported` 8 (row 7).

## 5. Limits

- Static reachability: a reader of the context's cache outside the 17 builders and 31 draw-context functions, called from a
  callee the sweeps did not open (`25D590`, `25D700`, `25E660`, `25E720`, `C5EB0`, `59470`, `2BA3A0`), is not excluded.
- Row 2's property (resource `+0x280`) is not tied to an FXR field name, and no trail with it was seen.
- The ring's reserve (`213B690`, `vtbl[1]`) can fail and skip a primitive silently in the builders; natively it is `RingMap`,
  which counts `ring_refusals` and grows the arena next frame (`native_ffx_arena_grown`, logged by default).
- The device-absent counts are a lower bound for the obfuscated fragments (sweep 4's raw scan).
