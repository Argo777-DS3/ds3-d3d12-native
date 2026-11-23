# I-0139: the closed list of the game's tour

The list ROADMAP workstream 6 asks for, closed on 2026-11-12. Its rows are in six sheets, written by Sonnet 5.5 workers from
[SHEETS-BRIEF](SHEETS-BRIEF.md) and read from the game's own data (the regulation's BonfireWarpParam, the English FMG text, the
maps' MSB3 and EMEVD files, the Remo bundles' names in the archives' indexes) and the repository's records; each fact carries its
status (STATIC, CURATED, RECORD, INFERRED, OPEN). The pilot checked against the primary data: the Travel tab and row reading
(Road of Sacrifices' picture `go-4.png`), and every boss name the sheets changed in `boss-flags.json` (below).

| Sheet | Rows | What it holds |
|---|---|---|
| [zones-bonfires](sheets/zones-bonfires.md) | Z01-Z87 | 75 bonfires on Eta's fourteen Travel tabs (tab and row in pilot units, warp flag, entity, MSB position, the nearest player start), 3 bonfires the page does not list (Farron Keep Perimeter, Kiln of the First Flame, The First Flame), the param's placeholder rows, 8 areas without a bonfire; R11c's 20 visits and kept saves |
| [bosses](sheets/bosses.md) | B01-B26 | 26 bosses (the 26 registered defeat flags; the Stray Demon's candidate flag 13300560 outside them), each with its entry (fog object and gate, intro cutscene), phases and death: 128 rows with the sub-rows |
| [cutscenes-events](sheets/cutscenes-events.md) | C01-C45, E01-E16 | 79 Remo bundles (69 base, 5 DLC1, 5 DLC2; 55 called by an event, 24 by none recovered) and the two Bink movies; 16 families of scripted world events with their witnesses |
| [npcs-merchants](sheets/npcs-merchants.md) | N01-N62 | 31 named NPCs at 47 places (15 open a shop, level up, transposition or the blacksmith), the 7 covenant altars, the families (bonfires, NPC phantoms) |
| [menus](sheets/menus.md) | M01-M59 | every menu and window, from the logos to the Rekindled pages; about 38 already seen natively, 9 never |
| [endings-dlc](sheets/endings-dlc.md) | F01-F07, D01-D08 | the endings (which flag 20-23 is which ending is OPEN), the credits (forced by flag 20 in 2026-10-18's menu tests), the NG+ question, both DLC entrances and arrivals |

## The three answers of a row

A row is closed when its run answers: (1) a fault: the journal's faults, latches, `prefix_refused`, unserved (magenta) frames, the
controller's exit (I-0132's [Get-R11cJournal.py](../I-0132-rt-optimisation-zones/Get-R11cJournal.py)); (2) a skipped family: the
journal's skip, unsupported, missing and passes-off rows, set apart from the Firelink load's rows common to every run (R11c);
(3) the picture beside the original's at the same place: the original travels or replays the same way in the same session. A row
the launcher cannot reach stays « not reached » with the tool it needs. Each defect goes to ROADMAP workstream 4 (a fault, a
refusal; since its close on 2026-11-13, workstream 16, where its leftovers go: the coordinator, 18:25), 5 (a family not native or
drawn wrong) or 7 (the rays, the arrivals).

## Tranches

1. **The bonfires** (Z rows, 55 not visited by R11c): [Run-I139Tour.ps1](Run-I139Tour.ps1), Eta's travel natively (the bonfire kept
   as `eta-i139-<label>`) then the original's, the arrival's look sweep. R11c's 20 keep their verdict (no fault; its tree older).
   Night of 2026-11-12, two halves (the coordinator's slots).
2. **The bosses** (B rows): revive (`test-revive.txt`, or `witness-flag.txt "<flag> 0"`), travel to the nearest bonfire, teleport
   to the nearest player start, walk through the fog (E), F4, the phases by waiting under invincibility, F6 for the death (bosses
   over 20,000 HP are out of F6's reach: to check per boss). Needs the runner's keys for F4 and F6.
3. **The cutscenes** (C rows) and the events (E rows): first tried by their flags cleared and a teleport to the trigger (fragile:
   2026-11-13 16:19), then by id with the native test tool `witness-cutscene.txt` (the game's own EMEVD handler, the player its entity;
   Run-AlphaCast `-Cutscene`, Run-I139Cutscenes.ps1; `-SetFlag "id v;id v"` for the flags). First, the three witnesses the YEBIS study (ROADMAP workstream 11, I-0138) names as where the native post-processing
   departs most from the original (its depth of field refused, to be seen in a run): Vordt's intro `s30_00_0030` (C01), Friede's
   `s45_00_0000` (C31) and the ending scene `s41_00_0000` (C30: its own glare and chromatic aberration).
4. **The NPCs, merchants and menus** (N and M rows): the menus by `-MenuTour`, `-OptionsTour`, `-BonfirePages`; the shops and
   dialogue by `-TalkKeeper`-like approaches at each NPC's place; YOU DIED by `test-death.flag`; presence per character OPEN.
5. **The endings, credits and DLC entrances** (F and D rows): the Soul of Cinder revived and killed, the ending flags, the credits
   by flag 20; the DLC entrances by their events' flags and teleports.

## Corrections made from the sheets

- `boss-flags.json` (I-0089): 13200800 is the Ancient Wyvern and 13200850 the Nameless King; 14500800 Sister Friede and 14500860 the
  Champion's Gravetender (both pairs were swapped: `-Bosses namelessking` revived the Wyvern); m38's flags are High Lord Wolnir and
  the Old Demon King, m50's the Demon Prince, m51's Halflight and Darkeater Midir. Checked by the pilot in the EMEVD dumps: template
  20005500 pairs each defeat flag with the bonfire of the boss's name (BonfireWarpParam and the FMG), Midir's character c6211 is entity
  5100850, Halflight's 5100800 a c0000 character.

## What the sheets could not settle (their « Gaps »)

The phases' HP thresholds (the AI scripts, unread); which ending flag is which and the endings' conditions (the talk scripts); the
24 Remo bundles no recovered event calls; the NPCs' presence in Eta, Gamma and the Shebang characters; the DLC shops; the m51_01
MSB; the headings of the teleports.
