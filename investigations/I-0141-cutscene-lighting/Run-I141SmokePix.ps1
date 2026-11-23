# I-0141, the ceremony's smoke (RESULT « The ceremony's smoke »): one native run of run-smoke.ps1's protocol (Eta's save, the King
# revived by the test tools, the flags cleared, the travel to Archdragon Peak whose bell event plays the ceremony scene 32000000; the
# arrival's sweep a shot a second) with PIX's GPU capturer (pix-capture.flag) and an F8 burst (pix-f8.txt « delay spacing ») pressed by
# Run-AlphaCast -SweepKeyAt at the sweep's second where the smoke stands (29 to 34 s in run-smoke's runs): four captures
# pix-f8-<frame>.wpix beside the module, read afterwards with pixtool. No 32000010 (the cutscene tool) in this run. The ASUS by its
# key, offline, restored by hash, no measure.
# -Original: the same run in the original (Reference) with renderdoc-capture.flag in place of pix-capture.flag: RenderDoc's layer
# takes the capture at its own key F12 (Run-AlphaCast -SweepKeyAt <s>,123), under %TEMP%\RenderDoc.
# -Dump: in place of a capture, F9 (-SweepKeyAt <s>,120) logs the next 3000 effect draws' pixel-shader block (rows ffx_ps_image: the
# blend, near/far, g_vColorScale, the blend types, fog mode, soft-sprite words, pre-alpha) in both modes: ffx-record.flag natively,
# ffx-draw-observe.flag in the original (Reference), the same hook (264230, I-0113 gap 12); with each the draw's PSCommonBuffer,
# PSLightBuffer and cbFog whole (rows ffx_ps_block) and the next 300 VSCommonBuffer images (rows ffx_vs_image); compared by
# Compare-I141FxBlocks.py.
# -Dump -Light: F13 (-SweepKeyAt <s>,124) in place of F9: the same entity, camera, frustum, census and visibility rows and the
# library's spawn tally (ffx_spawn_tally; the rows ffx_spawn of 141ACCA20 from the start in both modes), without the 3000 draws'
# blocks and the VS and patch rows, which slow the native to about 28 ms a frame.
# -ArmMs <ms> (with -Dump -Light, I-0141 tranche 12): no key; the light press made by the module itself at that moment of the scene's
# own clock (ffx-entity-arm.txt; the clock starts at the first creation of FXR 732035, rows ffx_scene_clock, ffx_scene_press), so
# that both modes are sampled at the same moment. -Life (with -Dump): ffx-entity-life.flag, each entity offered on pass 0x45 followed
# for 90 s of the scene clock (rows ffx_entity_life, ffx_entity_life_count), and the effect objects' state 3, the FXR state machines
# and the external-value tables observed (ffx_state3, ffx_machine, ffx_ext_set; I-0141 tranche 13).
param([Parameter(Mandatory)][string]$Build,[ValidatePattern('^[a-z0-9-]+$')][string]$RunName='i141-smokepix-c32000000-nat',[ValidateRange(1,60)][int]$KeySecond=29,
    [ValidatePattern('^([0-9]|[1-9][0-9]|[1-5][0-9]{2}|600) ([1-9]|[1-9][0-9]|1[01][0-9]|120)$')][string]$Burst='0 40',[string]$ScreenKey='1920x1080@60',[int]$Width=1920,[int]$Height=1080,[string]$Log='',[switch]$Original,[switch]$Dump,[switch]$Light,[ValidateRange(-1,90000)][int]$ArmMs=-1,[switch]$Life)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
$flagPath=Join-Path $Build 'test-tools.flag';$revivePath=Join-Path $Build 'test-revive.txt';$burstPath=Join-Path $Build 'pix-f8.txt';$armPath=Join-Path $Build 'ffx-entity-arm.txt'
$code=99;$placed=$false   # the step review's L4: only files this run placed are removed
try{
    if($Light -and -not $Dump){throw '-Light goes with -Dump'}
    if($ArmMs -ge 0 -and -not ($Dump -and $Light)){throw '-ArmMs goes with -Dump -Light'};if($ArmMs -ge 0 -and $PSBoundParameters.ContainsKey('KeySecond')){throw '-KeySecond goes without -ArmMs'};if($Life -and -not $Dump){throw '-Life goes with -Dump'}
    if($Burst -notmatch '^[0-9]{1,3} [0-9]{1,3}$'){throw "Burst « $Burst » is not « delay spacing »"}
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){throw 'DarkSoulsIII already runs'}
    if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$RunName"){throw "run $RunName exists: give another -RunName"}
    foreach($f in $flagPath,$revivePath,$burstPath,$armPath){ if(Test-Path -LiteralPath $f){throw "$f already in the build folder: another run's? (the step review's L4)"} }
    $started=Get-Date
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $ScreenKey).device
    if(-not $device){throw "no screen $ScreenKey"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device, $RunName"
    $placed=$true;Set-Content -LiteralPath $flagPath -Value 'test';Set-Content -LiteralPath $revivePath -Value '13200850' -Encoding ascii
    Set-Content -LiteralPath $burstPath -Value $Burst -NoNewline -Encoding ascii;if($ArmMs -ge 0){Set-Content -LiteralPath $armPath -Value "$ArmMs" -NoNewline -Encoding ascii}
    try{
        $castArgs=@{RunName=$RunName;BootstrapMode=$(if($Original){'Reference'}else{'Native'});SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;BonfirePages=$true;TravelPress='803,191;400,280'
            ArriveSeconds=12;StaySeconds=0;Casts=0;Seconds=480;BuildFolder=$Build
            EffectFlags=$(if($Dump){$(if($Original){'test-no-ai,ffx-draw-observe'}else{'test-no-ai,ffx-record'})+$(if($Life){',ffx-entity-life'}else{''})}elseif($Original){'test-no-ai,renderdoc-capture'}else{'test-no-ai,world-pictures,pix-capture,effect-light-observe,effect-fog-observe'})
            SetFlag='13200446 0;13205852 0;13205441 0;13200445 0';FlagShots=@(3)
            LookSweep='1,0,60';SweepKeyAt=$(if($ArmMs -ge 0){''}else{"$KeySecond,$(if($Dump -and $Light){124}elseif($Dump){120}elseif($Original){123}else{119})"})
            GraphicsLevels="ScreenMode=WINDOW;Resolution-WindowScreenWidth=$Width;Resolution-WindowScreenHeight=$Height"}
        # The final review of 20d70b554's M5: the run fails unless Run-AlphaCast reports its run's exit 0 and the save restored.
        $castOut=@(& $cast @castArgs *>&1 | ForEach-Object { $l="$_";& $say $l;$l })
        if(-not @($castOut | Where-Object { $_ -match '"exit":0,' }).Count -or -not @($castOut | Where-Object { $_ -match '^restoration ok: True$' }).Count){throw 'Run-AlphaCast did not report exit 0 and the save restored'} }
    finally{ foreach($f in $flagPath,$revivePath,$burstPath,$armPath){ if(Test-Path -LiteralPath $f){ Remove-Item -LiteralPath $f -Force } }; $placed=$false }
    $folder="$repo/evidence/I-0066/artifacts/$RunName"
    if(Test-Path -LiteralPath "$folder/startup.jsonl"){ foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"native_pix_capture"|"event":"test_revive"|"event":"native_render_job_failure"|"event":"reference_renderdoc_layer"' | Select-Object -First 12)){ & $say "  $($l.Line)" }
        foreach($e in 'ffx_ps_image','ffx_ps_block','ffx_vs_image','ffx_entity_offered','ffx_create','ffx_spawn','ffx_spawn_tally','ffx_scene_clock','ffx_scene_press','ffx_entity_life','ffx_entity_life_count','ffx_scene_pre','ffx_state3','ffx_state3_end','ffx_machine','ffx_ext_set','ffx_ext_set_end'){ $pattern='"event":"'+$e+'"';$rows=@(Select-String -LiteralPath "$folder/startup.jsonl" -SimpleMatch -Pattern $pattern).Count;& $say "  rows $($e): $rows" } }   # the step review's L3
    & $say "  captures: $(@(Get-ChildItem -LiteralPath $Build -Filter 'pix-f8-*.wpix' -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $started } | ForEach-Object { "$($_.Name) $([math]::Round($_.Length/1MB)) MB" }) -join ', ')"
    if($Original -and -not $Dump){ & $say "  RenderDoc: $(@(Get-ChildItem -LiteralPath "$env:TEMP/RenderDoc" -Filter '*.rdc' -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $started } | ForEach-Object Name) -join ', ')" }
    $code=0
}catch{ & $say "THROW $_" }
finally{ if($placed){ foreach($f in $flagPath,$revivePath,$burstPath,$armPath){ if(Test-Path -LiteralPath $f){ Remove-Item -LiteralPath $f -Force } } }; & $say "EXIT $code"; Write-Output "EXIT $code" }
