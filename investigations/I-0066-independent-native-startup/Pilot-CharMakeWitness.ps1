param([Parameter(Mandatory)][string]$Run,[ValidateRange(10,180)][int]$StaySeconds=50,[switch]$KeepOpen,[switch]$FacePresets,[switch]$FaceGrid,[switch]$MovieShots,[int]$SkipVk=0,[switch]$ColorSelect,[switch]$FaceDetail,[switch]$DeleteProfile,[switch]$OnClock)
# I-0066 (1173): announced run only, Thomas's save (restored by hash by the runner). Trace-driven version of
# Pilot-CharMakeThomas.ps1 for slow native frames: each step is retried until the trace shows it happened
# (ui_movie_first_advance and menu_window_close rows of game_bootstrap): the title, then the offline notices through
# Pilot-TitleContinue (I-0113 tranche 28), New Game until the creation screen
# (04_000_ChrMake_Top) or the opening movie (01_920_Movie, or natively the Bink's native_movie_open row) appears; screenshots, then window close.
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
# I-0113 tranche 28: the two offline notices through Pilot-TitleContinue (its pixel tests, its OK positions 428 and 451, retries;
# -MenuY 0: it stops at the title menu), in place of this pilot's 2026-10 steps, whose single OK click at (648, 472) misses both
# notices' OK buttons today: in i66-*-i113-t27-face no window closed after 01_010_MessageBox (the two vtable 43092088 closes are the
# logo screens', before 05_000_Title; the notices still close with 43029208, i66-native-2378-m32-archdragon-sweep).
$title=pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace none -MenuY 0 2>&1 | Select-Object -Last 1
if($LASTEXITCODE){ throw "the title pilot failed: $title" }; "$title"
# The fix review's M1: both notices' close rows (vtable 43029208; the logo screens close with 43092088) before going on, the second
# notice's OK (648, 451) clicked again every 5 s for up to 60 s.
function NoticeCloses{ if(-not (Test-Path -LiteralPath $trace)){ return 0 }; @(Select-String -LiteralPath $trace -Pattern '"event":"menu_window_close".*"vtable":43029208').Count }
$closeBy=[DateTime]::UtcNow.AddSeconds(60); while((NoticeCloses) -lt 2){ if(-not (Alive)){ throw 'game gone while waiting for the offline notices' }; if([DateTime]::UtcNow -gt $closeBy){ throw 'the second offline notice did not close' }; Start-Sleep 5; if((NoticeCloses) -lt 2){ P '-Action','click','-X','648','-Y','451' } }
Start-Sleep 3; P '-Action','shot','-Path',"$o/c1.jpg"
# I-0113 tranche 30: the click on New Game only selects it (as 1175's Face Presets entry: a click selects, E confirms): in
# i66-reference-i113-t30-face the menu stayed on the title for 120 s of clicks; E (0x45) after the click (i66-native-i113-t30-face:
# still on the title, New Game highlighted: E held 400 ms, as Face Presets' confirm, only when the click opened nothing); the screen after each try kept (newgame-try.jpg).
# Tranche 31: in i66-reference-i113-t30-face2 each try opened a 01_010_MessageBox that closed 1.8 s later (result 3), before the
# shot: the screen 600 ms after the click and after E kept too (newgame-click.jpg, newgame-box.jpg), to read the box (INFERRED:
# Thomas's save has no free slot).
# I-0113 tranche 31: -DeleteProfile (only with Run-PilotPair -TestSave, a hash-recorded copy, Thomas 2026-11-08 « Vas y »): a save
# whose ten profiles are full answers New Game with « Cannot create any more profiles. Delete an existing profile and create a new
# one? » (YES at (500, 438), NO default; newgame-box-original.jpg): YES, the first profile chosen with E, a confirmation's YES
# clicked where the box's was, a shot after each step (del-0..3.jpg); the copy is restored away at the run's end.
# In i66-*-i113-t31-face2 YES opened 05_010_ProfileSelect (« Select profile to delete »), E chose Alpha and the box « Delete selected
# profile and begin a new game? » came with NO highlighted; the click at (500, 438) missed its YES (416-440 on the shot, del-3.jpg):
# now (500, 427), then E held (not yet run).
# The final review's L7: the steps only after New Game opened a 01_010_MessageBox (the box), else skipped.
function Boxes{ if(-not (Test-Path -LiteralPath $trace)){ return 0 }; @(Select-String -LiteralPath $trace -Pattern '"name":"01_010_MessageBox"').Count }
$boxesBefore=Boxes
if($DeleteProfile){ P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','click','-X','648','-Y','586'; Start-Sleep 2; P '-Action','shot','-Path',"$o/del-0.jpg"
  if((Boxes) -le $boxesBefore){ "no box after New Game: the delete steps skipped" }else{
    P '-Action','click','-X','500','-Y','438'; Start-Sleep 3; P '-Action','shot','-Path',"$o/del-1.jpg"
    P '-Action','hold','-Vk','69','-Ms','400'; Start-Sleep 3; P '-Action','shot','-Path',"$o/del-2.jpg"
    # The fixes' step review's L3: the confirmation only once the profile list (05_010_ProfileSelect) and its second box opened.
    if(-not (Seen '"name":"05_010_ProfileSelect"') -or (Boxes) -lt $boxesBefore+2){ "no profile list or confirmation box: the confirmation skipped" }else{
    P '-Action','click','-X','500','-Y','427'; Start-Sleep 1; P '-Action','hold','-Vk','69','-Ms','400'; Start-Sleep 4; P '-Action','shot','-Path',"$o/del-3.jpg" }
    "delete steps done $([DateTime]::UtcNow.ToString('o')); creation or movie seen: $(Seen '("name":"(04_000_ChrMake_Top|01_920_Movie)"|"event":"native_movie_open")')"
    # I-0137 tranche 4 (i66-native-i137-t4-charmake2): after the deletion the game showed the Rekindled title for about 212 s (no
    # 01_920_Movie row) before 04_000_ChrMake_Top; the New Game clicks below gave up at 120 s. Wait for it, up to 300 s, first.
    $until=[DateTime]::UtcNow.AddSeconds(300); while([DateTime]::UtcNow -lt $until -and -not (Seen '("name":"(04_000_ChrMake_Top|01_920_Movie)"|"event":"native_movie_open")')){ if(-not (Alive)){ break }; Start-Sleep 2 }
    "after the wait $([DateTime]::UtcNow.ToString('o')); creation or movie seen: $(Seen '("name":"(04_000_ChrMake_Top|01_920_Movie)"|"event":"native_movie_open")')" } }
Until '("name":"(04_000_ChrMake_Top|01_920_Movie)"|"event":"native_movie_open")' { P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','click','-X','648','-Y','586'; Start-Sleep -Milliseconds 600; P '-Action','shot','-Path',"$o/newgame-click.jpg"; $w=[DateTime]::UtcNow.AddSeconds(5); while([DateTime]::UtcNow -lt $w -and -not (Seen '("name":"(04_000_ChrMake_Top|01_920_Movie)"|"event":"native_movie_open")')){ Start-Sleep -Milliseconds 400 }; if(-not (Seen '("name":"(04_000_ChrMake_Top|01_920_Movie)"|"event":"native_movie_open")')){ P '-Action','hold','-Vk','69','-Ms','400'; Start-Sleep -Milliseconds 600; P '-Action','shot','-Path',"$o/newgame-box.jpg"; Start-Sleep 1 }; P '-Action','shot','-Path',"$o/newgame-try.jpg" } 6 120 'New Game'
"new game $([DateTime]::UtcNow.ToString('o'))"
# 1250 -MovieShots: screenshots of the opening movie 10, 60 and 150 s after its screen opens (both modes play it).
# I-0137 tranche 5 (R's New Game run, 2026-11-12 16:57): the opening movie is a Bink with no 01_920_Movie row now (its row is
# native_movie_open, accepted by every wait above too); once the creation screen is seen there is no movie left to shoot: the
# shots are skipped and said so, not timed out.
if($MovieShots -and (Seen '"name":"04_000_ChrMake_Top"')){ "movie shots skipped: the creation screen already open" }
if($MovieShots -and -not (Seen '"name":"04_000_ChrMake_Top"')){ Until '"name":"01_920_Movie"|"event":"native_movie_open"' { } 2 60 'the movie screen'; $start=[DateTime]::UtcNow
    foreach($at in 10,60,150){ while(([DateTime]::UtcNow-$start).TotalSeconds -lt $at){ if(-not (Alive)){ "dead in the movie"; return }; Start-Sleep -Milliseconds 500 }
        P '-Action','shot','-Path',"$o/movie-$at.jpg"
        # 1256 -SkipVk vk (BINK-NATIVE K6): after the 10 s shot, that key held 200 ms; "skip" when the creation screen follows.
        if($SkipVk -and $at -eq 10){ P '-Action','hold','-Vk',"$SkipVk",'-Ms','200'; $t0=[DateTime]::UtcNow
            while(([DateTime]::UtcNow-$t0).TotalSeconds -lt 15 -and -not (Seen '"name":"04_000_ChrMake_Top"')){ Start-Sleep -Milliseconds 400 }
            "skip key ${SkipVk}: creation screen $(if(Seen '"name":"04_000_ChrMake_Top"'){'opened'}else{'not opened'}) after $([int]([DateTime]::UtcNow-$t0).TotalSeconds) s"
            P '-Action','shot','-Path',"$o/movie-skip.jpg"; if(Seen '"name":"04_000_ChrMake_Top"'){ break } } } }
# The original plays the opening movie first (about 200 s; clicks and Enter/E do not skip it): wait for the creation screen,
# so both modes close the window with Create Character open.
Until '"name":"04_000_ChrMake_Top"' { } 5 300 'the creation screen'
"creation $([DateTime]::UtcNow.ToString('o'))"
# I-0140 tranche 31 (D-0299) -OnClock: the creation's shots c2..c4 and the Face Detail's steps scheduled on the module's clock from
# the page's own first-advance row (its GetTickCount64 tick, the clock Pilot-Game's capture tick reads), in both modes: c<i> at the
# 04_000_ChrMake_Top row + <step> x (i - 1) s (no row: the former fixed waits, said so); the Face Detail: a focus click at (600, 15)
# and a press at (206, 410) open the 04_010_ChrMake_CommandList page, the Appearance press at (222, 410) at that page's row + 2 s
# (no row in 20 s: said, and pressed at once), no press on the 04_060_ChrMake_ChildCommandList page (the Appearance page frames the
# face; shot face-appearance.jpg when its row comes), its shots face-0 at that page's row + 14 s and face-1..3 (each after a zoom
# press) at + 20, 26, 32 s (no row in 15 s: the face shots skipped, said so). Each shot's line names its capture tick minus the
# row's. I-0140 tranche 38 (its final review's M2): a clock shot starts Pilot-Game 2.5 s ahead and waits inside it for its target
# tick (-AtTick), so the copy begins at the target, not after Pilot-Game's start; the line names the copy's start and late too.
function RowTick([string]$page){ if(-not (Test-Path -LiteralPath $trace)){ return $null }
    $t=@(Select-String -LiteralPath $trace -Pattern """event"":""ui_movie_first_advance"".*""name"":""$page""" -Raw | ForEach-Object { if($_ -match '"tick":(\d+)'){ [int64]$Matches[1] } })
    if($t.Count){ $t[-1] }else{ $null } }
function AtTick([int64]$at){ $cap=(Get-Date).AddSeconds(60); while([Environment]::TickCount64 -lt $at -and (Get-Date) -lt $cap -and (Alive)){ Start-Sleep -Milliseconds 50 } }
function ShotAt([string]$name,$row,[int64]$at=0){ $a=@('-Action','shot','-Path',"$o/$name"); if($at -gt 0){ $a+=@('-AtTick',"$at") }
    $line="$(pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a 2>&1)"; if($line -match '^gone '){ return "shot ${name}: game gone" }
    if($null -ne $row -and $line -match 'tick=(\d+) t0=(\d+)'){ $c=[int64]$Matches[1];$t0=[int64]$Matches[2]; "shot ${name}: capture tick $c, row +$($c-$row) ms (copy from +$($t0-$row) ms$(if($line -match 'late=(\d+)'){", late $($Matches[1]) ms"}))" }elseif($null -eq $row){ "shot ${name}: no row" }else{ "shot ${name}: no capture tick (Pilot-Game: $($line -replace '\x1b\[[0-9;]*m',''))" } }
$step=[Math]::Max(5,[int]($StaySeconds/3))
$top=if($OnClock){ RowTick '04_000_ChrMake_Top' }else{ $null }; if($OnClock){ "creation row tick $top" }
foreach($i in 2..4){ $at=0L; if($null -ne $top){ $at=$top+1000L*$step*($i-1); AtTick ($at-2500) }else{ Start-Sleep $step }; if(-not (Alive)){ "dead in New Game"; return }
    if($OnClock){ ShotAt "c$i.jpg" $top $at }else{ P '-Action','shot','-Path',"$o/c$i.jpg" } }
# The page names below (« Appearance » for (206, 410), « Face Detail » for (222, 410)) are the earlier tranches', not checked
# against the page rows; tranche 31's -OnClock rows: (206, 410) opens 04_010_ChrMake_CommandList, (222, 410) the Appearance page
# 04_060_ChrMake_ChildCommandList.
# 1175 -FacePresets: open the face grid (FaceSelectDialog, 15 ChrMakeGrid face views) from the "Face Presets" entry,
# screenshots, then back to the creation top (Esc).
if($FacePresets){
    # Face Presets (202, 343) is a list of ten presets (1175); the grid is Appearance (206, 410), then its first entry (MENUS-MAP 41).
    P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','click','-X','206','-Y','410'; Start-Sleep 2; P '-Action','hold','-Vk','69','-Ms','400'; Start-Sleep 5
    # A click only selects the entry (1175); confirm with E, then Enter, then a held click, until the grid's 128x128 Face
    # views are set up natively (more than the ten Load Game portraits' native_offscreen_scene_cpu rows).
    function GridViews{ @(Select-String -LiteralPath $trace -Pattern '"native_offscreen_scene_cpu","width":128,').Count }
    foreach($confirm in @({ P '-Action','hold','-Vk','69','-Ms','400' },{ P '-Action','hold','-Vk','13','-Ms','400' },{ P '-Action','click','-X','202','-Y','343'; Start-Sleep -Milliseconds 300; P '-Action','click','-X','202','-Y','343' })){
        if((GridViews) -gt 10){ break }; & $confirm; Start-Sleep 6 }
    "grid views: $(GridViews)"; Start-Sleep 6
    if(-not (Alive)){ "dead in Face Presets"; return }; P '-Action','shot','-Path',"$o/grid-1.jpg"
    Start-Sleep 10; P '-Action','shot','-Path',"$o/grid-2.jpg"
    P '-Action','hold','-Vk','27','-Ms','400'; Start-Sleep 6; if(-not (Alive)){ "dead leaving Face Presets"; return }; P '-Action','shot','-Path',"$o/grid-back.jpg" }
# 1180 -FaceGrid: Appearance (206, 410) -> Face Detail (222, 410 on the Appearance page) -> Similar Face (first entry):
# FaceSelectDialog's fifteen 128x128 ChrMakeGrid views (MENUS-MAP 49); presses held 400 ms for native frames.
if($FaceGrid){
    function GridRows{ @(Select-String -LiteralPath $trace -Pattern '"native_offscreen_scene_cpu","width":128,').Count }
    $before=GridRows
    P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','press','-X','206','-Y','410','-Ms','400'; Start-Sleep 6; P '-Action','shot','-Path',"$o/grid-appearance.jpg"
    P '-Action','press','-X','222','-Y','410','-Ms','400'; Start-Sleep 6; P '-Action','shot','-Path',"$o/grid-facedetail.jpg"
    P '-Action','press','-X','220','-Y','176','-Ms','400'; Start-Sleep 15; P '-Action','shot','-Path',"$o/grid-1.jpg"
    "grid views: before $before after $(GridRows)"; Start-Sleep 10; P '-Action','shot','-Path',"$o/grid-2.jpg" }
# Menu audit (MENU-TESTS-PREP 3.4 family 5): -ColorSelect: Appearance (206, 410) -> Base Skin Color (190, 343 on the Appearance
# page, facegrid5-1180 grid-appearance.jpg), the colour select page 04_030_ChrMake_ColorSelect (cache-as-bitmap: natively expect
# native_ui_display_offscreen_unavailable unless it is drawn); screenshots color-*.jpg.
if($ColorSelect){
    P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','press','-X','206','-Y','410','-Ms','400'; Start-Sleep 6; P '-Action','shot','-Path',"$o/color-appearance.jpg"
    P '-Action','press','-X','190','-Y','343','-Ms','400'; Start-Sleep 8; P '-Action','shot','-Path',"$o/color-1.jpg"
    "color select page: $(Seen '"name":"04_030_ChrMake_ColorSelect"')"; Start-Sleep 10; if(-not (Alive)){ "dead in the colour select"; return }; P '-Action','shot','-Path',"$o/color-2.jpg" }
# I-0113 tranche 27 (an SSS_Dither face close up, the face parts' materials #03 and #04): -FaceDetail: (206, 410) then (222, 410)
# (tranche 31's -OnClock rows: the command list, then the Appearance page 04_060), whose camera frames the face (facegrid5-1180
# grid-facedetail.jpg), shots face-appearance.jpg 6 s after the first press and face-0.jpg 8 s after the second; then the zoom-in icon
# (1214, 592, the view's lower right) pressed three times, a shot after each (face-<k>.jpg).
if($FaceDetail -and $OnClock){ $before=@(Select-String -LiteralPath $trace -Pattern '"name":"04_060_ChrMake_ChildCommandList"').Count
    # Tranche 31: the creation's command list (04_010_ChrMake_CommandList) opens only after an input, 3.5 to 4 s after this click and
    # press in its three series (the row at +34, +55 and +110 s of the creation's row, each just after the press): this first press
    # opens it and is not Appearance's. The Appearance press waits for that page's row (20 s at most), then comes at its row + 2 s.
    P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','press','-X','206','-Y','410','-Ms','400'
    $until=(Get-Date).AddSeconds(20); while((Get-Date) -lt $until -and (Alive) -and $null -eq (RowTick '04_010_ChrMake_CommandList')){ Start-Sleep -Milliseconds 400 }
    $list=RowTick '04_010_ChrMake_CommandList'; if($null -ne $list){ "command list row tick $list"; AtTick ($list+2000) }else{ "command list row not seen in 20 s" }
    # A press at the same point is not taken (the fourth series): the cursor moved, at (222, 410), as tranche 30's runs did, whose
    # page then framed the face with no further press.
    P '-Action','press','-X','222','-Y','410','-Ms','400'
    $until=(Get-Date).AddSeconds(15); while((Get-Date) -lt $until -and (Alive) -and @(Select-String -LiteralPath $trace -Pattern '"name":"04_060_ChrMake_ChildCommandList"').Count -le $before){ Start-Sleep -Milliseconds 400 }
    $row=RowTick '04_060_ChrMake_ChildCommandList'
    if(@(Select-String -LiteralPath $trace -Pattern '"name":"04_060_ChrMake_ChildCommandList"').Count -le $before){ "appearance row not seen in 15 s: the face shots skipped (the page's moment unknown)" }
    else{ "appearance row tick $row"; P '-Action','shot','-Path',"$o/face-appearance.jpg"
        AtTick ($row+14000-2500); if(-not (Alive)){ "dead in Face Detail"; return }; ShotAt 'face-0.jpg' $row ($row+14000)
        foreach($k in 1..3){ P '-Action','press','-X','1214','-Y','592','-Ms','400'; AtTick ($row+14000+6000*$k-2500); if(-not (Alive)){ "dead zooming"; return }; ShotAt "face-$k.jpg" $row ($row+14000+6000*$k) } } }
elseif($FaceDetail){
    P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','press','-X','206','-Y','410','-Ms','400'; Start-Sleep 6; P '-Action','shot','-Path',"$o/face-appearance.jpg"
    P '-Action','press','-X','222','-Y','410','-Ms','400'; Start-Sleep 8; if(-not (Alive)){ "dead in Face Detail"; return }; P '-Action','shot','-Path',"$o/face-0.jpg"
    foreach($k in 1..3){ P '-Action','press','-X','1214','-Y','592','-Ms','400'; Start-Sleep 4; if(-not (Alive)){ "dead zooming"; return }; P '-Action','shot','-Path',"$o/face-$k.jpg" } }
if($KeepOpen){ return }
P '-Action','click','-X','1265','-Y','15'; Start-Sleep 25
"alive after close: $(Alive)"
