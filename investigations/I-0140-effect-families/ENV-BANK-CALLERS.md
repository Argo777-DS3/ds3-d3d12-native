# The env-map bank's callers (GXRS+0x42A8: 41990 has, 416A0 get): tranche 29's list, read again and witnessed

A static list made 2026-11-21 ~07:10 by the `ghidra-research` subagent (Sonnet) for the inventory Thomas agreed to (2026-11-21 06:56, the E thread: « Oui tu peux faire l'inventaire »). Unverified: each line is to be read again by the pilot before it is built on (LEDGER). Added by the pilot after the merge of tranche 28 (READ in its disassembly): D0D060 (936660's, no static caller in code; its xrefs are unwind data) calls 66640 on GXRS+0x42B0 and clears GXRS+0x4568 only when 41990({1, 0}) is true on the engine's bank; natively 41990 answers from the native banks only inside D0C2F0's scope (NativeEnvBankHas), so D0D060's answer natively is the engine bank's alone (witnessed below: never called).

All 23 call sites (a byte scan of the executable's sections in the final review: 18 calls to 416A0, 5 to 41990, no stored pointer) read the env bank (loaded from [0x144961b50]+0x42A8, or Blend's argument). 416A0 falls back to {1,0} then bank+0x1A8; {-1,-1} rewritten {1,0}. Entry +0xC scale, +0x40 cube handle object.

| Site | Caller | Key | Use | Caller's caller | Native |
|---|---|---|---|---|---|
| 7710F/77120/77139/7714A | 770C0 | {bank+0xD8}, {bank+0xE0} | has gate then entry+0x40; both fail: [param1+0x178] (REM) | 4E2A0 only, when a key id is -1 | none; bank+0xD8/+0xE0 never written natively |
| D0D094 has, D0D0AA get | D0D060 | {1,0} (GparamKindToArea(0xD)=1) | true: entry+0x40=0, 66640(RDM) (the whole region manager cleared), [GXRS+0x4568]=0 | 936660 (no static caller found) | none. Pair of 936700 -> D0CA20 (the set's region add, [GXRS+0x4568]=1) |
| D0C8E8 | D0C2F0 | {1,10} | gate of the menu region add | D65DD0 (menu renderer ctors) | t28 wrapper |
| D0C9C0 | D0C990 | {1,10} | release gate | D66280 | t28 replacement |
| 4122D/4123C/4147D/4148C get | 40DD0 Blend | args | blend A,B into dest (+0x8, +0xC, +0x40) | CC8180 (remo act) | hooked NativeRemoBankWrite<3> |
| 4E2E7/4E2F9 get | 4E2A0 | group +0xB0, +0xB8 | entry+0x40 -> +0xA0/+0xA8, +0xC -> +0x140/+0x144 | 5C0E0 (RecordDrawRows, PackSingleDrawRow) | observer only; native recomputes (09:77-90, 142-146; 05:263-266, 904) |
| 75B68/75B7A get | 75B30 | arg2+0x2F0/+0x2F8 | cube to PS slots 0x1E/0x1F | 5A120, 10E660, 10ED10, 1A9C30 | observer only; not bound natively (01_state:409-411) |
| 1A8781/1A8793 get | 1A86C0 | arg1+0x2F0/+0x2F8 | scale only, entry+0xC | world draws, decals, FFX model renderer | hooked; original for non-native contexts (effect_draw.cpp:473): default entry scale |
| 67A5F get | 675B0 EnvMapRegion_Execute | region record key | scale, cube | stub 67050 | not run natively; reproduced (12:333-345, light_producers.cpp:716-722) |
| 6AD7C get | 6A490 | record key | wraps entry+0x40 (export?) | 68C80 <- 68AF0 (from a registered callback, below) | none |
| 1BF745 get | 1BF180 | [arg1+0xC178] | cube handle bound | 1C2310 (vtable slot) | none |
| 1C197D/1C198F get | 1C10C0 | stack locals (unwritten?) | scale into CB | 1C0DC0 <- 1C2310 | none |

## Read again by the pilot (tranche 29, GhidraMCP and the executable's bytes, 2026-11-21 08:20 to 08:50)

- **41990, 416A0** (READ, their disassembly): 41990 is a two-level hash lookup (the area in the outer map, the id in the area's
  inner map), true when both are found, with no fallback. 416A0 reads {-1, -1} as {1, 0}, returns the key's entry, else {1, 0}'s,
  else the bank's default +0x1A8 (the constructor 3F720 gives it id 0, scale 1.0, the name 143D42BD8 and cube +0x40 = 0). The
  constructor also sets +0xD8 and +0xE0 to {0, -1} (READ); no other writer of them was found in 3E000 to 44000 (a byte scan).
- **D0D060** (READ): its key {GparamKindToArea(0xD), 0} comes from D0D0E0; its RCX is unused. Its only caller 936660 is the step
  `ModelViewerInsStep_forSkybox::STEP_Init_forReleseEnvMap` and D0CA20's caller 936700 is `STEP_Wait_forLoadResource` of the same
  class (the step table at 144785CA0, filled by the static initializer 14267A970 with each step's name, UTF-16 at 142876C90 to
  142877010). The class belongs to the developer's model viewer (`ModelViewerStep`, `ModelViewerListStep`, `SprjModelViewerList`
  with its `DbgMenuEzCbItem`, the `ModelViewerInsStep_for*` family: Msb, Mapenv, Map, Parts, Obj, Loadlist, Facegen, ChrId,
  Skybox; RTTI names in .rdata), created through the runtime classes (no direct constructor call found). So D0D060 and D0CA20 run
  only inside the developer's model viewer, never in play (INFERRED from the classes).
- **6A490** (READ): a debug window of the region manager (its items are sliders and a texture preview of entry+0x40 through
  142300380, 14001B370, 142300790); 68AF0 builds the window (1422FF460) and is reached by a jump from a thunk at 14004A7C0
  (`RCX+0xF0`, then 68AF0), whose address 4AC60 registers as a callback (LEA at 14004ACAE). The list's « no callers » was wrong;
  debug only either way.
- **1BF180, 1C10C0** (READ): reached from 1C2310, the first slot of the vtable at 143D5D5A0 whose RTTI is
  `GXSeaWaveRenderObj@GXSR` (the sea wave renderer, the SeaWave gparam group). Whether a zone draws sea waves: the witness.
- **40DD0** (READ): Blend builds or updates the bank's node {key} from the entries 416A0 gives for its two source keys (id +8,
  scale +0xC lerped, name +0x10, cube +0x40 kept with a reference). Natively the bank holds no file, so a cutscene act's blend
  builds a node from the default entry (scale 1.0, no cube) where the original blends the area's real entries: a lead, to witness
  in a cutscene (I-0141's acts).

Candidates to witness (engine answer vs our banks): 1A86C0's non-native contexts (scale 1.0 natively vs 1.5?), 1BF180/1C10C0 (pass 1C2310: runs natively?), 770C0, D0D060 (the set's removal: has({1,0}) false natively would skip 66640 and the 4568 clear), 6A490.

## Witnessed (tranche 29, `i140-t29c-witness-{nat,orig}`, 2026-11-21 09:13 to 09:18, Firelink, about 85 s in play each)

OBSERVED on one path; the counts and the reading in RESULT « Tranche 29 ». In the original 11 game sites call the bank (all on the
GXRS bank): 1A86C0's two 416A0 (667977 calls each, 6 % exact, the rest {1, 0}'s), 75B30's two (93534 each), 675B0's (60195,
almost all exact), 4E2A0's two ({0, -1}, all {1, 0}'s, 1.5), 770C0's two 41990 (19548 each, never true: the original also takes
its REM fallback, [param+0x178], READ by the final review), D0C2F0's and D0C990's 41990 (11 each, true). Natively only D0C2F0's gate (11: the engine's bank false, the
game given true by the native banks) and the module's NativeEnvMapCube (2672, the default entry); the bank empty. No call to
either reader from D0D060, 6A490, 1BF180, 1C10C0 or 40DD0 in either mode. D0CA20 calls neither reader (READ), so this witness
does not see it: natively its admission is absent from tranche 28's rows (`set_region` false on all 47 of
`i140-t29c-witness-nat`), in the original not witnessed. So the « Native » column above holds; the candidates: 770C0 false on
all its calls in the original, natively not run (the empty bank would answer false, INFERRED); D0D060 never in play (INFERRED,
the classes); 1A86C0's non-native contexts not met; 40DD0 and the sea waves not met (a cutscene witness; SeaWave inert by data,
not re-read).
