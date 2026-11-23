# I-0113 gap 8 (streaks on the character at the Firelink Shrine bonfire, Thomas 2026-10-31: right on a first load, wrong after a
# return to the title and a reload): the Firelink save (Eta at the bonfire) loaded, half a camera turn and a step forward (the bonfire behind the character, as Thomas's 07:18 capture), -Shots shots 3 s apart (load1-<n>.png), Quit
# Game to the title (title.png), Continue again (Pilot-TitleContinue: the offline notice comes back), the same shots (load2-<n>.png), the window closed. One process: Run-StartupTrace.ps1
# in a job restores every protected file by hash; the module's native_local_shadow rows give each load's local shadows and their
# caster rasterizer. -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateSet('Native','Reference')][string]$BootstrapMode='Native',[ValidateRange(1,6)][int]$Shots=3,[string]$NotBefore='',[ValidateRange(180,900)][int]$Seconds=420)
$ErrorActionPreference='Stop'
$d=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup';$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$hash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){'skipped: DarkSoulsIII already runs';return}
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$mode,$save,$hash,$build)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode $mode -TestSave $save -TestSaveSha256 $hash -EffectFlags test-no-ai -BuildFolder $build 2>&1 } -ArgumentList $d,$RunName,$Seconds,$BootstrapMode,$save,$hash,$BuildFolder
$o="$repo/evidence/I-0066/artifacts/$RunName";$trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
function Turn{ if(Alive){ P '-Action','look','-X','2000','-Y','0'; Start-Sleep 2; P '-Action','hold','-Vk','90','-Ms','900'; Start-Sleep 3 } }   # half a turn, then forward: the bonfire behind the character
function Shoot([string]$name){ for($k=1;$k -le $Shots -and (Alive);$k++){ P '-Action','shot','-Path',"$o/$name-$k.png"; Start-Sleep 3 } }
try {
    $deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace $(if($BootstrapMode -eq 'Native'){$trace}else{'none'}) 2>&1 | Select-Object -Last 1
    Start-Sleep 30; Turn; Shoot 'load1'
    # Quit Game (Pilot-QuitAndKeepSave's clicks, without its save copy), then Continue from the title.
    P '-Action','key','-Vk','27'; Start-Sleep -Milliseconds 1500
    P '-Action','click','-X','525','-Y','390'; Start-Sleep -Milliseconds 1500
    P '-Action','click','-X','742','-Y','191'; Start-Sleep -Milliseconds 1500
    P '-Action','click','-X','648','-Y','556'; Start-Sleep -Milliseconds 1500
    P '-Action','click','-X','501','-Y','428'; Start-Sleep -Seconds 15
    if(Alive){ P '-Action','shot','-Path',"$o/title.png" }
    pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace $(if($BootstrapMode -eq 'Native'){$trace}else{'none'}) 2>&1 | Select-Object -Last 1   # the network notice again, then Continue
    Start-Sleep 35; Turn; Shoot 'load2'
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 4
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
