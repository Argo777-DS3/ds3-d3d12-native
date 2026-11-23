param([Parameter(Mandatory)][string]$Tag,[string[]]$Modes=@('Reference','Native'),[int]$Seconds=420,[int]$StaySeconds=40,[string]$Look='',[string]$EffectFlags='',
    [int]$Slot=0,[int]$WalkMs=0,[int]$Forward=0,[string]$Teleport="",[int]$TeleportWait=25,[int]$Phantom=0,[string]$PhantomList="",[switch]$Summon,[switch]$ColorSelect,[switch]$FaceDetail,[string]$SummonLook="",[int]$PressMs=1200,[int]$SettleMs=600,[int]$ConfirmKey=0,[switch]$MotionBlur,[ValidateSet('LoadGame','CharMake')][string]$Pilot='LoadGame',[string]$TestSave='',[string]$TestSaveSha256='',[switch]$DeleteProfile,[switch]$InGameTaps,[string]$QualityPreset='',[switch]$EquipPage,[switch]$StatusPage,[switch]$Measure,[switch]$NoDebugLayer,[switch]$NoKProfile,[switch]$NoModelRecords,[switch]$OneFrameSlot,[switch]$NoWaitablePresent,[switch]$InlineRender,[switch]$InlinePrepare,[switch]$NoPersistentDraws,[switch]$GraphShadow,[switch]$GpuValidation,[switch]$HandBarriers,[string]$GraphicsLevels="",[ValidatePattern("^(|no-gamma|legacy-present|ui-cache|brightness=(10|[0-9]))$")][string]$DisplayFlags="",[string]$BuildFolder='',[switch]$OnClock)
# I-0066 (1186): announced Reference/Native pair on Thomas's save (Load Game row -Slot), one run per mode in order:
# Run-StartupTrace.ps1 in a job (it restores every protected file by hash, GraphicsConfig included), Pilot-LoadGameThomas.ps1
# in the foreground once the trace exists. -MotionBlur sets GraphicsConfig MotionBlur HIGH for both runs.
$ErrorActionPreference='Stop'
# pwsh -File passes "Reference,Native" as one string.
$Modes=@($Modes -split '[,;\s]+' | Where-Object { $_ })
if($TestSave -and -not $TestSaveSha256){ throw '-TestSave needs -TestSaveSha256' }
if($BuildFolder){ $BuildFolder=(Resolve-Path -LiteralPath $BuildFolder -ErrorAction Stop).Path }   # I-0113 tranche 35's step review L4: resolved here, not in the job
if($DeleteProfile -and -not $TestSave){ throw '-DeleteProfile needs -TestSave: a profile is deleted in the installed copy only (I-0113 tranche 31, the step review L6)' }
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
foreach($mode in $Modes){
    $run="i66-$($mode.ToLower())-$Tag"
    "[$((Get-Date).ToString('HH:mm:ss'))] start $run"
    # 1274: switches go to the job as ($x.IsPresent); "[bool]$x" in an argument list is the string "[bool]False", which is
    # true: until 1273 every pair run passed -MotionBlur (and 1271-1273 -Measure -NoDebugLayer) whatever was asked.
    $job=Start-Job -ScriptBlock { param($inv,$run,$mode,$s,$blur,$preset,$measure,$nodebug,$nokprofile,$norecords,$oneslot,$nowaitable,$inlinerender,$inlineprepare,$levels,$nokept,$graphshadow,$gpuvalidation,$handbarriers,$displayflags,$effectflags,$testsave,$testhash)
        $extra=@(); if($blur){ $extra+='-MotionBlur' }; if($displayflags -and $mode -eq "Native"){ $extra+="-DisplayFlags",$displayflags }; if($preset){ $extra+='-QualityPreset',$preset }; if($measure -and $mode -eq 'Native'){ $extra+='-Measure' }; if($nodebug -and $mode -eq 'Native'){ $extra+='-NoDebugLayer' }; if($nokprofile -and $mode -eq 'Native'){ $extra+='-NoKProfile' }; if($norecords -and $mode -eq 'Native'){ $extra+='-NoModelRecords' }; if($oneslot -and $mode -eq 'Native'){ $extra+='-OneFrameSlot' }; if($nowaitable -and $mode -eq 'Native'){ $extra+='-NoWaitablePresent' }; if($inlinerender -and $mode -eq 'Native'){ $extra+='-InlineRender' }; if($inlineprepare -and $mode -eq 'Native'){ $extra+='-InlinePrepare' }; if($nokept -and $mode -eq 'Native'){ $extra+='-NoPersistentDraws' }; if($graphshadow -and $mode -eq 'Native'){ $extra+='-GraphShadow' }; if($gpuvalidation -and $mode -eq 'Native'){ $extra+='-GpuValidation' }; if($handbarriers -and $mode -eq 'Native'){ $extra+='-HandBarriers' }; if($levels){ $extra+='-GraphicsLevels',$levels }; if($effectflags){ $extra+='-EffectFlags',$effectflags }
        if($testsave){ $extra+='-TestSave',$testsave,'-TestSaveSha256',$testhash }   # I-0113 tranche 31: a hash-recorded copy installed for the run
        if($using:BuildFolder){ $extra+='-BuildFolder',$using:BuildFolder }   # I-0113 tranche 35: a build of the thread's own (Run-StartupTrace -BuildFolder), both modes
        pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode $mode @extra 2>&1 } -ArgumentList $PSScriptRoot,$run,$mode,$Seconds,($MotionBlur.IsPresent),$QualityPreset,($Measure.IsPresent),($NoDebugLayer.IsPresent),($NoKProfile.IsPresent),($NoModelRecords.IsPresent),($OneFrameSlot.IsPresent),($NoWaitablePresent.IsPresent),($InlineRender.IsPresent),($InlinePrepare.IsPresent),$GraphicsLevels,($NoPersistentDraws.IsPresent),($GraphShadow.IsPresent),($GpuValidation.IsPresent),($HandBarriers.IsPresent),$DisplayFlags,$EffectFlags,$TestSave,$TestSaveSha256
    $trace="$repo/evidence/I-0066/artifacts/$run/startup.jsonl"
    $deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    # I-0113 tranche 27: -FaceDetail (with -Pilot CharMake) the creation screen's Face Detail page, zoomed (Pilot-CharMakeWitness).
    # 1250 -Pilot CharMake: New Game with movie screenshots (Pilot-CharMakeWitness.ps1 -MovieShots, -SkipVk with -ConfirmKey's value) instead of Load Game.
    if($Pilot -eq 'CharMake'){ try { pwsh -NoProfile -File "$PSScriptRoot/Pilot-CharMakeWitness.ps1" -Run $run -StaySeconds $StaySeconds -MovieShots -SkipVk $ConfirmKey -ColorSelect:$ColorSelect -FaceDetail:$FaceDetail -DeleteProfile:$DeleteProfile -OnClock:$OnClock 2>&1 } catch { "pilot error: $_" } }
    else { try { pwsh -NoProfile -File "$PSScriptRoot/Pilot-LoadGameThomas.ps1" -Run $run -Slot $Slot -StaySeconds $StaySeconds -Look $Look -WalkMs $WalkMs -Forward $Forward -Teleport $Teleport -TeleportWait $TeleportWait -Phantom $Phantom -PhantomList $PhantomList -Summon:($Summon.IsPresent) -SummonLook $SummonLook -PressMs $PressMs -SettleMs $SettleMs -ConfirmKey $ConfirmKey -InGameTaps:$InGameTaps -EquipPage:$EquipPage -StatusPage:$StatusPage 2>&1 } catch { "pilot error: $_" } }
    Wait-Job $job -Timeout ($Seconds+180) | Out-Null
    Receive-Job $job | Select-Object -Last 8
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ "game still alive after $run" }
    # D-0166 (options review): with a configuration edit, a run without a verified restoration stops the pair, so the next
    # run never backs up an edited configuration as Thomas's.
    if($GraphicsLevels){$restoration="$repo/evidence/I-0066/artifacts/$run/restoration.json"
        if($job.State -ne 'Completed' -or (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -or -not (Test-Path -LiteralPath $restoration) -or
           -not (Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok){ throw "pair stopped after $($run): no verified restoration" }}
    "[$((Get-Date).ToString('HH:mm:ss'))] end $run"
}
