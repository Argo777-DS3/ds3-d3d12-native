[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[ValidateSet('Native','Reference')][string]$BootstrapMode='Native',
    [string]$EffectFlags='',[ValidateRange(120,900)][int]$Seconds=260,[int]$Look1=250,[int]$Look2=-90,[ValidateRange(0,6000)][int]$WalkMs=3000,
    [int]$AimLook=0,[ValidateRange(0,3000)][int]$StepMs=0,[ValidateRange(0,20)][int]$Hits=6,[ValidateRange(1,10)][int]$HitEvery=2,
    [int[]]$ShotsMs=@(300,900),[ValidateRange(0,120)][int]$AfterSeconds=20,[string]$BuildFolder='')
# DECALS-WAVE1 (Thomas, 2026-10-18: striking the Fire Keeper is allowed and harmless): one announced run, as one process, on the
# Shebang Eta save at Firelink Shrine (m40 zone save, Eta bare-handed): Run-StartupTrace.ps1 in a job (it restores every protected
# file by hash), Pilot-TitleContinue.ps1 (notices, Continue), then from Eta's spot at the bonfire the camera turned (-Look1, then
# -Look2), -WalkMs forward (AZERTY Z), the aim (-AimLook, then a -StepMs step so the character faces the camera), -Hits punches
# (left click, the right hand's light attack) every -HitEvery seconds with screenshots -ShotsMs after each (hit-<k>-<ms>.png),
# shots every 5 s for -AfterSeconds (after-<s>.png), then the window closed and the restoration awaited.
$ErrorActionPreference='Stop'
$d=$PSScriptRoot;$repo=Split-Path -Parent (Split-Path -Parent $d)
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$hash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$extra=@();if($EffectFlags){$extra+='-EffectFlags',$EffectFlags};if($BuildFolder){$extra+='-BuildFolder',$BuildFolder}   # D-0193: a session's own build folder, as Run-ZoneWalk
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$mode,$save,$hash,$extra)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode $mode -TestSave $save -TestSaveSha256 $hash @extra 2>&1 } -ArgumentList $d,$RunName,$Seconds,$BootstrapMode,$save,$hash,$extra
$o="$repo/evidence/I-0066/artifacts/$RunName";$trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
try {
    $deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    $pilotTrace=if($BootstrapMode -eq 'Native'){$trace}else{'none'}
    pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace $pilotTrace 2>&1 | Select-Object -Last 2
    Start-Sleep 30; P '-Action','shot','-Path',"$o/ingame.png"
    P '-Action','look','-X',"$Look1",'-Y','0'; Start-Sleep 2; P '-Action','look','-X',"$Look2",'-Y','0'; Start-Sleep 1
    if($WalkMs){ P '-Action','hold','-Vk','90','-Ms',"$WalkMs" }; Start-Sleep 1; P '-Action','shot','-Path',"$o/walk.png"
    if($AimLook){ P '-Action','look','-X',"$AimLook",'-Y','0'; Start-Sleep 1 }; if($StepMs){ P '-Action','hold','-Vk','90','-Ms',"$StepMs" }; Start-Sleep 1
    P '-Action','shot','-Path',"$o/aim.png"
    for($k=1;$k -le $Hits;$k++){ if(-not (Alive)){ 'game gone'; break }
        P '-Action','click','-X','648','-Y','300';$t0=Get-Date
        foreach($ms in $ShotsMs){ $wait=$ms-((Get-Date)-$t0).TotalMilliseconds; if($wait -gt 0){ Start-Sleep -Milliseconds ([int]$wait) }; P '-Action','shot','-Path',"$o/hit-$k-$ms.png" }
        "hit $k $($t0.ToUniversalTime().ToString('o'))"; Start-Sleep $HitEvery }
    for($s=5;$s -le $AfterSeconds -and (Alive);$s+=5){ Start-Sleep 5; P '-Action','shot','-Path',"$o/after-$s.png" }
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 6
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing: run Restore-StartupTrace.ps1 on the baseline' }
