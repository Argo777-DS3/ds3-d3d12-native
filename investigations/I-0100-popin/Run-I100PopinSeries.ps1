param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Series,[Parameter(Mandatory)][string]$Build,
    [string[]]$Runs=@('n1','o1','n2','o2'),[string]$PathFile='',[string]$Screen='1920x1080@60',[string]$EffectFlags='popin-observe;test-no-ai',
    [double]$Every=2.0,[int]$CaptureSeconds=330,[ValidateRange(0.1,1.0)][double]$Scale=1.0,[string]$GraphicsLevels='')
# I-0100 T0 (POPIN-STUDY §4 M1, M3): a series of announced runs (D-0116) on one replayed path, native (n*) and original (o*)
# alternated, each with popin-observe.flag (the per-object rows of what each renderer draws; Get-PopinJournal.ps1 reads them)
# and pictures of -Screen every -Every seconds from each run's call (scripts/Capture-ScreenSeries.ps1, hidden, D-0199). The
# native window is placed on the screen before its swap chain (-LaunchOnScreen). Runs: evidence/I-0066/artifacts/<series>-<run>;
# pictures: evidence/I-0100/captures/<series>-<run>. Study runs, never a gate record (test-no-ai). Tranche 8: -Scale (pictures reduced,
# for 10 a second, -Every 0.1) and the loads' rows read by Get-I100Load.ps1; with AI (-EffectFlags popin-observe) the path's deaths reload.
# I-0113 tranche 19: -GraphicsLevels passed to Invoke-ReplayRun (the OLED's 2560x1440 window: Resolution-WindowScreenWidth=2560;Resolution-WindowScreenHeight=1440).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(-not $PathFile){ $PathFile="$repo/scripts/replay/settlement.path.json" }
foreach($run in $Runs){
    $name="$Series-$run";$shots="$repo/evidence/I-0100/captures/$name"
    "== $name $(Get-Date -Format HH:mm:ss)"
    $capture=Start-Process pwsh -WindowStyle Hidden -PassThru -ArgumentList "-NoProfile -File `"$repo/scripts/Capture-ScreenSeries.ps1`" -Out `"$shots`" -Seconds $CaptureSeconds -Every $Every -Screen $Screen -Scale $Scale"
    try {
        if($run.StartsWith('o')){ & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $name -Build $Build -Kind Reference -PathFile $PathFile -Screen $Screen -EffectFlags $EffectFlags -GraphicsLevels $GraphicsLevels 2>&1 | Select-Object -Last 2 }
        else{ & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $name -Build $Build -Kind Measure -PathFile $PathFile -Screen $Screen -LaunchOnScreen -EffectFlags $EffectFlags -GraphicsLevels $GraphicsLevels 2>&1 | Select-Object -Last 2 }
    } catch { "run error: $_" }
    finally { if(-not $capture.HasExited){ Stop-Process -Id $capture.Id -Force -ErrorAction SilentlyContinue } }
}
"== end $(Get-Date -Format HH:mm:ss)"
