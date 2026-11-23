# I-0066 / I-0068 part BP: the other YEBIS passes of wave 1 (depth of field, motion blur, feedback blur, auto exposure and focus read-back, history), ready to implement

Request (coordinator, 2026-10-11): check whether the other YEBIS post-processing passes (depth of field, glare, auto exposure, after-image, motion blur) already have a ready sheet, and write the wave-1 sheet for those that do not; the DLSS sheet only if everything is covered. Static: nothing was launched, native code was read and not changed. Format of [YEBIS-WAVE1-READY](YEBIS-WAVE1-READY.md): this sheet is the full specification of the commit **YB-4b** of that sheet (its section 1.4 names the words and leaves the records open) plus the passes it never mentions (auto exposure, focus read-back, history).

Labels: **verified** = read in the decompilation of `DarkSoulsIII.exe` 1.15.2.0 (or the exported blends `evidence/I-0068/queries/yebis-ready/b1`, `b2`) or in the gparam dumps; **observed** = retained captures or the native tree; **note** = a linked note; **proposed**; **inferred**; **open**. Abbreviations: `GB` = `native/frame/game_bootstrap.cpp`, `LP` = `native/world/light_producers.cpp`, `FB890` = `FUN_1400FB890` (per-frame feed of the library), `E69A0` = `FUN_1400E69A0` (the driver that sets the library from the per-frame block).

Evidence (outside Git, `evidence/I-0068/queries/yebis-second/`): `second-blocks.tsv` (`Get-YebisSecondBlocks.ps1`): the DoF (34 words), MotionBlur (21 words) and Feedback blocks in the engine's order for the default file, the seven zone files, `mapbase` and `remobase` at id 0, the DoF flags and the AutoExposure / Feedback enables of the 18 gparam files.

## 0. Answer first

1. **Coverage (section 1): glare (bloom, star, ghost, anamorphic), light shafts, colour grading, tone map and Setting Info are ready; depth of field, motion blur (the YEBIS half), feedback blur, auto exposure and the history rules were not.** The DLSS sheet is therefore not the next one; this sheet closes them. The after-image of the glare is part of the Bloom record (`AfterimageLuminance`, `AfterimageLength`, already in the three-record sheet); what was missing is its history rule (2.6).
2. **Depth of field**: the 34-word record is the file order with two moves: **word 12 = the second `Enable` (a bool, file entry 13) and word 33 = `BackgroundMaskThreshold` (file entry 12)**; every other word k is file entry k (15 words carry a registered name in `converter-words.tsv` and agree: 1 to 7, 10, 11, 13 to 17 and 33; words 8 and 9 are named through the globals file; words 0, 12 and 18 to 32 are INFERRED from the pattern, section 2.2). Blend `14021E720`: words {0, 8, 9, 10, 11, 12, 13, 18, 24, 25} are copied from B, all the others lerped. The driver overrides three words when `context+0x413D` is set: **word 2** (`DofFocusDistance`) ← `context+0x4140`, word 12 and word 24 ← 0. This corrects YEBIS-WAVE1-READY 1.1 ("word 3"). Section 2.2.
3. **The record that a zone feeds is PROPOSED to be the `mapbase` one for both depth of field and motion blur; the bank built-in record and `default` are the other candidates**, because no zone file carries the `Dof` or `MotionBlur` group (all seven zone files were checked). The native bank (`Banks::GroupFloat`, `LP:1320`, a native design) takes an empty area group from `mapbase` at the same key id, else from `default` id 0. The original's rules (LIVE-REDUCTION-AM section 5, which warns against keying a read to `mapbase` by file name) are, as the exports of the lookups confirm: the exact registered `(area, id)` (MapBase registers with id 4, the map files with ids 5 to 9), else the entry `(1, 0)` that nothing registers, else **the bank's built-in record**. The candidates differ in a visible way (DoF `Enable` 0 / 1, aperture 5.6 / 2.0, far-field custom range, edge quality; blur time ratio 0.06 / 0.2, recurrences 1 / 2, fall-off 2..0 / 0..3); the built-in record is not dumped. Section 2.1, row YP1.
4. **Motion blur**: the YEBIS half is the first 14 words of a 21-word record (file order again permuted: `Enable`, mode (not serialised, 2), `BlurTimeRatio`, `MaxBlurLength`, `NumBaseSamples`, ...); the blend `1401D9910` leaves **word 12 (`MotionBlurScale`) unwritten in the in-between branch** (a stale word of the previous record in the original); `E4B50` writes fourteen globals and zeroes the stage unless the option byte and the factor allow it. The native velocity code already computes the one word it needs (`MaxBlurLength`, `GB:5136-5149`). Section 2.3.
5. **Feedback blur is off in every file that carries the group** (8 of the 18 gparam files and the 7 zone files: `Enable` 0): the sheet specifies the record and the globals but the builder only refuses a non-zero enable (row). Its order of words after `Enable` is not decodable from the exports (OPEN). Section 2.4.
6. **Auto exposure is not off everywhere: `MapOverride_0001` and `MapOverride_0003` set `AutoExposure` to 1** (13 other files that carry the Tone Map group, 6 gparam files and the 7 zone files: 0), which corrects the scope of YEBIS-CHAIN-CHECKLIST section 0 and the three-record sheet ("no shipped file sets it" was read on the default and zone files). The dumped `Ceremony` table (Data0, with no row for the DLC maps m45, m50, m51) has a non-zero MapOverride id only for `30001020` (600), so nothing in the dump selects those two overrides: what does is OPEN (YP8). The mechanism is fully read: the blend `140218BA0` swaps the file exposure for the **stored exposure** (record words 0x35 and 0x36) when a key has the flag; `FB890` stores `_DAT_144962008` into word 0x35 of both records before the blend; `E69A0` reads the library's adapted exposure back each frame (`FUN_1416CA160`) into that global; the focus distance of the auto-focus likewise (`FUN_1416CA2E0` -> `144962120`). Both are CPU values read from the library's state (INFERRED: the library computes them from a GPU read-back), so the HAL needs a read-back with a stated latency. Section 2.5, 4.3.
7. **The reset word is never written, but the driver has two other reset routes.** The word `post+0x227C` (bit 0: previous camera = current; bit 1: motion-blur mode 1) is cleared by the constructor and by `E69A0`'s read, and no other instruction writes it. But `E69A0` (lines 143 to 152) (a) calls `FUN_1416C92B0(S, 0)`, which **clears every entry of the history list** (`[S+0x108]+0x270[]`, the after-image pair), in every frame where the object at `[GX1+0xB80]` has byte `+0x3C` set or a count above 1 at `+0x30` (the same condition sets the local that blocks the exposure and focus read-back), and (b) calls `FUN_1416C9270(S, 0)` when the byte `[post parameter block + 0x4E0]` is set: it clears the same history, clears the `+0x1B0` targets and **resets the adaptation state**. The meaning of the two conditions is OPEN (YP11). Native needs a reset service that models both triggers (4.4); it is also the prerequisite of any temporal upscaler. Section 2.6.
8. **Design**: five commits YB-4b.1..YB-4b.5 under the existing `yebis.flag` family (they complete YB-4b together with the light-shaft half, FFX-WAVE1-READY FW-7), tests T2d..T2h with goldens, twelve live rows YP1..YP12 (LIVE-CHECKS-CONSOLIDATED 1.16), and the corrections of section 9. The DoF converter can run on a seeded scratch object, so YB-4b.1 does not wait for the library host (2.2).

## 1. Coverage audit

| Pass / stage | Library or engine entry | Where it is specified | Ready sheet? |
|---|---|---|---|
| Tone map, exposure, gamma, Setting Info | `E42A0`, `E4070` | YEBIS-WAVE1-READY 1.2, 1.3 (records, blends, goldens), GAMMA-COLOUR-PATH-READY | yes |
| Glare: bloom, star, ghost, anamorphic | `E4460` | YEBIS-WAVE1-READY 1.2 (42-word permutation), YEBIS-TECHNIQUES 5 (arithmetic) | yes (the tap-count rule of TECHNIQUES 8.3 is a numeric OPEN, not a missing sheet) |
| After-image of the glare | Bloom words `AfterimageLuminance`, `AfterimageLength` (engine words 40, 41); stage `_Afterimage` with a history pair | record: YEBIS-WAVE1-READY; draws: YEBIS-NATIVE 3, YEBIS-HAL-READY (ping-pong pair 17195 / 17196) | record yes; **history rule: this sheet (2.6)** |
| Light shafts | `E4700` | [FFX-WAVE1-READY](FFX-WAVE1-READY.md) 4.1 (28 words, blend, goldens) | yes |
| Colour grading | bank 4, `14021C150` | FFX-WAVE1-READY 4.2 | yes |
| **Depth of field** | `E4890`, blend `14021E720` | YEBIS-TECHNIQUES 8 (draws and arithmetic), YEBIS-CHAIN-CHECKLIST P7b; YEBIS-WAVE1-READY 1.4 names the words only | **no: this sheet (2.2)** |
| **Motion blur, YEBIS half** | `E4B50`, blend `1401D9910`, setters A0..A9 | [YEBIS-MOTIONBLUR-NATIVE](../I-0067-static-menus-subsystems-map/YEBIS-MOTIONBLUR-NATIVE.md) (words, calls, planner, draws, checklist), [VELOCITY-NATIVE](VELOCITY-NATIVE.md) (the velocity pass, implemented) | specification yes, **not in the ready form** (no builder, no goldens, no commits): this sheet (2.3) |
| **Feedback blur** | blend `140221640`, globals `144962178..188` | YEBIS-CHAIN-CHECKLIST 4.3 (values only) | **no: this sheet (2.4)** |
| **Auto exposure, auto focus read-back** | stored words 0x35 / 0x36 of the Tone Map record, `FUN_1416CA160`, `FUN_1416CA2E0` | "off in every file" (CHAIN-CHECKLIST 0, YEBIS-WAVE1-READY 9) | **no, and the statement is wrong: this sheet (2.5)** |
| Anti-alias flag | blend `140220B50` | YEBIS-WAVE1-READY 1.1 (the flag), DLAA is a GX filter | yes (rule restated in 2.7) |
| Glow filter, SSAO | `2227C0`, SSAO layer | [GLOW-WAVE1-READY](GLOW-WAVE1-READY.md), [SSAO-WAVE1-READY](SSAO-WAVE1-READY.md) | yes |

## 2. The original (verified unless a label says otherwise)

### 2.1 Which record a zone feeds

`FB890` looks up two records per group (keys A and B, `context+0x3FD8 + 8i` and `+0x4040 + 8i`) with the bank lookup (`FUN_14016EED0` for `Dof`, `FUN_140175C60` for `MotionBlur`, `FUN_140178CF0` for `FeedBackBlur`): key `(-1, -1)` becomes `(1, 0)`, then the exact `(area, id)`, then the fallback entry `(1, 0)`, then the bank's built-in record (LIVE-REDUCTION-AM 5; the exports show the two hashed probes with the constants `0x5EADBEEE`/`0x5EADBEEF`). Of the 18 gparam files only `default`, `mapbase`, `remobase` and `m_template` carry a `Dof` group, and the seven zone files (m30, m31, m37, m40, m45, m50, m51) carry `FeedBackBlur` but **neither `Dof` nor `MotionBlur`** (`second-blocks.tsv`). The native bank (`Banks::GroupFloat`, `LP:1320-1331`) is a native design, not a transcription: an area file with an empty group takes the `mapbase` group at the same key id (second chance `default` id 0), the way `ShadowParam` is read. The original's rules (above) end, for a key that matches nothing, at **the bank's built-in record** (`bank+0x220` in the exports of the two lookups), which is not dumped, and LIVE-REDUCTION-AM warns against keying a read to `mapbase` by file name. This sheet therefore PROPOSES `mapbase` (the record the Glow chain was observed to find, LIVE-CHECKS-CONSOLIDATED MAPBASE) as the working choice, lists `default` and the built-in record as candidates, and leaves the key id to the live rows (MAPBASE, YP1).

| Word | Name | `default` | `mapbase` | `remobase` |
|---|---|---|---|---|
| Dof 0 | Enable | 0 | **1** | 1 |
| Dof 1 | ApertuerF | 5.6 (`40B33333`) | **2.0** | 1.0 |
| Dof 11 | DofEdgeQuarity | -1 | 2 | 2 |
| Dof 12 | Enable (bool) | 0 | **1** | 1 |
| Dof 15, 16, 17 | Near end, CocSize, CocScale | 2, 1, 1 | 3, 0.2, 0.5 | same as mapbase |
| Dof 18..23 | Far custom enable, start, end, CocSize, CocScale, threshold | 0, 100, 150, 1, 1, 10000 | **1, 50, 10000, 0.4, 0.015, 20000** | same as mapbase |
| MotionBlur 2 | BlurTimeRatio | 0.06 | **0.2** | 0.15 |
| MotionBlur 4 | NumBaseSamples | 4 | 4 | 5 |
| MotionBlur 5 | MaxRecurrences | 1 | **2** | 2 |
| MotionBlur 10, 11 | FalloffDistanceMin, Max | 2, 0 | **0, 3** | 2, 3 |

Every other word of the three blocks is equal in the three files (the DoF auto-focus `Enable` and the near custom enable are 0 in all, the MotionBlur `Enable` is 1 in all, `MaxBlurLength` is 0.1 in all).

### 2.2 Depth of field

**Record** (`FB890` lines 345 to 391: a local 0x88-byte buffer, 34 words, filled by the blend; its defaults are overwritten by the blend, which covers every word). Engine word -> file entry, file entries from the dumps, with the registered names of `converter-words.tsv` where they exist (words 1 to 7, 10, 11, 13 to 17 and 33; 8 and 9 through the globals file); words 0, 12 and 18 to 32 are INFERRED from their neighbours (the near-field names fall on entries 14 to 18, the factor is applied at the two CocScales, word 24 gates the auto-focus global, word 12 is zeroed by `DAT_144961BF4`):

| Engine words | File entries | Names |
|---|---|---|
| 0 | 0 | `Enable` (int) |
| 1 to 11 | 1 to 11 | ApertuerF, DofFocusDistance, CCD-Size, DofAdaptiveApertureFactor, DofAdaptiveApertureBaseFov, DofRotateScale, DofRotateOffsetInRadians, ApertureForntNumLevels, ApertureBackNumLevels, DiaphragmRotate, DofEdgeQuarity |
| **12** | **13** | the second `Enable` (type 7, an int-like flag) |
| 13 to 17 | 14 to 18 | Enable-Near-Custom, DofNearDepthStart, DofNearDepthEnd, DofNear CocSize, DofNear CocScale |
| 18 to 23 | 19 to 24 | Enable-Far-Custom, DofFarDepthStart, DofFarDepthEnd, DofFar CocSize, DofFar CocScale, DofFar DistanceThrethold |
| 24 to 32 | 25 to 33 | the third `Enable` (auto focus), DonotPassOverFocus, FocusSensitivity, FocusSpeedLimit, FocusDelay, FocusDistanceRangeMin, FocusDistanceRangeMax, FocusScreenPositionX, FocusScreenPositionY |
| **33** | **12** | BackgroundMaskThreshold |

**Blend** (`14021E720`, verified by byte offsets): snap to A when `|w| <= eps`, to B when `|w - 1| <= eps` (`eps` = `[143D7ECE8]`). In between: words **{0, 8, 9, 10, 11, 12, 13, 18, 24, 25}** (the enables, the two level counts, `DiaphragmRotate`, `DofEdgeQuarity`) are copied from B and the 24 others are lerped `(b - a)·w + a`. Every one of the 34 words is covered.

**Driver overrides** (`FB890:383-387`, when `context+0x413D != 0`): record word 12 = 0, word 24 = 0 and **word 2 ← `context+0x4140`** (the earlier note said word 3). The producer of `context+0x413D/+0x4140` (a scripted focus distance) is OPEN (YP3); native sets the flag clear.

**Converter** (`E4890(post, record)`, exported `b2/1400e4890.c`), globals `1449620F8..14496226C`:

| Record word | Global | Notes |
|---|---|---|
| 24 | `1449620F8` | only when `post+0x2272 != 0`; auto-focus enable |
| 25..32 | `1449620FC .. 144962118` | DonotPassOverFocus, FocusSensitivity, SpeedLimit, Delay, RangeMin, RangeMax, ScreenPositionX, Y |
| 0 | `14496211C` | Enable |
| 2 | `144962120` | DofFocusDistance, only when `1449620F8 == 0` and `post+0x2271 != 0`; otherwise the auto-focus read-back writes it (2.5) |
| 1, 3, 4, 5, 6, 7, 10, 11 | `144962124`, `144962130`, `144962128`, `14496212C`, `144962134`, `144962138`, `14496213C`, `1449621B0` | ApertuerF, CCD-Size, adaptive factor, base fov, rotate scale, rotate offset, DiaphragmRotate, DofEdgeQuarity |
| 8, 9 | `144962140`, `144962144` | `min(cap, (int)((float)(int)word × f))` with `cap = [[GX2+0x5360]+0x18]` (front) or `+0x1C` (back), `f = [[GX2+0x5368]+0xC8]` (0 when the quality byte `+0x10` is 0, 1.0 when the objects are absent, caps then 7) |
| 12 | `144962240` | zero when `DAT_144961BF4 != 0` |
| 13 | `144962244` | Enable-Near-Custom |
| 18 | `144962248` | Enable-Far-Custom |
| 14, 15, 16 | `14496224C`, `144962250`, `144962254` | near start, end, CocSize |
| 17 | `144962258` | `f × CocScale` |
| 19, 20, 21 | `14496225C`, `144962260`, `144962264` | far start, end, CocSize |
| 22 | `144962268` | `f × CocScale` |
| 23 | `14496226C` | far distance threshold |
| 33 | `144962148` | BackgroundMaskThreshold |

`GX2` is the object at `144961B50`. The quality rows (MENUS-MAP 77): DISABLE 0 0 0 0, LOW 1 0 1 1, MEDIUM 1 2 3 3, HIGH 1 3 5 5, MAX 1 5 7 7 for `+0x10, +0x14, +0x18, +0x1C`. With `mapbase` (levels 3 / 3) and factor 1.0 the globals `144962140/144` are 3 and 3 at MEDIUM and above (1 at LOW); Thomas's setting is MAX. `E4890` runs only when `post+0x2270 == 0` and reads only `post+0x2271` and `post+0x2272` of the post object (the exports). The constructor `FUN_1400E2510` sets them to 1 and 1 (a word `0x100` at `+0x2270`, a byte 1 at `+0x2272`) unless the setters `FUN_140017E60/17EA0` change them, so a scratch object seeded with `+0x2270 = 0, +0x2271 = 1, +0x2272 = 1` is faithful and the converter can run before the library host exists (this corrects YEBIS-WAVE1-READY 4.4, which sends it to the real object).

**Draws** (note, YEBIS-TECHNIQUES 8, YEBIS-CHAIN-CHECKLIST P7b): the block of 10 draws (1 + 2 + 6 + 1) runs at every zone whatever the enable; its sizes and pass counts follow the globals above, so the record must be right even where the effect looks neutral. The depth input is the readable depth (YB20).

### 2.3 Motion blur, YEBIS half

**Record** (21 words; the I-0067 note gives the YEBIS half and its defaults; file entries from the dumps): w0 `Enable` (file 0), w1 mode (**not in the file**, constructor default 2), w2 `BlurTimeRatio` (1), w3 `MaxBlurLength` (3), w4 `NumBaseSamples` (2), w5 `MaxRecurrences` (5), w6 `SampleInterleaved` (6), w7 `CameraRotationPerspectiveThreshold` (7), w8 `CameraTranslationThreshold` (8), w9 (not in the file, 0), w10 `FalloffDistanceMin` (9), w11 `FalloffDistanceMax` (10), w12 `MotionBlurScale` (4), w13 `Bidirectional PermissionEnable` (11). Words 14 to 20 are the GX half (file 12 to 20: the enable byte, Motion / Object / Camera Blur Intensity, ReccurenceCount, the three flag bytes `+0x4C..0x4E` of word 19, Base Fps), read by the velocity pass (VELOCITY-NATIVE).

**Blend** (`1401D9910`, verified): at the snaps all 21 words come from A or B (the first two words are always taken from B in the in-between branch). In between: copied from B words 0, 1, 4, 5, 9, 13, the byte of word 14, word 18, the bytes of word 19; lerped words 2, 3, 6, 7, 8, 10, 11, 15, 16, 17, 20; **word 12 is not written** (it keeps whatever the caller's buffer held, in `FB890` the previous record of that buffer, the DoF record: a transition-only quirk; native writes the lerp, differing from the original only while a key transition lasts).

**Converter** `E4B50` (verified): `option = [[GX2+0x5360]+0x20]` byte, `cap = [...+0x24]`, `factor = [[GX2+0x5368]+0xE0]`, `count scale = [...+0xF8]`; `Enable` global `14496214C` = word 0 only when the option byte is set and `factor > [143D7ED28]`; `144962150` = w1; `144962154` = w2; `144962158` = `max(w3, 0.001)`; `1449622C0` = `factor × w12`; `14496215C` = w4; `144962160` = the recurrence count `max(1, min(cap, (int)((float)w5 × count scale)))` (the decompile shows a pointer alias); `144962164, 168, 16C, 170` = w6..w9; `14496221C`, `144962220` = w10, w11; `DAT_144962174` = (w13 != 0). The option rows (MENUS-MAP 77): MotionBlur `+0x20, +0x21, +0x24` = 0 0 0 / 1 0 1 / 1 1 1 / 1 1 5 / 1 1 8 for DISABLE..MAX. Thomas's setting is DISABLE, so the stage is off in his configuration (the +2 draws appear only with the option on, YEBIS-CHAIN-CHECKLIST P8).

**Calls** into the library (`E69A0` lines 584 to 666): A0..A9 of YEBIS-MOTIONBLUR-NATIVE 1.2, unchanged; `A7a` needs the velocity object of the native velocity pass (`pipeline+0x3EB9`).

### 2.4 Feedback blur

Bank 5, lookup `178CF0`, blend `140221640` used only when the weight `context+0x40BC` is above zero (otherwise record A is used as it is, `FB890:421-447`): word 0 is taken from B, words 1 to 7 are copied from B at weight >= 1 and lerped otherwise (floats). The record has 8 words (0 to 7), the file 7 entries in this order: `Enable`, FeedbackWeight, FeedbackUnitTimeInSeconds, FeedbackAspectRatio, FeedbackRate, FeedbackBlurFallOff, FeedbackBlurPower. Which record word holds which float is **not decodable from the exports** (the driver defaults word 2 to 0.5; the constructor and serializer were not read): the file order into words 0..6 is an assumption. Globals: `144962178` = word 0, `14496217C` = word 1, `144962180` = word 2, `144962184` = word 3, `144962188` = word 4 (a float lerped by the blend; Ghidra types the local as int), and `14496218C` keeps the previous `144962178`; they are read by `E69A0` (`1400E8014`). **`Enable` is 0 in every file that carries the group**, so the sheet specifies only the guard: the builder reads the file, and when `Enable != 0` it refuses and logs (row), leaving the globals at zero.

### 2.5 Auto exposure and auto focus

- **Where it is on**: `AutoExposure` (Tone Map file entry 9, engine word 9) is 1 in `MapOverride_0001` and `MapOverride_0003` (with `AutoExposure Speed`, `Middle Gray` -2, `Center Weight` 4); it is 0 in the six other gparam files that carry a Tone Map group (`m_template`, `default` and the four m40 files) and in the seven zone files; `mapbase`, `remobase` and `MapOverride_0000` / `0002` carry no Tone Map group. The second flag, `Enable AutoExposure` (GX half, word 0x30), is 0 in the 14 files that carry it. The dumped `Ceremony` table selects an override only for `30001020` (id 600) and has no row for the DLC maps (OPEN, YP8).
- **The stored-exposure rule** (blend `140218BA0` lines 195 to 250, snaps at lines 60 to 80 and 130 to 145): at the snaps the whole record is copied, then if that record's word 9 is positive, **out word 0 = its stored word 0x35**, and if its word 0x30 is positive out word 0x23 = its word 0x36. In between: for word 0 with flags `a = A.w9`, `b = B.w9` (as ints): both positive -> `lerp(A[0x35], B[0x35])`; `a > 0, b <= 0` -> `lerp(A[0x35], B[0])`; `a <= 0, b > 0` -> `lerp(A[0], B[0x35])`; both <= 0 -> the generic lerp. Word 0x23 follows the same table with the flag at word 0x30 and the stored word 0x36 (the GX half: GX tone-map filter).
- **The feed**: `FB890` writes `_DAT_144962008` into word 0x35 (bank entry offset `+0xDC`, record word 0x35) of **both** looked-up records before the blend (`FB890:334-335`); `E69A0` (line 288), when the post parameter word 0x14 (AutoExposure) is set and the local `cStack_2b8` is clear (it is set when the object at `[GX1+0xB80]` has its byte `+0x3C` set or a count above 1 at `+0x30`, lines 148-152), writes `_DAT_144962008 = FUN_1416CA160(S, 0)`, the library's current exposure (`[S+0x1F0] + slot × 0x68 + 0x54`), right after `FUN_1416CA110` (the enable) and **before** the setters for the min / max exposure, metering and speed words (`FUN_1416CA1D0/1B0/230/1F0/210`, lines 289 to 300), so the value read is the one of the previous frame.
- **Auto focus**: the same driver (line 551), when the DoF auto-focus word is set, calls the setters `FUN_1416CA290/370/350/330/390` and writes `_DAT_144962120 = FUN_1416CA2E0(S, -1)` (`[S+0x1F8] + slot × 0x48 + 0x34`), the focus distance the library measured. Both getters are plain CPU reads of library state; (INFERRED) the library fills it from a GPU read-back (adaptation target 640 x 540 R11G11B10F for the exposure, YEBIS-CHAIN-CHECKLIST 2).
- **Consequence**: with a witness-free flag in two override files only, the first implementation **refuses** auto exposure and auto focus with a row (as the earlier sheet did) but the refusal is now keyed on the real condition: a record whose word 9, word 0x30 or auto-focus word is positive, anywhere in the blend.

### 2.6 The after-image and the history

- **After-image record**: Bloom engine words 40 (`AfterimageLuminance`) and 41 (`AfterimageLength`), part of the 42-word block of the three-record sheet. In the library the stage `_Afterimage` (`FUN_141750000`, techniques `MadOffsetTn`) blends the current glare with a ping-pong pair kept across frames (targets 17195 / 17196, the history `+0x270[]` with its count `+0xF0`, each entry initialised with weight 1.0: YEBIS-NATIVE 3, YEBIS-HAL-READY).
- **What resets the histories in the original**: the explicit reset word `post+0x227C` is written by exactly two instructions, the constructor (0) and the read-and-clear in `E69A0` (lines 153-154); no other instruction in the executable refers to it (instruction search over the whole database), so bit 0 (previous camera = current, `E69A0:165`) and bit 1 (motion-blur mode 1) are never raised. **Two other routes exist in the same driver** (lines 143 to 152): `FUN_1416C92B0(S, 0)`, called in every frame where `[GX1+0xB80]` has byte `+0x3C` set or a count above 1 at `+0x30`, walks the history list `[S+0x108]+0x270[slot]` (count `+0xF0`) and clears each target through `FUN_141705F50` (device clear, null colour: zero); `FUN_1416C9270(S, 0)`, called when `[post parameter block + 0x4E0]` is set, clears the same history (`141752960`), the `+0x1B0` targets (`1416F26C0`) and **resets the adaptation state** (`1416F9E10`: 1.0 at `+0x190` and `+0x198`, `[S+0x698]` at `+0x1A0`, zero at `+0x1A8`). What `[GX1+0xB80]` (an object with a mode byte and a count, INFERRED: a multi-view or stereo output state) and the parameter byte `+0x4E0` mean is OPEN (YP11); nothing in the witnesses of this sheet sets them.
- **Consequence**: native has to provide the reset itself (4.4). Until YP11 reads the two conditions, the default is: no reset except at creation and at an extent change, plus the velocity pass's own camera-cut rule (`CollectNativeVelocityInputs`, `GB:5141-5148`: a jump of more than 10 units or a forward-vector dot below 0.5), with both original routes modelled as named reasons that the native never raises today.

### 2.7 The anti-alias flag

`140220B50` / `FB890:390-417`: `144962190 = 1` when record word 0 is non-zero and word 1 is zero, forced to 0 when the quality byte `[[GX2+0x5360]+8]` is 0 or the factor `[[GX2+0x5368]+0x98]` is not positive; the library AA is off in every shipped file (`AntiAliasType` 4 selects the GX DLAA filter). Unchanged from the three-record sheet.

## 3. Native today

| Item | Native | Anchor |
|---|---|---|
| Library, post object, converters | none: `E2B20`, `E5110`, `E69A0`, `FB890` are not run (YEBIS-WAVE1-READY 3) | - |
| Banks | `Banks::GroupFloat` (floats only; area -> `mapbase` -> `default` id 0) | `LP:1320` |
| MotionBlur words | `MaxBlurLength` only, blended with the snap rule of `1401D9910` from the context keys `+0x4008/+0x4070` and weight `+0x40C0`, for the velocity view scale | `GB:5111-5149` |
| Quality object | the motion-blur option row (`NativeMotionBlurQualityRow`) and its factor `+0xE0` are read for the velocity pass (`GB:2142`, `GB:5113`); the four DoF dwords are copied by `native/frame/game_quality_object.cpp:14`; a `CustomDof` producer exists in `LP:937` | `GB:5104`, `GB:2142`, `LP:937` |
| History | the velocity history (`native_velocity_history`) with its own cut rule | `GB:5141` |
| Read-back | none for the library (the HAL is not started) | - |

## 4. Design

### 4.1 The typed blocks (YB-4b.1, YB-4b.2, YB-4b.3)

New block builders beside the three of YEBIS-WAVE1-READY 4.2, in the same file and with the same `Banks::GroupEntries` accessor:

- `BuildDof34(entries) -> std::array<uint32_t, 34>` with the permutation of 2.2 (the 34 entries must be present, else refuse), `BlendDof34(A, B, w, eps)` with the copy set {0, 8, 9, 10, 11, 12, 13, 18, 24, 25}, then the driver overrides (words 2, 12, 24 from the context scripted-focus fields; native: flag clear).
- `BuildMotionBlur21(entries)` with the permutation and the two non-serialised words (w1 = 2, w9 = 0) and the packed bytes of word 19; `BlendMotionBlur21` with the copy / lerp sets of 2.3 and **word 12 lerped** (documented difference).
- `BuildFeedback8(entries)`: the seven file entries in file order into words 0..6 (the order beyond `Enable` is an assumption, 2.4) behind the guard (`Enable` != 0 refuses, row `native_yebis_refused {group:"FeedBackBlur"}`).
- `BlendToneAuto`: the stored-exposure rule of 2.5 on the existing Tone Map blend, active only when a flag is positive (never today); until implemented the builder refuses with the same row.

### 4.2 The converter calls (YB-4b.1 to YB-4b.3)

`YebisConverters::Apply` gains `E4890(post, dof)` (on the scratch object seeded as 2.2 says until the real post object exists; the real one afterwards), `E4B50(motion)` (no object access: it writes the globals and reads only the quality and factor objects) and the feedback globals written by a native transcription of `FB890:421-447` (no engine function exists for them). Conditions as in `FB890`: `post+0x2270 == 0`. The three rows `native_yebis_dof`, `native_yebis_motion`, `native_yebis_feedback` hold `{keys, weight, block digest, globals digest, option / factor / quality inputs}` on the first three frames, on change and every 600th.

### 4.3 Read-back protocol for exposure and focus (YB-4b.4)

Both values are produced by the library from a GPU result, so the HAL must offer a read-back with a declared latency: a staging copy of the 1x1 (or small) result at the end of the frame, mapped `N` frames later (PROPOSED `N = 2`, the swap-chain depth; the original's D3D11 path maps the staging resource with whatever latency the driver imposes and the library tolerates one frame of staleness by construction). Until the library is hosted (YB-7) the commit implements only the **guard**: both features refused with a row, the exposure global left at the tone-map value, the focus distance at the record word 2. The read-back is a design obligation of YB-8, listed here so that the HAL's resource list is complete (a `Readback` lifetime class in the phase E sheet).

### 4.4 The history service (YB-4b.5)

`YebisHistory { generation; reason; }`: one object that the after-image pair, the feedback buffer (when ever enabled), the adaptation target, the velocity history and, later, a temporal upscaler register with. `Reset(reason)` is called by the native on: first frame, load / teleport (the `reset` of the velocity cut rule), render-extent change, device recreation, and the two original routes of 2.6 as named reasons (`GxOutputState`, `ParameterReset`) that the native does not raise until YP11 gives their meaning. The default policy is the one of 2.6; the velocity cut rule stays as it is and is exposed as a reason. The service ships inert: it counts and logs (`native_yebis_history {reason, generation}`).

## 5. Commits (each builds, passes the unit tests, is reversible alone; all behind the opt-in `yebis.flag` family of YEBIS-WAVE1-READY 5)

| # | Commit | Behaviour change | Flag | Test |
|---|---|---|---|---|
| YB-4b.1 | DoF block, blend, overrides, `E4890` call on the seeded scratch object (2.2), `native_yebis_dof` | none (the globals are read by nothing yet) | `yebis.flag` | T2d |
| YB-4b.2 | MotionBlur block, blend, `E4B50` call, `native_yebis_motion`; the velocity code takes `MaxBlurLength` from the block (one source) | none | `yebis.flag` | T2e |
| YB-4b.3 | Feedback block with the refusal guard; Anti-Alias flag rule | none | `yebis.flag` | T2f |
| YB-4b.4 | auto exposure and auto focus guards (the real condition of 2.5), the stored-exposure blend written but inactive, the read-back resource declared | none on the shipped zones; with a positive flag (MapOverride_0001/_0003) the feed now runs with the file's exposure where it used to stop (implementation log: approximated, flagged in both rows) | `yebis.flag` | T2g |
| YB-4b.5 | history service (inert) | none | `yebis.flag` | T2h |

Order: YB-4b.1, YB-4b.2, YB-4b.3 in any order after YB-4; YB-4b.4 after YB-4b.1 (its guard reads DoF word 24, the auto-focus enable); YB-4b.5 anywhere. Together with the light-shaft half (FFX-WAVE1-READY FW-7, test T2b) they complete YB-4b, which YB-7 requires.

## 6. Tests and goldens

- **Goldens** (`second-blocks.tsv`): the DoF34, MotionBlur21 and Feedback blocks of `default`, `mapbase`, `remobase` and the seven zones (id 0); the DoF flags and the AutoExposure / Feedback enables of the gparam files. **The seven zone rows of Dof and MotionBlur are copies of the `mapbase` rows by construction** (the script's fallback), so they pin nothing about the open choice of 2.1; the choice is YP1.

| ID | What | Pass |
|---|---|---|
| T2d | `BuildDof34` against the TSV for `default`, `mapbase`, `remobase`; the 15 registered names agree; `BlendDof34` at weights 0, 1 and 0.5 on `default` / `mapbase`: copy set and lerp set; the three overrides; `E4890` globals on the seeded scratch object for both records at quality MEDIUM and MAX | bit-exact; globals equal the table of 2.2 |
| T2e | `BuildMotionBlur21`, the two defaults, the packed bytes (`0x00000101` at the shipped values), `BlendMotionBlur21` (the documented word 12 difference); `E4B50` globals from a zero object with option / factor / cap inputs | bit-exact; globals equal the table of 2.3 |
| T2f | `BuildFeedback8` against the TSV (file order, flagged as an assumption); refusal when `Enable != 0` | bit-exact; row produced |
| T2g | the stored-exposure blend table (2.5) on synthetic records, all nine flag combinations for both words; the refusal condition | table equal; refusal only when a flag is positive |
| T2h | the history service: registration, reset reasons, generation counter | as specified |

## 7. Live rows (none taken; LIVE-CHECKS-CONSOLIDATED 1.16)

| ID | Kind | Read | Expected |
|---|---|---|---|
| YP1 | key | the keys A / B (`context+0x3FE8, +0x4050, +0x4008, +0x4070, +0x4000, +0x4068`) and weights at the five zones, and the record they resolve to (compare the DoF globals `14496211C`, `144962124` and the MotionBlur globals `14496214C..144962158` with 2.1) | `mapbase` values (DoF Enable 1, ApertuerF 2.0; blur time ratio 0.2) |
| YP2 | globals | the full DoF globals block `1449620F8..14496226C` after `FB890`, at the five zones, with the quality levels MEDIUM and MAX | the table of 2.2 with `mapbase` |
| YP3 | inputs | `context+0x413D/+0x4140`, `DAT_144961BF4`, `post+0x2271/+0x2272`, `[GX2+0x5368]+0xC8` and the DoF quality row `+0x10..0x1C` at Thomas's setting | flag clear, levels 7 / 7 at MAX, factor value |
| YP4 | draws | the DoF 10-draw block at m31 and m37 with the record Enable 1 and (forced) 0 | draw count unchanged (10), sizes follow the levels |
| YP5 | globals | the MotionBlur globals with the option ON at m37 (HIGH): `14496214C = 1`, `144962160`, `1449622C0` | 1, recurrences 2 capped by 5, factor |
| YP6 | transition | the MotionBlur record during a key transition: word 12 (stale or lerped) | stale = the DoF word of the same buffer |
| YP7 | feedback | the feedback globals `144962178..18C` at a zone | `Enable` 0 and the file values in some order (Rate 0.8, FallOff 0.9, Power 4.5): the order settles the layout |
| YP8 | exposure | what selects `MapOverride_0001` / `0003` at all (the loader of the override id, the DLC Ceremony rows) and the exposure global over 120 frames in one | the map and the sequence of `144962008` |
| YP9 | read-back | the library's exposure at `[S+0x1F0] + slot × 0x68 + 0x54` against the frame number after a light change | the latency in frames |
| YP10 | focus | `144962120` with the auto focus enabled (a DoF record with the third `Enable` set) | the library's measurement |
| YP11 | history | the two conditions of 2.6 (`[GX1+0xB80]` byte `+0x3C` and count `+0x30`; the parameter byte `+0x4E0`) at the five zones, over a load and a teleport; `post+0x227C`; the after-image pair content after both | both conditions clear in the world view (INFERRED), the histories survive a teleport |
| YP12 | draws | the draw list with the after-image on at m40 (bloom + after-image composite) over a camera cut | the YEBIS-HAL-READY list |

## 8. Risks and open points

1. **The record key of the zones** (DoF and MotionBlur from `mapbase`, the bank built-in record or `default`): the image differs (aperture, far-field range, blur ratio); YP1 decides, and the native lookup follows the engine's key once the registry exists.
2. **`context+0x413D/+0x4140`** (a scripted focus distance) has no known producer; native keeps it off.
3. **Feedback blur** has never been on; its word order beyond `Enable` is open.
4. **Auto exposure** is reachable only through two override files whose users are unknown; the read-back and the library's adaptation passes are not specified here (guard only).
5. **The stale word 12** of the MotionBlur blend is replicated as a lerp: a difference at key transitions only.
6. **The history routes** of 2.6 are read, their conditions are not: native raises neither until YP11. The statement that the reset word is never written rests on an instruction search over the whole database; a write through a computed address would not appear.

## 9. Corrections to earlier notes

| Note | Statement | Correction |
|---|---|---|
| YEBIS-WAVE1-READY 1.1 (DoF row) | "word 3 <- `context+0x4140` and word 24 = 0" (and "`out+0x30 = 0`") | word **2** <- `context+0x4140`, word 12 (`out+0x30`) = 0, word 24 = 0 (`FB890:383-387`; the `uStack_c70` low dword is record word 2); and `E4890` does not need the real post object (2.2) |
| YEBIS-CHAIN-CHECKLIST 0 (4), 2, 4.1; YEBIS-WAVE1-READY 1.2, 9 (2) | AutoExposure off in every file read / no shipped file sets it | a scope gap: it holds for the default, mapbase-less zone files and the m40 files that were read; `MapOverride_0001` and `MapOverride_0003` set `AutoExposure` = 1 |
| YEBIS-TECHNIQUES 8.3 (zone table) | DoF quality and aperture levels "`*` = default used" | the zone files carry no `Dof` group: `mapbase`, the built-in record or `default` per YP1; the table's `3*/3*` levels are the same in `default` and `mapbase` |
| YEBIS-MOTIONBLUR-NATIVE 1.1 | "(w12) not blended in the interpolation branch" | confirmed; the effect is a stale word, not a missing blend (2.3) |

## 10. Review log

Independent reader: an Opus 5.5 subagent that re-derived the claims from the exports, the DXBC-free disassembly, the dumps and the native tree (it edited nothing). 12 findings; the author re-checked the blocking one against the exports (`FUN_1416C92B0`, `FUN_1416C9270`) before applying them.

| # | Severity | Finding | Disposition |
|---|---|---|---|
| 1 | blocking | "The engine never raises a history reset" is false: `E69A0` also calls `FUN_1416C92B0` (clears the history list, every frame the `[GX1+0xB80]` condition holds) and `FUN_1416C9270` (history, `+0x1B0` targets and the adaptation state, when the parameter byte `+0x4E0` is set). Only the reset word itself is never written | applied: 0.7, 2.6, 4.4, 8.6, YP11 |
| 2 | important | YB-4b.1 was circular (E4890 on the "real" post object, which exists only with the host). `E4890` reads only `post+0x2271/0x2272`, which the constructor sets to 1; a seeded scratch object is faithful (corrected by I-0141 tranche 9: the game camera writes `+0x2271` through `FUN_140017ea0`, in play and in cutscenes). YB-4b also has the light-shaft half | applied: 2.2, 4.2, 5, 0.8; correction row for YEBIS-WAVE1-READY 4.4 |
| 3 | important | The `mapbase` choice was filed as verified but rests on a native design; the original ends at the bank built-in record; the zone goldens copy `mapbase` by construction | applied: 0.3, 2.1 (relabelled PROPOSED, candidates listed), 6, YP1, risk 1 |
| 4 | important | "Checked against the 33 registered names" is false (15 words carry a name) | applied: 0.2, 2.2, T2d |
| 5 | important | Feedback blur: inconsistent layout (6 floats into 7 words), YP7 expectation, the "int" is a Ghidra local type | applied: 2.4, 4.1, T2f, YP7 |
| 6 | minor | File counts (Tone Map group in 8 gparam files; AutoExposure 0 in 13; Enable AutoExposure 14; FeedBackBlur in 8 + 7) | applied: 0.5, 0.6, 2.5 |
| 7 | minor | The dumped Ceremony table includes more rows than stated; the premise of YP8 is weak | applied: 0.6, YP8 (what selects the overrides at all) |
| 8 | minor | `FUN_1416CA160` is called before the other setters, not after | applied: 2.5 |
| 9 | minor | `E4B50` writes fourteen globals | applied: 0.4 |
| 10 | minor | YB-4b.4 depends on DoF word 24, not on the Tone Map blend | applied: 5 |
| 11 | minor | Section 3 quality row cited the wrong rows; `game_quality_object.cpp:14` and `CustomDof` (`LP:937`) omitted | applied |
| 12 | minor | Labels and wording (type 7, record offset, "both <= 0", the stale word duration, the CHAIN-CHECKLIST scope gap, the read-back source inferred) | applied |

## Exports and tools

`Get-YebisSecondBlocks.ps1` (this part); `Get-YebisBankBlocks.ps1`, `Invoke-Map.ps1`, `I0068Grep.java` (reused). Read: `FUN_1400FB890` (lines 226 to 460), `FUN_1400E4890`, `FUN_1400E4B50`, `FUN_1400E69A0` (lines 153 to 175, 268 to 300, 536 to 556), `FUN_14021E720`, `FUN_1401D9910`, `FUN_140221640`, `FUN_140218BA0`, `FUN_14016EED0`, `FUN_1416CA160`, `FUN_1416CA2E0` (exports under `decompiled/` and `evidence/I-0068/queries/yebis-ready/b1`, `b2`). Constants read: `143D7ECE8` eps, `143D7ED28` (motion-blur factor threshold), `143D7ED10` 0.001.

## Implementation log (thread « Source du soleil puis W6 », 2026-10-13: YB-4b.1 depth of field, YB-4b.2 motion blur)

`yebis_blocks`: `DofBlock` (2.2: word 12 = entry 13, words 13-32 = entries 14-33, word 33 = entry 12), `BlendDof`
(`14021E720`: words 0, 8-13, 18, 24, 25 from B), `MotionBlurBlock` (2.3: the mode word 1 = 2 and word 9 = 0 not in the file,
the GX half with the enable byte and the three flag bytes of word 19), `BlendMotionBlur` (`1401D9910`, word 12 lerped where
the original leaves a stale word); T2d/T2e in `yebis_blocks_test` (argument 5, `yebis-second/second-blocks.tsv`, file
`default`): every word of both records, and the blends' copy sets on distinct synthetic records. The feed (`yebis.flag`, under
FB890's gate) reads the keys i = 2 and 6, applies the scripted-focus override (`context+0x413D`: words 12 and 24 zero, word 2
= `context+0x4140`), and calls the genuine `E4890` on the scratch object seeded `+0x2270 = 0, +0x2271 = 1, +0x2272 = 1` (the
constructor's values, 2.2) and `E4B50` (only with `[GXRS+0x5368]` when the quality object exists: it dereferences it
unguarded); both pinned, both observed in Reference with `yebis-observe.flag`. **Row YP1 answered at m37** (Reference1607,
Native1608): the original's DoF and motion-blur records are **bit-identical to the native ones built from `mapbase`** at the
zone key (`DAC3AA31…`, `A70CD00E…`: aperture 2.0, Enable 1, levels 3/3, far custom range 50..10000; blur time ratio 0.2,
recurrences 2, fall-off 0..3), so the working choice of 2.1 holds there (one zone; m31/m50 not run); the globals follow
(`14496211C` 1, `144962124` 2.0, `144962140/144` 3/3; `14496214C` 1, `144962154` 0.2, `144962158` 0.1, `144962160` 2); 0
debug errors. Not done: the auto-exposure and auto-focus read-back (YB-4b.4), the history service (YB-4b.5), the feedback
record's order (see YEBIS-WAVE1-READY: word 2 is a record-only 0.5, INFERRED), the colour-grading bank (YB-4c).
**Review (fresh Opus 5.5; applied).** `E4890` dereferences `[GXRS+0x5368]` unguarded whenever `[GXRS+0x5360]` exists
(`1400E48F6`), like `E4B50`: it now runs only with both objects, or without the quality object (the log above said
"guarded": wrong). In `1401D9910`'s in-between branch the original leaves word 12, the top bytes of word 14 and byte 3 of word
19 as the shared buffer holds them, which in FB890 is the DoF record just built: the feed copies those words from the DoF
record in that branch (so `1449622C0 = factor x word 12` follows the original during a transition too; correction to 2.3,
which called the word stale). The globals were compared with the expected values natively only (the reference observer logs
the records, not these globals); the quality object at m37 gives the motion-blur option 1/1/5 here (Native1609: same
records, 0 debug errors).

## Implementation log (thread « Suite W10 YEBIS », 2026-10-13: YB-4b.4 auto exposure and auto focus guards, YB-4b.5 history service)

**YB-4b.4.** `yebis_blocks`: `BlendToneStored` transcribes `140218BA0`'s two exposure words (asm `14021934C..14021941A`: the
flags are compared as signed dwords, `> 0`): at a snap the record's word 0 (0x23) is replaced by its stored word 0x35 (0x36)
when its word 9 (0x30) is positive; in between, both flags give the lerp of the stored words, one flag the lerp of the flagged
record's stored word with the other's plain word, none the generic lerp. Written, not applied: the stored words exist only
with the library (FB890 writes `144962008` into word 0x35 before the blend; word 0x36's producer is not read). The guard
(`ToneAutoFlags`, `DofAutoFocus`) reads the real condition of 2.5: word 9 or `Enable AutoExposure` (record word 0x30, found by
name) positive in **either** Tone Map record, or the blended DoF word 24 (file entry 25, after the scripted-focus override)
positive; the feed then logs `native_yebis_auto {approximated}` (once, and on each change) and goes on with the file's exposure and
the record's focus distance (4.3). `ToneBlock` no longer refuses a positive AutoExposure word (that refusal stopped the whole
feed; YEBIS-WAVE1-READY 4.2's builder rule is superseded here). The read-back the HAL owes is declared (`kReadbacks`:
exposure into `144962008`, focus distance into `144962120`, latency PROPOSED 2 frames) for YB-8. T2g in `yebis_blocks_test`:
the nine flag combinations (-1, 0, 1) for both words at t = 0.25 against values written out from the asm, both snaps, the
guards (positive only), and no flag in `default`. **YB-4b.5.** `History` (inert): members registered (after-image,
adaptation, feedback, velocity), `Observe` resets on the first frame, a view change, a render-extent change and a camera
cut; `Reset` takes the renderer's `DeviceRecreation` and the two original routes `GxOutputState`/`ParameterReset`, which
nothing raises until YP11. The camera-cut rule is one function (`CameraCut`), now also the velocity history's (same
arithmetic). The feed observes under FB890's gate and logs `native_yebis_history {reason, generation}`. T2h: reasons and
generations (10 units exactly is kept, 10.5 cuts). Build: every unit test passes (`yebis_blocks_test` 1062 checks). **Live
Native1614** (m37, Shebang Gamma save, `yebis.flag`, with YB-5): `native_yebis_auto {refused: false}` (the row is logged
once and on each change of the flags: it appeared before the zone, at keys -1, and never again, so no flag at the zone key by
construction), `native_yebis_history` first frame then one camera cut at the spawn, the YEBIS block digests unchanged (Setting Info
`2AF8C7FA…`, Tone Map `17685E68…`, Bloom `5F83CF0E…`, light shafts `381BCD12…`, DoF `DAC3AA31…`, motion blur `A70CD00E…`, the
values Reference1592/1603/1607 gave), 8,704 native frames, 0 faults, clean close, files restored by hash. The run goes through
`Run-LiveWitness.ps1` from now on (one command for the session's permission rules).
**Review of YB-4b.4/.5 and YB-5 (fresh Opus 5.5, no blocking finding; applied).** (1) With a positive AutoExposure flag the
converters now run with the file's exposure where the original holds the library's adapted value: the auto row's field is
`approximated` (was `refused`) and `native_yebis_globals` carries `auto_flags`, so a Reference/Native comparison in such a zone
is marked; the commit table says so. (2) T2g varies the word-9 and word-0x30 flags independently (81 combinations). (3) The
`ToneStored` field names are documented as hexadecimal word numbers. (4) T6's constants: the earlier build used literal
constants (no `game/` executable in the worktree); the build now sets `DS3_EXE` and the test requires the executable's
constants and prints which were used. (5) Native1614's auto row wording made exact (logged before the zone, never again). (6)
`Run-LiveWitness.ps1` never exits while the trace job runs (its end restores the protected files) and reports
`restoration.json`. (7) The unresolved-programme scan per gated frame is AO's pattern and resolved live: kept.
