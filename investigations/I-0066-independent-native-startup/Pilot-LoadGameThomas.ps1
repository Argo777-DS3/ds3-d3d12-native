param([Parameter(Mandatory)][string]$Run,[int]$Slot=-1,[int]$StaySeconds=40,[switch]$KeepOpen,[switch]$Summon,[string]$SummonLook="",[string]$Look='',[int]$WalkMs=0,[int]$Forward=0,[string]$Teleport="",[int]$TeleportWait=25,[int]$Phantom=0,[string]$PhantomList="",[int]$PressMs=1200,[int]$SettleMs=600,[int]$ConfirmKey=0,[switch]$InGameTaps,[switch]$EquipPage,[switch]$StatusPage)
# I-0066 (1178): announced run only, Thomas's save (restored by hash by the runner). Trace-driven like
# Pilot-CharMakeWitness.ps1: the offline notices through Pilot-TitleContinue (I-0113 tranche 28), Load Game until the list's
# portraits are set up (native_offscreen_scene_cpu 128 rows natively, a fixed wait in the original), screenshot of the
# list. With -Slot n (0-based row of the visible list), that save is loaded (click, then confirm) and the game is
# screenshot in-game after -StaySeconds; then the window is closed (unless -KeepOpen).
$ErrorActionPreference='Stop'
$d=$PSScriptRoot; $o=Join-Path (Split-Path -Parent (Split-Path -Parent $d)) "evidence/I-0066/artifacts/$Run"; $trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
function Seen([string]$pattern){ (Test-Path -LiteralPath $trace) -and [bool](Select-String -LiteralPath $trace -Pattern $pattern -Quiet) }
function Until([string]$pattern,[scriptblock]$act,[int]$every,[int]$timeout,[string]$what){
    $deadline=[DateTime]::UtcNow.AddSeconds($timeout)
    while(-not (Seen $pattern)){ if(-not (Alive)){ throw "game gone while waiting for $what" }
        if([DateTime]::UtcNow -gt $deadline){ throw "timeout waiting for $what" }
        & $act; $wait=[DateTime]::UtcNow.AddSeconds($every)
        while([DateTime]::UtcNow -lt $wait -and -not (Seen $pattern)){ Start-Sleep -Milliseconds 400 } } }
Until '"name":"05_000_Title"' { } 2 240 'the title movie'
Start-Sleep 3
# I-0113 tranche 28 (the final review's M2, as Pilot-CharMakeWitness): the offline notices through Pilot-TitleContinue (-MenuY 0:
# it stops at the title menu); the old single OK click at (648, 472) misses both notices' OK buttons today (428 and 451).
$title=pwsh -NoProfile -File "$PSScriptRoot/Pilot-TitleContinue.ps1" -Trace none -MenuY 0 2>&1 | Select-Object -Last 1
if($LASTEXITCODE){ throw "the title pilot failed: $title" }; "$title"
# The fix review's M1: both notices' close rows (vtable 43029208; the logo screens close with 43092088) before going on, the second
# notice's OK (648, 451) clicked again every 5 s for up to 60 s.
function NoticeCloses{ if(-not (Test-Path -LiteralPath $trace)){ return 0 }; @(Select-String -LiteralPath $trace -Pattern '"event":"menu_window_close".*"vtable":43029208').Count }
$closeBy=[DateTime]::UtcNow.AddSeconds(60); while((NoticeCloses) -lt 2){ if(-not (Alive)){ throw 'game gone while waiting for the offline notices' }; if([DateTime]::UtcNow -gt $closeBy){ throw 'the second offline notice did not close' }; Start-Sleep 5; if((NoticeCloses) -lt 2){ P '-Action','click','-X','648','-Y','451' } }
Start-Sleep 3
# I-0067 SCALEFORM-FILTERS-NATIVE L3: the title menu (logo and item drop shadows) before Load Game.
P '-Action','shot','-Path',"$o/title.jpg"
P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','click','-X','648','-Y','555'; Start-Sleep 15
P '-Action','shot','-Path',"$o/loadgame-1.jpg"; Start-Sleep 10; P '-Action','shot','-Path',"$o/loadgame-2.jpg"
if($Slot -ge 0){
    # Measured (1179): the row click opens "Start with selected profile?" (01_010_MessageBox); neither a lone click on OK,
    # E, Enter nor Space confirmed it natively; the one sequence that loaded (Native1179 argo2) was a click on OK followed
    # by E two seconds later. 1180: native frames last 130-170 ms: the row and OK are pressed 400 ms with the cursor placed first.
    # 1181: the load is TitleStep entering state 4 (new_game_mode 1); its destructor only runs once the area is loaded.
    # 1196 (I-0067 NATIVE-INPUT-LATCH): native Load Game updates take 0.67-0.84 s and sample the input once each, so the row
    # and OK presses last 1.2 s (a 400 ms press was lost in about 40 % of the tries).
    # -ConfirmKey vk: decide with that key (the keyboard path of the decide action, NATIVE-INPUT-LATCH D1-D2) instead of
    # the row and OK clicks; the list's highlighted row (the first) and the dialog's highlighted OK are taken.
    function RowPress{ if($ConfirmKey){ P '-Action','key','-Vk',"$ConfirmKey" } else { P '-Action','press','-X','648','-Y',"$y",'-Ms',"$PressMs",'-SettleMs',"$SettleMs" } }
    function OkPress{ if($ConfirmKey){ P '-Action','key','-Vk',"$ConfirmKey" } else { P '-Action','press','-X','500','-Y','427','-Ms',"$PressMs",'-SettleMs',"$SettleMs" } }
    $loaded='"title_step_state".*"state":4,'; $y=182+104*$Slot; P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; RowPress; Start-Sleep 6
    # 1183: the row press does not always open the confirmation natively: each try presses the row again (ignored behind an
    # open dialog), then OK.
    $k=0; while(-not (Seen $loaded) -and $k -lt 5){ $k++; if($k -gt 1){ RowPress; Start-Sleep 5 }; P '-Action','shot','-Path',"$o/confirm-$k.jpg"
        # 1243: the load (TitleStep state 4) comes up to 9 s after the OK pulse at native speed (Native1241): wait for it.
        OkPress; $until=(Get-Date).AddSeconds(20); while(-not (Seen $loaded) -and (Get-Date) -lt $until){ Start-Sleep 1 }
        "confirm try $k loaded=$(Seen $loaded)" }
    if(-not (Seen $loaded)){ 'no load'; P '-Action','click','-X','1265','-Y','15'; Start-Sleep 25; return }
    Start-Sleep $StaySeconds
    if(-not (Alive)){ 'dead after loading'; return }; P '-Action','shot','-Path',"$o/ingame-1.jpg"
    # 1253 -InGameTaps (I-0067 NATIVE-INPUT-LATCH NI4/NI5, in-game menu): a 50 ms Esc opens the pause menu and a second
    # one closes it (before the latch a 2.5 s hold was needed; in-game updates take about 200 ms), a 50 ms F tap (NI4), then forward held 3 s (NI5).
    if($InGameTaps){
        P '-Action','hold','-Vk','27','-Ms','50'; Start-Sleep 4; P '-Action','shot','-Path',"$o/esc-open.jpg"
        P '-Action','hold','-Vk','27','-Ms','50'; Start-Sleep 4; P '-Action','shot','-Path',"$o/esc-closed.jpg"
        P '-Action','hold','-Vk','70','-Ms','50'; Start-Sleep 3; P '-Action','shot','-Path',"$o/f-tap.jpg"
        # Forward is the key at QWERTY W's place: Z on this AZERTY keyboard (1253 held VK W, the key at QWERTY Z's place).
        P '-Action','hold','-Vk','90','-Ms','3000'; Start-Sleep 2; P '-Action','shot','-Path',"$o/w-held.jpg"
        "in-game taps done $([DateTime]::UtcNow.ToString('o'))" }
    # 1270 -EquipPage (I-0067 SCALEFORM-FILTERS-NATIVE PW3): Esc opens the menu on its highlighted Equipment entry, E (the
    # keyboard decide) opens the Equipment page (02_010_equiptop: drop shadows 2x2 p3 and 7x7 p3), screenshot, Esc closes.
    if($EquipPage){
        P '-Action','hold','-Vk','27','-Ms','50'; Start-Sleep 4; P '-Action','hold','-Vk','69','-Ms','400'; Start-Sleep 6
        P '-Action','shot','-Path',"$o/equip-page.jpg"; Start-Sleep 4; P '-Action','shot','-Path',"$o/equip-page-2.jpg"
        P '-Action','hold','-Vk','27','-Ms','50'; Start-Sleep 4; P '-Action','hold','-Vk','27','-Ms','50'; Start-Sleep 4
        if(-not (Alive)){ 'dead after equipment page'; return }; "equipment page done" }
    # I-0137 (A2's menu proxy witness) -StatusPage: Esc opens the menu on Equipment, two Right taps reach Status (the menu's first
    # row: Equipment, Inventory, Status), E opens it (the character's view, a menu proxy), screenshots; the page is left open, so the
    # run's close comes with a menu open (workstream 4 step 5: E602F0 once the HAL is withdrawn).
    if($StatusPage){
        P '-Action','hold','-Vk','27','-Ms','50'; Start-Sleep 4; P '-Action','shot','-Path',"$o/status-menu.jpg"
        foreach($i in 1..2){ P '-Action','hold','-Vk','39','-Ms','80'; Start-Sleep 1 }; P '-Action','shot','-Path',"$o/status-cursor.jpg"
        P '-Action','hold','-Vk','69','-Ms','400'; Start-Sleep 6; P '-Action','shot','-Path',"$o/status-page.jpg"; Start-Sleep 4; P '-Action','shot','-Path',"$o/status-page-2.jpg"
        if(-not (Alive)){ 'dead at status page'; return }; "status page done (left open)"
        # A run whose -Seconds ends here meets the controller's stop with the page open (I-0137: the stopped Close's kept thread).
        Start-Sleep 40; if(-not (Alive)){ 'gone while the status page was open'; return } }
    # 1207: -Teleport "x,y,z": the module moves the player there (witness-teleport.txt next to it, applied on the game thread
    # and logged as witness_teleport), then after -TeleportWait seconds a screenshot ingame-teleport.jpg; the looks follow.
    if($Teleport){ $file=Join-Path (Split-Path -Parent (Split-Path -Parent $d)) "evidence/I-0066/artifacts/game-bootstrap-build/witness-teleport.txt"
        $xyz=@($Teleport -split "[,;\s]+" | Where-Object { $_ }); if($xyz.Count -notin 3,4){ throw "Teleport needs x,y,z[,hold seconds]" }
        Set-Content -LiteralPath $file -Value ($xyz -join " ") -NoNewline -Encoding ascii
        $deadline=(Get-Date).AddSeconds(30); while(-not (Seen '"event":"witness_teleport"') -and (Get-Date) -lt $deadline){ Start-Sleep 1 }
        # The module removes the file whatever the outcome: success is the logged "applied":true (pre-merge review).
        if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force }
        if(Seen '"event":"witness_teleport","applied":true'){ "teleport applied" } else { "teleport not applied" }
        Start-Sleep $TeleportWait; if(-not (Alive)){ "dead after teleport"; return }; P "-Action","shot","-Path","$o/ingame-teleport.jpg" }
    # Phantoms: -Phantom id: the module forces the player's PhantomParam source (witness-phantom.txt, chr+0x1F38, logged as
    # witness_phantom), then ingame-phantom.jpg after 15 s; the looks follow.
    if($Phantom -gt 0){ $file=Join-Path (Split-Path -Parent (Split-Path -Parent $d)) "evidence/I-0066/artifacts/game-bootstrap-build/witness-phantom.txt"
        Set-Content -LiteralPath $file -Value "$Phantom" -NoNewline -Encoding ascii
        $deadline=(Get-Date).AddSeconds(30); while(-not (Seen '"event":"witness_phantom"') -and (Get-Date) -lt $deadline){ Start-Sleep 1 }
        if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force }
        if(Seen '"event":"witness_phantom","applied":true'){ "phantom applied" } else { "phantom not applied" }
        Start-Sleep 15; if(-not (Alive)){ "dead after phantom"; return }; P "-Action","shot","-Path","$o/ingame-phantom.jpg" }
    # Wave 1 last effects: -PhantomList "85,86,91": each id in turn (the module reads and deletes the file, a new id replaces the
    # previous one), a screenshot ingame-phantom-<id>.jpg 12 s after it is taken; 0 releases.
    foreach($id in @($PhantomList -split '[,;\s]+' | Where-Object { $_ })){ $file=Join-Path (Split-Path -Parent (Split-Path -Parent $d)) "evidence/I-0066/artifacts/game-bootstrap-build/witness-phantom.txt"
        Set-Content -LiteralPath $file -Value "$id" -NoNewline -Encoding ascii
        $deadline=(Get-Date).AddSeconds(30); while((Test-Path -LiteralPath $file) -and (Get-Date) -lt $deadline){ Start-Sleep 1 }
        if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force; "phantom $id not taken" } else { "phantom $id taken" }
        Start-Sleep 12; if(-not (Alive)){ "dead after phantom $id"; return }; P "-Action","shot","-Path","$o/ingame-phantom-$id.jpg" }
    # 1198: -Forward n: walk forward n ms (AZERTY Z, 5 s holds) without coming back, then a screenshot ingame-forward.jpg.
    if($Forward -gt 0){ $left=$Forward; while($left -gt 0){ $ms=[Math]::Max(50,[Math]::Min($left,5000)); P '-Action','hold','-Vk','90','-Ms',"$ms"; $left-=$ms }
        Start-Sleep 4; if(-not (Alive)){ 'dead after forward'; return }; P '-Action','shot','-Path',"$o/ingame-forward.jpg" }
    # 1184: -Look dx1,dx2..: relative camera turns (Pilot-Game look, pixels), each followed by a screenshot ingame-look-k.jpg
    # (non-player characters near Argo stand behind the default view).
    # Wave 1 last effects: an entry "dx:dy" also turns the camera vertically (negative dy looks up).
    $k=0; foreach($entry in @($Look -split '[;,\s]+' | Where-Object { $_ })){ $k++; $parts=@($entry -split ':'); $dx=$parts[0]; $dy=if($parts.Count -gt 1){$parts[1]}else{'0'}; P '-Action','look','-X',"$dx",'-Y',"$dy"; Start-Sleep 10; if(-not (Alive)){ 'dead after look'; return }; P '-Action','shot','-Path',"$o/ingame-look-$k.jpg" }
    # 1186: -WalkMs n: walk forward n ms (AZERTY Z), screenshot, then back the same time (S), for object velocity witnesses.
    if($WalkMs -gt 0){ P '-Action','hold','-Vk','90','-Ms',"$([Math]::Min($WalkMs,5000))"; P '-Action','shot','-Path',"$o/ingame-walk-1.jpg"
        P '-Action','hold','-Vk','83','-Ms',"$([Math]::Min($WalkMs,5000))"; Start-Sleep 4; if(-not (Alive)){ 'dead after walk'; return }; P '-Action','shot','-Path',"$o/ingame-walk-2.jpg" }
    # -Summon: Argo stands in front of an NPC summon sign (Thomas): interact (E), confirm (E), wait for the white phantom.
    if($Summon){ P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','hold','-Vk','69','-Ms','400'; Start-Sleep 4; P '-Action','shot','-Path',"$o/summon-1.jpg"
        P '-Action','hold','-Vk','69','-Ms','500'; Start-Sleep 20
        if(-not (Alive)){ 'dead after summon'; return }; P '-Action','shot','-Path',"$o/summon-2.jpg"
        Start-Sleep 15; P '-Action','shot','-Path',"$o/summon-3.jpg"
        # Wave 1 (Sword Master face): -SummonLook "dx[:dy],..": camera turns after the summon, a screenshot summon-look-k.jpg each.
        $k=0; foreach($entry in @($SummonLook -split '[;,\s]+' | Where-Object { $_ })){ $k++; $parts=@($entry -split ':'); $dy=if($parts.Count -gt 1){$parts[1]}else{'0'}
            P '-Action','look','-X',"$($parts[0])",'-Y',"$dy"; Start-Sleep 4; if(-not (Alive)){ 'dead after summon look'; return }; P '-Action','shot','-Path',"$o/summon-look-$k.jpg" } } }
if($KeepOpen){ return }
P '-Action','click','-X','1265','-Y','15'; Start-Sleep 25
"alive after close: $(Alive)"
