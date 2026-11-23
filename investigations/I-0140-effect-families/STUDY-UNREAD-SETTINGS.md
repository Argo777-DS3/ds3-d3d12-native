# STUDY-UNREAD-SETTINGS: the original's settings the native renderer never reads, or reads under the wrong name

Study thread, 2026-11-14 06:00 to 07:30 (Paris), read-only on the code (Thomas's yes on R's proposal, relayed by the coordinator).
Nothing was built or run; no game was launched (the tour replayed cutscenes on the PC meanwhile). Sources: the 168 GPARAM dumps
(`evidence/I-0066/queries/gparam-all`), the native code on `main` `30625003c` (`native/world/light_producers.cpp`,
`native/frame/game_bootstrap/12_scene_collect.inc`, `13b_prepare_model_frame.inc`, `18_resource_admission.inc`), the 62 run journals
written since 2026-11-14 00:00 (`evidence/I-0066/artifacts/*/startup.jsonl`), GhidraMCP on DarkSoulsIII.exe 1.15.2 (a
`ghidra-research` subagent mapped the 26 banks of `BindGparamFileToBanks` 140049D00; its claims rest on I-0140's
`evidence/I-0140/ghidra-gx-groups/`, and I re-read the Fog binder's slot myself), and the earlier sheets cited per row.
Statuses: OBSERVED (a journal or a data file read), VERIFIED (code or disassembly read), INFERRED (reasoning, not witnessed).

Readers: `tally_sources.py` and `cut_keys.py` (the journal sweep), kept in this thread's scratchpad; their outputs are summarised
below, not copied.

## 0. Answer first, ranked by visible impact

| # | Setting | What the native does | Visible where | Status | Workstream |
|---|---|---|---|---|---|
| 1 | A cutscene's **ColorGrading LUT** (26 of 49 s-files enable it; LUT ids 0, 1, 2, 3, 10) | refused « non-map record (prefix unread) »: the cutscene is drawn **without any grading** while the map's LUT is on outside it | every cutscene whose s-file enables grading (the s-file is read since I-0138 part 4b, so the refusal now meets them) | native refusal OBSERVED (`i140-e9-c31000010-nat`: `[231000010, 0/10]=s31_00_0010`, words Enable 1, LUT 1 and 10, refused 6 times); the original's LUT name for an s-file UNREAD (FUN_140095220's other branch) | R, workstream 11 |
| 2 | **mapbase** answering a **cutscene** key | `Banks::GroupEntries` / `FindGroupParam` fall back to `mapbase` for any area whose file lacks the group; mapbase registers only under map areas `1AABB0000` (LIVE-REDUCTION-AM §5 rule 2, STATIC), so the original's lookup for `{2AABBVVVV, id}` goes to `{1, 0}` (rule 3) | the cutscenes whose s-file lacks Setting Info (17 of 49), MotionBlur (46), Anti-Alias (48): GlareQuality 2 against default's 4, DepthOfFieldQuality 3 against 7, Enable ColorGrading 1 against 0, MotionBlur BlurTimeRatio 0.2 against 0.06 and its falloff distances (min 0, max 3) against (2, 0) | native rule VERIFIED in code; keys OBSERVED (`Setting Info[233000030,0]=mapbase`, `MotionBlur[233000030,0]`, `Anti-Alias[233000040,0]`, `Setting Info[237000020,0]`, `[235000050,0]`); the original's answer INFERRED (rule 2 and 3 are static; `remobase` may answer `{1, 0}` instead of `default`, item 11) | R, workstream 11 |
| 3 | The tour's 22 cutscene verdicts (I-0139 tranche 3c) | run on `build-tour6` (2026-11-13 22:00), whose branch lacks part 4b (`git merge-base`: `d365e26f` not an ancestor of `claude/kind-antonelli-18b0e1`): every cutscene draw-parameter record read `default` even where the s-file holds the id (`Dof[234010000,10..60]=default`, `s34_01_0010` 100 to 105, `s35_00_0000/0001` 10 and 20, `s37_00_0020/0021`, `s38_00_00x0` 10 and 20; labels « area » of the old rule) | the tone map, bloom, depth of field and shafts the tour judged in those scenes are not what `main` draws | OBSERVED | tour (I-0139), E (its cutscene defects, I-0140 « Next » 4) |
| 4 | LightSet's **hemisphere** colours, lights 2 to 4 (the fill lights), Env Map's IBL cubes x `EnvMap:ColorScale`, Dominant Light's SH scales | read by `Banks`, then **not used** by the rays since I-0132 R11d (D-0262: no ambient but the bounces; a miss takes the forward sky only) | the original's per-area cold grey ambient; workstream 7 step 7's « player warmer and darker » is the expected look of a frame without it | VERIFIED (shaders' comments and code, `rt_reflect.hlsl` R11d); a decision, not a defect | R, workstreams 7 and 12 |
| 5 | **eclipse.gparam** (kind 12, registered at area 7, Fog for 16 ids) | `AreaFileName(7)` names `m00_00_0007`: no file, every lookup at area 7 falls to `default`; only `SelectVariant`'s diagnostic tries the file | the eclipse story state's fog, if the engine ever keys `{7, id}` | native VERIFIED; when the original keys area 7 UNREAD | R (fog), workstream 7 |
| 6 | **Lens Simulation** (Enable 1 in 55 rows, constant values: Lens F 5.6, film 0.024, shutter 60) | never read | none expected: the original reads it only in FeedYebisLibraryPerFrame (1400FB890) for two GX lens filters (`pipeline+0x3A40`, `+0x3BF0`) gated by `+0x3EC1`/`+0x3EC2`, which 140085540 sets from view-policy bits 6 and 36; no GX lens draw in the original's captured post chains (I-0138 STUDY §1) | INFERRED inactive; the policy bits of the main view not read | R, workstream 11 (an ER-style DoF could start from it, STUDY §5.4) |
| 7 | **LightScattering** (90 files, 149 value ids) | never read | none found: the bank (`+0x30`, binder 14003BC70) is read through 14003D710 only by 14003C900, the cutscene act's insert (IngameDrawParam, 140CC8180); no renderer reader found | INFERRED dead data (direct readers of `[GXRS+0x41F0]` not scanned: needs headless Ghidra) | R, workstream 12 if it is alive |
| 8 | **Dominant Light** at unset keys | the bank's built-in record (197 rows in 35 runs, the title and loading frames); map keys read the zone files | raster lightmap rows only; nothing the rays use | OBSERVED | none |
| 9 | Inert in the shipped data | SeaWave (`Enable Draw Wave` 0 everywhere), ColorParaffin Filter (`Enable` 0 everywhere), WaterInteraction Filter (no MTD selects an Interaction family, STUDY-WATER-INTERACTION), AOFields and AOFields Model (`mapbase` only, no reader found), Cloud Shadow (registered, no group in any file), « Other PostEffects » and « Ls » (`s_template` only), MapOverride_0001/0003 (never selected: K-0448), MapOverride_0000/0002 (empty) | none | data OBSERVED | none |
| 10 | `Dominant RGB Scale` (one file, `m36_00_0002`, where every other file says `Dominant Color Scale`) | `DominantAt` reads the eleven names: that file's group fails and the built-in record serves | `m36_00`, which no witness visits | data OBSERVED, effect INFERRED | none (note only) |
| 11 | **remobase.gparam** (kind 10, resident, weight 0) and **MapOverride_forRemo** (kind 9) | never read; `Banks` has no selection for them (GLOW-WAVE1-READY open point) | if the original's `{1, 0}` resolves to remobase in cutscenes: Glow 2 / 2 against mapbase's 1 / 1.5, MotionBlur samples 5, Dof ApertuerF 1 | remobase's registration area UNREAD; no MapOverride_forRemo file in the 168 dumped (the name is in the executable, 14294F198) | R, workstream 11, with item 2 |

Not a gap any more (checked): the cutscene draw-parameter files themselves (I-0138 part 4b, K-0492: `sAA_BB_VVVV` read for every
group); the GPARAM files' discovery (the native's banks are the files the engine admitted by name, `18_resource_admission.inc`
808-830, so no name dictionary limits them); the positional group indices of `Banks::Sun`/`Fog` (0 LightSet, 1 Fog): the original
reads every group by a fixed registry index too (the 26 slots of `[GXRS+0x4788]`, I-0140 RESULT), so the files share one layout.

## 1. The original's 26 banks and the native reader of each

`BindGparamFileToBanks` (140049D00) hands each file to one binder per group; each binder takes its group's registry index from
`[[GXRS]+0x4788] + slot` (I-0140 RESULT, `regs.txt`). Bank X is `GXRS+0x41C0+X`.

| Bank (abs) | Binder | Slot | Group | Original readers (lookup → callers) | Native |
|---|---|---|---|---|---|
| `[GXRS+0x41E0]` | 140030D80 | +0x88 | LightSet | 140031A20 | `Banks::Sun` (26 params); the rays use the sun |
| 0x41E8 | 140037340 | +0x80 | Fog | 1400380A0, 1400387D0, 14014EB50 | `Banks::Fog` (11 words) |
| 0x41F0 | 14003BC70 | +0x84 | LightScattering | 14003D710 ← 14003C900 only | none (item 7) |
| 0x42A8 | 14003FEB0 | +0x8C | Env Map | 1400416A0 ← 19 sites (region IBL, model params) | `EnvColorScale`; cubes from the engine (`NativeEnvMapCube`); the rays dropped them (item 4) |
| 0x45F0 | 140155B90 | +0x6C | SeaWave | none found | none (inert) |
| 0x45F8 | 14012EA70 | +0x70 | Snow | 14012F400 | `SnowAt` |
| 0x4600 | 140045020 | +0x90 | Snow Model | 140046300 | `SnowModelAt` |
| 0x4608 | 14015A670 | +0x50 | SSAO | none found | none (no raster AO under the rays) |
| 0x4610 | 14015DFF0 | +0x30 | Setting Info | 15E920 / 15EC10 ← 140087FB0, 1400FB890, 14010A220 | `BackgroundColor`, `SettingBlock` (21 words) |
| 0x4618 | 140163030 | +0x34 | Tone Map | 1401638C0 ← FB890 | `GroupEntries` |
| 0x4620 | 1401674A0 | +0x3C | Bloom | 140167D30 ← FB890 | `GroupEntries` |
| 0x4628 | 14016A740 | +0x54 | LightShaft | 14016B030 ← FB890 | `GroupEntries` |
| 0x4630 | 14016E640 | +0x38 | Dof | 14016EED0 ← FB890 | `GroupEntries` |
| 0x4638 | 140171AA0 | +0x4C | Anti-Alias | 1401723C0 ← FB890 | `GroupEntries` (YEBIS AA; the TAA/DLSS draw) |
| 0x4640 | 140175360 | +0x48 | MotionBlur | 140175C60 ← 87FB0, FB890, 140102380 | `GroupEntries`, `GroupFloat(MaxBlurLength)` |
| 0x4648 | 1401783E0 | +0x44 | FeedBackBlur | 140178CF0 ← FB890, 140107800 | `GroupEntries` (Enable 0 everywhere) |
| 0x4650 | 14017B270 | +0x40 | ColorGrading | 14017BEE0 ← FB890 | `GroupEntries`; the LUT name for map records only (item 1) |
| 0x4658 | 14017E600 | +0x5C | Glow Filter | 14017EF20 ← FB890 | `GlowAt` |
| 0x4660 | 140181630 | +0x60 | ColorParaffin Filter | 140181ED0 ← FB890 (gate `+0x3EBB`) | none (inert) |
| 0x4668 | 140184D10 | +0x64 | Lens Simulation | 140185600 ← FB890 (gates `+0x3EC1`, `+0x3EC2`) | none (item 6) |
| 0x4670 | 14018BCF0 | +0x58 | ShadowParam | 14018C590 ← 140088A10 | `Banks::Shadow` (raster cascades; the traced sun does not read it) |
| 0x4678 | 14018EE30 | +0x74 | WaterInteraction Filter | 14018FA60 ← 140089960 (INFERRED) | none (inert) |
| 0x4680 | 140134150 | +0x78 | Dominant Light | 134A20 (WAVE1-LAST-EFFECTS) | `DominantAt` |
| 0x4688 | 140191D90 | +0x7C | AOFields | none found | none |
| 0x4690 | 140194C10 | +0x94 | AOFields Model | none found | none |
| 0x4698 | inline in 49D00 | +0x68 | Cloud Shadow | none found | none (no file carries it) |

« None found » means only that the subagent's GUI reads did not reach a reader; a headless scan of the readers of each bank's
`[GXRS+0x4xxx]` would close it (I-0067's `I0067FieldUsers`), not run here because a live run held the PC.

**A names-registry correction** (VERIFIED, `MOV EAX,[R14+0x80]` at 14003738C): `decompiled/GHIDRA-NAMES.tsv` calls 140037340
`BuildLightSetRecord_140037340`; it is the **Fog** binder (slot +0x80), and 14003BC70 the LightScattering binder (slot +0x84). The
light set is 140030D80's (slot +0x88). IMAGE-REGRESSION-STATIC-LEADS L2's « +0x28 37340 LightSet, +0x30 3BC70 Fog » is wrong the
same way. Not changed here (the registry has its writer, D-0269).

## 2. The journal sweep (62 runs since 2026-11-14 00:00)

From `native_yebis_sources`, `native_glow_inputs`, `native_dominant_source`, `native_snow_rows`, `native_light_set`,
`native_fog_set` (OBSERVED):

- Map keys: every YEBIS group reads the zone's own file (or its kind-7 layer, e.g. `m37_00_0100`, `m45_00_0300`) or `mapbase` for
  the groups the zones leave to it (Dof, MotionBlur, Anti-Alias, Glow, part of Setting Info), as the original's registration does.
  No map key fell to `default` except `FeedBackBlur` in 15 runs (the zones without the group; Enable 0 either way).
- Cutscene keys after part 4b (`i138-4b-*-nat`, `i140-e8`, `i140-e9`): every record from its s-file, or `default` for an id the
  s-file lacks (`Dof[241000000,1]`: `s41_00_0000` has id 0 only), as the original's `{1, 0}` fallback.
- Cutscene keys in the tour's runs: item 3 (the old build) and item 2 (mapbase).
- Glow keys stay map keys during cutscenes in every run (no cutscene area in `native_glow_inputs`).
- No key at a resident area (2 to 99) in any row: item 5 not met in these runs.

## 3. What would settle the open rows (cheapest first)

1. Item 1: read FUN_140095220's other branch (the LUT prefix of a non-map record), or capture the original in cutscene 31000010
   (`Run-I138Cutscenes.ps1`): the grading draw's t16 texture and its name. Then the native names the s-file's strip.
2. Items 2 and 11: read where kind 10 (`RemoBase`) registers (`FUN_140D27670`'s area for a resident holder) and capture the
   library's glare quality and motion-blur words in one cutscene whose s-file lacks Setting Info (33000030 or 37000020).
3. Item 3: rerun the tour's cutscene rows on a build that contains `d365e26f` before filing their defects.
4. Item 5: whether any engine key reaches area 7 (an eclipse-state save; none of the witness saves is known to be in it).
5. Items 6 and 7: the main view's policy bits 6 and 36 (one read of `pipeline+0x50` in the original), and a headless reader scan
   of `[GXRS+0x41F0]`.

## 4. Limits

- The original's side of items 2, 5 and 11 is static only; no capture of the original was made for this study.
- The subagent's group-to-slot pairs come from I-0140's headless output, re-read only for the Fog binder.
- The groups' parameters were compared by name across the 168 dumped files; a cutscene file the dump does not hold (e.g.
  `s35_00_0050`, `s35_00_0051`, which the tour's runs keyed) was not inspected.
- Workstream labels follow ROADMAP (R: 7, 11, 12; E: 5; the tour: 6); the owners decide.
