# I-0113 gap 3 (a Load Game portrait missing now and then: Delta's, the Dreg Heap character, in i100-tr7-night's sixth list): the
# final zone save (eight characters) opened natively, the title reached (Pilot-TitleContinue -MenuY 0), then -Cycles times: Load
# Game clicked (648, 555 on a 1280x720 client at (8, 31)), a shot after -ListMs (list-<k>.png), Escape back to the title. One
# process: Run-StartupTrace.ps1 in a job restores every protected file by hash; the module's native_offscreen_view_state rows name
# each portrait's outcome when it changes. -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateRange(1,40)][int]$Cycles=20,[ValidateRange(1000,20000)][int]$ListMs=4000,[string]$NotBefore='',[ValidateRange(120,900)][int]$Seconds=600)
$ErrorActionPreference='Stop'
$d=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup';$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-final-20261013-045344\DS30000.sl2'
$hash=(Get-Content -LiteralPath "$repo/evidence/I-0066/artifacts/i100-tr7-night/baseline.json" -Raw | ConvertFrom-Json).test_save.sha256
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){'skipped: DarkSoulsIII already runs';return}
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$save,$hash,$build)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode Native -TestSave $save -TestSaveSha256 $hash -BuildFolder $build 2>&1 } -ArgumentList $d,$RunName,$Seconds,$save,$hash,$BuildFolder
$o="$repo/evidence/I-0066/artifacts/$RunName";$trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
try {
    $deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace $trace -MenuY 0 2>&1 | Select-Object -Last 1
    for($k=1;$k -le $Cycles -and (Alive);$k++){
        P '-Action','click','-X','648','-Y','555'; Start-Sleep -Milliseconds $ListMs
        if(Alive){ P '-Action','shot','-Path',"$o/list-$k.png" }
        P '-Action','key','-Vk','27'; Start-Sleep 3 }
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 4
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
