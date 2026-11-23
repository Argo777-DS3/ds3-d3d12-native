# I-0140 (ROADMAP workstream 5 step 5, the spells' audit; STUDY-ATTUNE): every spell of a family cast once, natively then in the
# original, on a copy of Alpha's all-spells save at Firelink Shrine (alpha-allspells-pyro: the pyromancy flame in the right hand,
# Right arrow the next catalyst). Before each cast F11 (the module's attune tool, test-tools.flag and test-attune.txt placed here in
# the build folder and removed after) puts the family's next spell in slot 0, selects it and fills the FP; the casts filmed (-Video)
# and shot at -ShotsMs. One process at a time; offline, restored by hash (Run-AlphaCast). The ASUS by its key; -NotBefore HH:mm.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateSet('pyromancies','sorceries','miracles','sorceries2','miracles2','thomas','thomas-weapon','thomas-tools','thomas-buffs','thomas-arts','thomas-status','arts-1','arts-2','arts-3','arts-4','arts-shields','arts-catalysts','arts-bows','arrows','arts-stance','goods-thrown','goods-use','lead-crucifix','lead-havel','lead-rope','lead-crucifix2','lead-rope2','arts-farron','maggots','bows2','keeper-infusions','keeper-status','lead-rope3','keeper-status2','keeper-infusions2','bows3','lead-rope4','maggots2','crucifix3','keeper-status3','keeper-infusions3','bolts2','bows4','ember2','andre-infusions','andre-paint')][string[]]$Families=@('pyromancies','sorceries','miracles'),
    [ValidateSet('Native','Reference')][string[]]$Modes=@('Native','Reference'),[ValidateRange(0,40)][int]$First=0,
    [string]$NotBefore='',[string]$Screen='1920x1080@60',[string]$NativeFlags='',[string]$ReferenceFlags='',[string]$GraphicsLevels='')
$ErrorActionPreference='Stop'
$here=$PSScriptRoot;$repo=Split-Path -Parent (Split-Path -Parent $here)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$save='C:\Users\User\.ds3-i0039-backups\alpha-allspells-pyro-20261101-164410\DS30000.sl2';$hash='EF4634FDA017AC756EFF86A0F4002A902AFF1164ED5CBE12018ACDD2A8C1E786'
$switches=@{pyromancies=0;sorceries=1;miracles=2;sorceries2=1;miracles2=2}   # Right arrow presses from the pyromancy flame (I-0105: flame, staff, talisman); sorceries2 and miracles2: the second pass's lists
# Tranche 14 (the 25 spells left, Thomas 2026-11-16 09:46: his current Alpha, from a copy): « thomas » on thomas-alpha-cape-20261111-124806,
# the same bytes as his live save (SHA-256 2713CD6B…, read 2026-11-16 10:50); Intelligence, Faith and Attunement 50 (read from the
# save's slot); right hand Heretic's Staff, Pyromancy Flame, Sunlight Talisman (current), left Carthus Shield, Fists, Fists (current):
# Right arrow then Left arrow before cast 1 (the staff and the shield), Right arrow before cast 14 (the flame), Fire Surge (14)
# held 4 s, Right arrow before 15 (the talisman, Homeward), Homeward again (16: a press after cast 15, whose FP row may fall in the warp; Homeward is judged by its warp on the film).
# Carthus Flame Arc is not here (its step review's H1): the cast gate 140640890 tests a +0x2D bit 2 spell against the right hand's
# current weapon (EquipParamWeapon +0x102 bit 7; bit 3 the left hand's, +0x101 bit 5), and no copy holds a flame in the left hand. « thomas-weapon » on thomas-alpha-20261031-0851 (his Alpha of
# 2026-10-31, the same stats; the current one holds no melee weapon): right Claymore, left Talisman, Sorcerer's Staff (current),
# Fists, cast from the left hand; the five sorcery buffs, Left arrow, the four miracle buffs, Left arrow, the four again (whether
# the switch skips the Fists slot is not known: one of the two blocks holds the talisman), Blessed Weapon again (14, for 13's FP).
$plans=@{thomas=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-cape-20261111-124806\DS30000.sl2';hash='2713CD6BFEBC1D66B8A05D1BE99143923D831ABF88C4BF36074FA237963E2C6F';plan='1:k39;1:k37;14:k39;14:h4000;15:k39'}
    'thomas-weapon'=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-20261031-0851\DS30000.sl2';hash='AE8F53328EFBBC48F9233AD9E59BE2095F0F5A12FF122DEB53B775E8F1F884EF';plan='6:k37;10:k37';button='right'}
    # Tranche 15 (the module's F1 tool, test-effect.txt: L<id>/R<id> a weapon into a hand's current slot, s<id> a SpEffect on the player;
    # Thomas 2026-11-16 11:20: Carthus Flame Arc with a dagger in the right hand and the flame in the left). « thomas-tools » on the
    # current copy, cast from the left hand: F1 twice before cast 1 (the Dagger into the right hand, the flame into the left), Carthus
    # Flame Arc (1, and 2 for 1's FP), F1 before cast 3 (the Sunlight Talisman into the left hand), Homeward (3, last: its warp) and E
    # after its shots (the « Select destination » prompt). « thomas-buffs » on the current copy, the right hand's Dagger swung after each effect:
    # F4 (the invincibility test tool) and F1 twice before cast 1 (the Dagger, then the first effect), F1 before each later cast: the
    # resins' SpEffects (EquipParamGoods refId, STUDY-EQUIP-SPEFFECT: Charcoal Pine Resin 2100, its Bundle 2110, Gold Pine Resin 2120,
    # its Bundle 2130, Rotten Pine Resin 2150, Human Pine Resin 2170, Carthus Rouge 2180, Pale Pine Resin 2190) and the statuses'
    # candidates (103, 104, 102, 101: INFERRED from their stateInfo pairs). Its attune list only fills each cast's F11 row.
    'thomas-tools'=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-cape-20261111-124806\DS30000.sl2';hash='2713CD6BFEBC1D66B8A05D1BE99143923D831ABF88C4BF36074FA237963E2C6F';plan='1:k112;1:k112;3:k112;3:a69';button='right';effects=@('R1000000','L13400000','L13260000')}
    'thomas-buffs'=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-cape-20261111-124806\DS30000.sl2';hash='2713CD6BFEBC1D66B8A05D1BE99143923D831ABF88C4BF36074FA237963E2C6F';plan='1:k115;1:k112;1:k112;2:k112;3:k112;4:k112;5:k112;6:k112;7:k112;8:k112;9:k112;10:k112;11:k112;12:k112';effects=@('R1000000','s2100','s2110','s2120','s2130','s2150','s2170','s2180','s2190','s103','s104','s102','s101')}
    # Tranche 16 (Thomas 2026-11-16 11:23-11:29: the weapon arts with an effect of their own, the Moonlight Greatsword first, never seen
    # natively): « thomas-arts » on the current copy, the Moonlight Greatsword given (F7, test-give.txt) and put in the right hand
    # (F1), the left hand bare (Fists, its current slot): its art Moonlight Vortex with Shift and the right button (casts 1, 2), the
    # charged strong attack's wave with Shift and the left button held 4 s (3, 4: judged on the film, a strong attack may spend no FP),
    # the art again (5). The keys (Shift with
    # a button) are the game's keyboard defaults as the pilot knows them, INFERRED: the film shows which moves came.
    # Tranche 15's statuses (STUDY-EQUIP-SPEFFECT « The statuses »: a buildup row is itself the status SpEffect; 101 to 104 are cure rows (their -99999 amounts, READ; i140-e89 showed nothing for them)
    # 101 to 104 are cure rows): « thomas-status » on the current copy, F4 (invincible) and the Dagger first, then one row a cast
    # added directly (140888F20, no gauge): bleed 22019 (0.1 s), frost 24019 (14 s), poison 20019 (180 s), toxic 21019 (300 s),
    # the curse's endless row 33 last (INFERRED rows; the film decides).
    'thomas-status'=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-cape-20261111-124806\DS30000.sl2';hash='2713CD6BFEBC1D66B8A05D1BE99143923D831ABF88C4BF36074FA237963E2C6F';plan='1:k115;1:k112;1:k112;2:k112;3:k112;4:k112;5:k112';effects=@('R1000000','s22019','s24019','s20019','s21019','s33')}
    # Tranche 19 (Thomas 2026-11-17 00:11-00:14: the Corpse-grubs' maggots, which cause bleed and die when the player carries fire,
    # a torch in hand): « maggots » on the stats-99 copy, F4 (invincible) and F1 twice before cast 1 (the Dagger, then SpEffect 5100:
    # 80 s, its cycle 5101 every 0.06 s, stateInfo 261, SpEffectVfxParam 261 = FFX 4050, 4051 and 4053 (init, midst, finish) full body, commoneffects; READ in
    # SpEffectParam with the DS3 paramdef; that it is the Corpse-grubs' row is INFERRED: 5105, the same state with FFX 41000 to 41003
    # of the m45 bank, is Ariandel's, not drawn at Firelink), a swing, a second swing to watch them, then F1 puts the Torch (given) in
    # the left hand before cast 3: whether the maggots die is the film's.
    'maggots'=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-cape-stats99-20261116-1500\DS30000.sl2';hash='500C0479FF433D3BCAFEDA5F0AC53FB6976B29DFB25AE7804053A1AB1911822D';plan='1:k115;1:k112;1:k112;3:k112';effects=@('R1000000','s5100','L23000000');gives=@('w23000000');filler=3}
    'thomas-arts'=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-cape-20261111-124806\DS30000.sl2';hash='2713CD6BFEBC1D66B8A05D1BE99143923D831ABF88C4BF36074FA237963E2C6F';plan='1:k112;1:m16;1:b1;2:m16;2:b1;3:h4000;3:m16;4:h4000;4:m16;5:m16;5:b1';effects=@('R6320000');gives=@('w6320000')}}
# Tranche 16's batches « arts-1 » to « arts-4 » (arts-melee.txt: 118 weapons, one per art of its own, the Moonlight Greatsword
# apart), 30 a run (about 21 s a cast in i140-e89's swings: about 800 s of the run's 900), on the current copy with Strength,
# Dexterity, Intelligence and Faith 99 (thomas-alpha-cape-stats99-20261116-1500, New-StatsCopy.py: an art needs its weapon's
# requirements, its step review's H1; the game recomputes a stat written in memory, i140-e80): the batch's weapons the copy lacks given (F7,
# arts-give.txt), then a cast
# each: F1 puts it in the right hand (the left hand bare, Fists, its current slot), Shift and the right button use its art.
# The F11 list is filler (Soul Arrow): its press fills the FP the arts spend.
$artsGive=@(Get-Content -LiteralPath "$here/arts-give.txt" | Where-Object { $_ -match '^[0-9]+$' })   # the weapons the copy lacks
$artsList=@(Get-Content -LiteralPath "$here/arts-melee.txt" | Where-Object { $_ -match '^[0-9]+ [0-9]+ ' } | ForEach-Object { ($_ -split ' ')[1] })
if($artsList.Count -gt 120 -and @($Families | Where-Object { $_ -like 'arts-*' }).Count){ "arts-melee.txt: $($artsList.Count) weapons, more than the four batches of 30: the arts batches refused"; $Families=@($Families | Where-Object { $_ -notlike 'arts-*' }) }   # its step reviews' L5 and L4
for($b=1;$b -le 4;$b++){ $ids=@($artsList | Select-Object -Skip (30*($b-1)) -First 30); if(-not $ids.Count){continue}
    $plan=(1..$ids.Count | ForEach-Object { "$($_):k112;$($_):m16;$($_):b1" }) -join ';'
    $plans["arts-$b"]=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-cape-stats99-20261116-1500\DS30000.sl2';hash='500C0479FF433D3BCAFEDA5F0AC53FB6976B29DFB25AE7804053A1AB1911822D';plan=$plan;effects=@($ids | ForEach-Object { "R$_" });gives=@($ids | Where-Object { $artsGive -contains $_ } | ForEach-Object { "w$_" });filler=$ids.Count} }
# Tranche 17 (ROADMAP workstream 5 step 5's left list), on the stats-99 copy, from arts-t17.txt (« family word … »: R<id> a weapon
# into the right hand, L<id> into the left, A<id> an arrow, B<id> a bolt, Q<id> a goods into quick slot 0, selected, F1 a word
# each before the next cast; « cast » ends a cast and names its input: « art » Shift and the right button, « stance <ms> » Shift
# and the right button held, the left clicked ms after it (the Stance arts and the bows' arts: what spends the FP is the attack
# inside the art), « shot » the left button alone, « use » the R key (the selected quick item); « face » and « away » a tap of S or Z (tranche 21, Thomas 05:26: S turns the character toward the camera, Z away from it, the camera untouched); « grip » E held with the left click (tranche 19: the two-hand toggle of the right weapon: the Longbow then drawn and loosed in both renderers, OBSERVED in i140-t19); « after <vk> » ending a cast taps that key after its shots; « give » words for F7).
# arts-shields: each shield in the left hand with the Dagger in the right, then in the right hand with the left bare (which hand's
# art L2 takes beside a weapon is not read: the film says); arts-catalysts, arts-bows, arrows, arts-stance (the 17 arts that spent
# no FP in tranche 16 with a plain click); goods-thrown and goods-use (each goods used once from the quick slot).
# Its final review's L10: the file read only when one of its families is asked for.
$t17=@('arts-shields','arts-catalysts','arts-bows','arrows','arts-stance','goods-thrown','goods-use','lead-crucifix','lead-havel','lead-rope','lead-crucifix2','lead-rope2','arts-farron','bows2','keeper-infusions','keeper-status','lead-rope3','keeper-status2','keeper-infusions2','bows3','lead-rope4','maggots2','crucifix3','keeper-status3','keeper-infusions3','bolts2','bows4','ember2','andre-infusions','andre-paint')
$t17Lines=if(@($Families | Where-Object { $t17 -contains $_ }).Count){ @(Get-Content -LiteralPath "$here/arts-t17.txt" | Where-Object { $_ -match '^[a-z0-9-]+ ' }) }else{ @() }
foreach($line in $t17Lines){ $f,$rest=$line -split ' ',2
    if(-not $plans[$f]){ $plans[$f]=@{save='C:\Users\User\.ds3-i0039-backups\thomas-alpha-cape-stats99-20261116-1500\DS30000.sl2';hash='500C0479FF433D3BCAFEDA5F0AC53FB6976B29DFB25AE7804053A1AB1911822D';plan=@();effects=@();gives=@();filler=0;pending=@();names=@()} }
    # Its step review's M2: a « # » comment cut first, then every word checked before any run (the module's bound 0x0FFFFFFF;
    # a give word as the F7 parser takes it, so GiveConfirms counts only words the module gives; a stance's ms in 300 to 3000).
    $q=$plans[$f];$w=@(($rest -replace '#.*$','') -split ' ' | Where-Object { $_ })
    $id={ param($x) [long]($x -replace '^[A-Za-z]([0-9]+).*$','$1') -lt 0x0FFFFFFF }
    if($w[0] -eq 'give'){ foreach($x in @($w | Select-Object -Skip 1)){ if($x -notmatch '^[gwpa][1-9][0-9]{0,8}(x([1-9]|[1-9][0-9]))?$' -or -not (& $id $x)){ throw "arts-t17.txt: $x is not a give word" } }; $q.gives+=@($w | Select-Object -Skip 1); continue }
    if($w[0] -ne 'cast'){ foreach($x in $w){ if($x -cnotmatch '^([RLABQNPFSH][1-9][0-9]{0,8}|M1|I1|T[1-9][0-9]{0,2})$' -or -not (& $id $x) -or ($x -clike 'A*' -and ([long]$x.Substring(1) -lt 400000 -or [long]$x.Substring(1) -gt 403999)) -or ($x -clike 'B*' -and ([long]$x.Substring(1) -lt 404000 -or [long]$x.Substring(1) -gt 404999)) -or ($x -cmatch '^[RL]' -and [long]$x.Substring(1) -ge 400000 -and [long]$x.Substring(1) -le 404999) -or ($x -clike 'Q*' -and [long]$x.Substring(1) -ge 1000000)){ throw "arts-t17.txt: $x is not an F1 word" } }; $q.pending+=$w; continue }   # the module's refusals (its fourth step review's L3)
    # Its second step review's M1: « after <vk> » at a cast's end taps that key after its shots (E, 69, on Homeward Bone's prompt).
    # Tranche 18: « capture <ms> » at a cast's end asks F9 that many ms after the cast (Run-AlphaCast -CaptureAt k,ms,120; the key
    # goes down 0.5 to 1 s later, Pilot-Game's start, its time not recorded; the cast's shots taken after it): the effects' draw
    # rows, ffx_ps_image with -NativeFlags ffx-record natively and -ReferenceFlags ffx-draw-observe in the original (the slot-27
    # hook, 22_setup.inc), native_ffx_draw_dump natively; one a family, refused without those flags.
    if($w.Count -ge 2 -and $w[-2] -eq 'capture'){ if($w[-1] -notmatch '^[0-9]{3,4}$' -or $q.capture){ throw "arts-t17.txt: $line" }; $q.capture="$($q.filler+1),$($w[-1]),120";$w=@($w | Select-Object -SkipLast 2) }
    $after=$null;if($w.Count -ge 2 -and $w[-2] -eq 'after'){ if($w[-1] -notmatch '^[0-9]{1,3}$' -or [int]$w[-1] -lt 1 -or [int]$w[-1] -gt 254){ throw "arts-t17.txt: $line" }; $after=[int]$w[-1];$w=@($w | Select-Object -SkipLast 2) }
    $want=if($w[1] -eq 'stance'){3}else{2};if($w.Count -ne $want -or ($w[1] -eq 'stance' -and ($w[2] -notmatch '^[0-9]{3,4}$' -or [int]$w[2] -lt 300 -or [int]$w[2] -gt 3000))){ throw "arts-t17.txt: $line" }
    $q.filler++;$k=$q.filler;$q.names+=$(if($q.pending.Count){$q.pending -join '+'}else{'(same hands)'})
    foreach($x in $q.pending){ if($x -ceq 'M1'){ $q.plan+="$($k):k4" }elseif($x -ceq 'I1'){ $q.plan+="$($k):k115" }elseif($x -clike 'T*'){ $q.plan+="$($k):l$($x.Substring(1))" }else{ $q.effects+=$x;$q.plan+="$($k):k112" } };$q.pending=@()   # tranche 20: M1 a middle click (Run-AlphaCast k4), not an F1 word; tranche 21: I1 F4 (k115), T<dx> a camera turn (l<dx>); tranche 22: H<id> an F1 word (the nearest character's HP back to its maximum)
    switch($w[1]){ 'art'{ $q.plan+="$($k):m16";$q.plan+="$($k):b1" } 'stance'{ $q.plan+="$($k):m16";$q.plan+="$($k):s$($w[2])" } 'shot'{ $q.plan+="$($k):b0" } 'use'{ $q.plan+="$($k):u82" } 'face'{ $q.plan+="$($k):u83" } 'away'{ $q.plan+="$($k):u90" } 'grip'{ $q.plan+="$($k):m69";$q.plan+="$($k):b0" } default{ throw "arts-t17.txt: $line" } }
    if($after){ $q.plan+="$($k):a$after" } }
# Tranche 19: the keeper-* families walk toward the Fire Keeper (Run-AlphaCast -HitKeeper, the walk's end placed at -AimAt, then the
# lock-on: DECALS-PAINT-READY's parameters, which never hit her there, Native2138-2141: she stands elsewhere at each load, so a hit
# is not assumed): keeper-infusions swings an infused weapon with F1's P1 probe before each swing (the nearest living character's HP
# and position: an HP drop between two probes of the same position is a hit, unless a status of that character took HP between them; after an applied H, F1's refill, the reader compares the next probe with the maximum, not with the probe before it: no code does); keeper-status adds a status by F1's N<id> to the nearest living
# character within 25 m, its HP, maximum and position in the row (the Fire Keeper or not: the row and the film).
foreach($f in @('keeper-infusions','keeper-status','keeper-status2','keeper-infusions2','bows3','keeper-status3','keeper-infusions3','bolts2','andre-infusions','andre-paint')){ if($plans[$f]){ $plans[$f].hitkeeper=$true } }   # tranche 20: bows3 too; tranche 22: its three; tranche 23: andre-infusions, placed by -AimAt 1.5 m from Blacksmith Andre's
# standing point (his NPC warp point 230.8 -56.4 596.7, I-0139 sheets/npcs-merchants.md) toward the bonfire (Thomas 09:46: « faut
# plutôt taper André, la firekeeper meurt trop vite »)
# Tranche 20 (its step review's M1): the families with M1 words make their own middle click after F1's F, so the walk's lock-on is
# left out for them (a second click would release a lock); their M1 is on their first F only, so a lock taken stays. Tranche 22's
# keeper-status3 makes no middle click and turns the camera instead (T500): the walk's lock-on is left out for it too.
foreach($f in @($plans.Keys)){ if($plans[$f].ContainsKey('pending')){ if($plans[$f].pending.Count){ throw "arts-t17.txt: $f has words after its last cast" }; $plans[$f].plan=$plans[$f].plan -join ';' } }
$flag=Join-Path $BuildFolder 'test-tools.flag';$list=Join-Path $BuildFolder 'test-attune.txt';$effectList=Join-Path $BuildFolder 'test-effect.txt';$giveList=Join-Path $BuildFolder 'test-give.txt';$made=@()
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device;if(-not $device){throw "no $Screen screen"};$env:DS3_PILOT_SCREEN=$device
    # Every family's list read and checked before the first run (its sixth step review's L5, its final review's L3): one word a
    # line, a spell's goods or Magic id with or without a leading g (the module's parser); the int= and fth= words are gone (the
    # game recomputes those stats, i140-e80: its final review's M1).
    $lists=@{}
    foreach($family in $Families){
        $words=if($plans[$family] -and $plans[$family].filler){@('g1300000')*$plans[$family].filler}else{@(Get-Content -LiteralPath "$here/attune-$family.txt" | ForEach-Object { $_.Trim() } | Where-Object { $_ })}   # tranche 16's batches: filler
        $badWords=@($words | Where-Object { $_ -notmatch '^[gG]?[1-9][0-9]{0,8}$' -or [long]($_.TrimStart('g','G')) -ge 0x0FFFFFFF })   # the module's bound (its review's L2)
        if($badWords){throw "attune-$family.txt: not a spell id: $($badWords -join ' ')"}
        $lists[$family]=@($words)
        # Its final review's M2: a family with a capture refused before any run unless both renderers' flags hook slot 27 (the whole
        # series refused, the other families with it: one call, one set of flags).
        if($plans[$family] -and $plans[$family].capture){ foreach($pair in @(@('Native',$NativeFlags,'ffx-record'),@('Reference',$ReferenceFlags,'ffx-draw-observe'))){
            if($Modes -contains $pair[0] -and @($pair[1] -split '[;,]') -notcontains $pair[2]){ throw "$family has a capture: its $($pair[0]) runs need $($pair[2]) in their flags" } } } }
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);$d=($at-(Get-Date)).TotalHours;if($d -lt -12){$at=$at.AddDays(1)}elseif($d -gt 12){$at=$at.AddDays(-1)};if(($at-(Get-Date)).TotalMinutes -lt -10){throw "-NotBefore $NotBefore is more than 10 minutes past (or more than 12 h ahead): refused rather than started early"}   # tranche 18 (its step and final reviews' M1): the HH:mm nearest now, within 12 h either side; more than 10 minutes past refused; after the lists' checks (its last step review's M1)
        while((Get-Date) -lt $at){Start-Sleep 5}}
    foreach($family in $Families){
        $ids=$lists[$family]
        # Its step review's L1 (tranche 14): a plan names casts by their place in the whole list, so -First does not cut a plan family.
        if($First -and $plans[$family]){"$family refused: -First with a cast plan";continue}
        if($First){$ids=@($ids | Select-Object -First $First)}
        foreach($mode in $Modes){
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$family $mode skipped: DarkSoulsIII already runs";continue}
            if((Test-Path -LiteralPath $flag) -or (Test-Path -LiteralPath $list) -or (Test-Path -LiteralPath $effectList) -or (Test-Path -LiteralPath $giveList)){"$family $mode refused: a test file already beside the module";continue}
            # Its step review's M1 (tranche 14): the family's save copy checked before any test file is written (a refusal leaves none).
            $p=$plans[$family];$s=if($p){$p.save}else{$save};$h=if($p){$p.hash}else{$hash}
            $got=$null;try{ $got=(Get-FileHash -LiteralPath $s -Algorithm SHA256 -ErrorAction Stop).Hash }catch{}   # its second step review's L4: an unreadable copy refused too
            if(-not $got -or $got -ne $h){"$family $mode refused: $s missing, unreadable or not at its hash";continue}
            New-Item -ItemType File -Path $flag -Value 'test' | Out-Null;$made=@($flag)
            Set-Content -LiteralPath $list -Value ($ids -join "`n") -NoNewline -Encoding ascii;$made+=$list
            if($p -and $p.effects){Set-Content -LiteralPath $effectList -Value ($p.effects -join "`n") -NoNewline -Encoding ascii;$made+=$effectList}   # tranche 15: the F1 tool's words
            if($p -and $p.gives.Count){Set-Content -LiteralPath $giveList -Value ($p.gives -join "`n") -NoNewline -Encoding ascii;$made+=$giveList}   # tranche 16: the F7 give tool's words
            $run="$Prefix-$family-$(if($mode -eq 'Native'){'nat'}else{'orig'})";"$(Get-Date -Format HH:mm:ss) $run ($($ids.Count) spells)"
            $a=@{RunName=$run;BootstrapMode=$mode;BuildFolder=$BuildFolder;SaveCopy=$s;SaveCopySha256=$h;Continue=$true;
                Casts=$ids.Count;AttuneEach=$true;CastEvery=6;Video=$true;Seconds=900;ShotsMs=@(400,900,1600,3000)}
            # Its step review's L4: the native side without the D3D12 debug layer, as the earlier cast series (a renderer as fast as in play).
            if($mode -eq 'Native'){$a.NoDebugLayer=$true}
            if($switches[$family]){$a.SwitchRight=$true;$a.SwitchRightTimes=$switches[$family]}
            if($p -and $p.capture){$a.CaptureAt=$p.capture}   # tranche 18 (its flags checked before any run, with the lists)
            if($p -and $p.hitkeeper){$a.HitKeeper=$true;$a.AimAt=$(if($family -like 'andre-*'){'232.3 -56.4 596.4'}else{'262.434 -55.177 602.014'});$a.LockOn=-not (@('keeper-status2','keeper-infusions2','bows3','keeper-status3','keeper-infusions3','bolts2','andre-infusions','andre-paint') -contains $family)}   # tranche 19: a walk toward the Fire Keeper (whether a swing hits her is the P1 probes' HP, not this setting); the families listed face by F1's f and a camera turn, no lock (andre-infusions: Andre beside the player)
            if($p){$a.CastPlan=$p.plan;if($p.button){$a.Button=$p.button};if($p.gives.Count){$a.GiveItems=$true;$a.GiveSeconds=10;$a.GiveConfirms=$p.gives.Count}}
            if($mode -eq 'Native' -and $NativeFlags){$a.EffectFlags=$NativeFlags}
            if($mode -eq 'Reference' -and $ReferenceFlags){$a.EffectFlags=$ReferenceFlags}   # tranche 18: ffx-draw-observe hooks the original's slot 27 (F9's ffx_ps_image)
            if($GraphicsLevels){$a.GraphicsLevels=$GraphicsLevels}   # tranche 21: e.g. the 1080p full screen (Thomas's film of the maggots); Run-AlphaCast's close click, made for the window's title bar, misses in full screen (its step review): the run then ends at -Seconds
            # tranches 21 and 22: maggots2's and crucifix3's teleport by the Cemetery of Ash's bonfire was applied, then released at
            # the start position (witness_teleport_released in startup.jsonl, tranche 22's final review's M3): removed; both ran in
            # Firelink Shrine.
            $lines=@();try{ & $cast @a *>&1 | ForEach-Object { $l="$_";$lines+=$l;$l } }
            finally{ foreach($f in $made){ Remove-Item -LiteralPath $f -Force -ErrorAction SilentlyContinue }; $made=@() }
            $j="$repo/evidence/I-0066/artifacts/$run/startup.jsonl"
            # Its step reviews' L3 and M1: the spells attuned and selected (the module's "attuned":true: set, slot 0 read back as the id,
            # slot 0 selected) against the family's count, and cast by cast (the runner's « attune k row=k' attuned=… » after each press:
            # a cast whose row is not its own, or not attuned, casts another spell; its sheet is wrong).
            $bad=@($lines | Where-Object { $_ -match '^attune ([0-9]+) row=(\S+) attuned=(\S+)$' -and ($Matches[1] -ne $Matches[2] -or $Matches[3] -ne 'true') })
            # Its final review's L6: a stance whose click time was not read is named (its shots and film index timed from its end).
            $untimed=@($lines | Where-Object { $_ -match '^cast ([0-9]+) .* untimed$' } | ForEach-Object { ($_ -split ' ')[1] })
            if($untimed){"  stance casts without their click time: $($untimed -join ' ')"}
            if($bad){"  casts without their own spell: $($bad.Count)";$bad | ForEach-Object { "    $_" }}else{"  every cast had its own spell (by the runner's rows)"}
            if(Test-Path -LiteralPath $j){ $rows=@(Select-String -LiteralPath $j -Pattern '"event":"test_attune' | ForEach-Object { $_.Line });$rows | ForEach-Object { "  $_" }
                $done=@($rows | Where-Object { $_ -match '"attuned":true' }).Count;$refused=@($rows | Where-Object { $_ -match '"row":false' }).Count
                "  attuned $done of $($ids.Count), $refused without a Magic row$(if($done -ne $ids.Count){': MISMATCH'})"
                # Its final review's L3: each attune row's id is the list's k-th (an index match alone would hide a shifted list).
                $shifted=@($rows | Where-Object { $_ -match '"event":"test_attune","k":([0-9]+),"listed":([0-9]+),"id":([0-9]+)' -and
                    ([int]$Matches[2] -ne $ids.Count -or [int]$Matches[1] -gt $ids.Count -or [int]$Matches[3] -ne [int]($ids[[int]$Matches[1]-1].TrimStart('g','G'))) })
                "  rows naming another spell than the list's: $($shifted.Count)$(if($shifted){': MISMATCH'})"
                # Its integrity review's F1: attuned is not cast. Each press's row reads the FP before the fill, so press k+1 at its
                # maximum means cast k spent none (not cast); the last cast has no next press (its sheet decides).
                $fp=@{};foreach($r in $rows){ if($r -match '"k":([0-9]+),.*"fp":(-?[0-9]+),"fp_max":(-?[0-9]+)'){$fp[[int]$Matches[1]]=@([int]$Matches[2],[int]$Matches[3])} }
                $notCast=@(1..([math]::Max(1,$ids.Count-1)) | Where-Object { $fp.ContainsKey($_+1) -and $fp[$_+1][0] -ge $fp[$_+1][1] } | ForEach-Object { "$($_):$(if($p -and $p.names){$p.names[$_-1]}elseif($family -like 'arts-*'){$p.effects[$_-1]}else{$ids[$_-1]})" })
                # Its second step review's L2: a cast whose next press wrote no FP row is named, not silently judged cast.
                $unjudged=@(1..([math]::Max(1,$ids.Count-1)) | Where-Object { -not $fp.ContainsKey($_+1) } | ForEach-Object { "$($_):$(if($p -and $p.names){$p.names[$_-1]}elseif($family -like 'arts-*'){$p.effects[$_-1]}else{$ids[$_-1]})" })
                if($unjudged){"  not judged by FP (no row for the next press): $($unjudged -join ' ')$(if($family -eq 'thomas-buffs'){' (thomas-buffs: swings, FP does not apply)'})"}
                "  not cast (no FP spent, by the next press): $($notCast.Count)$(if($notCast){' : '+($notCast -join ' ')}); the last cast not judged by FP$(if($family -eq 'thomas-buffs'){' (thomas-buffs: dagger swings spend no FP, this line does not apply)'}elseif($family -eq 'thomas-arts'){' (thomas-arts: casts 3 and 4 are strong attacks, judged on the film)'}elseif($family -eq 'arrows' -or $family -like 'goods-*'){" ($($family): shots and goods spend no FP, this line does not apply: judged on the film)"}elseif($family -like 'arts-*'){' (a weapon art: an art that spends no FP is judged on the film)'}elseif($family -eq 'thomas-status'){' (thomas-status: swings, FP does not apply)'}elseif(@('maggots','keeper-infusions','keeper-status','lead-rope2','lead-rope3','keeper-status2','keeper-infusions2','lead-rope4','keeper-status3','keeper-infusions3','bolts2','andre-infusions','andre-paint') -contains $family){" ($($family): swings, shots and goods spend no FP, this line does not apply: judged on the film and the rows)"}elseif($family -eq 'bows3'){' (bows3: casts 1, 2 and 4, a grip and shots, spend no FP; cast 3, the art, is judged by this line; cast 5, the last, has no FP row)'}elseif($family -eq 'bows4'){' (bows4: casts 1 and 4, a grip and a shot, spend no FP; casts 2 and 3, the art, are judged by this line)'}elseif($family -eq 'bows2'){' (bows2: casts 1, 2 and 4, a grip and shots, spend no FP; cast 3, the bow''s art, is judged by this line)'})"
                # Tranche 15's step review M3: the F1 tool's rows, each word in list order and applied (s) or equipped (L, R).
                if($p -and $p.effects){ $er=@(Select-String -LiteralPath $j -Pattern '"event":"test_effect","k"' | ForEach-Object { $_.Line })
                    $bad=@();for($w=0;$w -lt $p.effects.Count;$w++){ $want=$p.effects[$w];$row=@($er | Where-Object { $_ -match ('"k":'+($w+1)+',') } | Select-Object -First 1)
                        $ok=$row -and $row[0] -match ('"id":'+[int]$want.Substring(1)+',') -and $row[0] -match '"(applied|equipped)":true'
                        if(-not $ok){$bad+="$($w+1):$want"} }
                    "  F1 words: $($er.Count) rows for $($p.effects.Count) words$(if($bad){'; not applied or not in order: '+($bad -join ' ')+': MISMATCH'}else{', every word applied in order'})"
                    # Its second step review's M1: press by press (the runner's « effect <cast>-<i> <row> » lines), the w-th F1 press of the
                    # plan must have taken word w (its row's "k"); a press without a row, or with another word, names the casts after it.
                    $presses=@($lines | Where-Object { $_ -match '^effect [0-9]+-[0-9]+ ' -and $_ -notmatch ' pressed again$' })
                    $off=@();for($w=0;$w -lt $presses.Count;$w++){ if($presses[$w] -notmatch ('"k":'+($w+1)+',')){$off+=($presses[$w] -replace '^(effect \S+) .*$','$1')} }
                    "  F1 presses: $($presses.Count)$(if($presses.Count -ne $p.effects.Count -or $off){'; '+$(if($presses.Count -ne $p.effects.Count){"$($presses.Count) presses for $($p.effects.Count) words"}else{''})+$(if($off){' without their own word: '+($off -join ' ')}else{''})+': MISMATCH (every later cast suspect)'}else{', each took its own word'})" } }
            else{ "  attuned 0 of $($ids.Count): MISMATCH (no journal)" } } }
}catch{"ERROR $_"}
finally{ foreach($f in $made){ Remove-Item -LiteralPath $f -Force -ErrorAction SilentlyContinue }; "END $(Get-Date -Format HH:mm:ss)" }
