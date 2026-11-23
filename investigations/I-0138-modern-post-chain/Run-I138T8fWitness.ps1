# I-0138 tranche 8f: the native witness of GlareShape 3 (« Standard », cutscene 32000020) against the original at the same
# cutscene moments (D-0299): an earlier run's Run-AlphaCast arguments (by default i138-t8e-c32000020-orig, the original's shots 2 to
# 90 s into the scene at 1920x1080) replayed on a tranche 8f build in -Mode (Native by default), with -EffectFlags in place of the
# source's and -Seconds in place of its length when given. From Run-I138T8dWitness.ps1, which keeps the source's mode.
# Offline, the ASUS by its key, the save restored by hash (Run-AlphaCast -> Run-StartupTrace).
param([Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][string]$RunName,[Parameter(Mandatory)][string]$Log,
    [string]$Source='i138-t8e-c32000020-orig',[string]$Mode='Native',[string]$EffectFlags='test-no-ai,shaft-observe',[int]$Seconds=0,
    [string]$Screen='1920x1080@60')
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
try{
    $a=Get-Content -Raw "$repo/evidence/I-0066/artifacts/$Source/alphacast-args.json" | ConvertFrom-Json
    $p=@{}
    foreach($k in $a.bound){$p[$k]=$a.$k}
    $p.RunName=$RunName;$p.BuildFolder=$Build;$p.EffectFlags=$EffectFlags;$p.BootstrapMode=$Mode
    if($Seconds -gt 0){$p.Seconds=$Seconds}
    if($a.CutsceneShots){$p.CutsceneShots=[int[]]@($a.CutsceneShots)}
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device
    "START $(Get-Date -Format HH:mm:ss) $RunName from $Source in $Mode, flags $EffectFlags, levels $($p.GraphicsLevels), screen $device" | Out-File -FilePath $Log -Append -Encoding utf8
    & "$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1" @p *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
    $folder="$repo/evidence/I-0066/artifacts/$RunName"
    if(Test-Path -LiteralPath "$folder/startup.jsonl"){
        foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"(native_yebis_glare|native_yebis_shafts|native_render_job_failure)"' | Select-Object -First 12)){
            "  $($l.Line)" | Out-File -FilePath $Log -Append -Encoding utf8 } }
}catch{ "FAILED: $($_.Exception.Message)" | Out-File -FilePath $Log -Append -Encoding utf8 }
finally{ "END $(Get-Date -Format HH:mm:ss)" | Out-File -FilePath $Log -Append -Encoding utf8 }
