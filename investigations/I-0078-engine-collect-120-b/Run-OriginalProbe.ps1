param([Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][int]$First,[string[]]$Only=@('m45','m45unlock','walkunlock'))
# I-0078 (Thomas, 2026-10-20: the original as a reference study): the original renderer (a reference run) with only its present
# pacer timed (original-probe.flag, no observation hook), at m45 (Epsilon) and on the Firelink walk (Eta), with
# original-unlock.flag for the unlocked frame rate (requested interval 0, in memory only). Runs i78-original-NN-<scene>; each is
# one process whose trace job restores every protected file by hash. Read with Get-OriginalFrames.ps1.
$ErrorActionPreference='Continue'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$inv="$repo/investigations/I-0066-independent-native-startup"
$eta='C:/Users/User/.ds3-i0039-backups/shebang-zones-eta-m40-20261014-095203/DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$eps='C:/Users/User/.ds3-i0039-backups/shebang-zones-epsilon-m45-20261013-045053/DS30000.sl2';$epsHash='8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C'
$walk='Z5000,D1500,Z5000,Q1500,Z5000,S6000,Z5000'
$n=$First
function Next([string]$what){ $name='i78-original-{0:D2}-{1}' -f $script:n,$what; $script:n++; Write-Host "== $name $(Get-Date -Format HH:mm:ss)"; return $name }
function Idle{ $t=(Get-Date).AddMinutes(3); while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 } }
foreach($scene in @($Only -split "[,; ]+" | Where-Object {$_})){
    switch($scene){
        'm45' { $r=Next 'm45'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -BootstrapMode Reference -TestSave $eps -TestSaveSha256 $epsHash -EffectFlags 'original-probe' -SettleSeconds 40 -Shots 1 -StaySeconds 90 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'm45unlock' { $r=Next 'm45-unlock'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -BootstrapMode Reference -TestSave $eps -TestSaveSha256 $epsHash -EffectFlags 'original-probe,original-unlock' -SettleSeconds 40 -Shots 1 -StaySeconds 90 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'walkunlock' { $r=Next 'm40-walk-unlock'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 300 -BootstrapMode Reference -TestSave $eta -TestSaveSha256 $etaHash -EffectFlags 'original-probe,original-unlock' -SettleSeconds 40 -KeyPath $walk -Shots 1 -StaySeconds 20 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'walk' { $r=Next 'm40-walk'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 300 -BootstrapMode Reference -TestSave $eta -TestSaveSha256 $etaHash -EffectFlags 'original-probe' -SettleSeconds 40 -KeyPath $walk -Shots 1 -StaySeconds 20 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        # The same with the sampler on the presenting thread and the main thread (sample-profile.flag, from the 1,200th frame).
        'm45sample' { $r=Next 'm45-sample'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -BootstrapMode Reference -TestSave $eps -TestSaveSha256 $epsHash -EffectFlags 'original-probe,sample-profile' -SettleSeconds 40 -Shots 1 -StaySeconds 90 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'm45unlocksample' { $r=Next 'm45-unlock-sample'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -BootstrapMode Reference -TestSave $eps -TestSaveSha256 $epsHash -EffectFlags 'original-probe,original-unlock,sample-profile' -SettleSeconds 40 -Shots 1 -StaySeconds 90 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        default { throw "unknown scene $scene" }
    }
    Idle
}
"== series end $(Get-Date -Format HH:mm:ss)"
