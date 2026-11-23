[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{7,12}$')][string]$Commit,[string]$BuildFolder='',
    [string]$Screen='1920x1080@60',   # the ASUS, by its key (scripts/Get-Screens.ps1; final review L5: device names can change)
    [string]$GraphicsLevels='')       # WORKFLOW (Thomas, 2026-11-06): the OLED's checks in a 2560x1440 window, through Run-LiveWitness
# D-0229 (Thomas, 2026-10-28 03:33: « Une minute je veux bien, pas dix »): the start check after a merge. One announced run
# (D-0116: offline, the saves restored by hash) of the launcher's module: launched, carried past the title to the world on the
# Firelink path's save, then closed as by its close box (WM_SYSCOMMAND SC_CLOSE) 5 s after the first world frame, so the quit
# is checked too; the controller's limit (110 s) is only a bound. No measure, no input beyond the title. Passes when the world
# was drawn, the module neither refused nor failed, the game left by itself after the close (not stopped or killed by the
# controller) and the restoration held. The network-stop row (I-0099) is reported; any crash in the run fails the check, the exit heap fault at the quit included (STUDY step 4).
# I-0109: the device's capabilities and the ray-tracing self-test rows are reported; a self-test that ran and failed fails the check.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
if(-not $BuildFolder){$BuildFolder="$repo/evidence/I-0066/artifacts/game-bootstrap-build"}
$recording=Get-Content -LiteralPath "$repo/scripts/replay/firelink.path.json" -Raw | ConvertFrom-Json
$run="start-$Commit"
$out="$repo/evidence/I-0066/artifacts/$run"
$trace="$out/startup.jsonl"
# Step review M1: an earlier run of the same name would be read as this one (and its world row would close another game).
if(Test-Path -LiteralPath $out){throw "Run $run exists: never overwrite evidence"}
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){throw 'DarkSoulsIII is already running'}
$device=(& "$PSScriptRoot/Get-Screens.ps1" -Key $Screen).device   # refuses a screen not attached
$closer=Start-Job -ScriptBlock { param($trace)
    Add-Type -Namespace StartCheck -Name User32 -MemberDefinition '[DllImport("user32.dll")] public static extern bool PostMessage(System.IntPtr h,uint m,System.IntPtr w,System.IntPtr l);'
    $deadline=(Get-Date).AddSeconds(90)
    while((Get-Date) -lt $deadline -and -not ((Test-Path -LiteralPath $trace) -and (Select-String -LiteralPath $trace -Pattern '"native_model_draw_submitted"' -SimpleMatch -Quiet))){Start-Sleep -Milliseconds 500}
    if((Get-Date) -ge $deadline){return [pscustomobject]@{text='no world frame: not closed';exited=$false}}
    Start-Sleep -Seconds 5
    $game=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1
    if(-not $game){return [pscustomobject]@{text='game gone before the close';exited=$false}}
    if($game.MainWindowHandle -eq [IntPtr]::Zero){return [pscustomobject]@{text='no game window: not closed';exited=$false}}
    if(-not [StartCheck.User32]::PostMessage($game.MainWindowHandle,0x0112,[IntPtr]0xF060,[IntPtr]::Zero)){return [pscustomobject]@{text='close not posted';exited=$false}}
    $closed=$game.WaitForExit(30000)
    [pscustomobject]@{text="close posted at $((Get-Date).ToString('HH:mm:ss')), exited: $closed";exited=$closed} } -ArgumentList $trace
$previous=$env:DS3_PILOT_SCREEN;$env:DS3_PILOT_SCREEN=$device
$levels=@(if($GraphicsLevels){'-GraphicsLevels',$GraphicsLevels})
try{
    pwsh -NoProfile -File "$repo/investigations/I-0066-independent-native-startup/Run-LiveWitness.ps1" -RunName $run -Seconds 110 `
        -TestSave $recording.save.path -TestSaveSha256 $recording.save.sha256 -BuildFolder $BuildFolder -NoDebugLayer -EffectFlags rt-fail-magenta @levels 2>&1 | Select-Object -Last 4
}finally{Wait-Job $closer -Timeout 150 | Out-Null;$close=Receive-Job $closer | Select-Object -Last 1;Remove-Job $closer -Force
    if($null -eq $previous){Remove-Item Env:DS3_PILOT_SCREEN -ErrorAction SilentlyContinue}else{$env:DS3_PILOT_SCREEN=$previous}}
$world=0;$magenta=0;$causes=0;$arrivals=@();$fatal='';$refusals=@();$rtOff=@();$failures=@();$networkStop='';$soundStop='';$deviceCaps='';$rtSelfTest='';$exposure='';$exposureFailed='';$taa='';$taaOff='';$dlssChosen=$false;$aaOn=$false;$unresolved=0
if(Test-Path -LiteralPath $trace){foreach($line in [IO.File]::ReadLines($trace)){
    if($line.Contains('"native_model_draw_submitted"')){$world++}
    # Step review L2: prefix_refused at setup too. I-0125 R6f's final review H1 (the rays the default, D-0253): by the event's name
    # only (the rays' row carries a "pt_refused":false key).
    elseif($line -match '^\{"event":"(refusal|[a-z_]+_refused)"'){$refusals+=$line}
    # D-0253: a traced path latched off in the run (its row's "on":false) fails the check when the self-test passed.
    elseif($line -match '^\{"event":"native_rt_(scene|shadow|lights|reflect|water|gi)",.*"on":false'){$rtOff+=$line}
    elseif($line.Contains('"event":"native_failure"')){$failures+=$line}
    elseif($line.Contains('"native_exit_network_stop"')){$networkStop=$line}
    elseif($line.Contains('"native_exit_sound_stop"')){$soundStop=$line}   # I-0124
    elseif($line.Contains('"native_device_caps"')){$deviceCaps=$line}   # I-0109 (R0)
    elseif($line.Contains('"native_rt_selftest"')){$rtSelfTest=$line}
    # I-0132 R11h (D-0264, the automatic exposure on by default): its last reading, and its latched failure (fails the check).
    elseif($line.Contains('"native_auto_exposure_failed"')){$exposureFailed=$line}
    elseif($line.Contains('"native_auto_exposure"')){$exposure=$line}
    # I-0132 R11j (D-0265, the TAA the default without a DLSS mode): its first frames' row, and its latch (fails the check).
    elseif($line.Contains('"native_taa_latched_off"')){$taaOff=$line}
    elseif($line.Contains('"native_taa_frames"') -and -not $taa){$taa=$line}
    # The step review's M-a: with no DLSS mode chosen and the Antialiasing option on, the TAA must have run (it is the default).
    elseif($line.Contains('"native_dlss_mode"') -and $line.Contains('"chosen":true')){$dlssChosen=$true}
    # The final review's L1: jittered frames neither DLSS nor the TAA resolved, beyond the first two (a frame or two before the
    # velocity or at a latch), fail the check.
    elseif($line.Contains('"native_dlss_unresolved"') -and $line -match '"frames":(\d+)'){$unresolved=[math]::Max($unresolved,[int64]$Matches[1])}
    elseif($line.Contains('"native_dlss_option"') -and $line -match '"aa_byte":(\d+),"read":true'){$aaOn=[int]$Matches[1] -ne 0}
    # I-0132 R11d (Thomas, 2026-11-09): the frames the rays did not serve, shown magenta (rt-fail-magenta.flag), reported from the module's
    # native_rt_fail_magenta rows (written as the count changes) with the OR of their causes (1 sun, 2 top level, 4 GI, 8 local lights).
    if($line.Contains('"native_rt_fail_magenta"') -and $line -match '"frames":(\d+),"causes":(\d+)'){$magenta=[math]::Max($magenta,[int64]$Matches[1]);$causes=$causes -bor [int]$Matches[2]}
    # I-0132 R11e (D-0262, no fallback): each arrival's hold (its reason and milliseconds), and the fatal close, which fails the check.
    if($line.Contains('"native_rt_arrival"') -and $line -match '"reason":"(\w+)","walks":(\d+),"ms":(\d+)'){$arrivals+="$($Matches[1]) $($Matches[3]) ms ($($Matches[2]) walks)"}
    if($line.Contains('"native_rt_fatal"')){$fatal=$line}}}
# I-0109: the ray-tracing self-test that ran inside DS3 must have passed (one not run, flag or adapter, is only reported).
$rtFailed=($rtSelfTest.Contains('"ran":true') -and -not $rtSelfTest.Contains('"passed":true')) -or ($rtSelfTest.Contains('"passed":true') -and $rtOff.Count -gt 0)
# I-0124 (final review M1): the quit's sound stop, when its row is written, must have run (signalled) or been left out by its flag; a
# timeout, a failure or a heap already destroyed fails the check (the other skips are reported only).
$soundFailed=$soundStop -match '"result":"(timeout_helper_left|failed|no_helper|skipped_heap_destroyed)"'
$ready='';$exit=$null;$crash=''
if(Test-Path -LiteralPath "$out/controller.jsonl"){$rows=@(Get-Content -LiteralPath "$out/controller.jsonl");$ready=$rows[0]
    # Final review M1: 0xD3126612 is the controller stopping the game at its bound and killing it after its drain, not the game
    # leaving by itself (0xD3126611, its kill before the module is ready, never reaches this row; counted for safety).
    # Any crash in the run fails the check, the heap fault 0xC0000374 included since I-0099's fix and its STUDY step 4.
    foreach($row in $rows){if($row -match '"exit":(\d+)'){$exit=[uint32]$Matches[1]
        if($row -match '"crash_kind":"([^"]*)","crash_code":"([^"]*)"' -and $Matches[1] -ne 'none'){$crash="$($Matches[1]) $($Matches[2])"}}}}
$killed=$exit -in 3541198353,3541198354   # 0xD3126611, 0xD3126612 (the controller prints the code in decimal)
$crashed=[bool]$crash
$restored=if(Test-Path -LiteralPath "$out/restoration.json"){(Get-Content -LiteralPath "$out/restoration.json" -Raw | ConvertFrom-Json).ok}else{$false}
[ordered]@{run=$run;ready=$ready;controller_exit=$(if($null -ne $exit){'0x{0:X8}' -f $exit}else{''});world_frames=$world;close=$close.text;crash=$crash;
    network_stop=$networkStop;sound_stop=$soundStop;device_caps=$deviceCaps;rt_selftest=$rtSelfTest;auto_exposure=$exposure;auto_exposure_failed=$exposureFailed;taa=$taa;taa_latched=$taaOff;dlss_chosen=$dlssChosen;aa_on=$aaOn;unresolved_jittered=$unresolved;fail_magenta_frames=$magenta;fail_magenta_causes=$causes;arrivals=@($arrivals | Select-Object -First 4);rt_fatal=$fatal;refusals=@($refusals | Select-Object -First 3);rt_off=@($rtOff | Select-Object -First 3);failures=@($failures | Select-Object -First 3);
    restored=$restored;passed=($world -gt 0 -and -not $refusals -and -not $failures -and [bool]$close.exited -and -not $killed -and -not $crashed -and $restored -and -not $rtFailed -and -not $soundFailed -and -not $fatal -and -not $exposureFailed -and -not $taaOff -and ($taa -or $dlssChosen -or -not $aaOn) -and $unresolved -le 2)} | ConvertTo-Json -Depth 3
