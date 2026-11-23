# I-0138 tranche 8c: the witness of the glare shapes other than 1: an earlier native run's Run-AlphaCast arguments replayed on a
# tranche 8c build (by default i141-t14h-c32000010-nat's: 32000010, where shape 0 and shape 3 are met). -GraphicsLevels adds the
# levels the witness needs to the source run's own (the final review's M2: this PC's GraphicsConfig has the depth of field and the
# motion blur off, so their rows would read the same before and after the tranche).
param([Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][string]$RunName,[Parameter(Mandatory)][string]$Log,
    [string]$Source='i141-t14h-c32000010-nat',[string]$GraphicsLevels='DepthOfField=HIGH;MotionBlur=HIGH')
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
try{
    $a=Get-Content -Raw "$repo/evidence/I-0066/artifacts/$Source/alphacast-args.json" | ConvertFrom-Json
    $p=@{}
    foreach($k in $a.bound){$p[$k]=$a.$k}
    $p.RunName=$RunName;$p.BuildFolder=$Build;$p.EffectFlags='test-no-ai,world-pictures,pictures-from-cutscene'
    if($GraphicsLevels){$p.GraphicsLevels=(@($a.GraphicsLevels,$GraphicsLevels) | Where-Object { $_ }) -join ';'}
    if($a.CutsceneShots){$p.CutsceneShots=[int[]]@($a.CutsceneShots)}
    & "$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1" @p *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
}catch{ "FAILED: $($_.Exception.Message)" | Out-File -FilePath $Log -Append -Encoding utf8 }
finally{ "END $(Get-Date -Format HH:mm:ss)" | Out-File -FilePath $Log -Append -Encoding utf8 }
