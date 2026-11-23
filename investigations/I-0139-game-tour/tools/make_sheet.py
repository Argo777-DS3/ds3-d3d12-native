import sys,os,re,json
sys.stdout.reconfigure(encoding='utf-8')
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
import ent
# Builds sheets/cutscenes-events.md rows from the EMEVD readings of this sheet (the row texts are written by hand below,
# the coordinates and the nearest player start are looked up in the MSB entity dump).
def at(i,mp):
    r=[x for x in ent.get(i) if x[0].startswith(mp[:6])] or ent.get(i)
    return r[0] if r else None
def P(i,mp):
    r=at(i,mp)
    if not r: return f"entity {i} (no recovered MSB holds it: OPEN)"
    x,y,z=float(r[3]),float(r[4]),float(r[5])
    s=ent.nearest_start(r[0],x,y,z)
    st=f"; nearest start `{s[1]}` `{s[2]:.1f} {s[3]:.1f} {s[4]:.1f}` ({s[0]:.0f} m)" if s else ""
    return f"{i} {r[1]} `{x:.1f} {y:.1f} {z:.1f}`{st}"
def clear(*f): return "clear "+", ".join(f"`{x}`" for x in f)
GEN="Replay: (1) set the flags with `witness-flag.txt` (`id 0`, one id per file, so one `Run-AlphaCast -SetFlag \"id 0\"` run per id; a boss defeat flag among the 26 is also cleared by `-Bosses`/`test-revive.txt`), (2) reload the map (rest at a bonfire or travel: the event instances read their flags at load), (3) `-Teleport \"x y z hold\"` to the trigger."
C=[];E=[]
def row(*a): C.append(a)
def ev(*a): E.append(a)
SRC="emevd/"  # the dumps: evidence/I-0139/artifacts/static/cutscenes-events/emevd/
row("30000030 `s30_00_0030`: Vordt's arrival (boss intro)","m30_00 High Wall of Lothric",
 "Remo cutscene, 3 cuts, 67 KB MQB. Event 13005810 plays 2002[4] 30000030 (modes 0 then 10) and 2002[3] 30000030,2; the player is warped to region 3002810 (`領域_フルアーマー獣_ポリ劇後ワープ地点`, 'Full-Armor Beast: warp point after the cutscene'). Full-Armor Beast is the code name in the region; that it is Vordt's intro is INFERRED (boss flag 13000800 = `vordt` in boss-flags.json, itself INFERRED).",
 f"The event ends at once if 13000800 is ON and skips the scene if 13000801 is ON (13000801 is set at its end by 2003[62]). {clear('13000800','13000801')}. Trigger region 3002815 {P(3002815,'m30_00')}; also 3002801 {P(3002801,'m30_00')}. {GEN}",
 "STATIC: `evidence/I-0139/artifacts/static/cutscenes-events/emevd/m30_00_00_00.txt` event 13005810; `Data1` `remo/s30_00_0030.remobnd.dcx`. Boss meaning INFERRED. I-0069 RW1")
row("30000040 `s30_00_0040`: boss scene with subtitles","m30_00",
 "Remo cutscene, 8 cuts, 55 KB MQB (3.1 MB entry), carries subtitles (`talkParamId`, I-0069 RW3). Event 13005830 plays 2002[3] 30000040 (0, 10, 2), no warp. Trigger region 3002830 (`領域_竜の御使い_開始領域`, 'Dragon's Herald: start region'). Boss registry: 13000830 = `oceiros` (INFERRED).",
 f"Ends if 13000830 ON; skips if 13000838 ON (set at the end). {clear('13000830','13000838')}. Region 3002830 {P(3002830,'m30_00')}. {GEN}",
 "STATIC: m30_00 event 13005830; census. Boss meaning INFERRED")
row("30000020 `s30_00_0020`: ladder-lowering scene","m30_00",
 "Remo cutscene, 6 cuts, 108 KB MQB (1.4 MB entry). Event 13005880: action prompt (text 3001895) at part `o302568_6000`, cutscene 30000020 (0, 10, 2) with warp to region 3002880 (`領域_E3版追跡者_梯子下ろすポリ劇後のワープ地点`, 'E3-version Pursuer: warp point after the ladder-lowering cutscene'), then `2005[3] 3001870` and `2009[0]` RegisterLadder. Shows a ladder being lowered; the actor is INFERRED to be the 'Pursuer' (region name).",
 f"Waits for 13000890 ON (3[0] 256,13000890; boss-flags.json: `dancer`); skips if 13000885 ON (set at the end). Set `13000890` to 1, {clear('13000885')}, reload, stand at the prompt part {P(3001896,'m30_00')}, press E. {GEN}",
 "STATIC: m30_00 event 13005880; meaning INFERRED from the region name")
row("30000010 `s30_00_0010`: the Pursuer scene (mode 8)","m30_00",
 "Remo cutscene, 4 cuts, 141 KB MQB. Event 13005899: waits for flag 13004890 ON, plays 2002[4] 30000010 (8 then 10) and [3] 2, warps to region 3002897 (`領域_E3版追跡者_ポリ劇後ワープ地点`, 'E3-version Pursuer: warp point after the cutscene'), removes object SFX 3001360-63, sets 13005894.",
 f"Ends at once if 13000890 is ON (so it plays only before that flag); skips if 13005894 ON. {clear('13000890','13005894')}; flag 13004890 must be ON (who sets it: OPEN). Region {P(3002897,'m30_00')}. {GEN}",
 "STATIC: m30_00 event 13005899; the setter of 13004890 OPEN")
row("31000010 `s31_00_0010`: descent from the High Wall to the Undead Settlement","m30_00 to m31_00",
 "Remo cutscene, 6 cuts (918 KB entry). Event 13005550 (m30_00): prompt (text 3001810) at helper `o000499_0000`, two generic dialogs (10012001, 10010170), 2002[9] 31000010 (ceremony arguments, point 3102515 `王城_ワープポイント`, area 31), then `2003[14] 31,3100974` and flag 13100330. A lift or gate (INFERRED from the pairing). Map change.",
 f"Skips if 13100331 ON. {clear('13100330','13100331')}, reload m30_00, stand at the helper {P(3001650,'m30_00')}, press E. Arrival {P(3100974,'m31_00')}. {GEN}",
 "STATIC: m30_00 event 13005550; lift meaning INFERRED")
row("31000000 `s31_00_0000`: carried across the Undead Settlement","m31_00 (also called from m33_00)",
 "Remo cutscene, 7 cuts, 101 KB MQB. Event 13105320: prompt (text 3100000) at NPC part `c1260_0005`, 2002[4] 31000000 with warp to region 3102270 (`領域_下男オンブによるワープ先`, 'warp destination by the servant Onbu'; who that is: OPEN). The same id is passed by m33_00's `2000[6] 20005341,13300384,3300384,31000000`.",
 f"Skips if 1598 ON (unnamed: OPEN); no seen flag set by the event. NPC {P(3100350,'m31_00')}; destination {P(3102270,'m31_00')}. {GEN}",
 "STATIC: m31_00 event 13105320; m33_00 `2000[6]` line")
row("32000010 `s32_00_0010`: Storm King fight start scene","m32_00 Archdragon Peak",
 "Remo cutscene, 6 cuts, 87 KB MQB. Event 13205861: 2002[4] 32000010 (8, 10) and [3] 2, warp to region 3202858 (`【ボス】嵐の王（単体）_ボス戦開始時ワープポイント`, 'Boss Storm King (alone): warp point at fight start'), sets 13205852. Storm King = the Nameless King (INFERRED). boss-flags.json names 13200850 `wyvern`, but this event and 32000020 treat 13200850 as the Storm King's flag: the registry's two Archdragon Peak names look swapped (INFERRED from the region names).",
 f"Ends if 13200850 ON. {clear('13200850','13205852')}, reload, enter the fog. Warp target {P(3202858,'m32_00')}. {GEN}",
 "STATIC: m32_00 event 13205861. Name swap INFERRED")
row("32000020 `s32_00_0020`: Storm King defeat scene (ceremony)","m32_00",
 "Remo cutscene, 7 cuts, 121 KB MQB. Event 13200862: 2002[9] 32000020 with ceremony arguments, warp to region 3202859 (`【ボス】嵐の王_終了時ワープポイント`, 'Storm King: warp point at the end') and guest point 3202857; sets 13200850, 9304, 6304, 13200855, 13200856.",
 f"Plays at the Storm King's death; it sets the boss flag itself and goto-tests 13200855/13200856. {clear('13200855','13200856','13200850')} and kill him (F6). Warp {P(3202859,'m32_00')}. {GEN}",
 "STATIC: m32_00 event 13200862")
row("32000030 `s32_00_0030`: scene ending inside the temple","m32_00",
 "Remo cutscene, 1 cut, 12 KB MQB. Event 13200811: 2002[4] 32000030 (mode 0), warp to region 3202809 (`【ワープポイント】神殿内部`, 'warp point: inside the temple'), sets 13200800, 9305, 6305. By the previous rows 13200800 is probably the Ancient Wyvern's flag (INFERRED).",
 f"Ends if 13200800 ON. {clear('13200800')}; the wait condition was not read (OPEN). Warp {P(3202809,'m32_00')}. {GEN}",
 "STATIC: m32_00 event 13200811")
row("32000000 `s32_00_0000`: Archdragon Peak ceremony scene","m32_00",
 "Remo cutscene, 4 cuts, 115 KB MQB. Event 13205441: while 13200850 is OFF and 13200445 ON, objects 3203850-53 off, `2002[10] 32000000` (ceremony 10), `2003[16]` multiplayer event 1, `2003[59] 32,0,10`, objects on, sets 13200446. The area's world state changes (what is drawn: OPEN).",
 f"Ends if 13200850 ON; waits 13200445 ON (the setter: OPEN). {clear('13200850','13200446')} and set `13200445` to 1. {GEN}",
 "STATIC: m32_00 event 13205441")
row("33000010 `s33_00_0010`: Farron boss intro","m33_00 Farron Keep",
 "Remo cutscene, 4 cuts, 102 KB MQB (488 KB entry). Event 13305810: region 3302800 (`領域_ボス戦闘開始`, 'boss fight start'), 2002[3] 33000010 (0, 10, 2), sets 13300801 (2003[62]) and toggles 13304810. Boss flag 13300800 = `watchers` (INFERRED).",
 f"Goto tests on 13300800 / 13300801; waits 13305805 ON. {clear('13300800','13300801')}. Region {P(3302800,'m33_00')}. {GEN}",
 "STATIC: m33_00 event 13305810")
row("33000000 `s33_00_0000`: Farron boss second scene","m33_00",
 "Remo cutscene, 8 cuts, 72 KB MQB. Event 13305814: NPC 3300801 gets SpEffect 12246, 2002[3] 33000000 (0, 10, 2), `2004[42]` warps it to region 3302802, sets 13305802. A phase scene (INFERRED).",
 f"Ends if 13300800 ON. {clear('13300800','13305802')}; the fight is needed (F4). {GEN}",
 "STATIC: m33_00 event 13305814")
row("33000020..23 `s33_00_0020..0023`: four-slot scene family, first cutscene of each slot","m33_00",
 "Four remo cutscenes (3 to 4 cuts, 14 to 54 KB MQB). `2000[0]` slots 0..3 start template event 13300412 with ids (33000020+k, 33000030+k, 33000040+k), flag 13300410, base flag 9412+k, regions 3301412+k, 3301416+k, 3303412+k, 3303416+k, 3303422+k. The template shows a prompt (text 9330), a short player warp (`2004[41]`), then plays three cutscenes (0, 10, 2) chosen by an event value (`3[12]`). Subject: OPEN.",
 f"Gate: 13300410 and the counter (OPEN). Region 3301412 {P(3301412,'m33_00')}. Not reproducible until the template's 29 parameters are read: the dump's `2000[0] 0,13300412,13300410,9412,3301412,3301416,3303412,3303416,3303422,33000020,33000030,33000040,0` gives them; the instruction conditions still need reading.",
 "STATIC ids and template; subject OPEN")
row("33000030..33 `s33_00_0030..0033`: four-slot family, second cutscene","m33_00","Same template (2 to 3 cuts, 11 to 27 KB MQB).","As the previous row.","STATIC ids; subject OPEN")
row("33000040..43 `s33_00_0040..0043`: four-slot family, third cutscene","m33_00","Same template (2 to 5 cuts, 28 to 34 KB MQB).","As the previous rows.","STATIC ids; subject OPEN")
row("34010010 `s34_01_0010`: Grand Archives boss intro","m34_01 Grand Archives",
 "Remo cutscene, 5 cuts, 70 KB MQB (321 KB entry). Event 13415830: regions 3412846 (`領域_闇の双皇子_テスト用　初戦`, test first fight) and 3412830 (boss start); 2002[4] 34010010 with warp to region 3412840 (`領域_闇の双皇子_ポリ劇後　プレイヤー位置`, 'Twin Princes: player position after the cutscene'), area argument 290. Boss flag 13410830 = `princes` (INFERRED).",
 f"Ends if 13410830 ON; skips if 13410831 ON (set at the end). {clear('13410830','13410831')}. Region {P(3412830,'m34_01')}; after the warp {P(3412840,'m34_01')}. {GEN}",
 "STATIC: m34_01 event 13415830")
row("34010000 `s34_01_0000`: Twin Princes second scene","m34_01",
 "Remo cutscene, 9 cuts, 73 KB MQB (506 KB entry). Event 13415831: 2002[3] 34010000 (0, 8, 2); NPC 3410832 gets its health bar, the princes' state changes (names 905250/905251).",
 f"Ends if 13410830 ON; waits 13415835 ON. {clear('13410830')}; needs the fight. {GEN}",
 "STATIC: m34_01 event 13415831; meaning INFERRED")
row("35000050 and 35000051 `s35_00_0050`/`0051`: Cathedral of the Deep to Ariandel","m35_00 to m45_00",
 "Two remo cutscenes (35000050: 4 cuts, 81 KB MQB, 820 KB entry; `s35_00_0051`: a 165 KB entry that the I-0069 census did not list, unread). Event 13505100 plays both with `2002[4]` and warps to region 4502100 (`イベント 聖堂教会からのワープ先`, 'event: warp destination from the cathedral') in m45_00; sets 73500304 and 14500001, clears 73500339. The DLC1 entry (INFERRED from the region name).",
 f"Waits 73500339 ON and 1623. Set `73500339` to 1; prompt region 3500725 {P(3500725,'m35_00')}. Arrival {P(4502100,'m45_00')}. See `endings-dlc.md` D rows. {GEN}",
 "STATIC: m35_00 event 13505100")
row("35000000 and 35000001 `s35_00_0000`/`0001`: Patches scene","m35_00",
 "Two remo cutscenes (3 and 2 cuts, 37 KB MQB each). Event 13500783: region 3502722 (`会話NPC：カットシーン再生`, 'talk NPC: play cutscene') and guest warp 3502725 (`会話NPC：パッチ カットシーン後ゲストワープ`, 'talk NPC: Patches, guest warp after the cutscene'); plays both as 2002[3] and 2002[4].",
 f"Waits 1361, 1375, 9017 (unnamed: OPEN); sets 73500271. Region {P(3502722,'m35_00')}; after {P(3502725,'m35_00')}. Needs Patches' questline state (OPEN). {GEN}",
 "STATIC: m35_00 event 13500783")
row("37000020 and 37000021 `s37_00_0020`/`0021`: two large scenes","m37_00",
 "Two remo cutscenes (6 and 5 cuts, 42 KB MQB each; entries 2.9 MB and 809 KB, the largest base files). Event 13705644 (a template, entity 0 in the dump): waits for a prompt (text 6290), plays 37000020 then 37000021, sets 13700644 and clears or sets flag ranges 1340-1359, 1560-1579. Subject OPEN.",
 f"Ends if 50006193 ON; waits 1564. {clear('13700644')}; entities unknown (OPEN). {GEN}",
 "STATIC: m37_00 event 13705644")
row("37000030 `s37_00_0030`: after Aldrich, warp to the High Wall","common event 710 (from m37_00)",
 "Remo cutscene, 1 cut, 17 KB MQB. Common event 710: player inside region 3702890 (`【ボス】エルドリッチ_ワープイベント問題用`, 'Boss Aldrich: for the warp event problem'), flag 13700002 set, then `2002[12] 37000030` with points 3002100 (`領域_ワープポイント_ハレックorエルドリッチ撃破時`, 'warp point when Halek or Aldrich is defeated') and 3002890 (`領域_追跡者_エルドリッチ+ハレック撃破後のセーブ地点`, 'Pursuer: save point after Aldrich + Halek defeated'), both in m30_00. 'Halek' is INFERRED to be the Profaned Capital boss.",
 f"Tests 13000896 / 13000890 (3[0] groups 264, 8), 9314 / 9318. Region {P(3702890,'m37_00')}; destination {P(3002890,'m30_00')}. {GEN}",
 "STATIC: `emevd/common.txt` event 710")
row("39000030 `s39_00_0030`: after Yhorm, warp to the High Wall","common event 710 (from m39_00)",
 "Remo cutscene, 1 cut, 17 KB MQB. Common event 710, second branch: region 3902890 (`【イベント】ハレック撃破時カットシーン判定領域`, 'event: Halek defeat cutscene test region'), flag 13900001 set, `2002[12] 39000030` to the same m30_00 points.",
 f"Region {P(3902890,'m39_00')}. Flags as the previous row. {GEN}",
 "STATIC: common event 710")
row("38000020 `s38_00_0020`: Catacombs boss defeat return","m38_00 Catacombs of Carthus",
 "Remo cutscene, 4 cuts, 54 KB MQB. Event 13800800: inside region 3802806 (`領域_死霊の王 ボス部屋全域`, 'King of the Dead: whole boss room') plays 2002[4] 38000020 (8, 10, 2) with return points 3802804 and guest 3802807 (`領域_死霊の王 撃破復帰ポイント`, 'defeat return point'); sets 13800800, 9315, 6315, 63800561, 13801800. 'King of the Dead' = INFERRED High Lord Wolnir.",
 f"Replay is the boss's death: {clear('13800800','13801800')} then kill (F6). Return {P(3802804,'m38_00')}. {GEN}",
 "STATIC: m38_00 event 13800800")
row("38000000 `s38_00_0000`: Catacombs boss intro","m38_00",
 "Remo cutscene, 3 cuts, 74 KB MQB. Event 13805809: prompt (text 9322) at part `o381053_7000`, 2002[4] 38000000 (8, 10, 2) to boss room points 3802802/3802803, sets 13805805.",
 f"Ends if 13800801 ON. {clear('13800800','13805805')}; prompt part {P(3801810,'m38_00')}. {GEN}",
 "STATIC: m38_00 event 13805809")
row("39000020 `s39_00_0020`: Profaned Capital boss intro","m39_00 Profaned Capital",
 "Remo cutscene, 6 cuts, 30 KB MQB (569 KB entry). Event 13905810: region 3902800 (`【ボス】ボスエリア`, 'boss area'), 2002[3] 39000020; sets 13900801, 13905812, 13900722, 13905730.",
 f"Goto tests 13900800 / 13905805 / 13900722 / 13905730. {clear('13900800','13900722','13905730','13900801')}. Region {P(3902800,'m39_00')}. {GEN}",
 "STATIC: m39_00 event 13905810; boss meaning INFERRED")
row("39000010 `s39_00_0010`: bridge warp scene to Archdragon Peak's start","m39_00 to m32_00",
 "Remo cutscene, 1 cut, 21 KB MQB. Event 13905900: region 3902900 (`【イベント】橋ワープイベント領域`, 'event: bridge warp event region'), 2002[4] 39000010 with warp to m32_00 region 3202900 (`【ワープポイント】スタート地点`, 'warp point: start point'). Whether the game ever plays it is OPEN.",
 f"No flag in the event. Region {P(3902900,'m39_00')}; arrival {P(3202900,'m32_00')}. Teleport into the region. {GEN}",
 "STATIC: m39_00 event 13905900")
row("40000040 `s40_00_0040`: first scene of a new character","m40_00 Cemetery of Ash",
 "Remo cutscene, 5 cuts, 50 KB MQB. Event 14000100: while map ceremony 0 of m40_00 is ON (`3[28]`), deactivates object 4006440, plays `2002[11] 40000040` at region 4002110 (`領域 スタート地点`, 'start point'), waits for `3[31]` (the cutscene finished), reactivates the object, sets 14000100. The post-creation scene (INFERRED).",
 f"{clear('14000100')} and the ceremony 0 state (OPEN how to set it); start point {P(4002110,'m40_00')}. A New Game is the true route (menus.md).",
 "STATIC: m40_00 event 14000100")
row("40000020 and 40000021 `s40_00_0020`/`0021`: Firelink to the Kiln","m40_00 to m41_00",
 "Two large remo cutscenes (7 cuts each, 433 KB MQB; entries 508 KB and 448 KB). Event 14005110: prompt (text 9101) at the bonfire object `o400100_1000`, `2002[4]` 40000020 then 40000021 with warp to m41_00 region 4102100 (`領域 拠点ワープ地点`, 'base warp point'); sets 14000110 and 14100000. The engine special-cases id 40000020 (`14065AB10`, I-0069). The Firelink tower and thrones scene before the Kiln: INFERRED.",
 f"Ends if 14000110 ON; waits 9210 / 74000123 / 1038 (OPEN). {clear('14000110','74000123')}; Firelink bonfire {P(4001950,'m40_00')}. Arrival {P(4102100,'m41_00')}. `endings-dlc.md`. {GEN}",
 "STATIC: m40_00 event 14005110")
row("41000060 and 41000070 `s41_00_0060`/`0070`: Soul of Cinder scenes","m41_00 Kiln of the First Flame",
 "Two remo cutscenes (3 and 2 cuts, 59 and 62 KB MQB). Event 14100100: prompt (text 9341) at helper `o000499_0000`, `2002[11] 41000060` (return point 4102110 `領域 インゲーム復帰地点`) then `2002[3] 41000070,2`; six hits and the fog toggled. Boss flag 14100800 = `cinder`.",
 f"Skips if 14105100; waits 14101100 ON. {clear('14100800','14100100')}. Prompt {P(4101100,'m41_00')}; return {P(4102110,'m41_00')}. {GEN}",
 "STATIC: m41_00 event 14100100")
row("41000000, 41000050, 41000051: the ending scenes","m41_00",
 "Three remo cutscenes (2 cuts / 61 KB MQB; 3 cuts / 117 KB each). Event 14100101 plays them with `2002[3] ,2` after prompt text 9342 at `o410100_3000`; sets flags 9920-9923 and 20-23. These belong to `endings-dlc.md` F rows.",
 f"Waits 14101100 ON. Replay: `endings-dlc.md` F01..F04 (`witness-flag.txt` `20 1`..`23 1`). Prompt {P(4101952,'m41_00')}.",
 "STATIC: m41_00 event 14100101")
row("45000000 `s45_00_0000`: Ariandel boss intro","m45_00 Painted World of Ariandel (DLC1)",
 "Remo cutscene, 6 cuts, 128 KB MQB (543 KB entry; `remo_dlc1:/`; I-0069 RW2). Event 14505810: prompt (text 9373) at `c6010_0001`, region 4502800 (`イベント ボス娘 開始領域`, 'event: boss girl start region'), 2002[4] 45000000 (0, 10, 2), warp to region 4502810 (`イベント ボス娘 初戦開始時プレイヤーワープ`, 'boss girl: player warp at first fight start'), sets 14500801.",
 f"Goto tests 14500800 / 14500801. {clear('14500800','14500801')}. Region {P(4502800,'m45_00')}; prompt {P(4500803,'m45_00')}. {GEN}",
 "STATIC: m45_00 event 14505810; I-0069 RW2")
row("45000010 `s45_00_0010`: Ariandel boss second phase","m45_00",
 "Remo cutscene, 12 cuts, 487 KB MQB (3.0 MB entry, the largest DLC1 file). Event 14505811: 2002[4] 45000010 with warp to region 4502812 (`イベント ボス娘 激昂時ホストプレイヤーワープ`, 'boss girl: host player warp at the enraged phase'); sets 14505802. A phase change of Sister Friede (INFERRED: `ボス娘`, 14500860 = `friede`).",
 f"Ends if 14500800 ON; waits 14505805 ON. {clear('14500800','14505802')}; needs the fight. Warp {P(4502812,'m45_00')}. {GEN}",
 "STATIC: m45_00 event 14505811")
row("45000030 and 45000031 `s45_00_0030`/`0031`","m45_00",
 "Two remo cutscenes (5 and 4 cuts, 80 and 72 KB MQB). Event 14505620 plays both (0, 10, 2), toggles flag 14505521 and object 4501622, sets 14505621 and 14500620. Subject OPEN.",
 f"Goto tests 14500620 / 14500743; waits 14505621. {clear('14500620','14505621')}. Object {P(4501622,'m45_00')}. {GEN}",
 "STATIC: m45_00 event 14505620")
row("51000000 `s51_00_0000`: arrival in the Ringed City","m50_00 to m51_00 (DLC2)",
 "Remo cutscene: `DLC2` entry `remo/s51_00_0000.remobnd.dcx` (1.47 MB; found by hash, its MQB not opened; path `remo_dlc2:/`). Event 15005100: prompt (text 9380) at helper `o000499_0000`, `2002[4] 51000000` with warp to m51_00 region 5102110 (`イベント 前半マップからのワープ`, 'event: warp from the first half map').",
 f"Waits 15000800 ON (boss-flags.json: `dregheap`). Set `15000800` to 1; prompt {P(5001100,'m50_00')}; arrival {P(5102110,'m51_00')}. `endings-dlc.md` D rows. {GEN}",
 "STATIC: m50_00 event 15005100; name by hash in `archive-remo-event-names.tsv`")
row("51010000 `s51_01_0000`: Ringed City scene to area 307","m51_00 to m51_01 (DLC2)",
 "Remo cutscene: `DLC2` entry `s51_01_0000` (1.14 MB). Event 15105500 (m51_00): prompt (text 9390) at NPC part `c6310_0000`, `2002[4] 51010000` with warp to point 5112110, area 307 (m51_01; its MSB is not in the recovered set: position OPEN); sets 15100500 and 15110001.",
 f"Skips if 15100500 is set. {clear('15100500')}; prompt part {P(5100890,'m51_00')}. {GEN}",
 "STATIC: m51_00 event 15105500")
row("51000010 `s51_00_0010`: Ringed City scene","m51_00",
 "Remo cutscene: `DLC2` entry `s51_00_0010` (571 KB). Event 15105713 (a template, entity 0 in the dump): `2002[4]` 51000010 (0, 10) and `2002[3]` 2; sets flag 1811 and clears the range 1800-1814.",
 f"Waits 1815 / 1810 (unnamed). {clear('1811')}; the trigger's parameters OPEN. {GEN}",
 "STATIC: m51_00 event 15105713")
row("51010005 `s51_01_0005`: Slave Knight Gael first intro","m51_01 (DLC2)",
 "Remo cutscene: `DLC2` entry `s51_01_0005` (573 KB; named by no earlier note). Event 15115810: entity 5110801 with a health bar (name 906200), `2002[4]` 51010005 (8, 10, 2, then 10 at 5112816), sets 15110801. Boss flag 15110800 = `gael`.",
 f"Ends if 15110800 ON; skips if 15110801 ON. {clear('15110800','15110801')} (`-Bosses gael`). Regions 5112800/5112801 are in m51_01 (MSB not recovered: OPEN). {GEN}",
 "STATIC: m51_01 event 15115810 (my dump; the I-0069 dump had no m51_01)")
row("51010010 `s51_01_0010`: Gael second phase","m51_01",
 "Remo cutscene: `DLC2` entry `s51_01_0010` (525 KB). Event 15115811: `2002[9]` 51010010 (8, 10, 2; ceremony arguments; points 5112805 / 5112806) and `2002[10]`; sets 15115802.",
 f"Ends if 15110800 ON. {clear('15110800','15115802')}; needs the fight. {GEN}",
 "STATIC: m51_01 event 15115811")
row("30000000, 30009000, 30009010, 30010000/10/20: no EMEVD call","m30_00 / m30_01 bundles",
 "Six `Data1` bundles with no `2002` instruction in any event file: `s30_00_0000` (7 cuts, 97 KB MQB, 975 KB entry), `s30_00_9000` (2 cuts, 16 KB), `s30_00_9010` (2.2 KB), and the three `s30_01_*` (2.2 KB each: bare fades or placeholders, INFERRED from size).",
 "Trigger OPEN (Lua, AI, ESD or game code). No flag replay. Smallest tool: a `witness-cutscene.txt` that calls the engine's request (`FUN_140CE2810`, REMO-CUTSCENES-NATIVE §2) with an id.",
 "STATIC existence (`archive-remo-event-names.tsv`); trigger OPEN")
row("34000000/10/20 and 34010020: no EMEVD call","m34_00 / m34_01",
 "`s34_00_0000` (9 cuts, 15 KB MQB, 269 KB entry), `s34_00_0010` and `0020` (1.7 KB, bare), `s34_01_0020` (2.2 KB).","As the previous row.","STATIC existence; trigger OPEN")
row("35000010, 37000000, 37000010, 38000010, 38000030: no EMEVD call","m35 / m37 / m38",
 "`s35_00_0010` (1.7 KB), `s37_00_0000` (bare fade, 2.2 KB), `s37_00_0010` (2.5 KB), `s38_00_0010` (3 cuts, 73 KB), `s38_00_0030` (2.2 KB).","As the previous row.","STATIC existence; trigger OPEN")
row("40000000/10/30/50/60/9000 and 41000010/30: no EMEVD call","m40_00 / m41_00",
 "`s40_00_0000` (2.7 KB), `0010`, `0030`, `0050` (2.2 KB), `0060` (1.7 KB), `s40_00_9000` (3 cuts, 23 KB, 128 KB entry), `s41_00_0010` and `0030` (2.2 KB).","As the previous row.","STATIC existence; trigger OPEN")
row("45000090 `s45_00_0090`: no EMEVD call; not in the I-0069 census","m45_00 (DLC1)",
 "`DLC1` entry of 72 KB, never opened (reason not recorded in I-0069).","As the previous row.","STATIC existence; content OPEN")
row("Opening movie `movie/fdp_opening.bik` (Bink)","title, New Game",
 "1920x1080, 4750 frames, 24 fps (197.9 s), 5 audio tracks; page `01_920_movie`, subtitles in caption slot 0; movie id 1 (INFERRED from the `movtae` table `FDP_Opening`).",
 "New Game route of `menus.md` M09 (`-MovieShots`, skip key). No event flag: the New Game chain requests movie id 1. The native Bink player is in main (`6d857eef`).",
 "RECORD: BINK-MOVIES-NATIVE.md §0, §4; STATIC file header `movie-headers.tsv`")
row("End roll `movie/fdp_endroll_ww.bik` (Bink)","any map (flag 20)",
 "13,002 frames at 30000/1001 fps (433.8 s): the credits; movie id 3 (INFERRED). `FDP_EndRoll_JP` is named in the `movtae` table but has no file in this install.",
 "`witness-flag.txt` `20 1` in a loaded world (`Run-AlphaCast -SetFlag \"20 1\" -Casts 0`), at least 520 s (`menus.md` M56, `endings-dlc.md` F05); a save copy only.",
 "RECORD: ENDING-CREDITS-NATIVE R5, Reference2064 / Native2065")
# ------------------------------------------------ world events
ev("Descent lift High Wall to Undead Settlement (WarpPlayer)","m30_00 to m31_00",
 "Event 13005550: prompt at helper `o000499_0000`, cutscene 31000010, then `2003[14] 31,3100974` and a map change. A lift or gate (INFERRED).",
 f"Travel to the Tower on the Wall bonfire (`zones-bonfires.md` Z07), teleport near the helper {P(3001650,'m30_00')}, press E. Repeat with `13100330`, `13100331` cleared. {GEN}",
 "STATIC m30_00 event 13005550. The instruction name `WarpPlayer` for `2003[14]` is CURATED (DarkScript3's DS3 EMEDF, soulsmods.github.io/emedf); its two int arguments here are read as area and region: INFERRED")
ev("Kiln: warp inside the Kiln (WarpPlayer 41,4100120)","m41_00",
 "Event 14105500: prompt (text 9343) at `o000100_1001`, a player rotation (`2004[14]`), then `2003[14] 41,4100120` (the region is the player start `c0000_0007`).",
 f"Object {P(4101101,'m41_00')}; destination {P(4100120,'m41_00')}. Reach the Kiln by its Travel row (`zones-bonfires.md`, Kiln tab).",
 "STATIC m41_00 event 14105500")
ev("Kiln to the Dreg Heap (WarpPlayer 50,5002110)","m41_00 to m50_00",
 "Event 14105510: prompt (text 9344) at `o000100_2000`, object 4106102 toggled, `2003[14] 50,5002110`, sets 14100510 / 14100512.",
 f"Object {P(4101102,'m41_00')}; arrival {P(5002110,'m50_00')}. Waits flag 14100511 (unnamed: OPEN). `endings-dlc.md` D rows.",
 "STATIC m41_00 event 14105510")
ev("Ariandel to the Dreg Heap (WarpPlayer 50,5002110)","m45_00 to m50_00",
 "Event 14505160: prompt (text 9344) at `o000100_9508` (entity 4501101), two `2005[3]`, `2003[14] 50,5002110`; waits flags 14500161 and 9322 (`endings-dlc.md` D03).",
 f"Object {P(4501101,'m45_00')}; arrival {P(5002110,'m50_00')}. `witness-flag.txt` `9322 1` then `14500161 1` (two runs).",
 "STATIC m45_00 event 14505160; RECORD endings-dlc.md D03")
ev("Cutscene warps (a scene, then a player warp: `2002[4]`/`[9]` with a point and an area)","m31, m35 to m45, m39 to m32, m40 to m41, m50 to m51, m51",
 "The scene is the transition: Undead Settlement (31000000), Cathedral of the Deep to Ariandel (35000050/51), Profaned Capital bridge to Archdragon Peak (39000010), Firelink to the Kiln (40000020/21), Dreg Heap to the Ringed City (51000000), Ringed City (51010000): rows C06, C18, C26, C28, C34, C35.",
 "As the cited C rows.","STATIC `2002[4]` lines in the dumps")
ev("Map ceremony 10 after Aldrich and Yhorm (SetMapCeremony on six maps, then m40_00)","m30_00, m30_01, m31_00, m33_00, m34_01, m35_00, m40_00",
 "Common event 230 sets ceremony 10 on m30_00, m30_01, m34_01, m31_00, m33_00 and m35_00 when flags 9314 and 9318 are both ON (9314 is set in m37_00 and 9318 in m39_00 in their boss-death handling: INFERRED Aldrich and Yhorm); common 231 sets ceremony 10 on m40_00 (condition `3[8] 2621697`); common 232 steps m31_00's ceremony through 0, 10, 20, 30 by event values. Ceremony objects use `2005[17]` and conditions `3[28]` (m40_00 dump lines 302, 357).",
 "Set `9314 1` then `9318 1` (two runs of `witness-flag.txt`), reload the map, look at Lothric Castle, the Undead Settlement, Farron Keep, the Cathedral, the Grand Archives and Firelink (Travel rows in `zones-bonfires.md`). What changes on screen: OPEN.",
 "STATIC `emevd/common.txt` events 230, 231, 232; the visible result OPEN")
ev("Ceremony-bound objects in Lothric Castle (49 `2005[17]`, ceremonies 10, 20, 30, 40, 110, 120, 130)","m30_01",
 "Event 13015470 (dump lines 526-565): objects 3011600.. such as `o302593_9601` (-20.6 30.3 -4.7) shown by ceremony id.",
 f"Object {P(3011600,'m30_01')}. Teleport there after setting `9314`/`9318` (previous row). Never seen in a run.",
 "STATIC m30_01_00_00.txt; meaning OPEN")
ev("Archdragon Peak ceremony and its four objects","m32_00",
 "Event 13205441 (C10): objects 3203850-53 off, the scene, `2003[59] 32,0,10`, objects on.",
 "As C10: set 13200445 ON, clear 13200850 and 13200446.","STATIC m32_00 event 13205441")
ev("Boss-arena map pieces after the Greatwood's death (`2012[1]` ActivateMapPart on 17 pieces)","m31_00 Undead Settlement",
 "Event 13105840 runs when 13100800 is ON: `2005[3]` the fog objects 3101801/02, hits 3104800/01, objects 3106451-54 and 3106460, and `2012[1] state 0` on 17 map pieces 3104850-68 (`m002350_7000`, ...; the pieces carry the map origin 50 -155 -580). Event 13105814 destroys `o312400_7006/7007/7008` (160.0 -186.2 -959.1, 142.7 -186.2 -923.8, 154.0 -186.2 -913.9); 13105811 handles piece 3104805. `2012[1]` is the only instruction that shows or hides a map piece in the DS3 EMEDF.",
 "Clear the Greatwood flag `13100800` (`-Bosses greatwood`) and set it back, reload m31_00, look at the arena and the bridge near `o312400_7006` (Undead Settlement Travel tab).",
 "STATIC m31_00_00_00.txt events 13105811, 13105814, 13105840; `object-ops-by-event.txt`")
ev("Fog walls and boss gates (`2005[3]` with `2006[4]` object effects)","every boss map",
 "Boss fog gates `o000401` and area fogs, toggled by events such as 13005890 (`o000401_4000` at 27.6 -8.6 130.1, 15 `2006[4]`). Described in PHANTOMS-SIGNS-FOGWALLS-NATIVE §3; left to the boss sheet.",
 "As the boss sheet.","RECORD I-0068 PHANTOMS-SIGNS-FOGWALLS-NATIVE")
ev("Animated map objects with a prompt (levers, doors, lifts, bridges: `2005[6]` SetObjactState, `2005[7]` ReproduceObjectAnimation)","m30_01, m32_00, m33_00, m35_00, m36_00, m37_00, m38_00, m39_00, m40_00, m45_00, m50_00",
 "Direct (non-template) witnesses, event, object, position: m30_01 13014242 `o300320_6000` (-0.7 56.6 211.3); m32_00 13205410 `o324051_1000` (-76.4 29.0 183.4) with `o320360_1000` (-68.6 29.0 186.5), 13205420 `o329250_6000` (-48.9 61.1 177.9), 13205430 `o320361_4000` (86.2 68.9 353.7), 13205435 `o320361_2000/2001` (-13.6 68.9 199.8 / 69.3 68.9 327.7), 13205440 `o324012_4000` (78.9 81.2 342.4); m33_00 13305408 `o331312_8000/8001` (136.7 -253.1 -323.2 / 127.7 -169.2 -319.1), 13305421 `o330052_5000` (347.8 -247.3 -499.5); m35_00 13500490 `o350700_2000` (-294.9 -207.2 -668.0, prompt text 9200); m36_00 13600500 `o361000_2000` (1126.6 -124.6 1331.8); m37_00 13700470 `o370508_3000` (605.9 -127.3 -1092.9, text 3701000), 13705532 `o371111_1000` (400.6 -238.2 -995.4); m38_00 13805400 `o389041_7000` (345.7 -283.5 -623.6), 13800500 `o381081_4000` (326.4 -288.8 -581.5); m39_00 13905445 `o390305_0000` (85.6 -331.6 -811.8); m40_00 14005442 `o400340_1002`, 14000621 `o400340_1001`, 14000816/42 `o400005_1000/1001` (171.7 -67.0 614.0, beside Iudex's doors: INFERRED the Cemetery's gate); m45_00 14505500 `o459001_7000`, 14505600 `o456300_6000`; m50_00 15005500 `o501000_1003`. Which is a lever, a lift or a bridge is OPEN: the EMEVD names the object and an animation id only. About 80 more instances run through `common_func` templates with parameters.",
 "Per object: Travel to the zone's nearest bonfire (`zones-bonfires.md`), `-Teleport \"x y z hold\"` at the object (teleport to the nearest `c0000_*` start in `object-ops-by-event.txt` when the object's own spot is not a floor), press E at the prompt. Telling the kinds apart needs the model's name: smallest tool = list each `oNNNNNN` model's asset name.",
 "STATIC: `object-ops-by-event.txt` (event, instruction, entity, MSB model, position); the kinds OPEN")
ev("Objects destroyed (`2005[1]`, `2005[8]`, `2005[19]`) with damaging objects (`2005[9]`) and restored (`2005[2]`)","m31_00, m34_01, m38_00, m45_00, m50_00, m51_00",
 "m38_00 13805553 / 13805557 destroy `o381070_6000` (374.0 -278.7 -504.7) and `o381070_4000` (325.8 -286.6 -547.8) with damaging objects (behaviours 5830/5831); m31_00 13105814 destroys `o312400_7006..08`; m45_00 14505610 destroys `o459150_6500` (-70.2 -37.1 -390.2) with behaviour 6200; m50_00 15005500 destroys `o501020_1000`, `o502070_1000`, `o502075_1000` (-65.2 71.5 186.0 and neighbours) and 15005510 `o501030_1000` (17.8 166.5 216.4); restores: m34_01 13415440 `o302530_0000..02`, m50_00 15005570, m45_00 14505699. A broken wall, floor or bridge: INFERRED.",
 "Teleport to the coordinates and trigger the event's own condition (not read for most: smallest tool = a condition listing per event from the dumps). First witness: the two Catacombs objects.",
 "STATIC: m38_00 13805552-57, m31_00 13105814, m45_00 14505610, m50_00 15005500/10/70, m34_01 13415440")
ev("Firelink Shrine: ceremony 10 and the Lords' thrones","m40_00",
 "Common event 231 sets m40_00 ceremony 10; m40_00 has two `3[28]` ceremony conditions and NPC templates 20005525/26, one of them with the Abyss Watchers' defeat flag 13300800 as a parameter. The thrones have no named instruction (map pieces or objects driven by a ceremony: OPEN).",
 "Firelink bonfire (`zones-bonfires.md` Z01, teleport `273.20 -56.22 606.28`); set the Lords' flags 13300800, 13700800, 13900800, 13410830 ON one per run, reload, look at the throne room; the Shrine Handmaid's chair view is `-Teleport \"254.8 -54.5 604.7 90\"` (LIVE-RUN-NOTES).",
 "STATIC common events; the thrones OPEN")
ev("A dragon or a giant attacking","m31_00, m32_00, m37_00 and others",
 "Not nameable from the EMEVD: attacks are character animations (`2003[18] ForceAnimationPlayback`, 1062 uses) on entities whose models are in the MSB (`cNNNN`) but whose names need the character table. RECORD: at Anor Londo the giant's rubble catches Gamma at `616.1 -128.6`.",
 "Anor Londo: Gamma's save, `-Teleport \"611.5 -129.6 -1042.4 30\"` (LIVE-RUN-NOTES; the point itself is dangerous). Other cases OPEN; smallest tool: the model-id to name table.",
 "RECORD LIVE-RUN-NOTES.md lines 88-93; the rest OPEN")
ev("Bells","none found",
 "No DS3 EMEDF instruction names a bell. A bell would be an object animation (`2005[7]`) with a sound (`2010[2]`); m31_00 13105930 and m37_00 13705520 combine prompt text 9600, two object effects and a `PlaySE` (candidates, unseen: INFERRED).",
 "Teleport to the entity of m31_00 event 13105930 (the dump has it) and look: not done.","INFERRED / OPEN")
ev("Ladder and gate scenery around Vordt","m30_00",
 "Event 13005890 removes the area fog object `o000401_4000` (27.6 -8.6 130.1) and animates `o300370_4000` (27.6 -8.6 130.3, `2005[6] 3001895,300370`) after 13000890; 13005830/31 animate `o000371_5000` (-92.1 -47.6 441.4) after 13000830.",
 f"Set 13000890 ON; teleport to {P(3001896,'m30_00')}.","STATIC m30_00 events 13005890, 13005830/31")
if __name__=='__main__':
    json.dump({'C':C,'E':E},open(os.path.join(os.path.dirname(os.path.abspath(__file__)),'_rows.json'),'w',encoding='utf-8'),ensure_ascii=False)
    print(len(C),len(E))
