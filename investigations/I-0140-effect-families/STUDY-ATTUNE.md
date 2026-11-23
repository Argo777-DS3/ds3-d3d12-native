# The attuned spells: where DS3 1.15.2 keeps them and how the Attune Spell menu writes them

Static read for the spells' audit (ROADMAP workstream 5 step 5), 2026-11-16: a `ghidra-research` subagent (Sonnet, GhidraMCP,
read-only, nothing named), its report below; the pilot re-read in GhidraMCP the setter `140590490`, the selector `140590FC0` and
the owner path at `140C23060` (READ, matching). Witnessed live in I-0140 tranche 13 (`i140-e76`, `i140-e80`).

## The data (EquipMagicData)

- Owner path: `[1447572B8]` (the game data, as the give tool's) `+0x10` (PlayerGameData) `+0x470` = EquipMagicData (READ at
  `140C23060`: `mov rax,[1447572B8]; mov rcx,[rax+10]; mov rcx,[rcx+470]`; the same path in `140B06810`, `1406D4AD0`,
  `1406D35B0`, `1407152A0`).
- Slots: `+0x18`, 14 slots of 8 bytes (READ: the clear loop `140591060`, the serializer `140590EB0` writing 0x70 bytes from +0x18
  then 4 from +0x88, the bounds `< 0xE` in `1405908E0` and `140590A00`). A slot = {dword the Magic row id, -1 empty; dword
  the Magic row's short +0x18 times 3, or -1} (READ in `140590490`: `*slot = CONCAT44(count, id)` after `140E32960`, a binary
  search of param table 0xE, INFERRED the Magic param: a slot-cost byte at +0x1D, an icon short at +0x10).
- The selected slot: `+0x88` (uint, -1 none; READ in `1405908E0`, `140591CD0`).
- The Attunement stat to a slot count: `140634810` (thresholds 10, 14, 18, 24, 30, 40, 50, 60, 80, 99 to 1..10), plus a ring
  bonus `1409FEB50` (INFERRED).

## The writers

- `140590490(eq, slot, id)`: the setter (READ, re-read by the pilot): checks only slot < 14, writes the slot, then
  `140591CD0(eq)` (compacts the array and re-resolves the selection). No cost, Attunement or inventory check.
- `140590FC0(eq, id)`: selects the slot holding `id` (+0x88), else the last filled one (READ, re-read by the pilot).
- `1407152A0`: the Attune menu's confirm: the cost check `140590BE0` (the slot bytes over the 14 slots against `140634810`),
  `140590B50` (stat requirements, a UI flag only), then `140590490` and the menu state 0x19; on failure sound 200 (READ by the
  subagent).
- `140B06810` (from `140BABE60`): sets a slot, or clears it if the same id is there, without a cost check. `14059B760`: the
  save's restore (clear all, the setter for each, trim, then `140590FC0` with the first id).

## The reader

- The player's per-frame update `1408C8590` reads `1405908E0(eq, [eq+0x88])` (the selected slot's id) and passes it to the
  character's module each frame; the HUD (`1406D4AD0`, `1406D35B0`) polls it too: no cached copy (READ by the subagent), so a
  write through the setter is taken the next frame (witnessed: every press attuned and selected its own spell; a cast is shown by its FP spent, I-0140 RESULT « Cast or not »). The cast's own checks: « The stats » below.

## The stats (a second `ghidra-research` read, 2026-11-16; the accessors re-read by the pilot)

- The player's attributes are 32-bit ints in PlayerGameData (`[1447572B8]+0x10`): +0x200 Vigor, +0x204 Attunement, +0x208
  Endurance, +0x20C Vitality, +0x210 Strength, +0x214 Dexterity, +0x218 Intelligence, +0x21C Faith, +0x220 Luck (READ: the
  accessors `14063F090` = +0x218 and `14063F070` = +0x21C, re-read by the pilot; the save's copy `1405A7580`).
- The Magic row's requirements: +0x1E Intelligence, +0x1F Faith (bytes; `14063F9B0` from the menu's `140590B50`), +0x22 the
  category (READ by the subagent).
- The cast gate `140640890` reads the character's PlayerGameData (`vtable[0x1B8]`) +0x218 and +0x21C live against the row's
  bytes, besides other gates (the row's flags +0x2D and +0x2E, the FP and slots, the weapon row for some): READ by the subagent;
  that the local player's object is `[1447572B8]+0x10` INFERRED.
- Witnessed (`i140-e80`): a write of 99 at +0x218 reads back 99, but the next press reads 25 again: the game recomputes these
  values from a base not found here, and the casts needing more stayed refused. The base's writer is not read.

## Not established

- (Witnessed since: table 0xE holds the spells' rows and a spell's goods id is its row id: 106 of 106 ids of
  `test-give-spells.txt` found and attuned, `i140-e76`.)

- The slot's second dword. The cast executor. EquipMagicData+0x10's identity.
