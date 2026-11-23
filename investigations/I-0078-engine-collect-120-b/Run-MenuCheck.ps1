param([Parameter(Mandatory)][string]$RunName,[Parameter(Mandatory)][string]$Build)
# I-0078: the UI built on the render worker under the D3D12 debug layer: title, Continue (Epsilon at m45), the in-game menu
# and the Status screen (Pilot-StatusScreen.ps1: menu.jpg, status.jpg), one process whose trace job restores every protected
# file by hash.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
$eps='C:/Users/User/.ds3-i0039-backups/shebang-zones-epsilon-m45-20261013-045053/DS30000.sl2';$epsHash='8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C'
$job=Start-Job -ScriptBlock { param($inv,$run,$save,$hash,$build)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds 240 -SteamLaunch -BootstrapMode Native -TestSave $save -TestSaveSha256 $hash -BuildFolder $build 2>&1 } -ArgumentList $inv,$RunName,$eps,$epsHash,$Build
$o="$repo/evidence/I-0066/artifacts/$RunName";$deadline=(Get-Date).AddSeconds(120)
while(-not (Test-Path -LiteralPath "$o/startup.jsonl") -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
try { pwsh -NoProfile -File "$inv/Pilot-StatusScreen.ps1" -RunName $RunName 2>&1 | Select-Object -Last 3 } catch { "pilot error: $_" }
Wait-Job $job -Timeout 480 | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 4
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
