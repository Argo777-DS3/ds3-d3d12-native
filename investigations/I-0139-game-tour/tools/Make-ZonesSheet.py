# I-0139 zones-bonfires: builds sheets/zones-bonfires.md from the static dumps under evidence/I-0139/artifacts/static/zones-bonfires/
# (BonfireWarpParam.tsv, menu_dlc2.tsv, bonfire-objects.tsv, msb-summary.tsv, joined.json) and the sl2-analyzer's bonfires.json.
# Usage: python -I investigations/I-0139-game-tour/tools/Make-ZonesSheet.py   (run from the worktree root)
import json, math, re, glob
D = 'evidence/I-0139/artifacts/static/zones-bonfires/'
J = json.load(open(D + 'joined.json', encoding='utf-8'))
cur = json.load(open('C:/Users/User/.codex/I0033-tools/sl2-analyzer/db_ds3/bonfires.json', encoding='utf-8'))
curmap = {}
for z, l in cur.items():
    for off, bit, n in l:
        curmap.setdefault(n, []).append((off, bit, z))
players = {}
for l in open(D + 'msb-summary.tsv', encoding='utf-8'):
    p = l.rstrip('\r\n').split('\t')
    if len(p) >= 6 and p[1] == 'player' and p[2].startswith('c0000_'):
        players.setdefault(p[0], []).append((p[2], float(p[3]), float(p[4]), float(p[5])))
byname = {}
for r in J:
    byname.setdefault(r['name'], []).append(r)
TABS = [('Cemetery of Ash', ['Firelink Shrine', 'Cemetery of Ash', 'Iudex Gundyr', 'Untended Graves', 'Champion Gundyr']),
        ('High Wall of Lothric', ['High Wall of Lothric', 'Tower on the Wall', 'Vordt of the Boreal Valley', 'Dancer of the Boreal Valley', 'Oceiros, the Consumed King']),
        ('Undead Settlement', ['Foot of the High Wall', 'Undead Settlement', 'Cliff Underside', 'Dilapidated Bridge', 'Pit of Hollows']),
        ('Road of Sacrifices', ['Road of Sacrifices', 'Halfway Fortress', 'Crucifixion Woods', 'Crystal Sage', 'Farron Keep', 'Keep Ruins', 'Old Wolf of Farron', 'Abyss Watchers']),
        ('Cathedral of the Deep', ['Cathedral of the Deep', 'Cleansing Chapel', 'Deacons of the Deep', "Rosaria's Bed Chamber"]),
        ('Catacombs of Carthus', ['Catacombs of Carthus', 'High Lord Wolnir', 'Abandoned Tomb', "Old King's Antechamber", 'Demon Ruins', 'Old Demon King']),
        ('Irithyll of the Boreal Valley', ['Irithyll of the Boreal Valley', 'Central Irithyll', 'Church of Yorshka', 'Distant Manor', 'Pontiff Sulyvahn', 'Water Reserve', 'Anor Londo', 'Prison Tower', 'Aldrich, Devourer of Gods']),
        ('Irithyll Dungeon', ['Irithyll Dungeon', 'Profaned Capital', 'Yhorm the Giant']),
        ('Lothric Castle', ['Lothric Castle', 'Dragon Barracks', 'Dragonslayer Armour', 'Grand Archives', 'Twin Princes']),
        ('Archdragon Peak', ['Archdragon Peak', 'Dragon-Kin Mausoleum', 'Great Belfry', 'Nameless King']),
        ('Kiln of the First Flame', ['Flameless Shrine']),
        ('The Painted World of Ariandel', ['Snowfield', 'Rope Bridge Cave', 'Corvian Settlement', 'Snowy Mountain Pass', 'Ariandel Chapel', 'Sister Friede', 'Depths of the Painting', "Champion's Gravetender"]),
        ('The Dreg Heap', ['The Dreg Heap', 'Earthen Peak Ruins', 'Within the Earthen Peak Ruins', 'The Demon Prince']),
        ('The Ringed City', ['Mausoleum Lookout', 'Ringed Inner Wall', 'Ringed City Streets', 'Shared Grave', 'Church of Filianore', 'Darkeater Midir', "Filianore's Rest", 'Slave Knight Gael'])]
saves = {}
for f in sorted(glob.glob('evidence/I-0132/artifacts/runs-r11c[0-9]*.log')):
    for l in open(f, encoding='utf-8', errors='replace'):
        m = re.match(r'^(\w+) kept (eta-r11c-\S+) ([0-9A-F]{64})\s*$', l)
        if m:
            saves.setdefault(m.group(1), []).append((m.group(2), m.group(3)))
visit = {(2, 1): 'hw1', (2, 2): 'hw2', (2, 3): 'hw3', (2, 4): 'hw4', (3, 1): 'us1', (3, 2): 'us2', (3, 3): 'us3', (3, 4): 'us4', (3, 5): 'us5',
         (4, 5): 'farron', (6, 1): 'carthus', (6, 4): 'smouldering', (6, 5): 'demonruins', (8, 1): 'irithylldungeon', (8, 2): 'profaned',
         (9, 4): 'archives', (2, 5): 'garden', (11, 1): 'kiln', (14, 3): 'ringed', (12, 1): 'ariandel'}
warm = "fire-lit interior takes the fires' warm colour where the original keeps a cool grey ambient (D-0262, expected)"
verdict = {'farron': 'no fault; views i132-r11c5', 'carthus': 'no fault; views i132-r11c5 (label meaning inferred)', 'smouldering': warm,
           'demonruins': warm, 'irithylldungeon': 'no fault', 'profaned': warm, 'archives': 'no fault; 17.2 M triangles in the travel journal',
           'garden': '4 nonfinite_placement objects skipped (by design), otherwise no fault', 'kiln': 'no fault',
           'ringed': "arrival timed out at 6.7 s (map walked after the hold, then 2,025 instances); the original's picture not filmed",
           'ariandel': 'nonfinite_placement skipped; snow a little greyer and darker under fog; arrival released early (fixed in 12_scene_collect.inc)',
           'hw1': 'no fault', 'hw2': 'no fault', 'hw3': "arrival released 'ready' early at 2.5 s (fixed)", 'hw4': 'no fault',
           'us1': 'no fault', 'us2': 'nonfinite_placement skipped', 'us3': 'no fault', 'us4': 'water traced (native_rt_water)',
           'us5': 'water traced; arrival released early at 1.75 s (fixed)'}
BOSSY = ('Vordt of the Boreal Valley', 'Dancer of the Boreal Valley', 'Oceiros, the Consumed King', 'Iudex Gundyr', 'Champion Gundyr', 'Dragonslayer Armour',
         'Twin Princes', 'Nameless King', 'Crystal Sage', 'Abyss Watchers', 'Deacons of the Deep', 'High Lord Wolnir', 'Old Demon King', 'Pontiff Sulyvahn',
         'Aldrich, Devourer of Gods', 'Yhorm the Giant', 'Sister Friede', 'The Demon Prince', 'Darkeater Midir', 'Slave Knight Gael', 'Old Wolf of Farron',
         "Champion's Gravetender", 'The First Flame')


def teleport(map_, x, y, z):
    best = None
    for n, px, py, pz in players.get(map_, []):
        d = math.dist((x, y, z), (px, py, pz))
        if best is None or d < best[0]:
            best = (d, n, px, py, pz)
    return best


ETA = 'shebang-zones-eta-m40-20261014-095203'
LOOK = "Arrival, the bonfire's flame, the area's first views (look sweep `-LookSweep 4,-300,2`)"
rows = []
n = 0
for ti, (tz, names) in enumerate(TABS, 1):
    x = 263 + 60 * (ti - 1)
    for ri, nm in enumerate(names, 1):
        cand = byname.get(nm, [])
        r = None
        for c in cand:
            if c['obj']:
                r = c
                break
        if r is None and cand:
            r = cand[0]
        n += 1
        y = 280 + 33 * (ri - 1)
        if r is None:
            rows.append((f'Z{n:02d}', nm, '?', '', '', 'OPEN: no BonfireWarpParam row for this name'))
            continue
        o = r['obj']
        map_ = o[0] if o else '?'
        ox, oy, oz = (float(o[2]), float(o[3]), float(o[4])) if o else (0, 0, 0)
        tp = teleport(map_, ox, oy, oz) if o else None
        reach = f'Eta save `{ETA}` `-Continue -BonfirePages -TravelPress "{x},191;400,{y}"` (tab {ti} {tz}, row {ri})'
        if nm == 'Firelink Shrine':
            reach += '; also the fixed bottom row (380,613) of every tab'
        key = visit.get((ti, ri))
        if key and key in saves:
            reach = f'kept test save `{saves[key][0][0]}` (SHA-256 {saves[key][0][1][:12]}..., full in runs-r11c*.log) or ' + reach
        if tp:
            reach += f'; teleport `{tp[2]:.2f} {tp[3]:.2f} {tp[4]:.2f}` (`{tp[1]}`, {tp[0]:.1f} m from the bonfire; heading OPEN)'
        c = curmap.get(nm)
        cs = ('; CURATED sl2 offset.bit ' + '/'.join(f'{a}.{b}' for a, b, z in c)) if c else ''
        src = (f'STATIC BonfireWarpParam row {r["row"]}: warp flag {r["flag"]}, entity {r["entity"]}, text {r["text"]}; MSB {map_} {o[1] if o else ""} '
               f'at {ox:.1f} {oy:.1f} {oz:.1f}; tab and row RECORD (go-{ti}.png){cs}')
        if key:
            src += f'; R11c visited natively: {verdict[key]}'
        elif tz == 'The Dreg Heap' and ri == 1:
            src += '; R11c: seen before, not visited in the tour'
        what = LOOK
        if nm in BOSSY:
            what += '; boss-arena bonfire (INFERRED from the name: the boss fog or arena beside it)'
        rows.append((f'Z{n:02d}', f'{nm} ({tz}, tab {ti} row {ri})', map_[:6], what, reach, src))
for nm in ('Farron Keep Perimeter', 'Kiln of the First Flame', 'The First Flame'):
    r = byname[nm][0]
    o = r['obj']
    n += 1
    tp = teleport(o[0], float(o[2]), float(o[3]), float(o[4]))
    zs = 'zone save `shebang-zones-iota-m33-20261018-165305`' if o[0].startswith('m33') else 'Eta via the Flameless Shrine row (tab 11 row 1)'
    rows.append((f'Z{n:02d}', f"{nm} (real bonfire, not on Eta's Travel page)", o[0][:6], LOOK + ('; the last arena of the game' if nm == 'The First Flame' else ''),
                 f'not a row of the Travel pictures: from {zs}, teleport `{tp[2]:.2f} {tp[3]:.2f} {tp[4]:.2f}` (`{tp[1]}`, {tp[0]:.1f} m; heading OPEN); or the warp flag {r["flag"]} set so it is listed',
                 f'STATIC BonfireWarpParam row {r["row"]}: flag {r["flag"]}, entity {r["entity"]}, text {r["text"]}; MSB {o[0]} {o[1]}; why it is off the page INFERRED (not lit in Eta or not warp-listed); not visited in R11c'
                 + ('; CURATED has it (sl2 offset.bit ' + '/'.join(f'{a}.{b}' for a, b, z in curmap[nm]) + ')' if nm in curmap else '; CURATED has no row for it')))
n += 1
rows.append((f'Z{n:02d}', 'BonfireWarpParam placeholder rows 0, 7, 12, 13', 'm40_00 / m30_00',
             'nothing: no usable bonfire object (rows 12 and 13: no MSB object with entity 3001959 or 3001958; row 7: the object is parked at y -999; row 0 repeats row 1, Firelink Shrine)',
             'not reachable and not needed', 'STATIC BonfireWarpParam rows 0, 7, 12, 13 and bonfire-objects.tsv; their purpose INFERRED (placeholders or alternate states)'))
sm = saves['smouldering'][0][0]
dr = saves['demonruins'][0][0]
area = [('Smouldering Lake (Catacombs of Carthus)', 'm38_00', 'the lava lake, the crystals, the fire-lit light sources (R11-PLAN §4)',
         f"no bonfire of its own: nearest Old King's Antechamber (tab 6 row 4, kept save `{sm}`) and Demon Ruins (tab 6 row 5, `{dr}`)",
         "RECORD R11c (visited from those two bonfires); the area name from the records, not found in the menu FMG (event text 'Tell of the Smouldering Lake' only); the bonfire-less status INFERRED"),
        ('Anor Londo great staircase and main hall', 'm37_00', "the great stairs, the cathedral's entrance, the giants' rubble",
         'nearest bonfire Anor Londo (tab 7 row 7): Gamma save `shebang-zones-gamma-m37-anorlondo-20261026-123818` (SHA-256 E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411) or the Eta travel; the hall: `-Teleport "611.5 -129.6 -1042.4 30"` (never 616.1 -128.6: the rubble kills the character)',
         'RECORD LIVE-RUN-NOTES (I-0113 gap 1)'),
        ("Firelink Shrine: the Shrine Handmaid's chair", 'm40_00', "the Handmaid's chair, the shrine hall",
         'Eta save, `-Teleport "254.8 -54.5 604.7 90"` then four `-300,0` looks (`Run-I113Chair.ps1`)', 'RECORD LIVE-RUN-NOTES'),
        ('High Wall of Lothric water', 'm30_00', 'the water pieces (in view from none of the four bonfires)',
         'teleport `-89.76 -48.58 425.36` (`c0000_0003`; the R11c `m30water` view), from the High Wall save', 'RECORD I-0132 RESULT R11c (water traced, 406 to 1,422 frames); the position STATIC (msb-summary.tsv)'),
        ('Undead Settlement water', 'm31_00', 'the water seen from the Dilapidated Bridge and the Pit of Hollows',
         'kept saves `eta-r11c-us4` (Dilapidated Bridge, tab 3 row 4) and `eta-r11c-us5` (Pit of Hollows, tab 3 row 5)', 'RECORD I-0132 RESULT R11c (native_rt_water frames from +600 after the arrival)'),
        ('m34_00 (stub map)', 'm34_00', "nothing to walk: 0 enemies, 8 regions, 3 player starts, one at the Dragonslayer Armour's start (-99.14 59.09 222.81) of m30_01, one near the Twin Princes'",
         "not reachable with today's tools; probably a companion of the Lothric Castle and Grand Archives maps (INFERRED)", 'STATIC msb-summary.tsv; purpose OPEN'),
        ('m53_00 and m54_00 (small maps)', 'm53_00 / m54_00', 'm53_00: 457 map pieces, 5 objects, no enemies, one player start (0 6.57 0); m54_00: 46 map pieces, 2 player starts; likely arenas or test maps (INFERRED)',
         "not reachable with today's tools: no bonfire or Travel row points there; needs an EMEVD warp-target read to find the way in", 'STATIC msb-summary.tsv; role OPEN'),
        ('m47_00 (DLC1, the Painted World second map)', 'm47_00', 'light-bake rows exist (8); no MSB extracted for it here; no bonfire object is on it',
         "not reachable with today's tools; the way in is OPEN", 'RECORD I-0110 light rows; everything else OPEN')]
for nm, mp, wl, rc, sc in area:
    n += 1
    rows.append((f'Z{n:02d}', nm, mp, wl, rc, sc))

hdr = '''# I-0139 sheet: zones and bonfires (Z01...)

Written by a Sonnet 5.5 worker from SHEETS-BRIEF; non-canonical until the pilot checks it.

**Covers**: every bonfire of the base game and both DLC, grouped by zone in the order of Eta's fourteen Travel tabs, the three real bonfires the Travel page does not list, the param's placeholder rows, and the zones or areas walked that have no bonfire of their own.

**Sources**
- STATIC: `BonfireWarpParam` of `Data0.bdt` (the regulation, decrypted with the public key as in `investigations/I-0067-static-menus-subsystems-map/Get-ParamRows.ps1`: 82 rows of 64 bytes; dump `evidence/I-0139/artifacts/static/zones-bonfires/BonfireWarpParam.tsv`, `Data0.bdt` SHA-256 955016AB...82B87). No paramdef is on the PC: the fields are read by offset: `0x00` warp event flag, `0x04` bonfire entity id, `0x08` name text id (the rest unread; `0x10` looks like a sort order, not used). English names: the `FDP_メニューテキスト.fmg` entries of `msg/engUS/menu_dlc2.msgbnd.dcx` (in Data1; `menu_dlc2.tsv`; the Travel tabs' names are ids 211100 to 211113 of that file). Map and position: the MSB3 `o000100` objects whose `EventEntityID` equals the entity id (`bonfire-objects.tsv`; the MSBs were read from `C:/Users/User/Tools/DS3-Decompilation/gamedata-i0138/bosses/`, extracted by another sheet; my own extraction, the two msgbnd, is in `gamedata-i0139/zones-bonfires/`). Teleport targets: the nearest `c0000_*` player-start part of the same MSB (`msb-summary.tsv`; the MSB frame is the teleport frame, LIVE-RUN-NOTES). Tools: `tools/Dump-Fmg.ps1`, `Find-BonfireObjects.ps1`, `Get-MsbSummary.ps1`, `Make-ZonesSheet.py` (builds this file).
- RECORD: the tab and row from the pictures `go-1.png` to `go-14.png` of run `i132-r11c0-eta-tabs` (`evidence/I-0066/artifacts/i132-r11c0-eta-tabs/`), read by eye by this worker; tab x = 263 + 60 (tab-1), y 191; row y = 280 + 33 (row-1); the row's x is 400 as in LIVE-RUN-NOTES (`-TravelPress "803,191;400,280"`). Firelink Shrine is also the fixed bottom row (380,613) of every tab. Eta's page opens on its last tab: check the picture before keeping a save. Kept saves and verdicts: I-0132 RESULT « R11c and R11b », R11-PLAN §12, `Run-R11cTour.ps1`, logs `evidence/I-0132/artifacts/runs-r11c*.log` (full SHA-256), folders `C:/Users/User/.ds3-i0039-backups/eta-r11c-<label>-<stamp>`.
- CURATED: `db_ds3/bonfires.json` (save-file offset.bit per bonfire, grouped by zone); used only to cross-check names and counts.

**How to read « reach »**: `Eta save` is `C:/Users/User/.ds3-i0039-backups/shebang-zones-eta-m40-20261014-095203/DS30000.sl2` (SHA-256 1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4; `Run-AlphaCast -SaveCopy ... -SaveCopySha256 ... -Continue`). A kept test save `eta-r11c-<zone>` was made AT that bonfire: load it with `Run-R7aGi -ExtraSaves zone=folder=sha256` or `Run-R11cTour -SkipTravel -ExtraSaves`; use it when it exists (no travel needed). Tab and row are pilot units (screen pixels). Every live run follows WORKFLOW « Live game rule ».

**Look at** (every bonfire row): the arrival (`native_rt_arrival` ready time), the bonfire's flame, the first views (a look sweep). Boss-named bonfires sit beside the boss arena (INFERRED from the name).

**R11c's visits** (RESULT « R11c and R11b »): 20 bonfires travelled natively (the first tour's eleven, the High Wall's and the Undead Settlement's other nine), PT and original views at 1600x900 from each, no fault in any run; the second Ariandel and Ringed City saves come from the `r11b8` runs. Not visited: the Dreg Heap (seen before) and the other listed bonfires. Thomas's judgement of the sheets was pending.

**Differences found**: (1) Eta's Road of Sacrifices tab has 8 rows, the param 9: `Farron Keep Perimeter` is not listed. (2) The Kiln tab has one row (Flameless Shrine): `Kiln of the First Flame` and `The First Flame` are param rows with bonfire objects but are not on the page. (3) Irithyll's tab has 9 rows (Aldrich last; hidden below the crop of the first look at it). (4) CURATED puts Firelink Shrine inside « Cemetery of Ash », as the Travel page does.

**OPEN**: the row x per bonfire (400, from one record); the headings of the teleports; the meaning of the kept label `carthus` (taken as the Catacombs of Carthus bonfire, row 1: the log gives only the label); the roles of m34_00, m47_00, m53_00, m54_00; the HUD's area-name strings for areas with no bonfire.

| Row | Name | Map | What to look at | How to reach it with the test launcher | Source and status |
|---|---|---|---|---|---|
'''
out = hdr + '\n'.join('| ' + ' | '.join(c.replace('|', '/') for c in r) + ' |' for r in rows) + '''

## Gaps

- **A one-call « travel to bonfire X » tool**: each bonfire is a hand-read tab and row passed to Run-AlphaCast `-TravelPress`; a table (this sheet's tab, row and flag) read by `Run-R11cTour.ps1` for every listed row would do it. R11c kept 20 bonfires; the rest (the Dreg Heap, most of the Ringed City and Ariandel, and the others) are not yet test saves.
- **Bonfires off Eta's page** (Farron Keep Perimeter, Kiln of the First Flame, The First Flame): reachable only by teleport from a save of that map; the smallest tool is a warp-flag setter (the flags above) so the Travel page lists them, or `-Teleport` with this sheet's coordinates (needs a heading).
- **Areas with no bonfire**: the list is what the records and this worker's reading name (RECORD and INFERRED); a full list needs the game's area-name table (not found in the menu FMGs) or the MSB regions read per map.
- **m34_00, m47_00, m53_00, m54_00**: roles OPEN; the smallest tool is an EMEVD warp-target scan (`investigations/I-0067-static-menus-subsystems-map/Get-EmevdDump.ps1`) for these map ids.
- **Teleport heading**: the player-start parts' Rotation is not printed by `Get-MsbSummary.ps1`; one more column.
'''
open('investigations/I-0139-game-tour/sheets/zones-bonfires.md', 'w', encoding='utf-8', newline='').write(out)
print(len(rows))
