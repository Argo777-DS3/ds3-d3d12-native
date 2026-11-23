[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[ValidateSet('Native','Reference')][string]$BootstrapMode='Native',
    [string]$EffectFlags='',[ValidateRange(120,900)][int]$Seconds=300,[ValidateRange(0,120)][int]$Casts=4,[ValidateRange(2,30)][int]$CastEvery=6,
    [int[]]$ShotsMs=@(400,900,1600),[ValidateRange(200,3000)][int]$PressMs=1200,[ValidateRange(0,800)][int]$HoldSeconds=0,[ValidateRange(0,120)][int]$HoldShotEvery=0,[string]$TestLanguage='',[int]$OfflineOkY=450,[switch]$TalkKeeper,[switch]$HitKeeper,[switch]$SwitchRight,[ValidateSet('left','right')][string]$Button='left',[switch]$NoDebugLayer,[string]$TravelPress='',[ValidateRange(10,180)][int]$ArriveSeconds=45,[ValidateRange(0,600)][int]$StaySeconds=0,[switch]$Measure,[switch]$NoKProfile,[ValidatePattern('^([A-Z][0-9]{2,4})?(,[A-Z][0-9]{2,4})*$')][string]$KeyPath='',[switch]$ResetCamera,[string]$Teleport='',[ValidateRange(1,30)][int]$TeleportWait=6,[int]$KeeperLook=70,[ValidateRange(0,3000)][int]$KeeperStepMs=700,[ValidateRange(500,5000)][int]$WalkMs=3000,[string]$SetFlag='',[string]$SetFlagAfterTravel='',[int[]]$FlagShots=@(10,30,60,120,240,420,480,540),[switch]$LockOn,[string]$AimAt='',[switch]$BonfirePages,[ValidateRange(0,3000)][int]$ApproachMs=0,[switch]$SweepBefore,[switch]$MenuTour,[switch]$OptionsTour,
    # Code cleanup (D-0191): a session's own build folder (Build-GameBootstrap -Out), passed to Run-StartupTrace, as Run-ZoneWalk.
    [string]$BuildFolder='',
    # I-0113 tranche 19 (Thomas, 2026-11-06 19:18: the OLED at 2560x1440 while it is on): Run-StartupTrace -GraphicsLevels, e.g.
    # « ScreenMode=WINDOW;Resolution-WindowScreenWidth=2560;Resolution-WindowScreenHeight=1440 » (restored by hash); Pilot-Game scales the clicks.
    [string]$GraphicsLevels='',
    # D-0193 (Thomas, 2026-10-19: Alpha placed and saved in front of the Fire Keeper): another save copy than the m45 zone save.
    [string]$SaveCopy='',[string]$SaveCopySha256='',[switch]$Continue,[ValidatePattern('^([0-9]{1,2},-?[0-9]{1,4},[0-9]{1,2})?$')][string]$LookSweep='',[ValidatePattern('^([0-9]{1,2},-?[0-9]{1,4},[0-9]{1,2})?$')][string]$LookSweepAfter='',
    # I-0141 (the ceremony's smoke, a PIX burst at a sweep second): -SweepKeyAt "s,vk" taps the key vk (119 = F8, pix-capture.flag) right
    # after the shot of second s of -LookSweep's first turn (the arrival's sweep).
    [ValidatePattern('^([1-9][0-9]?,1[12][0-9])?$')][string]$SweepKeyAt='',
    # I-0097 tranche 5 (D-0213: a character travelled to a zone and saved there, in its own save copy): after the travel and the
    # stay, Quit Game (which saves) and the active save kept as a test save under this label (Pilot-QuitAndKeepSave.ps1).
    [ValidatePattern('^([a-z0-9-]+)?$')][string]$KeepSaveLabel='',
    # I-0139 (the tour's boss rows): -KeySteps "vk,ms;w10;..." after -KeyPath: a virtual key tapped (ms 0) or held ms (50 to 5000, Pilot-Game's range), or w<seconds> a
    # wait with a shot every 5 s; a shot after each step (step-<i>.png). F4 115, F6 117, E 69, AZERTY Z 90.
    [ValidatePattern('^((w[0-9]{1,3}|T[1-9]?|[0-9]{1,3},(0|[5-9][0-9]|[1-9][0-9]{2}|[1-4][0-9]{3}|5000))(;(w[0-9]{1,3}|T[1-9]?|[0-9]{1,3},(0|[5-9][0-9]|[1-9][0-9]{2}|[1-4][0-9]{3}|5000)))*)?$')][string]$KeySteps='',
    # T in -KeySteps: the player placed at -Teleport2 "x y z hold" (witness-teleport.txt), e.g. near a boss once its fog is crossed (Thomas, 2026-11-13).
    # I-0139 tranche 4 (the NPCs, one process for a place's several NPCs): -Teleport2 may hold up to nine points « x y z hold|x y z hold|... »;
    # T<k> in -KeySteps takes the k-th, T the first.
    [ValidatePattern('^(-?[0-9.]+ -?[0-9.]+ -?[0-9.]+ ([1-9][0-9]?|[1-5][0-9]{2}|600)(\|-?[0-9.]+ -?[0-9.]+ -?[0-9.]+ ([1-9][0-9]?|[1-5][0-9]{2}|600)){0,8})?$')][string]$Teleport2='',   # the hold required (the final review's L3)
    # I-0139 tranche 3: -Cutscene "id [mode]" writes witness-cutscene.txt (the module plays that Remo cutscene, as EMEVD 2002[3]), after
    # the travel and the teleport, with a shot at each -CutsceneShots second (cutscene-<s>.png).
    [ValidatePattern('^((3[0-9]|4[0-9]|5[01])[0-9]{6}( (0|2|8|10))?)?$')][string]$Cutscene='',[int[]]$CutsceneShots=@(2,5,10,15,20,30,45,60),
    # I-0138 tranche 8g (D-0299): -CutsceneHolds "ms t1 t2 ..." (t a scene frame or p<percent> of the scene's length) writes
    # witness-cutscene-holds.txt before witness-cutscene.txt: the module holds the scene at each target for ms (the Remo cut's play
    # speed 0, its frame set to the target's) and the runner, reading the journal's witness_cutscene_hold rows, shoots each hold
    # -CutsceneHoldShotMs after its row (cutscene-hold-<index>.png) in place of -CutsceneShots' wall-clock shots, then drops
    # witness-cutscene-shot.txt: the module releases the hold on it (ms is only the hold's timeout). Each shot's start and end on the
    # runner's clock (.NET's TickCount64, QueryUnbiasedInterruptTime's, not the module's GetTickCount64: Make-I138HoldPairs.ps1's
    # header) go to cutscene-holds.tsv (step review H1: a shot proved inside its hold).
    [ValidatePattern('^([0-9]{3,5}( (p[0-9]{1,3}(\.[0-9]{1,2})?|[0-9]{1,6}(\.[0-9]{1,3})?)){1,32})?$')][string]$CutsceneHolds='',[ValidateRange(200,15000)][int]$CutsceneHoldShotMs=2500,
    # I-0141 tranche 26: -CutsceneHoldVariants "bits:ms bits:ms ..." (up to 16; bits the module's share variant: 1 the cutscene
    # sets' fill lights, 2 the glare family, 4, 8 and 16 the traced rays' sky misses, cache ambient and hits' local lights (the GI,
    # the specular rays on NRD's frames and the cache's update; the passes reading the cache through it), tranche 27's 32 the diffuse
    # ambient and 64 the specular ambient, tranche 28's 128 and 256 the fill lights and the sun out of the hits' light (natively), 512 the
    # original's instance SH census (a native run takes 0 to 511, a Reference run combinations of 1, 2, 32, 64 and 512 alone: checked here
    # by mode, as the module parses them, the step review's M1); 01_state.inc) shoots each
    # hold again once per entry after its first shot: witness-variant.txt written when the bits change, the module's witness_share_variant
    # row with those bits awaited, ms more, then cutscene-hold-<k>-v<bits>-<j>.png (the shot's ticks in cutscene-hold-variants.tsv);
    # back to 0 before the shot file releases the hold, which the holds' ms must leave room for. A return to 0 the module does not take
    # within 10 s stops the variants for the rest of the run, the file left for the module (the step review's M1: never a later hold's
    # base shot with a share still off); the comparator refuses a hold whose last variant row before it is not 0.
    [ValidatePattern('^([0-9]{1,4}:[0-9]{3,5}( [0-9]{1,4}:[0-9]{3,5}){0,15})?$')][string]$CutsceneHoldVariants='',
    # I-0105 (Thomas, 2026-10-28 22:55: Right arrow switches the right-hand catalyst, Up arrow the attuned spell): with n > 0, before
    # each cast after the first, Up arrow, and Right arrow instead every n casts (n spells tried per catalyst).
    [ValidateRange(0,12)][int]$SpellsPerCatalyst=0,[ValidateRange(0,6)][int]$SpellUpsFirst=0,[string]$SecondTravel='',[ValidatePattern('^(!.+)?$')][string]$SecondTravelNamed='',[ValidateRange(-1,30)][int]$SecondTravelNamedAfter=-1,
    # I-0105 (Thomas, 2026-10-29: frame by frame rather than screenshots at fixed times): -Video records the game's window with
    # ffmpeg (WORKFLOW Tools; gdigrab of the desktop over the window's rectangle, -VideoFps frames per second, hidden) from the first
    # cast to the last into casts.mp4, each cast's offset in the video printed ("cast k ... video_ms=").
    [switch]$Video,[ValidateRange(10,60)][int]$VideoFps=60,
    # I-0140 (gap 23, single frames judged): -VideoLossless records RGB without loss (libx264rgb, crf 0) in place of crf 16 4:2:0.
    [switch]$VideoLossless,
    # I-0113 tranche 15 (gap 21, the hair in a bonfire's warp): -VideoTravel records from the first -TravelPress to the arrival
    # into travel.mp4 (the warp's animation plays after the last press), as -Video records the casts.
    [switch]$VideoTravel,[switch]$VideoKeys,[switch]$VideoHold,
    # I-0132 R12e: -VideoTitle records the title screen into title.mp4, from FromSoftware's logo before it (the intro plays before the start notices) to the
    # Load Game list's shot (the title covered), through -TitleSteps.
    [switch]$VideoTitle,
    # I-0132 (Thomas, 2026-11-12 18:18: the title filmed with its sound): -TitleHoldSeconds waits at the title (after the offline notices,
    # before -TitleSteps) inside -VideoTitle's film; -AudioRecorder, scripts/audio/loopback_record.cpp built, records Windows' default
    # output (WASAPI loopback) beside each film into <film>.wav, then muxes both into <film>-sound.mp4 (the film itself kept).
    [ValidateRange(0,180)][int]$TitleHoldSeconds=0,[string]$AudioRecorder='',
    # I-0113 tranche 15: -ApproachKeys "vk,ms;vk,ms" keys held in order before resting (a character loaded beside its bonfire but
    # not in reach: Thomas's copies at Firelink, Q (81) steps left).
    [ValidatePattern('^([0-9]{2,3},[0-9]{1,4}(;[0-9]{2,3},[0-9]{1,4})*)?$')][string]$ApproachKeys='',
    # I-0105 (the control spells, 2026-10-30): -SwitchRight presses Right arrow this many times (the right hand's catalysts in
    # order: pyromancy flame, sorcerer's staff, talisman: 2 reaches the talisman from the flame).
    [ValidateRange(1,4)][int]$SwitchRightTimes=1,
    # I-0113 gap 20 (the System menu's Quit Game portrait black in play): after the load (and again after -TravelPress's arrival),
    # Esc, System, its last tab (Quit Game, the 512x256 portrait view) with two shots (quit-<when>-<k>.png), then Esc twice.
    [switch]$QuitPortrait,
    # I-0140 tranche 30 (the menu views left after tranche 28, D-0301): -StatusView, after -QuitPortrait's load opening, the in-game
    # menu's Status page (the character's StatusBody 560x768 and StatusFace 512x256 views) shot at the page's own moment (below);
    # -StatusSwitchVk, the key pressed for the page's « Y :Switch » prompt (the body view; F by default, witnessed in both modes in tranche 30's second slot; 0 none).
    [switch]$StatusView,[ValidateScript({$_ -eq 0 -or ($_ -ge 65 -and $_ -le 90)})][int]$StatusSwitchVk=70,   # a letter key or 0
    # I-0140 tranche 32 (tranche 31's L7: the Player views' held and neutral frames under the frame graph's executor): -StatusViewTimes n
    # opens, leaves and reopens the Status page n times (the shots of opening k > 1 named status<k>-1.png and so on).
    [ValidateRange(1,8)][int]$StatusViewTimes=1,
    # I-0140 tranche 37 (tranche 36's next step: the Player views' hard and blurred self-shadow in one run): -StatusBlurToggle, with
    # the effect flag menu-shadow-blur-toggle, writes menu-shadow-hard.now in the build folder before each even-numbered opening and
    # removes it before each odd one (the module reads it every 250 ms: blurred on openings 1, 3, ..., hard on 2, 4, ...), and after
    # each opening's switch shot flips it and shoots <stem>-switchflip.png 16 s after the page's row (the other state on the same opening). The file
    # is removed after the last opening (the later steps blurred, as in play) and in the outer finally on any error. In Reference
    # mode the file does nothing: its lines name the file only.
    [switch]$StatusBlurToggle,
    # I-0113 tranche 26 (gap 20's closing picture: an opening that logs a 512x256 native_offscreen_view_partial): -QuitPortraitTimes n
    # opens the Quit Game tab n times after the load (quit-open<k>-<1|2>.png from the second), each its own menu view.
    [ValidateRange(1,20)][int]$QuitPortraitTimes=1,
    # I-0113 tranche 33 (the portrait's own inputs; tranche 31's final review H1: the named oracle's 24,000 draws a name were spent
    # on the world before the menu opened): -QuitPortraitNamed "!name" writes that line in the build folder's hide-texture.txt between
    # the last Quit Game opening's System click and its tab click (the module re-reads it every 2 s, so the oracle's count starts there) and
    # removes it after that opening; a hide-texture.txt already there is left alone and the witness skipped.
    [ValidatePattern('^(!.+)?$')][string]$QuitPortraitNamed='',
    # I-0140 tranche 27 (gap 3, a Load Game portrait missing after a quit from a zone): -QuitToLoadList, at the end, Quit Game from the
    # zone (Esc, System, its Quit Game tab, Quit Game, Yes: the clicks of Pilot-QuitAndKeepSave), the title's new 05_000_Title row
    # awaited (60 s at most; none: the rest skipped), its shot 8 s later (title-after-quit.png), PRESS ANY BUTTON and the start's two
    # offline notices pressed until closed (title-after-notices.png, taken either way),
    # Load Game pressed until a new 05_010_ProfileSelect row, its list shot twice (loadlist-after-quit-1.png, -2.png 4 s later), Esc.
    [switch]$QuitToLoadList,
    # I-0113 tranche 32 (gap 14, Lightning Arrow's bow drawn while the button is held): -CastHoldMs n holds the cast's button n ms
    # (Pilot-Game press, in a job, so the -ShotsMs pictures fall inside the hold; left button only). The shots are timed from the
    # job's start, about a second before the press lands (the child pwsh's start): keep the last shot under the hold.
    [ValidateRange(0,5000)][int]$CastHoldMs=0,
    # I-0113 tranche 11 (a test save with every spell): after the load, F7 (the module's give tool: test-tools.flag and test-give.txt
    # placed in the build folder by hand) and -GiveSeconds for the items' popups, 5 s, -GiveConfirms presses of E (the « item obtained » windows close with E, not Esc), then
    # -GiveSeconds and a shot (given.png); -KeepSaveLabel keeps the result.
    # I-0113 tranche 11 (gaps 13, 12): -CaptureAt "k,ms[,vk]" presses F8 (or the key vk: 120 = F9, the effect draw dump of ffx-record.flag) ms after
    # cast k (F8: the module's PIX capture of the next frame, pix-capture.flag); the key goes down about 0.5 to 1 s later (Pilot-Game's own start),
    # and the cast's shots of a smaller ms are taken after it, under their nominal names.
    # I-0113 tranche 12: a negative ms presses the key just before cast k's click instead (the module's burst delay, pix-f8.txt, then times it).
    [ValidatePattern('^([0-9]{1,2},-?[0-9]{1,5}(,1[12][0-9])?)?$')][string]$CaptureAt='',
    # I-0140 (the spells' audit, STUDY-ATTUNE): -AttuneEach presses F11 before each cast (the module's attune tool: test-tools.flag
    # and test-attune.txt in the build folder by hand): the next listed spell in slot 0, selected, the FP filled; a shot (attune-k.png).
    # Without it -Casts stays at 12 at most.
    [switch]$AttuneEach,
    # I-0140 tranche 14 (the 25 spells left: a catalyst or a hand changed, a held cast, inside one series): -CastPlan "k:item;..."   (I-0140 tranche 20: the key k4, VK_MBUTTON, is a middle click)
    # before cast k (after its F11): k<vk> taps that key (37 Left arrow: the left hand's next slot, 39 Right arrow: the right
    # hand's), 2 s, a shot (plan-<k>-<i>.png); h<ms> holds cast k's left button ms (500 to 5000) in place of -CastHoldMs;
    # a<vk> (tranche 15) taps that key after cast k's shots (after-<k>-<i>.png); m<vk> (tranche 16) holds that key around cast k's
    # click or press (Shift, 16: the strong attack and the weapon art); b1 / b0 (tranche 16) cast k with the right / left button;
    # s<ms> (tranche 17) cast k as a stance: the right button held (with m<vk>'s key), the left clicked ms (300 to 3000) after it;
    # u<vk> (tranche 17) cast k as a tap of that key instead of a click (R, 82: the game's use of the selected quick item).
    [ValidatePattern('^([0-9]{1,3}:([kamul][0-9]{1,3}|[hs][0-9]{3,4}|b[01])(;[0-9]{1,3}:([kamul][0-9]{1,3}|[hs][0-9]{3,4}|b[01]))*)?$')][string]$CastPlan='',
    [switch]$GiveItems,[ValidateRange(10,300)][int]$GiveSeconds=20,[ValidateRange(0,60)][int]$GiveConfirms=0,
    # I-0132 R12c: -OptionsSteps "c:x,y;k:vk;..." after -OptionsTour: a press at x,y (pilot units) or a key, a screenshot after each (options-step-<i>.png).
    # I-0141 tranche 11: w:x,y,n the mouse wheel over x,y, n notches, 1 to 20 (Pilot-Game's range; negative: toward the player, scrolling down).
    [ValidatePattern('^((c:[0-9]{2,4},[0-9]{2,4}|k:[0-9]{1,3}|w:[0-9]{2,4},[0-9]{2,4},-?([1-9]|1[0-9]|20))(;(c:[0-9]{2,4},[0-9]{2,4}|k:[0-9]{1,3}|w:[0-9]{2,4},[0-9]{2,4},-?([1-9]|1[0-9]|20)))*)?$')][string]$OptionsSteps='',
    # I-0132 R12f: -TitleSteps, the same grammar, at the title screen before the save is loaded (the pages opened from the title's
    # System), a screenshot after each (title-step-<i>.png); the steps must bring the title menu back.
    [ValidatePattern('^((c:[0-9]{2,4},[0-9]{2,4}|k:[0-9]{1,3})(;(c:[0-9]{2,4},[0-9]{2,4}|k:[0-9]{1,3}))*)?$')][string]$TitleSteps='')
# FFX-DRAW, spell families (Thomas, 2026-10-17 16:43: Alpha may be used for spells from a copy of the save only): one announced
# run, as one process (the trace job dies with the shell that started it): Run-StartupTrace.ps1 in a job on a copy of the Shebang
# save (the m45 zone save, which holds Alpha at Firelink Shrine with Fire Orb attuned), the title, the two offline notices at their
# measured OK positions (the same in both modes), Load Game, the list
# scrolled to its first row (Alpha), loaded, then -Casts left clicks (the right hand's pyromancy flame casts the attuned spell)
# every -CastEvery seconds with a screenshot -ShotsMs after each click (cast-<k>-<ms>.png), then the window closed and the job's
# restoration awaited (it restores every protected file by hash).
$ErrorActionPreference='Stop'
if($CastHoldMs -and $Button -ne 'left'){throw '-CastHoldMs holds the left button only'}
if($QuitToLoadList -and $KeepSaveLabel){throw '-QuitToLoadList and -KeepSaveLabel both quit: give one'}   # tranche 27's step review L4
if($StatusViewTimes -gt 1 -and -not $StatusView){throw '-StatusViewTimes needs -StatusView'}   # I-0140 tranche 32's step review L3
# I-0140 tranche 37's final review L5 (its fix's step review L1): a floor from the logs, about 110 s for the load and teardown (t31, t37)
# and 27 s an opening (t32, t34), 31 with the toggle (t37); an opening without its row takes longer (t37's eighth: 46 s).
if($StatusView){ $need=110+$StatusViewTimes*$(if($StatusBlurToggle){31}else{27}); if($Seconds -lt $need){throw "-StatusViewTimes $StatusViewTimes needs -Seconds of about $need or more (the load, then about $(if($StatusBlurToggle){31}else{27}) s an opening)"} }
if($StatusBlurToggle -and (-not $StatusView -or -not $StatusSwitchVk -or @($EffectFlags -split '[;,]') -notcontains 'menu-shadow-blur-toggle')){throw '-StatusBlurToggle needs -StatusView, a -StatusSwitchVk and the effect flag menu-shadow-blur-toggle'}
if($AudioRecorder -and -not (Test-Path -LiteralPath $AudioRecorder -PathType Leaf)){throw "-AudioRecorder: no file $AudioRecorder (build scripts/audio/loopback_record.cpp)"}   # checked before the game starts
if($VideoKeys -and ($Video -or $SecondTravel -or -not $KeyPath)){throw '-VideoKeys needs -KeyPath and records alone (not with -Video or -SecondTravel)'}   # I-0113 tranche 37's final review L3
if($VideoHold -and ($Video -or $VideoKeys -or $VideoTravel -or $SecondTravel -or $KeyPath -or $Casts -gt 0 -or -not $HoldSeconds)){throw '-VideoHold needs -HoldSeconds and -Casts 0, and records alone (not with -Video, -VideoKeys, -VideoTravel, -KeyPath or -SecondTravel)'}
$d=$PSScriptRoot;$repo=Split-Path -Parent (Split-Path -Parent $d)
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-epsilon-m45-20261013-045053\DS30000.sl2';$hash='8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C'
if($SaveCopy){$save=$SaveCopy;$hash=$SaveCopySha256}   # PowerShell names are case-blind: $Save would be $save
$extra=@();if($EffectFlags){$extra+='-EffectFlags',$EffectFlags};if($NoDebugLayer){$extra+='-NoDebugLayer'};if($Measure){$extra+='-Measure'};if($NoKProfile){$extra+='-NoKProfile'};   # native frame rate close to play (the camera turns per frame)
if($TestLanguage){$extra+='-TestLanguage',$TestLanguage};if($BuildFolder){$extra+='-BuildFolder',$BuildFolder};if($GraphicsLevels){$extra+='-GraphicsLevels',$GraphicsLevels}
# I-0140 tranche 13 (its step review's L1, L2): -AttuneEach's checks before the launch: -Casts above 12 needs it; it needs the tool
# armed (test-tools.flag and test-attune.txt beside the module); -SpellsPerCatalyst would move the selection off slot 0.
$buildDir=if($BuildFolder){$BuildFolder}else{Join-Path $repo 'evidence/I-0066/artifacts/game-bootstrap-build'}   # the build folder this run uses (its witness files)
if($Casts -gt 12 -and -not $AttuneEach){ throw '-Casts above 12 needs -AttuneEach' }
if($AttuneEach -and -not ((Test-Path -LiteralPath (Join-Path $buildDir 'test-tools.flag')) -and (Test-Path -LiteralPath (Join-Path $buildDir 'test-attune.txt')))){ throw '-AttuneEach needs test-tools.flag and test-attune.txt beside the module' }
if($AttuneEach -and $SpellsPerCatalyst){ throw '-AttuneEach and -SpellsPerCatalyst do not combine' }
$attuneCast=6+[int][math]::Ceiling((($ShotsMs | Measure-Object -Maximum).Maximum)/1000)+[int][math]::Ceiling($CastHoldMs/1000)+$CastEvery   # F11, shots, hold, spacing (its review's L3)
# I-0140 tranche 14: -CastPlan read before the launch: each k within -Casts, a hold only for the left button, at most one hold a
# cast, a key 1 to 254; its time (3 s a key, a hold's ms) added to the budget below.
$planKeys=@{};$planHold=@{};$planAfter=@{};$planMod=@{};$planButton=@{};$planStance=@{};$planUse=@{};$planLook=@{};$planSeconds=0
foreach($item in @($CastPlan -split ';' | Where-Object { $_ })){ $k=[int]($item -split ':')[0];$what=($item -split ':')[1]
    if($k -lt 1 -or $k -gt $Casts){ throw "-CastPlan: cast $k is not within -Casts $Casts" }
    if($what[0] -eq 'a'){ $vk=[int]$what.Substring(1); if($vk -lt 1 -or $vk -gt 254){ throw "-CastPlan: key $vk" }; if(-not $planAfter.ContainsKey($k)){ $planAfter[$k]=@() }; $planAfter[$k]+=$vk; $planSeconds+=3 }
    elseif($what[0] -eq 'l'){ $dx=[int]$what.Substring(1); if($dx -lt 1){ throw "-CastPlan: look $dx" }; if(-not $planLook.ContainsKey($k)){ $planLook[$k]=@() }; $planLook[$k]+=$dx; $planSeconds+=2 }   # I-0140 tranche 21: l<dx> a camera turn (Pilot-Game look, dx units, about 4,000 a turn: LIVE-RUN-NOTES), after the cast's keys
    elseif($what[0] -eq 'm'){ $vk=[int]$what.Substring(1); if($vk -lt 1 -or $vk -gt 254){ throw "-CastPlan: key $vk" }; if($planMod.ContainsKey($k)){ throw "-CastPlan: two held keys for cast $k" }; $planMod[$k]=$vk }
    elseif($what[0] -eq 'u'){ $vk=[int]$what.Substring(1); if($vk -lt 1 -or $vk -gt 254){ throw "-CastPlan: key $vk" }; if($planUse.ContainsKey($k)){ throw "-CastPlan: two use keys for cast $k" }; $planUse[$k]=$vk }
    elseif($what[0] -eq 's'){ $ms=[int]$what.Substring(1); if($ms -lt 300 -or $ms -gt 3000){ throw "-CastPlan: stance $ms ms (300 to 3000)" }
        if($planStance.ContainsKey($k)){ throw "-CastPlan: two stances for cast $k" }; $planStance[$k]=$ms; $planSeconds+=[int][math]::Ceiling($ms/1000)+1 }
    elseif($what[0] -eq 'b'){ if($planButton.ContainsKey($k)){ throw "-CastPlan: two buttons for cast $k" }; $planButton[$k]=$(if($what -eq 'b1'){'right'}else{'left'}) }
    elseif($what[0] -eq 'k'){ $vk=[int]$what.Substring(1); if($vk -lt 1 -or $vk -gt 254){ throw "-CastPlan: key $vk" }; if(-not $planKeys.ContainsKey($k)){ $planKeys[$k]=@() }; $planKeys[$k]+=$vk; $planSeconds+=$(if($vk -eq 112){6}else{3}) }   # F1 may be pressed again (the final review's L3)
    else{ $ms=[int]$what.Substring(1); if($ms -lt 500 -or $ms -gt 5000){ throw "-CastPlan: hold $ms ms (500 to 5000)" }
        if($planHold.ContainsKey($k)){ throw "-CastPlan: two holds for cast $k" }; $planHold[$k]=$ms; $planSeconds+=[int][math]::Ceiling($ms/1000) } }
foreach($k in $planUse.Keys){ if($planStance.ContainsKey($k) -or $planHold.ContainsKey($k) -or $planButton.ContainsKey($k) -or $planMod.ContainsKey($k)){ throw "-CastPlan: cast $k is a key: no stance, hold, button or held key with it" } }   # tranche 17
foreach($k in $planStance.Keys){ if($planHold.ContainsKey($k) -or $planButton.ContainsKey($k)){ throw "-CastPlan: cast $k is a stance: no hold or button with it" } }   # tranche 17
if($planStance.Count -and ($CastHoldMs -or $Button -ne 'left')){ throw '-CastPlan: a stance takes neither -CastHoldMs nor -Button right' }   # its step review's L3
if($planUse.Count -and ($CastHoldMs -or $Button -ne 'left')){ throw '-CastPlan: a key cast takes neither -CastHoldMs nor -Button right' }   # its second step review's L5
foreach($k in $planHold.Keys){ $b=if($planButton.ContainsKey($k)){$planButton[$k]}else{$Button}; if($b -ne 'left'){ throw "-CastPlan: cast $k holds the left button only" } }
if($CastHoldMs -and @($planButton.Values | Where-Object { $_ -eq 'right' }).Count){ throw '-CastHoldMs holds the left button only: no b1 in -CastPlan with it' }   # tranche 16's step review L1
if($CastPlan -and -not $AttuneEach -and $Casts*($attuneCast-6)+$planSeconds+180 -gt $Seconds){ throw "-CastPlan: $Casts casts need about $($Casts*($attuneCast-6)+$planSeconds+180) s, more than -Seconds $Seconds" }   # its step review's L2 (tranche 14): without F11 too
if($AttuneEach -and $Casts*$attuneCast+$planSeconds+180 -gt $Seconds){ throw "-AttuneEach: $Casts casts need about $($Casts*$attuneCast+$planSeconds+180) s, more than -Seconds $Seconds" }   # its final review's L5 (about 15 s a cast at -CastEvery 6, i140-e76)
# A run already there stops here, before the trace job and any click (I-0141 tranche 23's reviews; Run-StartupTrace's own rule).
if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$RunName"){ throw 'Run exists; never overwrite evidence' }
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$mode,$save,$hash,$extra)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode $mode -TestSave $save -TestSaveSha256 $hash @extra 2>&1 } -ArgumentList $d,$RunName,$Seconds,$BootstrapMode,$save,$hash,$extra
$o="$repo/evidence/I-0066/artifacts/$RunName";$trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
# -Video's recorder: 'q' on its input closes the file; killed after 20 s (the file then unreadable). Never throws: an error here
# once ended the script before Wait-Job, and the trace job, its restoration with it, died (i105-video-native-1).
$script:namedPath=''
$recorder=$null;$recorderReported=$false
function Stop-Recorder{ try{ if($script:recorder -and -not $script:recorder.HasExited){ try{ $script:recorder.StandardInput.Write('q');$script:recorder.StandardInput.Close() }catch{}
    if(-not $script:recorder.WaitForExit(20000)){ $script:recorder.Kill();[void]$script:recorder.WaitForExit(5000);'video killed: its file unreadable' }else{ "video stopped exit=$($script:recorder.ExitCode)" } }
    elseif($script:recorder -and -not $script:recorderReported){ "video had already ended exit=$($script:recorder.ExitCode)" }
    $script:recorderReported=$true }catch{ $script:recorderReported=$true;"video stop error: $_" }
    try{ if($script:audio){ $a=$script:audio;$script:audio=$null
        if(-not $a.HasExited){ try{ $a.StandardInput.WriteLine('q');$a.StandardInput.Close() }catch{} }
        $recorded=$false   # muxed only from a recorder that stopped by itself with exit 0 (a killed one leaves a header without its length)
        if(-not $a.WaitForExit(15000)){ $a.Kill();'audio killed: no muxed film' }else{ $recorded=$a.ExitCode -eq 0;"audio $($a.StandardOutput.ReadToEnd().Trim() -replace '\r?\n',' ') exit=$($a.ExitCode)" }
        $base=[IO.Path]::GetFileNameWithoutExtension($script:recFile);$ff=(Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
        if($recorded -and $ff -and (Test-Path -LiteralPath "$o/$base.wav") -and (Test-Path -LiteralPath "$o/$script:recFile")){
            $m=[Diagnostics.Process]::Start([Diagnostics.ProcessStartInfo]@{FileName=$ff;Arguments="-hide_banner -loglevel error -y -i `"$o/$script:recFile`" -i `"$o/$base.wav`" -map 0:v -map 1:a -c:v copy -c:a aac -b:a 192k `"$o/$base-sound.mp4`"";UseShellExecute=$false;CreateNoWindow=$true})
            [void]$m.WaitForExit(120000);"audio muxed exit=$($m.ExitCode) $base-sound.mp4" } } }catch{ "audio stop error: $_" } }
# The recorder (-Video's casts, -VideoTravel's warp): the game window's rectangle from a shot, ffmpeg's gdigrab into $file, bounded
# to $seconds; $v0 its start, from which the casts' video_ms count.
function Start-Recorder([string]$file,[int]$seconds){
    $info=pwsh -NoProfile -File "$d/Pilot-Game.ps1" -Action shot -Path "$o/video-start.png" | Select-Object -Last 1
    if("$info" -match 'rect=(-?\d+),(-?\d+),(-?\d+),(-?\d+)' -and [int]$Matches[3] -gt [int]$Matches[1]){ $L=[int]$Matches[1];$T=[int]$Matches[2];$W=([int]$Matches[3]-$L) -band -2;$H=([int]$Matches[4]-$T) -band -2
        # I-0113 tranche 26: a window as large as its screen (2560x1440 on the OLED) has its frame past the desktop, which gdigrab refuses: the rectangle kept inside the virtual desktop
        Add-Type -AssemblyName System.Windows.Forms;$vs=[System.Windows.Forms.SystemInformation]::VirtualScreen
        if($L -lt $vs.Left){$W-=$vs.Left-$L;$L=$vs.Left};if($T -lt $vs.Top){$H-=$vs.Top-$T;$T=$vs.Top};if($L+$W -gt $vs.Right){$W=$vs.Right-$L};if($T+$H -gt $vs.Bottom){$H=$vs.Bottom-$T};$W=$W -band -2;$H=$H -band -2
        $ff=(Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
        if($W -le 0 -or $H -le 0){ "video: the window lies off the desktop ($L,$T)" }elseif(-not $ff){ 'video: no ffmpeg on the path' }else{
        $psi=[Diagnostics.ProcessStartInfo]::new($ff,"-hide_banner -loglevel error -y -f gdigrab -framerate $VideoFps -draw_mouse 0 -offset_x $L -offset_y $T -video_size ${W}x$H -i desktop -t $seconds $(if($VideoLossless){'-c:v libx264rgb -preset ultrafast -crf 0'}else{'-c:v libx264 -preset ultrafast -crf 16 -pix_fmt yuv420p'}) `"$o/$file`"")
        $psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardInput=$true   # D-0199: no window; 'q' on its input ends the file cleanly; -t bounds it if this script is killed (final review)
        # I-0140 tranche 24 (D-0299, the game's own clock): ffmpeg's report beside the film (<film>.ffmpeg.txt; its input line « start: »
        # is gdigrab's first frame in seconds since 1970, UTC) and the clock pair <film>.clock.json (the QPC in ms, which the module's
        # ffx_create rows carry, and UTC, read together), so Get-EffectFrames.py finds a module row's frame in the film.
        $psi.WorkingDirectory=$o;$psi.Environment['FFREPORT']="file=$([IO.Path]::GetFileNameWithoutExtension($file)).ffmpeg.txt:level=32"
        $script:recorder=[Diagnostics.Process]::Start($psi);$script:v0=Get-Date;$script:recFile=$file
        if($AudioRecorder){ $api=[Diagnostics.ProcessStartInfo]::new($AudioRecorder,"`"$o/$([IO.Path]::GetFileNameWithoutExtension($file)).wav`" $seconds")
            $api.UseShellExecute=$false;$api.CreateNoWindow=$true;$api.RedirectStandardInput=$true;$api.RedirectStandardOutput=$true
            $script:audio=[Diagnostics.Process]::Start($api) }
        # Its step review's L2 and L3: read after the audio starts, and never thrown (the recorder never throws, above).
        try{ $qpc=1000.0*[Diagnostics.Stopwatch]::GetTimestamp()/[Diagnostics.Stopwatch]::Frequency;$utc=[DateTime]::UtcNow
            [ordered]@{qpc_ms=$qpc;utc=$utc.ToString('o');unix=($utc-[DateTime]::new(1970,1,1,0,0,0,[DateTimeKind]::Utc)).TotalSeconds;film=$file} | ConvertTo-Json | Set-Content -LiteralPath "$o/$([IO.Path]::GetFileNameWithoutExtension($file)).clock.json" -Encoding utf8 }catch{ "video clock: $_" }
        Start-Sleep 1;"video start $($script:v0.ToUniversalTime().ToString('o')) rect=$L,$T,${W}x$H $file" } }
    else{ "video: no window rectangle ($info)" } }
function Seen([string]$pattern){ (Test-Path -LiteralPath $trace) -and [bool](Select-String -LiteralPath $trace -Pattern $pattern -Quiet) }
try {
    $deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    # I-0113 tranche 10 (2026-11-01: a run opened on the OLED, the primary screen, while Thomas used it): with DS3_PILOT_SCREEN the
    # window is first placed on that screen (Pilot-Game -Action place), whose clicks and shots Pilot-Game then takes relative to it.
    if($env:DS3_PILOT_SCREEN){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" -Action place 2>&1 | Select-Object -Last 1;if($LASTEXITCODE){ throw 'the game window could not be placed on its screen' } }
    # The same notices at the same places in both modes (Reference2032, Native2033); presses of -PressMs (native frames are slower).
    # I-0132 R12e: -VideoTitle from FromSoftware's logo (3.5 s before the title: r12e-film2 started 2 s into the title, past the fade-in)
    if($VideoTitle){ $deadline=(Get-Date).AddSeconds(240); while(-not (Seen '"name":"05_900_Logo_FromSoft"') -and (Get-Date) -lt $deadline){ Start-Sleep -Milliseconds 250 }
        Start-Recorder 'title.mp4' (120+$TitleHoldSeconds+8*@($TitleSteps -split ';' | Where-Object { $_ }).Count) }
    $deadline=(Get-Date).AddSeconds(240); while(-not (Seen '"name":"05_000_Title"') -and (Get-Date) -lt $deadline){ Start-Sleep 1 }
    # Each notice closes with a menu_window_close row: press its OK until it closed (the first press can come before it is up).
    function Closed{ if(-not (Test-Path -LiteralPath $trace)){return 0}; @(Select-String -LiteralPath $trace -Pattern '"event":"menu_window_close"').Count }
    Start-Sleep 8; $base=Closed; P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','shot','-Path',"$o/notice-1.png"
    for($k=0;$k -lt 6 -and (Closed) -lt $base+1;$k++){ P '-Action','press','-X','648','-Y','427','-Ms',"$PressMs"; Start-Sleep 4 }; P '-Action','shot','-Path',"$o/notice-2.png"   # "Unable to connect" OK
    for($k=0;$k -lt 6 -and (Closed) -lt $base+2;$k++){ P '-Action','press','-X','648','-Y',"$OfflineOkY",'-Ms',"$PressMs"; Start-Sleep 4 }; P '-Action','shot','-Path',"$o/title.png"   # "Starting in offline mode" OK
    if($TitleHoldSeconds -gt 0){ Start-Sleep $TitleHoldSeconds; P '-Action','shot','-Path',"$o/title-held.png" }
    if($TitleSteps){ $i=0; foreach($step in $TitleSteps -split ';'){ if(-not (Alive)){ break }; $i++; $v=$step.Substring(2)
        if($step[0] -eq 'c'){ $xy=$v -split ','; P '-Action','press','-X',$xy[0],'-Y',$xy[1],'-Ms',"$PressMs" }else{ P '-Action','key','-Vk',$v }
        Start-Sleep 4; P '-Action','shot','-Path',"$o/title-step-$i.png" } }
    $loaded='"title_step_state".*"state":4,'
    # -Continue (D-0193): the save's last played character (Continue, 648 525) instead of Load Game's first row (Alpha in the zone saves).
    if($VideoTitle -and $Continue){ Stop-Recorder; $script:recorder=$null;$script:recorderReported=$false }   # the second step review's L3
    if($Continue){ for($k=1;$k -le 4 -and -not (Seen $loaded);$k++){ P '-Action','press','-X','648','-Y','525','-Ms',"$PressMs"
        $until=(Get-Date).AddSeconds(25); while(-not (Seen $loaded) -and (Get-Date) -lt $until){ Start-Sleep 1 } } }
    else{ for($k=0;$k -lt 4 -and -not (Seen '"name":"05_010_ProfileSelect"');$k++){ P '-Action','press','-X','648','-Y','555','-Ms',"$PressMs"; Start-Sleep 6 }; Start-Sleep 9 }   # Load Game
    if(-not $Continue){ P '-Action','shot','-Path',"$o/list.png"
    if($VideoTitle){ Stop-Recorder; $script:recorder=$null;$script:recorderReported=$false }
    P '-Action','wheel','-X','648','-Y','300','-Notches','3'; Start-Sleep 2 }
    for($k=1;$k -le 4 -and -not $Continue -and -not (Seen $loaded);$k++){
        P '-Action','press','-X','648','-Y','182','-Ms',"$PressMs"; Start-Sleep 4; P '-Action','shot','-Path',"$o/confirm-$k.png"
        P '-Action','press','-X','500','-Y','427','-Ms',"$PressMs"; $until=(Get-Date).AddSeconds(20); while(-not (Seen $loaded) -and (Get-Date) -lt $until){ Start-Sleep 1 } }
    "loaded=$(Seen $loaded)"
    Start-Sleep 45; P '-Action','shot','-Path',"$o/ingame.png"
    # I-0140 tranche 28 (Thomas, 2026-11-21: a measure of the portrait, not the eye): the Quit Game pane's mean luma, chroma and
    # saturation (ffmpeg signalstats YAVG, UAVG, VAVG, SATAVG) over its fixed rectangle 510x250 at (987, 437) of a 1920x1080 shot of
    # the ASUS window (title bar included: the pane of i140-t27b-thomas-orig/quit-load-1.png), one line per shot in both modes; a shot
    # of another size (another window or DPI) is not measured, nor one ffmpeg could not read (its step review's M3, L7).
    function PortraitStats([string]$png){ $ff=(Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
        if(-not $ff -or -not (Test-Path -LiteralPath $png)){ "portrait $([IO.Path]::GetFileName($png)): not measured (no ffmpeg or no shot)"; return }
        $probe=Join-Path (Split-Path $ff) 'ffprobe.exe'   # the shot's size (the pilot's shots are JPEG data under a .png name)
        if(-not (Test-Path -LiteralPath $probe)){ "portrait $([IO.Path]::GetFileName($png)): not measured (no ffprobe beside ffmpeg)"; return }
        $size="$(& $probe -v error -select_streams v:0 -show_entries stream=width,height -of csv=p=0 $png 2>$null)"
        $parts=@($size.Trim() -split ',')+@('',''); $w=$parts[0]; $hh=$parts[1]
        if($w -ne 1920 -or $hh -ne 1080){ "portrait $([IO.Path]::GetFileName($png)): not measured (a ${w}x$hh shot, the rectangle is the 1920x1080 window's)"; return }
        $m=@{}; foreach($l in (& $ff -hide_banner -loglevel error -i $png -vf 'crop=510:250:987:437,signalstats,metadata=print:file=-' -f null - 2>$null)){
            if($l -match 'lavfi\.signalstats\.(YAVG|UAVG|VAVG|SATAVG)=([0-9.]+)'){ $m[$Matches[1]]=$Matches[2] } }
        if($m.Count -lt 4){ "portrait $([IO.Path]::GetFileName($png)): not measured (ffmpeg gave no statistics)"; return }
        "portrait $([IO.Path]::GetFileName($png)): yavg $($m.YAVG) uavg $($m.UAVG) vavg $($m.VAVG) satavg $($m.SATAVG)" }
    function QuitPortrait([string]$when,[switch]$Named){ if(-not (Alive)){return}; P '-Action','key','-Vk','27'; Start-Sleep 4; P '-Action','click','-X','518','-Y','390'; if($Named){ Start-Sleep 2; ArmNamed; Start-Sleep 3 }else{ Start-Sleep 5 }
        P '-Action','click','-X','743','-Y','192'; Start-Sleep 4; P '-Action','shot','-Path',"$o/quit-$when-1.png"; Start-Sleep 4; P '-Action','shot','-Path',"$o/quit-$when-2.png"; PortraitStats "$o/quit-$when-1.png"; PortraitStats "$o/quit-$when-2.png"
        # I-0113 tranche 27: three Esc, not two: after two the in-game menu stayed open, so every second opening of -QuitPortraitTimes
        # closed it instead (its shots show the world: i113-t26-quit-delta-native, i113-t27-quit-delta-native openings 2, 4, 6, and the original's open2).
        P '-Action','key','-Vk','27'; Start-Sleep 3; P '-Action','key','-Vk','27'; Start-Sleep 3; P '-Action','key','-Vk','27'; Start-Sleep 3 }
    # Tranche 33: -QuitPortraitNamed arms the named oracle for the last opening only, between the System click and the tab's (the
    # module re-reads the file every 2 s; the final review's M1: armed 3 s before the Esc, the face's 24,000 draws were spent on the
    # world before the tab opened; 2 s after the System click, 3 s before the tab's: step review L2). New-Item refuses an existing file (another session's is never overwritten, L5).
    function ArmNamed(){ $named=Join-Path $buildDir 'hide-texture.txt'
        try{ New-Item -ItemType File -Path $named -Value $QuitPortraitNamed -ErrorAction Stop | Out-Null; $script:namedPath=$named }
        catch{ 'hide-texture.txt already in the build folder: -QuitPortraitNamed skipped' } }
    function NamedOpening([string]$when){ if(-not $QuitPortraitNamed){ QuitPortrait $when; return }
        try{ QuitPortrait $when -Named } finally { if($script:namedPath){ Remove-Item -LiteralPath $script:namedPath -Force -ErrorAction SilentlyContinue
            if(-not (Test-Path -LiteralPath $script:namedPath)){ $script:namedPath='' } } } }   # a failed delete left to the outer finally (step review L1)
    if($QuitPortrait){ if($QuitPortraitTimes -eq 1){ NamedOpening 'load' }else{ QuitPortrait 'load' }
        for($q=2;$q -le $QuitPortraitTimes -and (Alive);$q++){ if($q -eq $QuitPortraitTimes){ NamedOpening "open$q" }else{ QuitPortrait "open$q" } } }
    # -StatusView: Esc (the menu opened: -QuitPortrait's three Esc leave it closed), the Status icon (394, 390, LIVE-RUN-NOTES), the
    # page's 02_070_Status first-advance row awaited (15 s at most). The row's own tick (GetTickCount64, the module's clock) is the page's
    # moment in both modes (D-0299): the shots are scheduled 4, 8 and 12 s after it (status-1.png, status-2.png, status-switch.png,
    # -StatusSwitchVk pressed before the last), each taken at its target tick (ShotAt, below), and each line names the shot's capture
    # tick (Pilot-Game's, read right after the screen copy, the same clock) minus the row's, with the copy's start, the target and
    # late (the process ready only after the target); compare pairs by the copy's start. No row at the first opening: no shot (the page's moment unknown; a reopening's: below). The switch key pressed again after its
    # shot (the prompt toggles, INFERRED), then one Esc (it closes the whole in-game menu; a second one opens it again:
    # i140-t30-thomas-nat/status-closed.png) and a shot of the world (status-closed.png). A new 02_000_IngameTop first-advance row
    # within 3 s of that Esc means it opened the menu again (each opening of the t30 journals added one): said, and one more Esc.
    # Not detected: a menu left open with no new row (a sub-view that took the Esc; -StatusSwitchVk 0 presses no key), and a reopening
    # whose row is not written (the module writes one per new movie address, up to 256: a reused address or a full set writes
    # none) or comes after 3 s. The same limits can hide the Status row (then « the page's moment unknown »; a reopening's: below).
    function FirstAdvanceTicks([string]$page){ if(-not (Test-Path -LiteralPath $trace)){ return @() }
        @(Select-String -LiteralPath $trace -Pattern """event"":""ui_movie_first_advance"".*""name"":""$page""" -Raw | ForEach-Object { if($_ -match '"tick":(\d+)'){ [int64]$Matches[1] } }) }
    # I-0140 tranche 38 (tranche 37's shots 0.6 to 1.2 s after their target: Pilot-Game's own start came between the schedule and the
    # copy): ShotAt starts Pilot-Game 2.5 s before the target tick and the shot waits inside it for that tick (-AtTick), so the copy
    # starts at the target or within a few ms of it; a process ready only after the target says so (late, ms). The line names the
    # capture tick (after the copy), the copy's start and the target, each from the timing tick. A target not reached in 25 s is
    # said and not shot (its step review's M2), as a game gone before the copy (« gone », no picture).
    function ShotAt([string]$path,[int64]$at,[int64]$from,[string]$fromName){ $cap=(Get-Date).AddSeconds(25)
        while([Environment]::TickCount64 -lt $at-2500 -and (Get-Date) -lt $cap -and (Alive)){ Start-Sleep -Milliseconds 50 }
        if(-not (Alive)){ return 'game gone' }
        if([Environment]::TickCount64 -lt $at-2500){ return "no shot: the target $fromName +$($at-$from) ms not near in 25 s" }
        $line="$(pwsh -NoProfile -File "$d/Pilot-Game.ps1" -Action shot -Path $path -AtTick $at 2>&1)"
        if($line -match '^gone '){ return 'game gone' }
        if($line -match 'tick=(\d+) t0=(\d+) at=\d+ ready=(\d+) late=(\d+)'){ $tick=[int64]$Matches[1]; $t0=[int64]$Matches[2]; "capture tick $tick, $fromName +$($tick-$from) ms (copy from +$($t0-$from) ms, target +$($at-$from) ms, ready +$([int64]$Matches[3]-$from) ms, late $($Matches[4]) ms)" }else{ "no capture tick (Pilot-Game: $($line -replace '\x1b\[[0-9;]*m',''))" } }
    # A reopening (-StatusViewTimes) whose page writes no new row (a reused movie address, above) is shot on the same schedule from the
    # wait's end (15 s after its click), the page not confirmed open: never for a comparison (D-0299 wants the page's own moment). A pass
    # without a row ends the reopenings (the menu's state after it is not known: its Esc could close the menu instead of opening it).
    # -StatusBlurToggle: menu-shadow-hard.now present (hard) or absent (blurred), said with the runner's tick (TickCount64, the module's clock).
    $script:hardPath=[IO.Path]::Combine($buildDir,'menu-shadow-hard.now')
    # The step review's L2: the state said is the file's after the action, and a state not reached stops the run (its finally cleans up).
    function SetHard([bool]$hard,[string]$what){ if($hard){ Set-Content -LiteralPath $script:hardPath -Value 'I-0140 t37' -Encoding ascii }else{ Remove-Item -LiteralPath $script:hardPath -Force -ErrorAction SilentlyContinue }
        $now=Test-Path -LiteralPath $script:hardPath; "$($what): $(if($BootstrapMode -ne 'Native'){'no effect in Reference,'}else{"self-shadow $(if($now){'hard'}else{'blurred'})"}) (file $(if($now){'present'}else{'absent'}), tick $([Environment]::TickCount64))"
        if($now -ne $hard){ throw "menu-shadow-hard.now not $(if($hard){'written'}else{'removed'})" } }
    for($pass=1;$StatusView -and $pass -le $StatusViewTimes -and (Alive);$pass++){ $stem=$(if($pass -eq 1){'status'}else{"status$pass"})
        if($StatusBlurToggle){ SetHard ($pass % 2 -eq 0) "$stem opening" }
        $before=@(FirstAdvanceTicks '02_070_Status').Count; P '-Action','key','-Vk','27'; Start-Sleep 4; $click=[Environment]::TickCount64; if(Alive){ P '-Action','click','-X','394','-Y','390' }
        $until=(Get-Date).AddSeconds(15); while((Get-Date) -lt $until -and (Alive) -and @(FirstAdvanceTicks '02_070_Status').Count -le $before){ Start-Sleep -Milliseconds 400 }
        $rowTicks=@(FirstAdvanceTicks '02_070_Status'); $switched=$false; $row=$null; $timedFrom='row'
        if(-not (Alive)){ "$($stem) page: game gone" }
        elseif($rowTicks.Count -gt $before){ $row=$rowTicks[-1]; "$($stem) page row tick $row" }
        elseif($pass -gt 1){ $row=[Environment]::TickCount64; $timedFrom='wait end'; "$($stem) page row not seen in 15 s: shots timed from the wait's end (click +$($row-$click) ms), the page not confirmed open, not for a comparison" }
        else{ $timedFrom='none'; "status page row not seen in 15 s: no shot (the page's moment unknown)" }
        if($null -ne $row){
            foreach($s in @(@("$stem-1",4000),@("$stem-2",8000),@("$stem-switch",12000))){ if(-not (Alive)){ 'game gone'; break }
                if($s[0] -eq "$stem-switch"){ if(-not $StatusSwitchVk){ break }; $k0=[Environment]::TickCount64; P '-Action','key','-Vk',"$StatusSwitchVk"; $switched=$true
                    "status switch key $($stem): Pilot-Game started at $timedFrom +$($k0-$row) ms, returned at +$([Environment]::TickCount64-$row) ms (the menu views open with it; I-0140 tranche 38's final review M3)" }
                $said=ShotAt "$o/$($s[0]).png" ($row+$s[1]) $row $timedFrom
                if($said -eq 'game gone'){ 'game gone'; break }; "status shot $($s[0]): $said" } }
        # The final review's M1: the switchflip shot on the page's row clock too (16 s after the row, at least 1 s after the flip).
        if($switched -and $StatusBlurToggle -and (Alive)){ SetHard ($pass % 2 -ne 0) "$stem flip"; $flipped=[Environment]::TickCount64
            $said=ShotAt "$o/$stem-switchflip.png" ([Math]::Max($row+16000,$flipped+1000)) $row $timedFrom; "status shot $stem-switchflip: $said" }
        if($switched -and (Alive)){ P '-Action','key','-Vk',"$StatusSwitchVk"; Start-Sleep 2 }
        if(Alive){ $tops=@(FirstAdvanceTicks '02_000_IngameTop').Count; P '-Action','key','-Vk','27'; Start-Sleep 3
            if(@(FirstAdvanceTicks '02_000_IngameTop').Count -gt $tops -and (Alive)){ 'status close: the Esc opened the menu again (a new 02_000_IngameTop row): one more Esc'; P '-Action','key','-Vk','27'; Start-Sleep 3 }
            if(Alive){ P '-Action','shot','-Path',"$o/$stem-closed.png" } }
        if($timedFrom -ne 'row' -and $pass -lt $StatusViewTimes -and (Alive)){ 'status reopenings stopped: a pass without its row'; break } }
    if($StatusBlurToggle){ SetHard $false 'status openings done' }   # the step review's L1: the later steps (portraits) blurred as in play
    if($GiveItems -and (Alive)){ P '-Action','hold','-Vk','118','-Ms','500'; Start-Sleep 5; for($k=0;$k -lt $GiveConfirms -and (Alive);$k++){ P '-Action','hold','-Vk','69','-Ms','200'; Start-Sleep 2 }; Start-Sleep $GiveSeconds; P '-Action','shot','-Path',"$o/given.png" }
    # I-0100 tranche 2 (a zone save's character at its bonfire, the far lighting native against original): -LookSweep without
    # -TravelPress (or with -SweepBefore, before the travel too: pre-<i>-<s>.png) turns the camera where the character loaded.
    if($LookSweep -and (-not $TravelPress -or $SweepBefore)){ $v=$LookSweep -split ','; $stem=$(if($TravelPress){'pre'}else{'sweep'}); for($i=1;$i -le [int]$v[0] -and (Alive);$i++){ P '-Action','look','-X',$v[1],'-Y','0'
        for($s=1;$s -le [int]$v[2];$s++){ Start-Sleep 1; P '-Action','shot','-Path',"$o/$stem-$i-$s.png" } } }
    # -TalkKeeper (menu tests CL5): from Alpha's spot at the Firelink bonfire, the camera turned to the Fire Keeper (look 250 then -90), three
    # seconds forward (AZERTY Z), talk (E), screenshots of her first lines (talk-<s>.png).
    # -SetFlag "id value" (menu tests R5): the module sets that event flag on the game thread (witness-flag.txt); flag 20 runs the ending
    # chain and the credits; screenshots -FlagShots seconds after (flag-<s>.png).
    # I-0139 (the tour's cutscenes): several flags « id value;id value », each written once the module has taken the one before (up to 20 s).
    if($SetFlag){ $file=Join-Path $buildDir 'witness-flag.txt'; $flagList=@($SetFlag -split ';' | Where-Object { $_.Trim() })
        foreach($one in $flagList[0..($flagList.Count-2)]){ if($flagList.Count -lt 2){ break }; Set-Content -LiteralPath $file -Value $one.Trim() -NoNewline -Encoding ascii
            $w=0; while((Test-Path -LiteralPath $file) -and $w -lt 40 -and (Alive)){ Start-Sleep -Milliseconds 500; $w++ }; if(Test-Path -LiteralPath $file){ "flag $one not taken" } }
        Set-Content -LiteralPath $file -Value $flagList[-1].Trim() -NoNewline -Encoding ascii; $t0=Get-Date
        foreach($s in $FlagShots){ $wait=$s-((Get-Date)-$t0).TotalSeconds; if($wait -gt 0){ Start-Sleep -Seconds ([int]$wait) }; if(-not (Alive)){ 'game gone'; break }; P '-Action','shot','-Path',"$o/flag-$s.png" }
        if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force; 'flag file not taken' } }
    # -BonfirePages (menu audit): rest at the bonfire (E) with eight screenshots in a row while sitting down (sit-<k>.png, about one
    # every 0.7 s: the brown haze of Native2092 hold-33), the bonfire menu, then Travel (its map images).
    # -ApproachMs (I-0100 tranche 2: Gamma loads a step behind the Anor Londo bonfire): forward (AZERTY Z) for that time first.
    if($BonfirePages){ if($ApproachMs){ P '-Action','hold','-Vk','90','-Ms',"$ApproachMs"; Start-Sleep 1 }
        if($ApproachKeys){ foreach($step in $ApproachKeys -split ';'){ $v=$step -split ','; P '-Action','hold','-Vk',$v[0],'-Ms',$v[1]; Start-Sleep 1 }; P '-Action','shot','-Path',"$o/approach.png" }; P '-Action','hold','-Vk','69','-Ms','200'; foreach($s in 1..8){ P '-Action','shot','-Path',"$o/sit-$s.png" }; Start-Sleep 2; P '-Action','shot','-Path',"$o/bonfire.png"
        P '-Action','press','-X','328','-Y','206','-Ms',"$PressMs"; Start-Sleep 6; P '-Action','shot','-Path',"$o/travel.png"; Start-Sleep 4; P '-Action','shot','-Path',"$o/travel-2.png" }
    # Optimisation review (Thomas, 2026-10-19: Cathedral of the Deep, never witnessed): -TravelPress "x,y;x,y" presses in the travel
    # list (pilot units) 4 s apart with a shot after each (go-<k>.png), then -ArriveSeconds for the load and arrive.png, then
    # -StaySeconds before closing.
    if($TravelPress){ $k=0; if($VideoTravel){ Start-Recorder 'travel.mp4' ($ArriveSeconds+4*@($TravelPress -split ';').Count+20) }
        foreach($xy in $TravelPress -split ';'){ $k++; $v=$xy -split ','; P '-Action','press','-X',$v[0],'-Y',$v[1],'-Ms',"$PressMs"; Start-Sleep 4; P '-Action','shot','-Path',"$o/go-$k.png" }
        Start-Sleep $ArriveSeconds; if(Alive){ P '-Action','shot','-Path',"$o/arrive.png" }
        if($VideoTravel){ Stop-Recorder; $script:recorder=$null;$script:recorderReported=$false }
        for($t=0;$t -lt $StaySeconds -and (Alive);$t+=5){ Start-Sleep 5 }; if(Alive){ P '-Action','shot','-Path',"$o/stay.png" }
        # I-0141 tranche 2 (Thomas, 2026-11-14: the Great Belfry's bell): -SetFlagAfterTravel "id value;id value" sets event flags once
        # arrived (witness-flag.txt, each written once the module has taken the one before), so that an event of the loaded zone that
        # waits for a flag runs as in play; the shots after it are -LookSweep's. Needs -TravelPress (said below when it is missing). The
        # module does not take the file while a cutscene plays (i141-t2b: « not taken » under Archdragon Peak's ceremony scene, which
        # the game had started itself: its bell event sets the flag again at the load).
        if($SetFlagAfterTravel -and (Alive)){ $file=Join-Path $buildDir 'witness-flag.txt'
            foreach($one in @($SetFlagAfterTravel -split ';' | Where-Object { $_.Trim() })){ Set-Content -LiteralPath $file -Value $one.Trim() -NoNewline -Encoding ascii
                $w=0; while((Test-Path -LiteralPath $file) -and $w -lt 40 -and (Alive)){ Start-Sleep -Milliseconds 500; $w++ }
                if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force; "flag $one not taken" } } }
        # -LookSweep "n,dx,seconds" (D-0193, distant enemies): n camera turns of dx mouse units, each held `seconds` with a shot every
        # second (sweep-<i>-<s>.png), after the arrival.
        if($LookSweep){ $v=$LookSweep -split ','; for($i=1;$i -le [int]$v[0] -and (Alive);$i++){ P '-Action','look','-X',$v[1],'-Y','0'
            for($s=1;$s -le [int]$v[2];$s++){ Start-Sleep 1; P '-Action','shot','-Path',"$o/sweep-$i-$s.png"
                if($SweepKeyAt -and $i -eq 1 -and $s -eq [int]($SweepKeyAt -split ',')[0]){ P '-Action','key','-Vk',($SweepKeyAt -split ',')[1] } } } }
        if($QuitPortrait){ QuitPortrait 'arrive' } }
    # -MenuTour (menu audit, MENU-TESTS-PREP 3.4 families 1, 2, 4): the in-game menu's Inventory, Status and System pages (Esc, the icon,
    # then one Esc: it closes the page and the menu together; I-0140 tranche 30: the second Esc of the runs before it opened the menu
    # again, so the next page's Esc closed it and its click fell on the world), then the Firelink bonfire, its menu and Attune Spell, last
    # (no key tried took the attunement list back: Esc, Backspace, a right click opens the command list; Reference2079-2082); a screenshot
    # each (tour-<page>.png). Positions in pilot units, measured on Reference/Native2066-2067 and the language runs: the menu icons at
    # y 390 (LIVE-RUN-NOTES: Equipment 263, then 329, 394, 460, 525; the earlier 323, 388, 518 hit the same icons); the bonfire
    # rows 33 apart from Travel at 207.
    if($MenuTour){ function Shot([string]$n){ Start-Sleep 5; if(Alive){ P '-Action','shot','-Path',"$o/tour-$n.png" } }
        foreach($page in @(@('inventory',329),@('status',394),@('system',525))){ if(-not (Alive)){ break }; P '-Action','key','-Vk','27'; Start-Sleep 4
            if(Alive){ P '-Action','click','-X',"$($page[1])",'-Y','390' }; Shot $page[0]; if(Alive){ P '-Action','key','-Vk','27'; Start-Sleep 3 } }
        if(Alive){ Shot 'closed'; P '-Action','hold','-Vk','69','-Ms','200'; Shot 'bonfire' }
        if(Alive){ P '-Action','press','-X','328','-Y','240','-Ms',"$PressMs"; Shot 'attune' } }
    # -OptionsTour (menu audit, MENU-TESTS-PREP 3.4 family 2): Esc, System (Game Options, 02_040), then each tab icon of its row at
    # y 192 (pilot units are screen pixels; the eight icons 60 apart from the first at 323, Reference2085), a screenshot each (options-<k>.png).
    if($OptionsTour){ P '-Action','key','-Vk','27'; Start-Sleep 4; P '-Action','click','-X','518','-Y','390'; Start-Sleep 5; P '-Action','shot','-Path',"$o/options-0.png"
        for($k=1;$k -le 7;$k++){ if(-not (Alive)){ break }; P '-Action','press','-X',"$(323+60*$k)",'-Y','192','-Ms',"$PressMs"; Start-Sleep 5; P '-Action','shot','-Path',"$o/options-$k.png" } }
    if($OptionsSteps){ $i=0; foreach($step in $OptionsSteps -split ';'){ if(-not (Alive)){ break }; $i++; $v=$step.Substring(2)
        if($step[0] -eq 'c'){ $xy=$v -split ','; P '-Action','press','-X',$xy[0],'-Y',$xy[1],'-Ms',"$PressMs" }elseif($step[0] -eq 'w'){ $xy=$v -split ','; P '-Action','wheel','-X',$xy[0],'-Y',$xy[1],'-Notches',$xy[2] }else{ P '-Action','key','-Vk',$v }
        Start-Sleep 4; P '-Action','shot','-Path',"$o/options-step-$i.png" } }
    # -HitKeeper (DECALS-WAVE1, Thomas 2026-10-18: striking the Fire Keeper is allowed): the same walk without talking; the -Casts
    # that follow hit her (blood, burns).
    # -KeyPath "L700,Z3000,...": keys held in order (letter = the virtual key, then milliseconds; the keyboard camera keys turn at a
    # fixed rate whatever the frame rate, unlike a mouse move), a screenshot after each (key-<i>.png); it replaces -HitKeeper's mouse walk.
    # -Teleport "x y z seconds" (witness-teleport.txt, read on the game thread): the player placed there, then the camera and keys.
    if($Teleport){ $file=Join-Path $buildDir 'witness-teleport.txt'; Set-Content -LiteralPath $file -Value $Teleport -NoNewline -Encoding ascii; Start-Sleep $TeleportWait; P '-Action','shot','-Path',"$o/teleported.png"; if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force; 'teleport file not taken' } }
    # I-0113 tranche 37 (gap 24): -LookSweepAfter "n,dx,seconds", the same camera sweep after -Teleport (after-<i>-<s>.png), to
    # look round a teleported player (-LookSweep runs before the teleport).
    if($LookSweepAfter){ $v=$LookSweepAfter -split ','; for($i=1;$i -le [int]$v[0] -and (Alive);$i++){ P '-Action','look','-X',$v[1],'-Y','0'
        for($s=1;$s -le [int]$v[2];$s++){ Start-Sleep 1; P '-Action','shot','-Path',"$o/after-$i-$s.png" } } }
    if($ResetCamera){ P '-Action','mclick'; Start-Sleep 2; P '-Action','shot','-Path',"$o/reset.png" }   # the camera behind the character
    # I-0113 tranche 37 (gap 25): -VideoKeys films keys.mp4 from the first -KeyPath key to the end of -HoldSeconds (its step review's
    # M2: bounded by the keys' own times plus 2 s a step, the hold and 10 s; the casts' stop skipped while it films; refused above with -Video).
    if($VideoKeys -and $KeyPath -and (Alive)){ $keyMs=0; foreach($step in $KeyPath -split ','){ $keyMs+=[int]$step.Substring(1) }
        Start-Recorder 'keys.mp4' ([int][math]::Ceiling($keyMs/1000)+2*@($KeyPath -split ',').Count+$HoldSeconds+10); $script:keysFilm=$true }
    if($KeyPath){ $i=0; foreach($step in $KeyPath -split ','){ $i++; P '-Action','hold','-Vk',"$([int][char]$step[0])",'-Ms',"$([int]$step.Substring(1))"; Start-Sleep 1; P '-Action','shot','-Path',"$o/key-$i.png" } }
    if($SetFlagAfterTravel -and -not $TravelPress){ '-SetFlagAfterTravel ignored: no -TravelPress' }
    if($CutsceneHolds){ $hp=@($CutsceneHolds -split ' ');$hms=[int]$hp[0]
        if($hms -lt 500 -or $hms -gt 30000){throw "-CutsceneHolds: ms $hms outside 500..30000 (the module's bounds)"}
        if($CutsceneHoldShotMs+3000 -gt $hms){throw "-CutsceneHolds: ms $hms leaves no room for a shot at $CutsceneHoldShotMs ms (+3000 ms)"}
        if($CutsceneHoldVariants){ $vlist=@(foreach($e in $CutsceneHoldVariants -split ' '){ $b,$m=$e -split ':'; if($BootstrapMode -eq 'Reference'){ if(([int]$b -band -bnot 611) -ne 0){throw "-CutsceneHoldVariants: bits $b not a Reference run's (1, 2, 32, 64, 512)"} } elseif([int]$b -gt 511){throw "-CutsceneHoldVariants: bits $b above a native run's 511"}; [pscustomobject]@{bits=[int]$b;ms=[int]$m} })
            $need=$CutsceneHoldShotMs+3000+(($vlist | Measure-Object ms -Sum).Sum)+1000*($vlist.Count+1)
            if($need -gt $hms){throw "-CutsceneHolds: ms $hms leaves no room for the variants (about $need ms)"} }
        foreach($t in $hp[1..($hp.Count-1)]){ if($t -like 'p*' -and [double]$t.Substring(1) -gt 100){throw "-CutsceneHolds: $t above p100"} } }
    if($Cutscene -and $CutsceneHolds -and (Alive)){ $hfile=Join-Path $buildDir 'witness-cutscene-holds.txt'; Set-Content -LiteralPath $hfile -Value $CutsceneHolds -NoNewline -Encoding ascii
        $file=Join-Path $buildDir 'witness-cutscene.txt'; Set-Content -LiteralPath $file -Value $Cutscene -NoNewline -Encoding ascii; $t0=Get-Date
        $want=@($CutsceneHolds -split ' ').Count-1; $seen=@{}; $ended=0; $read=0L; $script:variantTick=[uint64]0; $script:variantsStopped=$false
        # The journal read as it grows (shared with its writer); a hold's row shot after -CutsceneHoldShotMs; done when every target is
        # released or missed, the game gone, or 15 minutes (the -Seconds bound ends the trace before that anyway).
        while((Alive) -and $ended -lt $want -and ((Get-Date)-$t0).TotalMinutes -lt 15){ Start-Sleep -Milliseconds 150
            $lines=@(); try{ $fs=[IO.File]::Open($trace,'Open','Read','ReadWrite'); try{ if($fs.Length -gt $read){ [void]$fs.Seek($read,'Begin'); $sr=[IO.StreamReader]::new($fs); $text=$sr.ReadToEnd()
                        $cut=$text.LastIndexOf("`n"); if($cut -ge 0){ $read+=[Text.Encoding]::UTF8.GetByteCount($text.Substring(0,$cut+1)); $lines=$text.Substring(0,$cut+1) -split "`n" } } }finally{ $fs.Dispose() } }catch{}
            foreach($l in $lines){ if($l -notmatch '"event":"witness_cutscene_hold"'){ continue }
                if($l -match '"phase":"(release|missed)"'){ $ended++; "hold row: $($l.Trim())"; continue }
                if($l -match '"phase":"hold","index":([0-9]+)' -and -not $seen.ContainsKey($Matches[1])){ $k=$Matches[1]; $seen[$k]=$true; "hold row: $($l.Trim())"
                    Start-Sleep -Milliseconds $CutsceneHoldShotMs; if(Alive){ $s0=[Environment]::TickCount64; P '-Action','shot','-Path',"$o/cutscene-hold-$k.png"; $s1=[Environment]::TickCount64
                        "$k`t$s0`t$s1" | Add-Content -LiteralPath "$o/cutscene-holds.tsv"
                        if($CutsceneHoldVariants -and -not $script:variantsStopped){ $vfile=Join-Path $buildDir 'witness-variant.txt'; $now=0; $j=0
                            # The module's row for bits b, read from the journal past the main loop's offset (that offset not moved:
                            # the hold rows are still read there) and newer than the last row taken (its module tick); 3 s at most, else
                            # the entry is skipped and named.
                            function Await-Variant([int]$b,[int]$s=3){ $from=$read; $until=(Get-Date).AddSeconds($s)
                                while((Get-Date) -lt $until -and (Alive)){ Start-Sleep -Milliseconds 100
                                    try{ $fs=[IO.File]::Open($trace,'Open','Read','ReadWrite'); try{ if($fs.Length -gt $from){ [void]$fs.Seek($from,'Begin'); $text=[IO.StreamReader]::new($fs).ReadToEnd()
                                        foreach($l in $text -split "`n"){ if($l -match '"event":"witness_share_variant","parsed":true[^}]*"bits":([0-9]+),"tick":([0-9]+)' -and [int]$Matches[1] -eq $b -and [uint64]$Matches[2] -gt $script:variantTick){ $script:variantTick=[uint64]$Matches[2]; return $l.Trim() } } } }finally{ $fs.Dispose() } }catch{} }
                                $null }
                            foreach($v in @($vlist)+@([pscustomobject]@{bits=0;ms=0;last=$true})){ if(-not (Alive)){ break }
                                if($v.bits -ne $now -or $v.last){ if($v.bits -eq $now -and $v.last){ break }
                                    Set-Content -LiteralPath $vfile -Value $v.bits -NoNewline -Encoding ascii; $row=Await-Variant $v.bits $(if($v.last){10}else{3})
                                    if(-not $row -and $v.last){ $script:variantsStopped=$true; "hold $k`: the module did not return to 0 in 10 s: variants stopped for the run, the file left for it"; break }
                                    if(-not $row){ "hold $k variant $($v.bits): no module row in 3 s"; $now=-1; continue }; $now=$v.bits }
                                if($v.last){ break }
                                Start-Sleep -Milliseconds $v.ms; $s2=[Environment]::TickCount64; P '-Action','shot','-Path',"$o/cutscene-hold-$k-v$($v.bits)-$j.png"; $s3=[Environment]::TickCount64
                                "$k`t$j`t$($v.bits)`t$($v.ms)`t$s2`t$s3" | Add-Content -LiteralPath "$o/cutscene-hold-variants.tsv"; "cutscene-hold-$k-v$($v.bits)-$j.png (ticks $s2 to $s3)"; $j++ }
                        }
                        Set-Content -LiteralPath (Join-Path $buildDir 'witness-cutscene-shot.txt') -Value $k -NoNewline -Encoding ascii
                        "cutscene-hold-$k.png at $(Get-Date -Format HH:mm:ss.fff) (ticks $s0 to $s1)" } } } }
        "holds: $($seen.Count) shot, $ended of $want ended"
        foreach($f in @($file,$hfile)){ if(Test-Path -LiteralPath $f){ Remove-Item -LiteralPath $f -Force; "$([IO.Path]::GetFileName($f)) not taken" } } }
    elseif($Cutscene -and (Alive)){ $file=Join-Path $buildDir 'witness-cutscene.txt'; Set-Content -LiteralPath $file -Value $Cutscene -NoNewline -Encoding ascii; $t0=Get-Date
        foreach($sec in $CutsceneShots){ $wait=$sec-((Get-Date)-$t0).TotalSeconds; if($wait -gt 0){ Start-Sleep -Milliseconds ([int]($wait*1000)) }; if(-not (Alive)){ 'game gone'; break }; P '-Action','shot','-Path',"$o/cutscene-$sec.png" }
        if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force; 'cutscene file not taken' } }
    if($KeySteps){ $i=0; foreach($step in $KeySteps -split ';'){ if(-not (Alive)){ 'game gone'; break }; $i++
        if($step -match '^T([1-9])?$'){ $k=$(if($Matches[1]){[int]$Matches[1]}else{1}); $point=@($Teleport2 -split '\|')[$k-1]
            if($Teleport2 -and $point){ $file=Join-Path $buildDir 'witness-teleport.txt'; Set-Content -LiteralPath $file -Value $point -NoNewline -Encoding ascii
            Start-Sleep ([int](($point -split ' ')[3])+2); P '-Action','shot','-Path',"$o/step-$i.png"; if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force; 'teleport2 file not taken' } }else{ "$step without its -Teleport2 point: nothing done" }; continue }
        if($step[0] -eq 'w'){ $wait=[int]$step.Substring(1); for($t=5;$t -le $wait -and (Alive);$t+=5){ Start-Sleep 5; P '-Action','shot','-Path',"$o/step-$i-$t.png" }
            if($wait%5 -and (Alive)){ Start-Sleep ($wait%5); P '-Action','shot','-Path',"$o/step-$i-$wait.png" }; continue }
        $v=$step -split ','; if([int]$v[1] -eq 0){ P '-Action','key','-Vk',$v[0] }else{ P '-Action','hold','-Vk',$v[0],'-Ms',$v[1] }; Start-Sleep 1; P '-Action','shot','-Path',"$o/step-$i.png" } }
    if($SwitchRight){ for($i=1;$i -le $SwitchRightTimes;$i++){ P '-Action','key','-Vk','39'; Start-Sleep 3 }; P '-Action','shot','-Path',"$o/switched.png" }   # the right hand's next slot (Right arrow): bare fists, so a click punches
    if($TalkKeeper -or ($HitKeeper -and -not $KeyPath)){ P '-Action','look','-X','250','-Y','0'; Start-Sleep 2; P '-Action','look','-X','-90','-Y','0'; Start-Sleep 1
        P '-Action','hold','-Vk','90','-Ms',"$WalkMs"; Start-Sleep 1; P '-Action','shot','-Path',"$o/keeper.png"
        # -AimAt "x y z" (DECALS-PAINT-READY rung 0, Native2138-2140: the walk's length varies between runs, so the aim missed): after
        # the walk the player is placed there for one second (witness-teleport.txt), then -KeeperLook and the step as usual. Native2133's
        # walk ended at "262.434 -55.177 602.014", from where the default look and step faced the Fire Keeper.
        if($HitKeeper -and $AimAt){ $file=Join-Path $buildDir 'witness-teleport.txt'; Set-Content -LiteralPath $file -Value "$AimAt 1" -NoNewline -Encoding ascii; Start-Sleep 4
            if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force; 'aim teleport file not taken' } }
        # -LockOn (Native2140: the middle click reset the camera, the Fire Keeper was not taken): the lock-on, then the step.
        if($HitKeeper -and $LockOn){ P '-Action','mclick'; Start-Sleep 1; if($KeeperStepMs){ P '-Action','hold','-Vk','90','-Ms',"$KeeperStepMs" }; Start-Sleep 1; P '-Action','shot','-Path',"$o/aim.png" }
        elseif($HitKeeper){ P '-Action','look','-X',"$KeeperLook",'-Y','0'; Start-Sleep 1; if($KeeperStepMs){ P '-Action','hold','-Vk','90','-Ms',"$KeeperStepMs" }; Start-Sleep 1; P '-Action','shot','-Path',"$o/aim.png" }   # facing her: the camera, then a step
        if($TalkKeeper){ P '-Action','hold','-Vk','69','-Ms','200'; $prev=0; foreach($s in 3,6,9,12,20,30){ Start-Sleep ($s-$prev); $prev=$s; P '-Action','shot','-Path',"$o/talk-$s.png" } } }
    # I-0113 tranche 18 (gap 16, Fire Orb only): -SpellUpsFirst n presses Up arrow n times before the first cast (the attuned spell
    # n slots on), a shot after them (spell.png).
    if($SpellUpsFirst -and (Alive)){ for($i=1;$i -le $SpellUpsFirst;$i++){ P '-Action','key','-Vk','38'; Start-Sleep 2 }; P '-Action','shot','-Path',"$o/spell.png" }
    if($Video -and (Alive)){ Start-Recorder 'casts.mp4' ($Casts*($CastEvery+12+[int][math]::Ceiling((($ShotsMs | Measure-Object -Maximum).Maximum)/1000))+$planSeconds+30) }   # tranche 17: a plan's F1 presses and stances lengthen the film
    for($k=1;$k -le $Casts;$k++){ if(-not (Alive)){ 'game gone'; break }
        # Its second step review's L2: the press is seen once per schedule call; when the journal has no test_attune row k after it,
        # F11 is held once more (a lost press would shift every later cast).
        if($AttuneEach){ P '-Action','hold','-Vk','122','-Ms','300'; Start-Sleep 2
            if(-not (Select-String -LiteralPath $trace -Pattern """event"":""test_attune"",""k"":$k," -Quiet -ErrorAction SilentlyContinue)){ "attune $k pressed again"; P '-Action','hold','-Vk','122','-Ms','500'; Start-Sleep 2 }
            # Its third step review's M1: the journal's last attune row named after each cast's press (k' and attuned), so a series
            # can check cast by cast that cast k casts spell k (two lost presses realigned by the next retry would hide in a count).
            $last=Select-String -LiteralPath $trace -Pattern '"event":"test_attune","k":([0-9]+),.*?"attuned":(true|false)|"event":"test_attune","k":([0-9]+),.*?"row":false' -ErrorAction SilentlyContinue | Select-Object -Last 1
            if($last){$m=$last.Matches[0];$rk=if($m.Groups[1].Success){$m.Groups[1].Value}else{$m.Groups[3].Value};$ok=if($m.Groups[2].Success){$m.Groups[2].Value}else{'refused'};"attune $k row=$rk attuned=$ok"}else{"attune $k row=none attuned=false"}
            P '-Action','shot','-Path',"$o/attune-$k.png" }
        if($SpellsPerCatalyst -and $k -gt 1){ $vk=if(($k-1)%$SpellsPerCatalyst -eq 0){39}else{38}; P '-Action','key','-Vk',"$vk"; Start-Sleep 2; P '-Action','shot','-Path',"$o/switch-$k.png" }
        $early=$CaptureAt -and $k -eq [int]($CaptureAt -split ',')[0] -and [int]($CaptureAt -split ',')[1] -lt 0
        if($early){ P '-Action','key','-Vk',$(if(($CaptureAt -split ',').Count -gt 2){($CaptureAt -split ',')[2]}else{'119'}) }
        # Tranche 15's step review M3: F1 (112, the module's effect tool) is pressed again once when the journal gained no
        # test_effect row (a lost tap would shift every later word), as F11 is; each F1 press's row named.
        if($planKeys.ContainsKey($k)){ $i=0; foreach($vk in $planKeys[$k]){ $i++
            $before=if($vk -eq 112){@(Select-String -LiteralPath $trace -Pattern '"event":"test_effect","k"' -ErrorAction SilentlyContinue).Count}else{0}
            if($vk -eq 4){ P '-Action','mclick' }else{ P '-Action','key','-Vk',"$vk" }; Start-Sleep 2   # I-0140 tranche 20: k4 (VK_MBUTTON) is Pilot-Game's middle click (lock-on, or the camera behind the player)
            if($vk -eq 112){ $rows=@(Select-String -LiteralPath $trace -Pattern '"event":"test_effect","k"' -ErrorAction SilentlyContinue)
                if($rows.Count -le $before){ "effect $k-$i pressed again"; P '-Action','hold','-Vk','112','-Ms','300'; Start-Sleep 2; $rows=@(Select-String -LiteralPath $trace -Pattern '"event":"test_effect","k"' -ErrorAction SilentlyContinue) }
                if($rows.Count -gt $before){ "effect $k-$i $($rows[-1].Line.Trim())" }else{ "effect $k-$i row=none" } }
            P '-Action','shot','-Path',"$o/plan-$k-$i.png" }; "plan $k keys $($planKeys[$k] -join ',')" }
        if($planLook.ContainsKey($k)){ foreach($dx in $planLook[$k]){ P '-Action','look','-X',"$dx",'-Y','0'; Start-Sleep 1 }; P '-Action','shot','-Path',"$o/look-$k.png"; "plan $k look $($planLook[$k] -join ',')" }   # tranche 21
        $holdMs=if($planHold.ContainsKey($k)){$planHold[$k]}else{$CastHoldMs}
        $btn=if($planButton.ContainsKey($k)){$planButton[$k]}else{$Button};$mod=if($planMod.ContainsKey($k)){[int]$planMod[$k]}else{0}
        $holdJob=$null;$stanceUntimed=$false;if($planUse.ContainsKey($k)){ P '-Action','key','-Vk',"$($planUse[$k])";$t0=Get-Date }   # tranche 17: the use key, the shots timed from its tap
        elseif($planStance.ContainsKey($k)){ $st=@('-Action','stance','-X','648','-Y','300','-Ms',"$($planStance[$k])");if($mod){$st+=@('-Vk2',"$mod")}
            # Tranche 17's step review L2: the cast timed from the stance's left click (its clicked= line), not from its end.
            $said="$(pwsh -NoProfile -File "$d/Pilot-Game.ps1" @st | Select-Object -Last 1)";$t0=Get-Date
            # Its third step review's L2: a stance without its click time is said, and its cast line marked.
            if($said -match 'clicked=(\S+)'){ $t0=[DateTime]::Parse($Matches[1],$null,[Globalization.DateTimeStyles]::RoundtripKind).ToLocalTime() }else{ "stance $k no click time (timed from its end)";$stanceUntimed=$true } }
        elseif($holdMs -and $btn -eq 'left'){ $holdJob=Start-Job -ScriptBlock { param($pilot,$ms,$vk2) pwsh -NoProfile -File $pilot -Action press -X 648 -Y 300 -Ms $ms -Vk2 $vk2 } -ArgumentList "$d/Pilot-Game.ps1",$holdMs,$mod;$t0=Get-Date }
        else{ $click=@('-Action',$(if($btn -eq 'right'){'rclick'}else{'click'}),'-X','648','-Y','300');if($mod){$click+=@('-Vk2',"$mod")}; P $click;$t0=Get-Date }   # right: the left hand (Alpha's bare left fist punches)
        if($CaptureAt -and $k -eq [int]($CaptureAt -split ',')[0] -and -not $early){ $wait=[int]($CaptureAt -split ',')[1]-((Get-Date)-$t0).TotalMilliseconds; if($wait -gt 0){ Start-Sleep -Milliseconds ([int]$wait) }; P '-Action','key','-Vk',$(if(($CaptureAt -split ',').Count -gt 2){($CaptureAt -split ',')[2]}else{'119'}) }
        # Its third step review's M1: a stance returns about a second after its click, so a shot whose time has passed by more
        # than 150 ms is skipped and said, never saved under a time it does not show (the film keeps that moment).
        foreach($ms in $ShotsMs){ $wait=$ms-((Get-Date)-$t0).TotalMilliseconds; if($wait -lt -150 -and $planStance.ContainsKey($k)){ "shot $k-$ms skipped: $([int]-$wait) ms late"; continue }; if($wait -gt 0){ Start-Sleep -Milliseconds ([int]$wait) }; P '-Action','shot','-Path',"$o/cast-$k-$ms.png" }
        if($holdJob){ Wait-Job $holdJob -Timeout 15 | Out-Null; Remove-Job $holdJob -Force }
        # I-0140 tranche 15: a<vk> taps that key after cast k's shots (Homeward's « Select destination » prompt confirmed with E, 69), a shot after each.
        if($planAfter.ContainsKey($k)){ $i=0; foreach($vk in $planAfter[$k]){ $i++; P '-Action','key','-Vk',"$vk"; Start-Sleep 2; P '-Action','shot','-Path',"$o/after-$k-$i.png" }; "plan $k after $($planAfter[$k] -join ',')" }
        "cast $k $($t0.ToUniversalTime().ToString('o'))$(if($recorder){" video_ms=$([int]($t0-$v0).TotalMilliseconds)"})$(if($stanceUntimed){' untimed'})"; Start-Sleep $CastEvery }
    if(-not $script:keysFilm){ Stop-Recorder }   # the video ends after the last cast (not through -HoldSeconds); -VideoKeys' film goes on through the hold
    # I-0113 tranche 18 (gap 21, the warp in daylight as Thomas's clip at Archdragon Peak): -SecondTravel "x,y;x,y" rests again where
    # the first travel arrived (a 60 ms step forward, then E: after the warp the rest prompt stays down until an input, i113-t18-warp-native and -warp2-native), opens Travel and presses those points (back-<k>.png), the warp filmed into travel2.mp4.
    if($SecondTravel -and (Alive)){ $script:recorder=$null;$script:recorderReported=$false; Start-Sleep 15; P '-Action','shot','-Path',"$o/before-sit2.png"; P '-Action','hold','-Vk','90','-Ms','60'; Start-Sleep 2; P '-Action','shot','-Path',"$o/nudge2.png"; P '-Action','hold','-Vk','69','-Ms','200'; Start-Sleep 4; P '-Action','shot','-Path',"$o/sit2.png"
        P '-Action','press','-X','328','-Y','206','-Ms',"$PressMs"; Start-Sleep 6; P '-Action','shot','-Path',"$o/travel2-page.png"
        # -SecondTravelNamed "!name": that line in the build folder's hide-texture.txt from the second travel's presses to its arrival (the
        # named oracle in Reference, native_named_draw_sources natively: re-read every 2 s), so their rows fall on the warp, not the load.
        $named=if($SecondTravelNamed){Join-Path $buildDir 'hide-texture.txt'}else{''}
        # The final review's L1: a hide-texture.txt already there (another session's or Thomas's) is left alone and the witness skipped.
        if($named -and (Test-Path -LiteralPath $named)){ 'hide-texture.txt already in the build folder: -SecondTravelNamed skipped'; $named='' }
        # -SecondTravelNamedAfter s: the line written s seconds after the first press instead of before it (the original's named oracle
        # followed 2,000 draws of a name, 2.5 s of the hair: i113-t18-warp4-original ran out before the fade; since I-0113 tranche 21 it follows 24,000, sparser after the 2,000th).
        if($named -and $SecondTravelNamedAfter -lt 0){ $script:namedPath=$named; Set-Content -LiteralPath $named -Value $SecondTravelNamed -NoNewline -Encoding ascii }
        Start-Recorder 'travel2.mp4' ($ArriveSeconds+4*@($SecondTravel -split ';').Count+20+[math]::Max(0,$SecondTravelNamedAfter)); $k=0
        foreach($xy in $SecondTravel -split ';'){ $k++; $v=$xy -split ','; P '-Action','press','-X',$v[0],'-Y',$v[1],'-Ms',"$PressMs"
            if($named -and $SecondTravelNamedAfter -ge 0 -and $k -eq 1){ Start-Sleep $SecondTravelNamedAfter; $script:namedPath=$named; Set-Content -LiteralPath $named -Value $SecondTravelNamed -NoNewline -Encoding ascii }
            Start-Sleep 4; P '-Action','shot','-Path',"$o/back-$k.png" }
        Start-Sleep $ArriveSeconds; if($named){ Remove-Item -LiteralPath $named -Force -ErrorAction SilentlyContinue }; if(Alive){ P '-Action','shot','-Path',"$o/arrive2.png" }; Stop-Recorder }
    # -HoldSeconds: the game stays open that long for hand piloting (from another shell, or Thomas at the keyboard), then closes;
    # -HoldShotEvery s: a screenshot every s seconds meanwhile (hold-<k>.png; menu audit, pages Thomas opens by hand).
    # I-0113 tranche 39: -VideoHold films hold.mp4 through -HoldSeconds alone (a fixed view's frame-to-frame stability), stopped
    # before the close (its final review's M1).
    if($VideoHold -and (Alive)){ $script:recorder=$null;$script:recorderReported=$false; Start-Recorder 'hold.mp4' ($HoldSeconds+6) }   # its step review's L1: a dead film reported
    $until=(Get-Date).AddSeconds($HoldSeconds); $next=(Get-Date).AddSeconds($HoldShotEvery); $k=0
    while($HoldSeconds -and (Alive) -and (Get-Date) -lt $until){ Start-Sleep 2
        if($HoldShotEvery -and (Get-Date) -ge $next){ $k++; P '-Action','shot','-Path',"$o/hold-$k.png"; $next=(Get-Date).AddSeconds($HoldShotEvery) } }
    if($VideoHold){ Stop-Recorder }
    if($KeepSaveLabel -and (Alive)){ pwsh -NoProfile -File "$d/Pilot-QuitAndKeepSave.ps1" -Label $KeepSaveLabel -EvidenceDir $o; P '-Action','shot','-Path',"$o/kept.png" }
    if($QuitToLoadList -and (Alive)){ function Lists([string]$page='05_010_ProfileSelect'){ if(-not (Test-Path -LiteralPath $trace)){return 0}; @(Select-String -LiteralPath $trace -Pattern """name"":""$page""").Count }; $titles=Lists '05_000_Title'
        P '-Action','key','-Vk','27'; Start-Sleep -Milliseconds 1500; P '-Action','click','-X','525','-Y','390'; Start-Sleep -Milliseconds 1500
        P '-Action','click','-X','742','-Y','191'; Start-Sleep -Milliseconds 1500; P '-Action','click','-X','648','-Y','556'; Start-Sleep -Milliseconds 1500
        P '-Action','click','-X','501','-Y','428'
        # The second step review's M1: the base of the notices' count taken as at the start, 8 s after the title's new page row (the
        # logo movies before it each close a window: in tranche 27's four runs the row came 9 s after the confirm), 60 s at most.
        $deadline=(Get-Date).AddSeconds(60); while((Alive) -and (Lists '05_000_Title') -le $titles -and (Get-Date) -lt $deadline){ Start-Sleep 1 }
        "title rows after the quit: $((Lists '05_000_Title')-$titles) new$(if(-not (Alive)){' (game gone)'})"
        # The third step review's L1: no title row (a quit click missed, the player still in the zone): no press in the world.
        if((Lists '05_000_Title') -le $titles){ 'no title row after the quit: the notices and Load Game skipped' }else{
        Start-Sleep 8; if(Alive){ P '-Action','shot','-Path',"$o/title-after-quit.png" }
        # Tranche 27's first series: after the quit the title shows PRESS ANY BUTTON (title-after-quit.png), then the start's two offline
        # notices; each closed as at the start (a menu_window_close row each; PRESS ANY BUTTON's press adds none) before Load Game.
        $base=Closed; for($k=0;$k -lt 6 -and (Alive) -and (Closed) -lt $base+1;$k++){ P '-Action','press','-X','648','-Y','427','-Ms',"$PressMs"; Start-Sleep 4 }
        for($k=0;$k -lt 6 -and (Alive) -and (Closed) -lt $base+2;$k++){ P '-Action','press','-X','648','-Y',"$OfflineOkY",'-Ms',"$PressMs"; Start-Sleep 4 }
        "notices after the quit: first $(if((Closed) -ge $base+1){'closed'}else{'not closed'}), second $(if((Closed) -ge $base+2){'closed'}else{'not closed'}) ($((Closed)-$base) window closes)$(if(-not (Alive)){' (game gone)'})"; if(Alive){ P '-Action','shot','-Path',"$o/title-after-notices.png" }
        $lists=Lists; for($k=0;$k -lt 4 -and (Alive) -and (Lists) -le $lists;$k++){ P '-Action','press','-X','648','-Y','555','-Ms',"$PressMs"; Start-Sleep 6 }
        "load list after the quit: $(if((Lists) -gt $lists){'opened'}else{'not opened'})$(if(-not (Alive)){' (game gone)'})"
        Start-Sleep 3; if(Alive){ P '-Action','shot','-Path',"$o/loadlist-after-quit-1.png" }; Start-Sleep 4; if(Alive){ P '-Action','shot','-Path',"$o/loadlist-after-quit-2.png" }
        P '-Action','key','-Vk','27'; Start-Sleep 3 } }
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" } finally {   # review: a witness file left by an error would act in the next session
    Stop-Recorder
    # I-0113 tranche 18 step review (M1, L2, L3): the witness files are removed from the build folder the run used as well as the
    # default one, never throwing (the trace job's restoration below must still be awaited); hide-texture.txt only if this run wrote it.
    $folders=@(Join-Path $repo 'evidence/I-0066/artifacts/game-bootstrap-build');if($BuildFolder){$folders+=$BuildFolder}
    $lefts=@(foreach($folder in $folders){ [IO.Path]::Combine($folder,'witness-flag.txt'); [IO.Path]::Combine($folder,'witness-teleport.txt'); [IO.Path]::Combine($folder,'witness-cutscene.txt'); [IO.Path]::Combine($folder,'witness-cutscene-holds.txt'); [IO.Path]::Combine($folder,'witness-cutscene-shot.txt'); [IO.Path]::Combine($folder,'witness-variant.txt') });if($script:namedPath){$lefts+=$script:namedPath};if($StatusBlurToggle -and $script:hardPath){$lefts+=$script:hardPath}   # Combine: no drive check that could throw here
    foreach($left in $lefts){ try{ if(Test-Path -LiteralPath $left){ Remove-Item -LiteralPath $left -Force -ErrorAction Stop } }catch{ "witness file not removed: $left ($_)" } } }
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 6
# D-0284 (the tour's runs as references): this run's parameters beside its journal, the defaults used included (`bound` lists those
# given), UTF-8 without a byte-order mark (pwsh 7); the module's hash is baseline.json's module_sha256 (Run-StartupTrace). Written
# at the end (the run folder is Run-StartupTrace's), so a run killed before it has none; never throws.
try{ if(Test-Path -LiteralPath $o){ $all=[ordered]@{bound=@($PSBoundParameters.Keys)}
    foreach($k in $MyInvocation.MyCommand.Parameters.Keys){ $v=Get-Variable -Name $k -ValueOnly -Scope Script -ErrorAction SilentlyContinue
        if($null -ne $v){ $all[$k]=$(if($v -is [System.Management.Automation.SwitchParameter]){[bool]$v}else{$v}) } }
    $all | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$o/alphacast-args.json" -Encoding utf8NoBOM } }catch{}
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing: run Restore-StartupTrace.ps1 on the baseline' }
