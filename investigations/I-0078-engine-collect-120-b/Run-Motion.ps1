param([Parameter(Mandatory)][string]$RunName,[Parameter(Mandatory)][string]$Build,[ValidateSet('walk','sprint','turn')][string]$Motion='walk',
    [ValidateRange(30,240)][int]$MotionSeconds=90)
# I-0078 (Thomas, 2026-10-21: measure while walking, sprinting with Space held, and turning the camera): Firelink (Eta's save at
# the shrine bonfire), measure mode without the debug layer, the collect split on (as the series' m45split); after the load and
# a 40 s settle, the motion repeated for -MotionSeconds: walk Z 4 s then S 4 s; sprint the same with Space held (Pilot-Game -Vk2);
# turn the camera L 5 s at a time (AZERTY keys as Run-ZoneWalk). One process; its trace job restores every protected file by
# hash. The windows of the motion are read with Get-CollectSplit.ps1 (the first ones, the load and settle, skipped by -Skip).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
$eta='C:/Users/User/.ds3-i0039-backups/shebang-zones-eta-m40-20261014-095203/DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$seconds=60+40+$MotionSeconds+60
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$save,$hash,$build)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode Native -TestSave $save -TestSaveSha256 $hash -Measure -NoDebugLayer -BuildFolder $build 2>&1 } -ArgumentList $inv,$RunName,$seconds,$eta,$etaHash,$Build
$o="$repo/evidence/I-0066/artifacts/$RunName";$deadline=(Get-Date).AddSeconds(120)
function P([string[]]$a){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
try {
    while(-not (Test-Path -LiteralPath "$o/startup.jsonl") -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    pwsh -NoProfile -File "$inv/Pilot-TitleContinue.ps1" -Trace 'none' 2>&1 | Select-Object -Last 1
    Start-Sleep 40; if(Alive){ P '-Action','shot','-Path',"$o/start.png" }
    $end=(Get-Date).AddSeconds($MotionSeconds);$step=0
    while((Get-Date) -lt $end -and (Alive)){
        switch($Motion){
            'walk' { P '-Action','hold','-Vk',"$([int][char]'Z')",'-Ms','4000'; P '-Action','hold','-Vk',"$([int][char]'S')",'-Ms','4000' }
            'sprint' { P '-Action','hold','-Vk',"$([int][char]'Z')",'-Ms','4000','-Vk2','32'; P '-Action','hold','-Vk',"$([int][char]'S')",'-Ms','4000','-Vk2','32' }
            'turn' { P '-Action','hold','-Vk',"$([int][char]'L')",'-Ms','5000' }
        }
        if(++$step -eq 2 -and (Alive)){ P '-Action','shot','-Path',"$o/motion.png" } }
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout ($seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 3
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
