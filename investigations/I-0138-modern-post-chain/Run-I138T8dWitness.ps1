# I-0138 tranche 8d: the witness of Alpha_R and the motion blur outside the depth-of-field branch: an earlier run's Run-AlphaCast
# arguments replayed on a tranche 8d build, in that run's own mode (its BootstrapMode kept: i141-t14a-c45000030-orig for the original's
# reference_shaft_depth rows, i141-t14a-c45000030-nat for the native's native_yebis_motionblur rows), with -EffectFlags in place of the
# source's, -GraphicsLevels added to its own (this PC's GraphicsConfig has the depth of field and the motion blur off) and -Seconds in
# place of its length when given. From Run-I138T8cWitness.ps1 (tranche 8c), which stays as its witnesses ran it.
param([Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][string]$RunName,[Parameter(Mandatory)][string]$Log,
    [Parameter(Mandatory)][string]$Source,[Parameter(Mandatory)][string]$EffectFlags,[string]$GraphicsLevels='DepthOfField=HIGH;MotionBlur=HIGH',
    [int]$Seconds=0)
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
try{
    $a=Get-Content -Raw "$repo/evidence/I-0066/artifacts/$Source/alphacast-args.json" | ConvertFrom-Json
    $p=@{}
    foreach($k in $a.bound){$p[$k]=$a.$k}
    $p.RunName=$RunName;$p.BuildFolder=$Build;$p.EffectFlags=$EffectFlags
    if($Seconds -gt 0){$p.Seconds=$Seconds}
    if($GraphicsLevels){$p.GraphicsLevels=(@($a.GraphicsLevels,$GraphicsLevels) | Where-Object { $_ }) -join ';'}
    if($a.CutsceneShots){$p.CutsceneShots=[int[]]@($a.CutsceneShots)}
    "START $(Get-Date -Format HH:mm:ss) $RunName from $Source ($($a.BootstrapMode)), flags $EffectFlags, levels $($p.GraphicsLevels)" | Out-File -FilePath $Log -Append -Encoding utf8
    & "$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1" @p *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
}catch{ "FAILED: $($_.Exception.Message)" | Out-File -FilePath $Log -Append -Encoding utf8 }
finally{ "END $(Get-Date -Format HH:mm:ss)" | Out-File -FilePath $Log -Append -Encoding utf8 }
