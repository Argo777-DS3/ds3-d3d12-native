param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$Build,
    [ValidateSet('Native','Reference')][string]$Mode='Native',[ValidateSet('walk','sprint','turn','stand')][string]$Motion='walk',
    [ValidateRange(30,240)][int]$MotionSeconds=80,[string]$EffectFlags='',[int]$SampleStart=0,[int]$SampleEnd=0,[switch]$DebugLayer,
    # I-0081 (Thomas's lead): the collect timers off (Run-StartupTrace -NoKProfile, native measure runs only).
    [switch]$NoKProfile)
# I-0079: the same Firelink path (Eta's save at the shrine bonfire, Run-Motion's walk Z 4 s then S 4 s, sprint the same with
# Space held, camera turn L 5 s) under the native renderer or the original (-Mode Reference, with original-probe.flag so the
# original's present pacer is timed). Measure mode without the debug layer. -SampleStart N writes sample-profile-start.txt in
# the build folder (the sampler from world frame N, 75 s on the engine thread, the render worker and the pool; native only) and
# removes it after the run. One process; its trace job restores every protected file by hash.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
$eta='C:/Users/User/.ds3-i0039-backups/shebang-zones-eta-m40-20261014-095203/DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$flags=@($EffectFlags -split '[;,]' | Where-Object {$_})
if($Mode -eq 'Reference' -and $flags -notcontains 'original-probe'){ $flags+='original-probe' }
# The original's sampler needs sample-profile.flag beside the start file (its start counts the present pacer's calls).
if($Mode -eq 'Reference' -and $SampleStart -gt 0 -and $flags -notcontains 'sample-profile'){ $flags+='sample-profile' }
$start=Join-Path $Build 'sample-profile-start.txt'
if($SampleStart -gt 0){ Set-Content -LiteralPath $start -Value "$SampleStart $SampleEnd" -NoNewline }elseif(Test-Path -LiteralPath $start){ Remove-Item -LiteralPath $start }
# -SampleStart counts world (content) frames, -SampleEnd presented frames (the sampler's mark).
$seconds=60+40+$MotionSeconds+60;$debugOn=$DebugLayer.IsPresent;$noK=$NoKProfile.IsPresent   # plain values for the job (a cast in -ArgumentList arrived as a string: i79-10..14 ran with the debug layer)
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$save,$hash,$build,$mode,$flags,$debug,$noK)
    $native=if($mode -eq 'Native' -and -not $debug){@{Measure=$true;NoDebugLayer=$true}}else{@{}}   # the debug-layer switch is native-only; -DebugLayer: validation on
    if($mode -eq 'Native' -and -not $debug -and $noK){$native.NoKProfile=$true}
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode $mode -TestSave $save -TestSaveSha256 $hash -BuildFolder $build -EffectFlags $flags @native 2>&1 } -ArgumentList $inv,$RunName,$seconds,$eta,$etaHash,$Build,$Mode,($flags -join ','),$debugOn,$noK
$o="$repo/evidence/I-0066/artifacts/$RunName";$deadline=(Get-Date).AddSeconds(120)
function P([string[]]$a){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
try {
    while(-not (Test-Path -LiteralPath "$o/startup.jsonl") -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    pwsh -NoProfile -File "$inv/Pilot-TitleContinue.ps1" -Trace 'none' 2>&1 | Select-Object -Last 1
    Start-Sleep 40; if(Alive){ P '-Action','shot','-Path',"$o/start.png" }
    Add-Content -LiteralPath "$o/motion-marks.txt" -Value "motion_start $([Environment]::TickCount64) $(Get-Date -Format o)"
    $end=(Get-Date).AddSeconds($MotionSeconds);$step=0
    while((Get-Date) -lt $end -and (Alive)){
        switch($Motion){
            'walk' { P '-Action','hold','-Vk',"$([int][char]'Z')",'-Ms','4000'; P '-Action','hold','-Vk',"$([int][char]'S')",'-Ms','4000' }
            'sprint' { P '-Action','hold','-Vk',"$([int][char]'Z')",'-Ms','4000','-Vk2','32'; P '-Action','hold','-Vk',"$([int][char]'S')",'-Ms','4000','-Vk2','32' }
            'turn' { P '-Action','hold','-Vk',"$([int][char]'L')",'-Ms','5000' }
            'stand' { Start-Sleep 5 }
        }
        if(++$step -eq 2 -and (Alive)){ P '-Action','shot','-Path',"$o/motion.png" } }
    Add-Content -LiteralPath "$o/motion-marks.txt" -Value "motion_end $([Environment]::TickCount64) $(Get-Date -Format o)"
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout ($seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 3
if(Test-Path -LiteralPath $start){ Remove-Item -LiteralPath $start }
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
