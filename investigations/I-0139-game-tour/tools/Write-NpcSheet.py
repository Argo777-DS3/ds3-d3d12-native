# I-0139 npcs-merchants: writes sheets/npcs-merchants.md from the joined static data (Build-NpcData.py) + the annotations below.
# Run from the worktree: python -I investigations/I-0139-game-tour/tools/Write-NpcSheet.py
import json, collections, re
S = 'evidence/I-0139/artifacts/static/npcs-merchants/'
R = json.load(open(S + 'npc-parts-joined.json', encoding='utf-8'))
TAB = {'m40_00_00_00': '263 Cemetery of Ash', 'm41_00_00_00': '863 Kiln', 'm30_00_00_00': '323 High Wall', 'm30_01_00_00': '743 Lothric Castle',
       'm31_00_00_00': '383 Undead Settlement', 'm33_00_00_00': '443 Road of Sacrifices', 'm35_00_00_00': '503 Cathedral',
       'm37_00_00_00': '623 Irithyll', 'm38_00_00_00': '563 Catacombs of Carthus', 'm39_00_00_00': '683 Irithyll Dungeon',
       'dlc1_m45_00_00_00': '923 Ariandel', 'dlc2_m50_00_00_00': '983 Dreg Heap', 'dlc2_m51_00_00_00': '1043 Ringed City'}
ZONE = {'m40_00_00_00': 'm40 Firelink Shrine / Cemetery of Ash', 'm41_00_00_00': 'm41 Kiln of the First Flame', 'm30_00_00_00': 'm30_00 High Wall of Lothric',
        'm30_01_00_00': 'm30_01 Lothric Castle', 'm31_00_00_00': 'm31 Undead Settlement', 'm33_00_00_00': 'm33 Road of Sacrifices',
        'm35_00_00_00': 'm35 Cathedral of the Deep', 'm37_00_00_00': 'm37 Irithyll of the Boreal Valley', 'm38_00_00_00': 'm38 Catacombs of Carthus',
        'm39_00_00_00': 'm39 Irithyll Dungeon', 'dlc1_m45_00_00_00': 'm45 Ashes of Ariandel (DLC1)', 'dlc2_m50_00_00_00': 'm50 The Dreg Heap (DLC2)',
        'dlc2_m51_00_00_00': 'm51 The Ringed City (DLC2)'}
KILL = {}
npcs = json.load(open('C:/Users/User/.codex/I0033-tools/sl2-analyzer/db_ds3/npcs.json', encoding='utf-8'))
for b, bit, desc in npcs['NPCs killed']:
    for e in re.findall(r'(\d{7})', desc):
        KILL.setdefault(e, []).append('%d.%d' % (b, bit))
PARK = (273.01, -98.42, 602.62)


def parts(name, mp):
    return [r for r in R if r['name'] == name and r['map'] == mp and r['talk'] > 0 and r['model'] != 'c1000']


def parked(r):
    return tuple(round(v, 2) for v in r['pos']) == PARK


def where(rs):
    r = rs[0]
    if parked(r):
        return 'parked at the shared Firelink hold point 273.0 -98.4 602.6 (not where the NPC stands: OPEN, set by the m40 event script)'
    return 'part %s at %.1f %.1f %.1f' % (r['part'], *r['pos'])


def reach(rs):
    r = rs[0]
    s = 'Eta (`-Continue`), Travel tab %s, row "%s" (row from the Travel list order, INFERRED)' % (TAB[r['map']], r['bonfire'])
    if parked(r):
        return s + '; the standing place is OPEN (travel there and look around)'
    return s + '; then `-Teleport "%.1f %.1f %.1f <rot>"` at the part (stand a few metres off: untested)' % tuple(r['pos'])


def flags(rs):
    f = []
    for r in rs:
        k = KILL.get(r['entity'])
        if k:
            f.append('%s: %s' % (r['entity'], '/'.join(sorted(set(k)))))
    return ('Killed flag (sl2 byte.bit, CURATED) ' + ', '.join(f) + '.') if f else ''


rows = []


def add(name, mp, what, cond, status, rs=None, extra_reach=''):
    rs = rs or parts(name, mp)
    assert rs, (name, mp)
    ids = 'entities ' + ', '.join(r['entity'] for r in rs) + '; TalkID ' + ', '.join(str(r['talk']) for r in rs) + '; NPCParam ' + ', '.join(sorted(set(str(r['npcparam']) for r in rs)))
    rows.append((name, ZONE[mp] + '; nearest bonfire ' + rs[0]['bonfire'] + ' (%d m)' % round(rs[0]['bdist']),
                 where(rs) + ' (' + ids + ')', what, (cond + ' ' + flags(rs)).strip(), reach(rs) + extra_reach, status))


DLG = 'dialogue with subtitles (`01_060_Caption`)'
ST = 'Name, part, ids STATIC; services and quest triggers INFERRED (the game as played)'
SH = 'ESD int constants (byte scan 0x82+int32, command not decoded) STATIC'

add('Fire Keeper', 'm40_00_00_00', 'Level Up window (attributes, soul cost), attune / estus shard / ember menus, ' + DLG,
    'Present from the start of Firelink. State in Eta, Gamma, the Shebang characters: OPEN.', ST + '; no shop id in her ESD t400120 (' + SH + ')')
add('Fire Keeper', 'm41_00_00_00', 'Level Up window and dialogue at the end of the game (Flameless Shrine / The First Flame)',
    'After the Soul of Cinder step (OPEN); Eta holds the Kiln bonfires.', ST)
add('Ludleth of Courland', 'm40_00_00_00', 'Transposition: boss-soul exchange list (ShopLineupParam rows 30000, 30001, 30020-30092, 30110-30141, 30153-30251), his throne, ' + DLG,
    'Appears once a first Lord of Cinder boss soul is available (INFERRED). State in Eta/Gamma: OPEN.', 'Part, ids STATIC; ESD t400130 constant 30000 and the rows STATIC (' + SH + '); trigger INFERRED')
add('Shrine Handmaid', 'm40_00_00_00', 'Shop (ShopLineupParam 110000-119899 per ESD t400210; rows exist in the base regulation) with her Dried Finger / White Sign Soapstone / Priestess Ring items; her chair; ' + DLG + '. The second part (TalkID 400211, shop 119900-119999, rows 119900-119953) is a variant of her',
    'Present from the start; the variant\'s trigger is OPEN. Items (CURATED): 980.3 Dried Finger, 982.7 White Sign Soapstone, Priestess Ring 86653.1.',
    'Position STATIC + RECORD (LIVE-RUN-NOTES chair teleport `254.8 -54.5 604.7 90` is 3 m from the part); shop ranges STATIC (ESD t400210, t400211); flags CURATED',
    extra_reach='; chair view: `Run-I113Chair.ps1` (RECORD)')
add('Blacksmith Andre', 'm40_00_00_00', 'Reinforce / infuse / repair windows and his shop (ESD t400200 range 100000-109999; the base ShopLineupParam has no row in that range: which rows he sells is OPEN), his forge, ' + DLG,
    'Present from the start. Hawkwood\'s Swordgrass 86654.3 (CURATED).', ST + '; shop range STATIC, rows OPEN')
add('Yuria of Londor', 'm40_00_00_00', 'Shop (ESD t400140 constant 40000, ShopLineupParam 40000-40006), dialogue, ' + 'her Ashes quest',
    'Appears at Firelink after a quest step (INFERRED). Yuria\'s Ashes given 243.5 (CURATED).', ST + '; shop id STATIC (byte scan)')
add('Yoel of Londor', 'm40_00_00_00', 'Shop (ESD t400150 constant 50000, ShopLineupParam 50000-50003), level-up service when the Fire Keeper is gone, Dark Sigil, ' + DLG,
    'Moves to Firelink after the Settlement (INFERRED).', ST + '; shop id STATIC (byte scan)')
add('Yoel of Londor', 'm31_00_00_00', 'Dialogue (first meeting near the Foot of the High Wall)', 'First place the player meets him (INFERRED).', ST)
add('Company Captain Yorshka', 'm37_00_00_00', 'Dialogue at the Prison Tower; Yorshka\'s Chime 86643.3 (CURATED)', 'Present once Irithyll is reached; OPEN beyond that.', ST)
add('Hawkwood the Deserter', 'm40_00_00_00', 'Dialogue (two parts: 4000760 and 4000761, the arena version), Heavy Gem gift', 'After the Abyss Watchers (INFERRED).', ST)
add('Hawkwood the Deserter', 'm33_00_00_00', 'Dialogue near the Abyss Watchers\' fog', 'Before and after the Watchers: OPEN.', ST)
add('Sirris of the Sunless Realms', 'm40_00_00_00', 'Dialogue (Firelink)', 'Quest completion flag 257.5 (CURATED).', ST)
add('Sirris of the Sunless Realms', 'm31_00_00_00', 'Dialogue (Undead Settlement, Pit of Hollows)', 'Quest start (OPEN beyond).', ST)
add('Sirris of the Sunless Realms', 'm37_00_00_00', 'Dialogue (Irithyll)', 'Quest step (OPEN).', ST)
add('Ringfinger Leonhard', 'm40_00_00_00', 'Dialogue (Firelink; the part has a real standing position), Cracked Red Eye Orb', 'After the Cathedral (INFERRED).', ST)
add('Ringfinger Leonhard', 'm35_00_00_00', 'Dialogue (Cathedral, near Rosaria\'s Bed Chamber), Black Eye Orb', 'First place the player meets him (INFERRED).', ST)
add('Ringfinger Leonhard', 'm37_00_00_00', 'Dialogue near the Aldrich bonfire', 'Quest end (INFERRED).', ST)
add('Greirat of the Undead Settlement', 'm30_00_00_00', 'Dialogue from his cell, trade errand', 'Present from the start of High Wall.', ST)
add('Greirat of the Undead Settlement', 'm40_00_00_00', 'Shop (ESD t400220 range 120000-129999, rows 120000-120214) and errand dialogue', 'After his errands (OPEN).', ST + '; shop range STATIC')
add('Orbeck of Vinheim', 'm33_00_00_00', 'Dialogue (Road of Sacrifices), scroll quest', 'After the Crystal Sage area (INFERRED).', ST)
add('Orbeck of Vinheim', 'm40_00_00_00', 'Shop (ESD t400230 ranges 130000-139999, rows 130100-130604): sorceries; scroll hand-in', 'After the Firelink move (INFERRED).', ST + '; shop range STATIC')
add('Cornyx of the Great Swamp', 'm31_00_00_00', 'Dialogue (Undead Settlement)', 'Present from the start of the Settlement.', ST)
add('Cornyx of the Great Swamp', 'm40_00_00_00', 'Shop (ESD t400240 ranges 140000-149999, rows 140000-140401): pyromancies', 'After his rescue (INFERRED).', ST + '; shop range STATIC')
add('Karla', 'm39_00_00_00', 'Dialogue (Irithyll Dungeon cell)', 'Present in the Dungeon.', ST)
add('Karla', 'm40_00_00_00', 'Shop (ESD t400250 ranges 150000-159999, rows 150100-150502): dark sorceries', 'After her rescue (INFERRED).', ST + '; shop range STATIC')
add('Irina of Carim', 'm31_00_00_00', 'Dialogue (Undead Settlement), Tower Key', 'Irina "Normal Placement" flag 271.6 (CURATED).', ST)
add('Irina of Carim', 'm40_00_00_00', 'Shop (ESD t400260 ranges 160000-169999, rows 160000-160502): miracles; three parts 4000725-4000727', 'After the quest (INFERRED).', ST + '; shop range STATIC')
add('Eygon of Carim', 'm31_00_00_00', 'Dialogue (Undead Settlement)', 'Hostile flag 275.2/3 (CURATED).', ST)
add('Eygon of Carim', 'm40_00_00_00', 'Dialogue (Firelink)', 'Quest step (OPEN).', ST)
for mp, w in [('m33_00_00_00', 'Dialogue (Halfway Fortress)'), ('m37_00_00_00', 'Dialogue (near the Church of Yorshka bonfire)'),
              ('m38_00_00_00', 'Dialogue (Catacombs of Carthus)'), ('m40_00_00_00', 'Dialogue (Firelink)')]:
    add('Anri of Astora', mp, w, 'Questline flag 286.1 "completed or killed" (CURATED); Anri\'s Straight Sword 86639.1, Ring of the Evil Eye 86659.1.', ST)
add('Old Woman of Londor', 'm37_00_00_00', 'Dialogue (two parts: 3700720, 3700721)', 'Appears with the Yuria quest (OPEN).', ST)
add('Unbreakable Patches', 'm40_00_00_00', 'Shop (ESD t400301 constant 200000, ShopLineupParam 200000-200200), dialogue', 'After the Cathedral episode (INFERRED).', ST + '; shop id STATIC (byte scan)')
add('Unbreakable Patches', 'm35_00_00_00', 'Cathedral dialogue and the shop constant 200000 in ESD t350301', 'Quest step (INFERRED).', ST + '; shop id STATIC (byte scan)')
add('Unbreakable Patches', 'dlc2_m51_00_00_00', 'Dialogue (Ringed City, Shared Grave)', 'DLC2 questline; Patches\' Ashes 86665.4 (CURATED).', ST)
add('Siegward of Catarina', 'm31_00_00_00', 'Dialogue (Undead Settlement)', 'Present early.', ST)
add('Siegward of Catarina', 'm37_00_00_00', 'Dialogue (Irithyll)', 'Quest step (OPEN).', ST)
add('Siegward of Catarina', 'm39_00_00_00', 'Dialogue (Irithyll Dungeon)', 'Quest step (OPEN).', ST)
add('High Priestess Emma', 'm30_00_00_00', 'Dialogue (High Wall, near the Dancer\'s bonfire); Basin of Vows 86670.1 (CURATED)', 'Present from High Wall.', ST)
add('Horace the Hushed', 'm33_00_00_00', 'Dialogue (Halfway Fortress)', 'Quest start (INFERRED).', ST)
add('Horace the Hushed', 'm40_00_00_00', 'Dialogue (Firelink)', 'Quest step (OPEN).', ST)
add('Slave Knight Gael', 'm35_00_00_00', 'Dialogue (Cathedral, Cleansing Chapel)', 'DLC prelude (OPEN).', ST)
add('Holy Knight Hodrick', 'm31_00_00_00', 'Dialogue (Undead Settlement, Pit of Hollows)', 'Present early.', ST)
add('Rosaria, Mother of Rebirth', 'm35_00_00_00', 'Dialogue and the Rosaria\'s Fingers covenant (flags CURATED: 971.7 joined, 971.6 Obscuring Ring, 971.5 Man-grub\'s Staff)', 'After Cathedral progress (INFERRED).', ST,
    rs=[r for r in R if r['name'].startswith('Rosaria') and r['map'] == 'm35_00_00_00'])
add('Sir Vilhelm', 'dlc1_m45_00_00_00', 'Dialogue (Ariandel Chapel; two parts 4500700 / 4500701); Contraption Key 86705.7 (CURATED)', 'DLC1 questline.', ST)
add('Amnesiac Lapp', 'dlc2_m50_00_00_00', 'Dialogue (Dreg Heap; three parts)', 'DLC2 questline, flag 340.5 (CURATED).', ST)
add('Amnesiac Lapp', 'dlc2_m51_00_00_00', 'Dialogue (Ringed City)', 'DLC2 questline (OPEN).', ST)
add('Shira, Knight of Filianore', 'dlc2_m51_00_00_00', 'Dialogue (Ringed City Streets)', 'DLC2 questline (OPEN).', ST)
add('Stone-humped Hag', 'dlc2_m50_00_00_00', 'Shop (ESD t500760 constant 760000, ShopLineupParam 760000-760005), dialogue', 'DLC2 (OPEN).', ST + '; shop id STATIC (byte scan)')

COV = {'Altar of Sunlight': 'Warrior of Sunlight (covenants.json: 963.3 joined, 963.2 Sunlight Shield, 963.1 Sunlight Straight Sword)',
       'Sacrificial Altar': 'which covenant: OPEN (covenants.json lists Mound-makers 970.1, Bloodlust 970.0, Warmth 969.7)',
       'Old Wolf of Farron': 'Watchdogs of Farron (968.7 joined, 968.6 Old Wolf Curved Sword, 968.5 Wolf Knight\'s Greatshield)',
       'Archdeacon McDonnell': 'Aldrich Faithful (967.5 joined, 967.4 Great Deep Soul, 967.3 Archdeacon\'s Great Staff)',
       'Velka, the Goddess of Sin': 'Velka\'s pact (no covenant entry in covenants.json: OPEN)',
       'Divine Spear Monument': 'Spears of the Church (979.1 covenant item, CURATED)', 'Purging Monument': 'DLC2 altar (what it gives: OPEN)'}
for nm, mp in [('Altar of Sunlight', 'm30_01_00_00'), ('Sacrificial Altar', 'm31_00_00_00'), ('Old Wolf of Farron', 'm33_00_00_00'), ('Archdeacon McDonnell', 'm37_00_00_00'),
               ('Velka, the Goddess of Sin', 'm31_00_00_00'), ('Divine Spear Monument', 'dlc2_m51_00_00_00'), ('Purging Monument', 'dlc2_m51_00_00_00')]:
    r = [r for r in R if r['nameid'] >= 890000 and r['name'] == nm and r['model'] == 'c1000' and r['talk'] > 0 and r['map'] == mp][0]
    rows.append((nm + ' (covenant altar)', ZONE[mp] + '; nearest bonfire ' + r['bonfire'] + ' (%d m)' % round(r['bdist']),
                 'part %s (model c1000, entity %s, TalkID %d, NPCParam %d) at %.1f %.1f %.1f' % (r['part'], r['entity'], r['talk'], r['npcparam'], *r['pos']),
                 'Pledge / altar window and dialogue; ' + COV[nm], '',
                 'Eta (`-Continue`), Travel tab %s, row "%s"; `-Teleport "%.1f %.1f %.1f <rot>"` (untested)' % (TAB[mp], r['bonfire'], *r['pos']),
                 'Name and part STATIC (NPCParam nameId 890xxx in the NPCName FMG); window INFERRED; flags CURATED'))
rows.append(('Covenant givers not located', 'several', 'Blade of the Darkmoon 973.1, Blue Sentinels 974.3, Way of Blue 978.5 (covenants.json, CURATED)',
             'Pledge dialogues', 'OPEN', 'not reachable with today\'s tools: needs the giver located (talk ESD decode)', 'OPEN'))
rows.append(('Bonfire (family)', 'all maps', '91 `o000100` bonfire objects in the MSBs, 78 with a BonfireWarpParam row; each bonfire has a c1000 talk part (ESD constants 1000 2030 2050)',
             'Rest / attune / estus / travel menu (the Travel list names are FDP menu text 211200-211345), bonfire level', 'Eta holds every bonfire; the bonfire sheet owns the zones',
             'Any Travel row (Eta); rest at the bonfire', 'STATIC (BonfireWarpParam 82 rows, MSB objects, FMG)'))
ph = collections.Counter(r['name'] for r in R if r['name'] and r['model'] == 'c0000' and r['talk'] == 0 and r['nameid'] < 890000)
rows.append(('NPC phantoms (family)', 'every map with a questline',
             '%d c0000 parts with an NPC name and TalkID 0 (summon signs, ally and hostile phantoms): ' % sum(ph.values()) + ', '.join('%s %d' % kv for kv in ph.most_common()),
             'White NPC summon sign (offline only the white NPC sign exists, PHANTOMS-SIGNS-NATIVE.md), the phantom shaders, the boss-fog arrival; not talkable',
             'Each at its quest step; the sign needs the NPC alive and the step: OPEN per NPC', 'Teleport to the part (MSB position) with F4; sign and fight need the quest flags: OPEN',
             'Parts STATIC (MSB Enemy parts + NpcParam nameId); sign behaviour RECORD (PHANTOMS-SIGNS-NATIVE.md)'))
dl = [r for r in R if r['map'] == 'dlc1_m45_00_00_00' and r['talk'] > 0 and r['name'] == '' and r['model'] != 'c1000']
rows.append(('DLC1 talk-capable characters, names unread', 'm45 Ashes of Ariandel (DLC1)',
             '%d parts: ' % len(dl) + '; '.join('%s (TalkID %d) at %.0f %.0f %.0f near %s' % (r['part'], r['talk'], *r['pos'], r['bonfire']) for r in dl),
             'Candidates by place only: the Painter, Champion\'s Gravetender, Forlorn Corvian Settler, Birch Woman, Drowsy Forlorn (NPCName FMG ids 26000-34000 exist; the DLC1 NpcParam rows are not in the base regulation read here)',
             'DLC1 questlines; killed flags 320.5, 323.5, 326.1, 335.1 (CURATED)',
             'Eta, Travel tab 923 Ariandel, the nearest bonfire, then `-Teleport` at the part', 'Parts STATIC; which NPC each is: OPEN'))

out = ['# I-0139 sheet: NPCs, merchants, blacksmith, trainers and services', '',
       'Closed-list rows `N01...`. Every named NPC of the base game and both DLC the player can talk to, the shops and services they open, the covenant altars, and the NPC phantoms as one family row.', '',
       '## Sources and method', '',
       '- **NPC names (STATIC)**: `NPC名.fmg`, `NPC名_dlc1.fmg`, `NPC名_dlc2.fmg` of `msg/engUS/item_dlc2.msgbnd.dcx` (Data1, recovered with `I-0066/Recover-GameDataByName.ps1`), dumped by `tools/Dump-Fmgs.ps1`.',
       '- **Who is who (STATIC)**: every MSB3 Enemy part of the recovered map files (`tools/Dump-MsbEnemies.ps1`, SoulsFormats MSB3) carries an `NPCParamID`; `NpcParam` (regulation in Data0.bdt, read with `I-0067/Get-ParamRows.ps1`) has the NPCName FMG id at row offset 0x08 (205 rows hit a FMG id there; offsets 0x00 to 0x10 tested). Positions are the MSB part positions (= the teleport frame, brief). Parts parked at `273.01 -98.42 602.62` share one Firelink hold point, so where the NPC stands is OPEN. `tools/Build-NpcData.py` joins them; `tools/Write-NpcSheet.py` writes this sheet.',
       '- **Shops (STATIC, partial)**: the `/script/talk/mXX_00_00_00.talkesdbnd.dcx` files (same archives) scanned by `tools/Scan-TalkShops.ps1` for 0x82+int32 constants; the ranges quoted are constants found in the NPC\'s ESD, which the `ShopLineupParam` rows (662 rows: ids 1, 2000-2011, 30000-30251, 40000-40006, 50000-50100, 110000-120303, 130100-160502, 200000-200300, 760000-760005, 900000-900065) confirm. The command that uses each constant is not decoded.',
       '- **Bonfires (STATIC)**: `o000100` objects (`tools/Dump-MsbBonfires.ps1`); names from `BonfireWarpParam` (entity id to FDP menu text). "Nearest bonfire" is the nearest in the same map file.',
       '- **CURATED**: sl2-analyzer `npcs.json` (kill/hostile flags are "byte.bit" in the save\'s flag byte table, not event flag ids), `questlines.json`, `covenants.json`.',
       '- **RECORD**: `.project/LIVE-RUN-NOTES.md` (the Handmaid\'s chair), `I-0067/SUBTITLES-DIALOGUE-NATIVE.md` (one page, `01_060_Caption.gfx`, shows every NPC line), `I-0067/PHANTOMS-SIGNS-NATIVE.md`, `I-0132/R11-PLAN.md` §12 (Travel tabs, pilot units: tab x, rows from y 280, 33 apart).',
       '- Dumps: `evidence/I-0139/artifacts/static/npcs-merchants/` (`msb-enemies.tsv`, `npc-nameid.tsv`, `npc-parts-joined.json`, `ShopLineupParam.rows.tsv`, `talk-shop-constants.tsv`). Licensed bytes: `C:/Users/User/Tools/DS3-Decompilation/gamedata-i0139/npcs-merchants/` (the I-0138 path was not used).',
       '- **Kept OPEN**: which NPCs are in place in Eta, Gamma or the Shebang characters (Eta holds every bonfire; quest presence unread); every quest trigger marked INFERRED; where parked Firelink NPCs stand; Andre\'s shop rows; the DLC shop rows; the names of the DLC1 characters; the givers of three covenants.',
       '', '| Row | Name | Map | What to look at | How to reach it with the test launcher | Source and status |', '|---|---|---|---|---|---|']
for i, (name, mp, loc, what, cond, reach_, st) in enumerate(rows, 1):
    out.append('| N%02d | %s | %s | %s<br>Where: %s<br>Presence: %s | %s | %s |' % (i, name, mp, what, loc, cond or 'OPEN', reach_, st))
out += ['', '## Gaps', '',
        '- **Quest presence per test character** (Eta, Gamma, Shebang): not readable from the data alone; the smallest tool is a flag reader on a save copy (event-flag dump of a Run-AlphaCast `-SaveCopy`) run once per character, then a per-NPC flag-set file beside `test-revive.txt` to put an NPC in place.',
        '- **Where the parked Firelink NPCs stand**: needs the m40 EMEVD (`I-0067/Get-EmevdDump.ps1`) read for the warp commands, or one walk through Eta\'s Firelink.',
        '- **Shop rows of Andre and of the DLC NPCs**: need the DLC regulation (not recovered here) and the full ESD decode; the byte scan does not name the command.',
        '- **Teleports to an NPC** are untested except the Handmaid (RECORD); a wrapper that teleports to a named part and reports the ground height would close it.',
        '- **Names of the DLC1 characters** and the givers of three covenants: need the DLC1 NpcParam and the DLC ESDs.', '']
open('investigations/I-0139-game-tour/sheets/npcs-merchants.md', 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
print(len(rows))
