# I-0139 bosses sheet generator (run with python -I from the worktree root).
# Reads the MSB dumps (evidence/I-0139/artifacts/static/bosses) for the nearest player-start part and writes the table rows
# of sheets/bosses.md to sheets/_bosses_rows.md. Facts per boss are the BOSSES list below (each cites its status in the text).
import io, math, glob

E = 'evidence/I-0139/artifacts/static/bosses'


def players(m):
    out = []
    f = glob.glob(f'{E}/*mapstudio_{m}_00_00.msb.txt')[0]
    for l in io.open(f, encoding='utf-8-sig'):
        p = l.rstrip('\n').split('\t')
        if p[0] == 'PLAYER':
            try:
                out.append((p[1], tuple(float(x) for x in p[4].split())))
            except Exception:
                pass
    return out


def nearest_player(m, g):
    best = min(players(m), key=lambda t: math.dist(t[1], g))
    return best[0], best[1], round(math.dist(best[1], g))


TAB = {'Cemetery of Ash': 263, 'High Wall of Lothric': 323, 'Undead Settlement': 383, 'Road of Sacrifices': 443,
       'Cathedral of the Deep': 503, 'Catacombs of Carthus': 563, 'Irithyll of the Boreal Valley': 623,
       'Irithyll Dungeon': 683, 'Lothric Castle': 743, 'Archdragon Peak': 803, 'Kiln of the First Flame': 863,
       'Painted World of Ariandel': 923, 'The Dreg Heap': 983, 'The Ringed City': 1043}

BOSSES = [
 dict(n='Iudex Gundyr', kind='mandatory', tab='Cemetery of Ash', m='m40_00', flag=14000800, key='iudex',
  gate=('o000401_1000', 4001800, (126.2, -64.1, 559.6)), boss='c5110_0000 entity 4000800 at 142.9 -68.9 594.6',
  post=('Iudex Gundyr', (142.7, -68.9, 594.8)), start=('Cemetery of Ash', 58),
  cs='none found: no cutscene call in m40_00 names 14000800 or 4000800 (STATIC)',
  ph=['Phase 1: the armoured Iudex Gundyr with the halberd (INFERRED, common knowledge); BGM region 4004800 and heat-up BGM region 4004801, fight flags 14005805 and 14005822 passed to templates 20005801 and 20005831 (STATIC ids, roles INFERRED)',
      'Phase 2: the evil spirit bursts out of his back (INFERRED); the NPC-name FMG has 905110 Iudex Gundyr and 905120 Evil Spirits (STATIC), so a second name exists; the HP threshold is not an EMEVD event (AI script not read): OPEN'],
  soul=None),
 dict(n='Vordt of the Boreal Valley', kind='mandatory', tab='High Wall of Lothric', m='m30_00', flag=13000800, key='vordt',
  gate=('o000401_2000', 3001800, (27.7, -25.5, 44.9)), boss='c2240_0000 entity 3000800 at 27.4 -25.0 -1.9',
  post=('Vordt of the Boreal Valley', (27.4, -25.0, 3.3)), start=('High Wall of Lothric', 101),
  cs='intro cutscene 30000030 (`s30_00_0030`, 3 cuts) from event 13005810, entity 3002810, sets 13000801 (STATIC call; the role intro is INFERRED, I-0069)',
  ph=['Phase 1: Vordt charges and swings; BGM heat-up region 3002806, fight flag 13005805 and heat-up flag 13005806 (STATIC ids)',
      'Phase 2: frost breath and faster moves at half health (INFERRED, common knowledge; the trigger is the boss AI, not EMEVD)'],
  soul='Soul of Boreal Valley Vordt (CURATED boss_souls.json)'),
 dict(n='Curse-Rotted Greatwood', kind='mandatory', tab='Undead Settlement', m='m31_00', flag=13100800, key='greatwood',
  gate=('o000402_1000', 3101800, (136.3, -179.7, -882.1)), boss='c5180_0000 entity 3100800 at 176.7 -181.2 -957.0',
  post=('Pit of Hollows', (165.2, -240.9, -965.4)), start=('Cliff Underside', 123),
  cs='none found in m31_00 for 13100800 (STATIC)',
  ph=['Phase 1: the tree with its bulbs; BGM regions 3103800 (normal) and 3103801 (heat-up); a heat-up move point region 3102820 (STATIC regions)',
      'Phase 2: the heat-up (INFERRED from the region names and common knowledge); health bar template 20005837 names NPC 5180 (STATIC)'],
  soul='Soul of the Rotted Greatwood (CURATED)'),
 dict(n='Crystal Sage', kind='optional', tab='Road of Sacrifices', m='m33_00', flag=13300850, key='sage',
  gate=('o000401_6001', 3301850, (-165.0, -246.7, -422.8)), boss='entity 3305850 spawned by the event (no MSB part), arena region 3302850 at -183.1 -248.9 -424.3',
  post=('Crystal Sage', (-199.6, -245.8, -425.3)), start=('Crucifixion Woods', 117),
  cs='none found in m33_00 for 13300850 (STATIC)',
  ph=['Phase 1: the Sage with crystal clones; BGM regions 3304851 and 3304852 (heat-up); a second fog object o000401_6000 at -199.0 -245.5 -439.9 (entity 3301851) is the other door (STATIC)',
      'Phase 2: more clones once the heat-up flag 13305856 is set (INFERRED)'],
  soul='Soul of a Crystal Sage (CURATED)'),
 dict(n='Abyss Watchers', kind='mandatory', tab='Road of Sacrifices', m='m33_00', flag=13300800, key='watchers',
  gate=('o000401_5000', 3301800, (281.1, -247.7, -486.3)), boss='entity 3305800 spawned by the event (no MSB part), arena region 3302800 at 283.9 -248.3 -486.7',
  post=('Abyss Watchers', (333.1, -247.4, -496.7)), start=('Farron Keep Perimeter', 153),
  cs='33000010 (event 13305810, sets 13300801) and 33000000 (event 13305814, waits on 13300800): roles intro and post-kill INFERRED (STATIC calls)',
  ph=['Phase 1: one Watcher, then more join (INFERRED); BGM regions 3304801 and 3304802 (heat-up) (STATIC)',
      'Phase 2: at the heat-up the warp region 3302802 at 321.6 -248.1 -493.8 exists (STATIC region, its role INFERRED); the Undead Legion joins (common knowledge)'],
  soul='Soul of the Blood of the Wolf (CURATED)'),
 dict(n='Deacons of the Deep', kind='mandatory', tab='Cathedral of the Deep', m='m35_00', flag=13500800, key='deacons',
  gate=('o000400_9000', 3501800, (-455.7, -196.8, -620.5)), boss='entity 3505800 not in the MSB (spawned), arena region 3502800 at -457.3 -196.8 -621.6',
  post=('Deacons of the Deep', (-468.6, -197.0, -628.4)), start=("Rosaria's Bed Chamber", 147),
  cs='none found in m35_00 for 13500800 (STATIC)',
  ph=['Single phase with several deacons on a group health bar (INFERRED); BGM regions 3504801 and 3504802 (heat-up) (STATIC)'],
  soul='Soul of the Deacons of the Deep (CURATED)'),
 dict(n='Stray Demon', kind='optional (named health bar, no fog gate found)', tab='Road of Sacrifices', m='m33_00', flag=13300560, key=None,
  gate=None, boss='c5200_0000 entity 3300560 at 180.9 -160.5 -240.2 (and c5200_0001 entity 3300561 at 172.9 -159.9 -243.3)',
  post=None, start=('Keep Ruins', 132),
  cs='none found (STATIC)',
  ph=['Single phase fight (INFERRED, common knowledge); the defeat flag 13300560 is a candidate read from templates 20005341 and 20005900 that name entity 3300560 (STATIC call, flag role INFERRED); it is not one of the 26 registered flags'],
  soul='Soul of a Stray Demon (CURATED)'),
 dict(n='High Lord Wolnir', kind='mandatory', tab='Catacombs of Carthus', m='m38_00', flag=13800800, key='m38a',
  gate=('o000400_7000', 3801800, (367.0, -274.4, -672.6)), boss='c5160_0000 entity 3800800 parked at 845.1 -344.5 -1416.0, arena BGM region 3804801 at 842.0 -395.9 -1404.3',
  post=('High Lord Wolnir', (361.2, -274.4, -697.9)), start=('Abandoned Tomb', 74),
  cs='38000000 (event 13805809) and 38000020 (event 13800800, which also sets 13800800, 9315, 6315): roles intro and death INFERRED (STATIC calls)',
  ph=['Phase 1: Wolnir with the bracelets; BGM regions 3804801 and 3804802 (heat-up) (STATIC)',
      'Phase 2: after the bracelets break he is exposed (INFERRED, common knowledge); fight flags 13805805 and 13805806 (STATIC ids)'],
  soul='Soul of High Lord Wolnir (CURATED)'),
 dict(n='Old Demon King', kind='optional', tab='Catacombs of Carthus', m='m38_00', flag=13800830, key='m38b',
  gate=('o000400_5000', 3801830, (293.2, -326.7, -923.8)), boss='c3050_0000 entity 3800830 at 306.3 -329.2 -959.6',
  post=('Old Demon King', (311.1, -329.1, -964.2)), start=('Demon Ruins', 78),
  cs='none found (STATIC)',
  ph=['Phase 1: flame swings; BGM regions 3804831 and 3804832 (heat-up); health bar template 20005837 names NPC 5290 (STATIC)',
      'Phase 2: the heat-up, a faster set (INFERRED, common knowledge)'],
  soul='Soul of the Old Demon King (CURATED)'),
 dict(n='Pontiff Sulyvahn', kind='mandatory', tab='Irithyll of the Boreal Valley', m='m37_00', flag=13700850, key='sulyvahn',
  gate=('o000402_3001', 3701850, (398.5, -224.8, -1120.6)), boss='entity 3705850 spawned (no MSB part), arena region 3702850 at 398.5 -225.8 -1123.5',
  post=('Pontiff Sulyvahn', (399.3, -223.8, -1186.2)), start=('Church of Yorshka', 114),
  cs='none found for 13700850 (STATIC)',
  ph=['Phase 1: Sulyvahn with the greatsword; BGM regions 3704851 and 3704852 (heat-up) (STATIC)',
      'Phase 2: the second blade and a clone (INFERRED, common knowledge); a second fog object o000401_3001 at 399.7 -223.1 -1205.7 is the exit (STATIC)'],
  soul='Soul of Pontiff Sulyvahn (CURATED)'),
 dict(n='Aldrich, Devourer of Gods', kind='mandatory', tab='Irithyll of the Boreal Valley', m='m37_00', flag=13700800, key='aldrich',
  gate=('o000401_3003', 3701800, (689.3, -121.4, -1079.1)), boss='c5150_0001 entity 3700800 at 724.9 -121.4 -1078.4',
  post=('Aldrich, Devourer of Gods', (732.9, -120.6, -1078.7)), start=('Anor Londo', 161),
  cs='none found for 13700800 (STATIC); the MSB labels of this boss\'s regions name Sulyvahn (the labels of the two m37_00 bosses are swapped, STATIC)',
  ph=['Phase 1: Aldrich with the bow and the sword; BGM regions 3704801 and 3704802 (heat-up) (STATIC)',
      'Phase 2: the giant form (INFERRED, common knowledge; the trigger is the boss AI)'],
  soul='Soul of Aldrich (CURATED)'),
 dict(n='Yhorm the Giant', kind='mandatory', tab='Irithyll Dungeon', m='m39_00', flag=13900800, key='yhorm',
  gate=('o000401_0000', 3901800, (122.2, -424.9, -612.4)), boss='c5260_0000 entity 3900800 at 133.9 -422.3 -536.0',
  post=('Yhorm the Giant', (131.1, -424.9, -554.9)), start=('Profaned Capital', 137),
  cs='39000020 (event 13905810, sets 13900801): the intro, INFERRED (STATIC call)',
  ph=['Phase 1: Yhorm; BGM regions 3904801 and 3904802 (heat-up); health bar template 20005837 names NPC 5260 (STATIC)',
      'Phase 2: he rages at half health (INFERRED, common knowledge)'],
  soul='Soul of Yhorm the Giant (CURATED)'),
 dict(n='Dancer of the Boreal Valley', kind='mandatory', tab='High Wall of Lothric', m='m30_00', flag=13000890, key='dancer',
  gate=('o000401_4000', 3001890, (27.6, -8.6, 130.1)), boss='c5270_0001 entity 3000899 at 28.2 -9.8 158.1',
  post=('Dancer of the Boreal Valley', (22.9, -9.9, 156.7)), start=('High Wall of Lothric', 160),
  cs='30000020 (event 13005880, waits on 13000885 and 13000890) and 30000010 (event 13005899, waits on 13000890, sets 13005894): roles intro and post-kill INFERRED (STATIC calls)',
  ph=['Phase 1: the two swords; BGM regions 3002899 and 3002898 (heat-up, labelled E3 version stalker) (STATIC)',
      'Phase 2: the flame-sword stance (INFERRED, common knowledge); flag 13000885 appears in the intro event (STATIC id)'],
  soul='Soul of the Dancer (CURATED)'),
 dict(n='Oceiros, the Consumed King', kind='optional', tab='High Wall of Lothric', m='m30_00', flag=13000830, key='oceiros',
  gate=('o000401_5000', 3001830, (-75.6, -49.4, 367.2)), boss='c2090_0000 entity 3000830 at -85.0 -50.0 409.9',
  post=('Oceiros, the Consumed King', (-88.9, -48.6, 428.9)), start=('Tower on the Wall (no pre-boss bonfire in the zone)', 314),
  cs='30000040 (`s30_00_0040`, 8 cuts with subtitles; event 13005830 waits on 13000830 and sets 13000838): role post-kill INFERRED (STATIC call)',
  ph=['Phase 1: Oceiros with the sceptre; BGM regions 3002835 and 3002836 (heat-up); health bar template 20005837 names NPC 2090 (STATIC)',
      'Phase 2: the dragon-arm form after the heat-up (INFERRED, common knowledge)'],
  soul='Soul of Consumed Oceiros (CURATED)'),
 dict(n='Dragonslayer Armour', kind='mandatory', tab='Lothric Castle', m='m30_01', flag=13010800, key='dragonslayer',
  gate=('o000401_4000', 3011800, (-37.2, 59.7, 223.1)), boss='entity 3010800 not among the MSB parts (spawned), start region 3012800 at -40.0 53.6 223.9',
  post=('Dragonslayer Armour', (-103.2, 59.7, 223.0)), start=('Dragon Barracks', 110),
  cs='none found (STATIC)',
  ph=['Phase 1: the armour with shield and greatsword; BGM region 3012803 and heat-up 3012804 (STATIC)',
      'Phase 2: he drops the shield (INFERRED, common knowledge)'],
  soul='Soul of Dragonslayer Armour (CURATED)'),
 dict(n='Twin Princes (Lorian, Elder Prince and Lothric, Younger Prince)', kind='mandatory', tab='Lothric Castle', m='m34_01', flag=13410830, key='princes',
  gate=('o000401_0001', 3411830, (15.4, 147.1, 328.9)), boss='c5250_0000 entity 3410830 (Lorian) at 45.7 146.2 341.1, c5251_0000 entity 3410831 (Lothric) at 37.8 146.2 341.6, c5250_0001 entity 3410832 at 49.0 145.8 342.9',
  post=('Twin Princes', (44.7, 145.8, 344.3)), start=('Grand Archives', 221),
  cs='34010000 (event 13415831) and 34010010 (event 13415830, which sets the flag 13410831, entity 3412840): the second is the mid-fight cutscene (INFERRED from the flag it sets; STATIC call); the role of 34010000 (intro or post-kill) OPEN',
  ph=['Phase 1: both princes; BGM regions 3412833 and 3412834 (heat-up) (STATIC)',
      'Phase 2: Lothric is out and Lorian carries him and fights on (INFERRED); event 13415830 plays 34010010 and sets 13410831; the m34_00 map holds a BGM region labelled complete body 3402863 (STATIC)'],
  soul='Soul of the Twin Princes (CURATED)'),
 dict(n='Ancient Wyvern', kind='optional', tab='Archdragon Peak', m='m32_00', flag=13200800, key='namelessking (wrong name: see corrections; this key revives the Wyvern)',
  gate=('o000402_1000', 3201800, (-65.5, 29.0, 191.1)), boss='c3141_0000 entity 3200800 at -70.5 62.7 282.0, start region 3202800 at -60.0 28.0 200.0',
  post=None, start=('Dragon-Kin Mausoleum', 63),
  cs='32000030 (event 13200811 waits on 13200800 and sets 13200800, 9305, 6305): the Wyvern\'s death cutscene, INFERRED (STATIC call)',
  ph=['Single phase on the bridge; BGM region 3204801 (STATIC)'],
  soul=None),
 dict(n='Nameless King', kind='optional', tab='Archdragon Peak', m='m32_00', flag=13200850, key='wyvern (wrong name: see corrections; this key revives the Nameless King)',
  gate=('o320400_2000', 3201850, (59.1, 75.5, 312.1)), boss='entity 3205850 spawned (no MSB part), start region 3202850 at 57.0 76.0 309.5',
  post=('Nameless King', (64.4, 68.2, 320.1)), start=('Great Belfry', 59),
  cs='32000010 (event 13205861, after 13200850, sets 13205852) and 32000000 (event 13205441, tests 13200850): post-kill, INFERRED (STATIC calls)',
  ph=['Phase 1 on the dragon: NPC name 905030 King of the Storm; BGM region 3204851 labelled riding (STATIC)',
      'Phase 2 on foot: NPC name 905010 Nameless King; BGM region 3204852 labelled alone (heat-up) (STATIC)'],
  soul='Soul of the Nameless King (CURATED)'),
 dict(n='Champion Gundyr', kind='optional', tab='Cemetery of Ash', m='m40_00', flag=14000830, key='champion',
  gate=('o000401_1000', 4001800, (126.2, -64.1, 559.6)), boss='c5110_0001 entity 4000830 at 142.9 -68.9 594.6',
  post=('Champion Gundyr', (142.7, -68.9, 594.8)), start=('Untended Graves', 142),
  cs='none found (STATIC)',
  ph=['Phase 1: Champion Gundyr (NPC name 905115, STATIC); BGM regions 4004830 and 4004831 (heat-up), flags 14005835 and 14005836 (STATIC)',
      'Phase 2: the evil spirit tendrils (INFERRED, common knowledge)'],
  soul='Soul of Champion Gundyr (CURATED)'),
 dict(n='Soul of Cinder', kind='mandatory (final boss)', tab='Kiln of the First Flame', m='m41_00', flag=14100800, key='cinder',
  gate=('o000402_3000', 4101800, (-140.5, 1590.0, 41.1)), boss='c5280_0000 entity 4100800 at -123.8 1582.1 94.3',
  post=None, start=('The First Flame', 55),
  cs='ending events 14100100 and 14100101 play 41000060 and 41000070, and 41000000, 41000050 and 41000051 (STATIC, I-0069 ENDING-CREDITS-NATIVE.md)',
  ph=['Prelude BGM region 4104802, main BGM 4104800, heat-up 4104801 (STATIC regions)',
      'Phase 2: the style changes at half health (INFERRED, common knowledge); flags 14105805 and 14105806 (STATIC ids)'],
  soul='Soul of the Lords (CURATED)'),
 dict(n="Champion's Gravetender", kind='optional (DLC1)', tab='Painted World of Ariandel', m='m45_00', flag=14500860, key='friede (wrong name: see corrections; this key revives the Gravetender)',
  gate=('o000402_9500', 4501860, (-11.1, -94.2, -301.8)), boss='entity 4505860 spawned (no MSB part), start region 4502860 at -79.9 -113.3 -328.1',
  post=("Champion's Gravetender", (-34.7, -99.1, -329.7)), start=('Depths of the Painting', 45),
  cs='none found for 14500860 (STATIC)',
  ph=['Phase 1: the Gravetender; BGM regions 4504861 (before the heat-up) and 4504862 (after) (STATIC)',
      'Phase 2: Gravetender Greatwolf (NPC name 906030, STATIC) joins at the heat-up (INFERRED)'],
  soul=None),
 dict(n='Sister Friede', kind='mandatory (DLC1)', tab='Painted World of Ariandel', m='m45_00', flag=14500800, key='gravetender (wrong name: see corrections; this key revives Friede)',
  gate=('o000401_8000', 4501800, (-193.1, 27.3, -230.1)), boss='entity 4505800 spawned (no MSB part), start region 4502800 at -199.8 26.0 -230.3',
  post=('Sister Friede', (-237.9, 27.3, -229.9)), start=('Ariandel Chapel', 35),
  cs='45000000 (`s45_00_0000`, 6 cuts; event 14505810, sets 14500801): intro, INFERRED (I-0069); 45000010 (event 14505811, waits on 14500800, 12 cuts): death cutscene, INFERRED (STATIC calls)',
  ph=['Phase 1: Sister Friede alone (NPC name 906020, STATIC); BGM region 4504801 before the heat-up (STATIC)',
      'Phase 2: Father Ariandel joins (NPC name 906021, STATIC); BGM region 4504802 after the heat-up, flags 14505805 and 14505806 (STATIC ids)',
      'Phase 3: Blackflame Friede (NPC name 906022, STATIC); its trigger is the AI (INFERRED)'],
  soul='Soul of Sister Friede (CURATED)'),
 dict(n='Demon Prince', kind='mandatory (DLC2)', tab='The Dreg Heap', m='m50_00', flag=15000800, key='dregheap',
  gate=('o000402_9000', 5001800, (120.3, -86.6, 67.9)), boss='entity 5005800 spawned (no MSB part), start region 5002800 at 146.6 -326.2 51.8',
  post=('The Demon Prince', (178.9, -325.0, 65.0)), start=('Within the Earthen Peak Ruins', 14),
  cs='51000000 (event 15005100 waits on 15000800): the post-kill cutscene, INFERRED (STATIC call)',
  ph=['Phase 1: Demon in Pain and Demon from Below (NPC names 905021 and 905020, STATIC); BGM region 5004801 (before the heat-up)',
      'Phase 2: the fused Demon Prince (NPC names 905022 and 905023, STATIC); BGM region 5004802 (after), flags 15005805 and 15005806 (STATIC ids)'],
  soul='Soul of the Demon Prince (CURATED)'),
 dict(n='Halflight, Spear of the Church', kind='mandatory (DLC2)', tab='The Ringed City', m='m51_00', flag=15100800, key='m51a',
  gate=('o000402_9500', 5101800, (-381.1, -55.3, -321.0)), boss='entity 5105800 spawned (no MSB part), start region 5102800 at -382.2 -55.8 -313.7',
  post=None, start=('Shared Grave', 68),
  cs='none found for 15100800 (STATIC)',
  ph=['Phase 1 before and phase 2 after the heat-up: BGM regions 5104801 and 5104802, flags 15105805 and 15105806 (STATIC); NPC name 45000 Halflight, Spear of the Church (STATIC)'],
  soul=None),
 dict(n='Darkeater Midir', kind='optional (DLC2)', tab='The Ringed City', m='m51_00', flag=15100850, key='m51b',
  gate=('o000401_9502', 5101850, (-481.9, -83.0, -396.3)), boss='c6211_0000 entity 5100850 at -481.5 -272.4 -327.3, start region 5102850 at -482.0 -281.3 -393.6',
  post=('Darkeater Midir', (-486.1, -272.2, -347.3)), start=('Ringed Inner Wall', 42),
  cs='none found; template 20005839 passes a camera-switch region 5102870 at -479.2 -274.0 -371.8 (STATIC)',
  ph=['Phase 1: Midir at range; BGM region 5104851 (before the heat-up) (STATIC)',
      'Phase 2: after the heat-up flag 15105856, BGM region 5104852 (STATIC); the second form is INFERRED from common knowledge'],
  soul='Soul of Darkeater Midir (CURATED)'),
 dict(n='Slave Knight Gael', kind='mandatory (DLC2, final)', tab='The Ringed City', m='m51_01', flag=15110800, key='gael',
  gate=('o000400_1000', 5111800, (217.1, 527.0, 190.5)), boss='entity 5115800 spawned (no MSB part), start region 5112800 at 214.8 524.3 189.5, BGM regions at 249.9 469.3 39.3',
  post=('Slave Knight Gael', (264.1, 476.1, -40.5)), start=("Filianore's Rest", 8),
  cs='none found for 15110800 (STATIC)',
  ph=['Phase 1: BGM region 5114801 (STATIC)',
      'Phase 2: BGM region 5114802, flag 15115802 (STATIC ids)',
      'Phase 3: BGM region 5114803, flag 15115803 (STATIC ids); the three BGM phases are the only direct evidence of three phases'],
  soul='Soul of Slave Knight Gael (CURATED)'),
]


def fmt(p):
    return f'{p[0]:.1f} {p[1]:.1f} {p[2]:.1f}'


rows = []
nsub = 0


def add(*c):
    rows.append('| ' + ' | '.join(c) + ' |')


for i, b in enumerate(BOSSES, 1):
    rid = f'B{i:02d}'
    registered = b['key'] is not None
    if registered:
        revive = f'`test-revive.txt` `{b["flag"]}` or `-Bosses {b["flag"]}` (registered flag, boss-flags.json key `{b["key"]}`)'
    else:
        revive = f'`witness-flag.txt` `{b["flag"]} 0` (the flag is not among the 26 registered ones)'
    if b['gate']:
        g = b['gate'][2]
        pl = nearest_player(b['m'], g)
        gate = f'fog object `{b["gate"][0]}` (entity {b["gate"][1]}) at {fmt(g)}; nearest player-start part `{pl[0]}` at {fmt(pl[1])} ({pl[2]} m)'
    else:
        gate = 'no fog object within 45 m of the boss (STATIC, MSB)'
    tabx = TAB[b['tab']]
    sb = b['start']
    if b['post']:
        post = f'Bonfire created after the kill: {b["post"][0]} at {fmt(b["post"][1])} (template 20005500 with this flag)'
        postshort = f'the bonfire {b["post"][0]} appears'
    else:
        post = 'No bonfire is created by template 20005500 for this flag (STATIC)'
        postshort = 'no new bonfire'
    reach = (f'Eta `-Continue`: Travel tab {b["tab"]} (pilot x {tabx}, y 191), bonfire **{sb[0]}** ({sb[1]} m from the gate, '
             f'nearest bonfire of the map that is not a post-boss one; row not derived, see Gaps); revive: {revive}; walk to the gate, E; F6 within 25 m')
    add(rid, b['n'], b['m'],
        f'{b["kind"]}. Defeat flag **{b["flag"]}**. {gate}. Boss: {b["boss"]}. {post}.',
        reach,
        'STATIC (flag, entities, positions: dumps of the game\'s MSB and EMEVD in `evidence/I-0139/artifacts/static/bosses/`); name INFERRED where the notes say so')
    add(rid + 'a', b['n'] + ': entry', 'as ' + rid,
        f'The fog traversal at the gate, then the cutscenes: {b["cs"]}',
        f'As {rid}; teleport only to the player-start part named there (a `c0000_*` part), then walk; the game sets the fight-started flag itself',
        'STATIC for the fog object and the calls; each cutscene role as stated')
    nsub += 1
    letters = 'bcdef'
    for k, ph in enumerate(b['ph']):
        label = f'phase {k + 1}' if len(b['ph']) > 1 else 'the fight'
        add(rid + letters[k], b['n'] + ': ' + label, 'as ' + rid, ph,
            'F4 (invincible) while looking; the heat-up phases come from the boss AI, so only a real fight reaches them today',
            'STATIC ids and regions as written; the behaviour of each phase INFERRED')
        nsub += 1
    last = letters[len(b['ph'])]
    soul = b['soul'] if b['soul'] else 'no soul item in the curated data (OPEN)'
    add(rid + last, b['n'] + ': death', 'as ' + rid,
        f'Death animation, the victory banner (`MENU_VICTORY_ACHIEVED`, drawn art, its text is not in the FMGs read: INFERRED), the fog removed, {postshort}; reward: {soul}',
        'F6 while the fight is live (writes 0 HP to the strongest enemy within 25 m whose HP maximum is up to 20,000, a code comment: a boss above that is skipped, OPEN per boss); or revive then F6',
        'STATIC for the flag and bonfire; reward CURATED; banner INFERRED')
    nsub += 1

open('investigations/I-0139-game-tour/sheets/_bosses_rows.md', 'w', encoding='utf-8', newline='\n').write('\n'.join(rows) + '\n')
print(len(BOSSES), nsub)
