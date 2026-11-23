import math
# label, boss, flag, fog(x,y,z), fogname, bonfire name, tab, row, bonfire xz(x,z), note
R=[
('b01-iudex','Iudex Gundyr','14000800',(126.2,-64.1,559.6),'o000401_1000','Cemetery of Ash',263,313,(104.8,507.2),''),
('b02-vordt','Vordt of the Boreal Valley','13000800',(27.7,-25.5,44.9),'o000401_2000','High Wall of Lothric',323,280,(-19.2,-14.8),''),
('b03-greatwood','Curse-Rotted Greatwood','13100800',(136.3,-179.7,-882.1),'o000402_1000','Cliff Underside',383,346,(142.3,-759.7),''),
('b04-sage','Crystal Sage','13300850',(-165.0,-246.7,-422.8),'o000401_6001','Crucifixion Woods',443,346,(-68.7,-488.7),'second door o000401_6000 at -199.0 -245.5 -439.9 is 139 m from the bonfire vs 116 m: 6001 kept'),
('b05-watchers','Abyss Watchers','13300800',(281.1,-247.7,-486.3),'o000401_5000','Old Wolf of Farron (substitute)',443,478,(136.2,-340.4),'sheet names Farron Keep Perimeter (153 m), which has no TravelPress (not on Eta page); nearest same-map bonfire with one and not post-boss: Old Wolf of Farron, 206 m (Keep Ruins 219 m)'),
('b06-deacons','Deacons of the Deep','13500800',(-455.7,-196.8,-620.5),'o000400_9000',"Rosaria's Bed Chamber",503,379,(-416.1,-481.3),''),
('b08-wolnir','High Lord Wolnir','13800800',(367.0,-274.4,-672.6),'o000400_7000','Abandoned Tomb',563,346,(337.8,-723.7),''),
('b09-odk','Old Demon King','13800830',(293.2,-326.7,-923.8),'o000400_5000','Demon Ruins',563,412,(228.7,-881.0),''),
('b10-sulyvahn','Pontiff Sulyvahn','13700850',(398.5,-224.8,-1120.6),'o000402_3001','Church of Yorshka',623,346,(467.2,-1029.5),'second fog o000401_3001 (exit) is farther from the bonfire: 3001 entry kept'),
('b11-aldrich','Aldrich, Devourer of Gods','13700800',(689.3,-121.4,-1079.1),'o000401_3003','Anor Londo',623,478,(530.5,-1085.6),''),
('b12-yhorm','Yhorm the Giant','13900800',(122.2,-424.9,-612.4),'o000401_0000','Profaned Capital',683,313,(130.9,-736.4),''),
('b13-dancer','Dancer of the Boreal Valley','13000890',(27.6,-8.6,130.1),'o000401_4000','High Wall of Lothric',323,280,(-19.2,-14.8),''),
('b14-oceiros','Oceiros, the Consumed King','13000830',(-75.6,-49.4,367.2),'o000401_5000','Tower on the Wall',323,313,(-111.6,66.9),'no pre-boss bonfire in the zone; 314 m'),
('b15-dsarmour','Dragonslayer Armour','13010800',(-37.2,59.7,223.1),'o000401_4000','Dragon Barracks',743,313,(23.1,136.8),''),
('b16-princes','Twin Princes','13410830',(15.4,147.1,328.9),'o000401_0001','Grand Archives',743,379,(-157.7,219.6),''),
('b17-wyvern','Ancient Wyvern','13200800',(-65.5,29.0,191.1),'o000402_1000','Dragon-Kin Mausoleum',803,313,(-23.0,157.4),'no bonfire after the kill; bosses.md corrects the boss-flags key (namelessking key revives the Wyvern)'),
('b18-namelessking','Nameless King','13200850',(59.1,75.5,312.1),'o320400_2000','Great Belfry',803,346,(65.6,370.4),''),
('b19-champion','Champion Gundyr','14000830',(126.2,-64.1,559.6),'o000401_1000','Untended Graves',263,379,(-10.5,520.2),'same fog object as B01'),
('b20-cinder','Soul of Cinder','14100800',(-140.5,1590.0,41.1),'o000402_3000','Flameless Shrine (substitute)',863,280,(-111.6,-302.4),'sheet names The First Flame, not on Eta page (Z78); the Kiln tab has only Flameless Shrine, 346 m from the gate; Z77/Z78 teleports exist but are not Travel rows'),
('b21-gravetender',"Champion's Gravetender",'14500860',(-11.1,-94.2,-301.8),'o000402_9500','Depths of the Painting',923,478,(-45.8,-273.3),''),
('b22-friede','Sister Friede','14500800',(-193.1,27.3,-230.1),'o000401_8000','Ariandel Chapel',923,412,(-159.1,-231.8),''),
('b23-demonprince','Demon Prince','15000800',(120.3,-86.6,67.9),'o000402_9000','Within the Earthen Peak Ruins',983,346,(108.2,75.1),''),
('b24-halflight','Halflight, Spear of the Church','15100800',(-381.1,-55.3,-321.0),'o000402_9500','Shared Grave',1043,379,(-398.0,-267.3),'no bonfire after the kill'),
('b25-midir','Darkeater Midir','15100850',(-481.9,-83.0,-396.3),'o000401_9502','Ringed Inner Wall',1043,313,(-475.7,-397.4),''),
('b26-gael','Slave Knight Gael','15110800',(217.1,527.0,190.5),'o000400_1000',"Filianore's Rest",1043,478,(223.7,195.8),''),
]
# bonfire y not needed
tx=[];md=['| Label | Boss | Flag | Bonfire (tab,row) | Fog object (x y z) | Point | Distance gate-bonfire | Doubt |','|---|---|---|---|---|---|---|---|']
for l,b,fl,f,fn,bn,tab,row,(bx,bz),note in R:
    dx,dz=bx-f[0],bz-f[2];d=math.hypot(dx,dz)
    x=f[0]+2*dx/d;z=f[2]+2*dz/d;y=f[1]+0.2
    assert len(l)<20
    tx.append(f"{l}={tab},{row}|{x:.1f} {y:.1f} {z:.1f}|{fl}")
    md.append(f"| {l} | {b} | {fl} | {bn} ({tab},{row}) | {fn} ({f[0]} {f[1]} {f[2]}) | {x:.1f} {y:.1f} {z:.1f} | {d:.0f} m | {note or 'none'} |")
md.append("| (none) | B07 Stray Demon | 13300560 | Keep Ruins (sheet) | none within 45 m of the boss (bosses.md) | no line | n/a | No fog gate. Use instead: boss c5200_0000 at 180.9 -160.5 -240.2, or the nearest m33_00 player start (not named in the sheet); flag not among the 26 registered, so Run-I139Boss would use SetFlag. Flag candidate only. |")
open('tranche2-rows.txt','w',encoding='utf-8',newline='').write('\r\n'.join(tx)+'\r\n')
hdr="# I-0139 tranche 2 rows\n\nRule: 2.0 m from the fog object toward the chosen bonfire (horizontal), y = fog y + 0.2. Sources: sheets/bosses.md, sheets/zones-bonfires.md. Bonfire positions are the zones sheet's MSB positions. Generated by a Sonnet worker; unchecked.\n\n"
open('tranche2-rows.md','w',encoding='utf-8',newline='').write(hdr+'\n'.join(md)+'\n')
print('\n'.join(tx))
