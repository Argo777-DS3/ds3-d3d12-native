# Equip a hand, add a SpEffect, use a goods: the static read for the next test tools (I-0140 tranche 14)

A `ghidra-research` subagent's read (Sonnet, GhidraMCP on Thomas's GUI copy of DarkSoulsIII.exe 1.15.2, SHA-256 EF5E07C5…,
2026-11-16 11:30-11:50), for the tools the rest of ROADMAP workstream 5 step 5 needs: Carthus Flame Arc (a flame in the left hand,
a weapon in the right), the resins and other buffs on a weapon, the infusions, the statuses. The calls the F1 tool makes were re-read by the pilot
(GhidraMCP, 2026-11-16 12:00-12:20) and by its step review (the disassembly) before the module calls them. READ: seen in the code by the subagent; INFERRED: not.

## Anchors

- EquipGameData (EGD) = `[[1447572B8]+0x10]+0x228` (READ, the menu `140B064E0`); the ChrAsm = EGD+0x88 = PlayerGameData+0x2B0.
- EGD+0x24+4*slot: the inventory index equipped in slot 0..0x15 (READ: written by `14058A130`); hands are slots 0..5 (L1 R1 L2 R2
  L3 R3, as the save's copy), 6..9 arrows and bolts, 0xC..0x10 protectors, 0x11..0x15 accessories. EGD+0x1A8 the inventory,
  EGD+0x250 the quick items (10 slots), EGD+0x178 the equipment sync target.
- ChrAsm+0x24+4*slot the item value (category nibble << 28 | id), ChrAsm+0x7C+4*slot the id (READ, `140869B70`).
- `14058C950(EGD+0x1A8, &out, index)`: an inventory index's item value (nibbles: weapon 0, protector 1, accessory 2, goods 4).

## A. Equip a hand

- The menu (`140B064E0(slot, entry)`, entry+0x38 = the inventory index) unequips the item's earlier slot with
  `1405828F0(EGD, oldSlot, 1)`, then `140A32290(tmp, EGD, index)`, `140A323E0(tmp, &value)`, **`140584FF0(EGD, slot, &value, index, 1, 1,
  0)`**, `1405859A0(EGD)` (the quick-item index refreshed), with a player (`[14477FDB8]+0x80`) `1408C0930(player)`, and last
  `1406C1DD0(slot)` (for a hand, `140890150(player)`: a flag at `[[player+0x1F90]+0x20]+0x50` raised); at its entry the gate
  `140B06930(slot)` and the record `140765220(0x47, index)` (READ, the pilot and the step review).
- `140584FF0` does nothing when the slot already holds that value and index; else a closure (vtable `1427BEF28`) and `14058A130(EGD,
  slot, index, closure)`, which writes EGD+0x24[slot], marks EGD+0x90 when the slot is a current hand, and syncs
  (`140869960(*(EGD+0x178), EGD+0x88)`, `1408A3900(…, 1)`) (READ). Whether the closure rebuilds the model: INFERRED, not read.
- `140581860(EGD, category, id, count, …)` adds to the inventory and may auto-equip (READ; callers `140B034F0`, `1407C2CC0`).
- The current-slot setter (the arrows' switch) not found; its readers `140868490`, `140868C50`, `1409F5BF0(ChrAsm+8, k)`.

## B. Add a SpEffect to a character

- The ChrIns's SpEffect module `*(ChrIns+0x11C8)` (READ); `[PlayerGameData+0x920]` the same object (INFERRED).
- **`140888F20(ChrIns, id, float scale)`**: null check, then the module's add with the defaults (owner -1, no broadcast, none of
  `1408858E0`'s gates); used by `1408C46B0` for equipment effects (READ). Scale 1.0 (it multiplies the duration).
- The full path `140885830(chr, id, source, …)` (gating, `14088EF10` broadcasts), `140891EF0`, the ChrIns virtual `140885620`.
- The module's list: +8 the head; an entry +0x60 id, +0x68 the param row, +0x78 next; row +0x156 stateInfo (READ); the cast
  gate's 0x60, 0xA7, 0x72, 0xC0 are stateInfo values (`140A00930`), not ids.
- **Not reached:** the statuses' SpEffect ids (poison, toxic, bleed, frost, curse), the EMEVD 2004[8] handler.
- The rows by stateInfo (the pilot, `Get-ParamRows -Param SpEffectParam`, `e84/speffect-param.tsv`, 3,450 rows, the u16 at
  +0x156): pairs of low ids share one value, 101 and 111 (13), 102 and 112 (6), 103 and 113 (2), 104 and 114 (5): one id for the
  player and one for the others, INFERRED. By Elden Ring's enum (2 poison, 5 rot, 6 haemorrhage) 103 would be the poison, 104 the
  toxic, 102 the bleed, 101 the frost or the curse: INFERRED, and superseded: they are cure rows (« B2 »), nothing shown (`i140-e89`).

## B2. The statuses (a second `ghidra-research` read, 2026-11-16 14:50; not yet re-read by the pilot)

- The buildup applier `14089A250` (READ): a SpEffect row's stateInfo (+0x156) picks a gauge: 2 poison (amount +0xCC), 5 toxic
  (+0xD0), 6 bleed (+0xD4), 0x74 curse (+0xD8), 0x104 frost (+0x1AC) (the names INFERRED from the HUD's order); an immunity test
  `140A01470`; the gauge `1409E6EA0` (module `[ChrIns+0x1F90]+0x20`: +0x10+4i the gauge, +0x24+4i its maximum) subtracts the
  amount and, at zero, sets a flag (+0x4C bit i) and resets the gauge. On a hit (`1408C34F0`) the SpEffect is added first
  (`140A01FD0`), kept when the gauge fills, removed (`1409FE680`) when not: **the buildup row is itself the status's SpEffect**.
- Rows (from `speffect-param.tsv`, the family INFERRED): poison 20000-20019 (stateInfo 2, endurance 180 s), toxic 21000-21019 (5,
  300 s), bleed 22000-22019 (6, 0.1 s), curse 23000-23019 (0x74, 0 s, +0x124 = 33), frost 24000-24019 (0x104, 14 s); the curse's
  endless row 33 (stateInfo 0x75, which the HUD tests with 0x74 and 0x88). 101 to 104 are cure rows (amount -99999): what
  i140-e89 added and saw nothing for.
- `140888F20` skips the gauge (it adds the row directly): the status's own visual and icon should show from the row (INFERRED).

## C. Use a goods

- `140E2F200(out, goodsId)` the goods row lookup (repository 3); row +0 refId, +0x3B refCategory (2 SpEffect, 1 bullet), +0x41 a
  level offset (READ). The dispatcher `140881250` → for refCategory 2 `140881470`: `id = refId + 14088ABA0(chr, row[0x41])`,
  then `140885830(chr, id, chr, 0, &pos, 1.0, 0, 0, 0)` (READ): a goods' effect is a SpEffect on the user.
- The resins' rows (READ by the pilot, `Get-ParamRows -Param EquipParamGoods`, `evidence/I-0140/e84/goods-param.tsv`): all
  refCategory 2 (+0x3B), +0x41 0; refId (+0) and +0x48: Charcoal Pine Resin 330 → 2100 / 2300, Charcoal Pine Bundle 340 → 2110 /
  2310, Gold Pine Resin 331 → 2120 / 2320, Gold Pine Bundle 341 → 2130 / 2330, Rotten Pine Resin 343 → 2150 / 2350, Human Pine
  Resin 334 → 2170 / 2370, Carthus Rouge 335 → 2180 / 2380, Pale Pine Resin 336 → 2190 / 2390 (which of +0 and +0x48 the
  dispatcher takes when: the « one variant » of `140881470` not read). The use animation and the count's decrement not found.

## What a tool would do (the plan before the tool; built instead as F1 and test-effect.txt, `ApplyTestEffect`)

1. F10 (`test-equip.txt`, words `L1 w<id>` … `R3 w<id>`): find the weapon's inventory index (give it first with F7 if absent),
   then the menu's sequence above, then `1408C0930`.
2. F9 is the effect dump; another key or a word of the same file for SpEffects: `s<id>` → `140888F20(player ChrIns, id, 1.0)`.
3. The resins: the goods row's refId (category 2) applied as in 2, the count left alone (a test tool).
