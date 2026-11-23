[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$BuildFolder,
    [string]$SaveCopy='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2',
    [string]$SaveCopySha256='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4',
    [string]$Revive='14000800 14000830',[ValidateRange(300,900)][int]$Seconds=420,[ValidateRange(0,120)][int]$ArenaWait=40,[string]$TravelPress='',[int]$ArriveSeconds=25,[string]$LookSweep='')
# I-0089, one announced live run (D-0116/D-0117), as one process (the trace job dies with the shell that started it): the zone
# save copy of the Cemetery of Ash (character Eta at its bonfire) installed by Run-StartupTrace.ps1 (restored by hash), with
# test-tools.flag and test-revive.txt placed next to the module of -BuildFolder (removed after): the title, the two offline
# notices, Continue, the world; F4 (invincibility on) with a shot; the bonfire rest (E) that reloads the zone with the revived
# bosses; a short walk toward the arena with shots; -ArenaWait seconds in front of the arena; F4 again (off); the window closed
# and the job's restoration awaited. The module's rows (test_*) are in startup.jsonl of the run.
$ErrorActionPreference='Stop'
$inv=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup';$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ throw 'game already running' }
$flagPath=Join-Path $BuildFolder 'test-tools.flag';$revivePath=Join-Path $BuildFolder 'test-revive.txt'
Set-Content -LiteralPath $flagPath -Value 'test';Set-Content -LiteralPath $revivePath -Value $Revive -Encoding ascii
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$save,$hash,$build)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode Native -TestSave $save -TestSaveSha256 $hash -BuildFolder $build 2>&1 } -ArgumentList $inv,$RunName,$Seconds,$SaveCopy,$SaveCopySha256,$BuildFolder
$o="$repo/evidence/I-0066/artifacts/$RunName";$trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
function Seen([string]$pattern){ (Test-Path -LiteralPath $trace) -and [bool](Select-String -LiteralPath $trace -Pattern $pattern -Quiet) }
try {
    $deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    $deadline=(Get-Date).AddSeconds(240); while(-not (Seen '"name":"05_000_Title"') -and (Get-Date) -lt $deadline){ Start-Sleep 1 }
    function Closed{ if(-not (Test-Path -LiteralPath $trace)){return 0}; @(Select-String -LiteralPath $trace -Pattern '"event":"menu_window_close"').Count }
    Start-Sleep 8; $base=Closed; P '-Action','click','-X','600','-Y','15'; Start-Sleep 1
    for($k=0;$k -lt 6 -and (Closed) -lt $base+1;$k++){ P '-Action','press','-X','648','-Y','427','-Ms','1200'; Start-Sleep 4 }
    for($k=0;$k -lt 6 -and (Closed) -lt $base+2;$k++){ P '-Action','press','-X','648','-Y','450','-Ms','1200'; Start-Sleep 4 }
    $loaded='"title_step_state".*"state":4,'
    for($k=1;$k -le 4 -and -not (Seen $loaded);$k++){ P '-Action','press','-X','648','-Y','525','-Ms','1200'
        $until=(Get-Date).AddSeconds(25); while(-not (Seen $loaded) -and (Get-Date) -lt $until){ Start-Sleep 1 } }
    "loaded=$(Seen $loaded)"
    Start-Sleep 45; P '-Action','shot','-Path',"$o/ingame.png"
    "revive row: $(Seen '"event":"test_revive"')"
    P '-Action','key','-Vk','115'; Start-Sleep 3; P '-Action','shot','-Path',"$o/f4-on.png"   # F4
    "invincible row: $(Seen '"event":"test_invincible"')"
    P '-Action','hold','-Vk','69','-Ms','200'; foreach($s in 1..4){ P '-Action','shot','-Path',"$o/sit-$s.png" }; Start-Sleep 14; P '-Action','shot','-Path',"$o/rested.png"
    # The bonfire menu is open once the character has sat down (Reference/Native menu audits): Travel at (328,207), then the presses in -TravelPress (pilot units "x,y;x,y", 4 s apart, a shot after each).
    P '-Action','press','-X','328','-Y','207','-Ms','1200'; Start-Sleep 6; P '-Action','shot','-Path',"$o/travel.png"
    if($TravelPress){ $k=0; foreach($xy in $TravelPress -split ';'){ $k++; $v=$xy -split ',';P '-Action','press','-X',$v[0],'-Y',$v[1],'-Ms','1200'; Start-Sleep 4; P '-Action','shot','-Path',"$o/go-$k.png" }
        Start-Sleep $ArriveSeconds; if(Alive){ P '-Action','shot','-Path',"$o/arrive.png" }
        # -LookSweep "n,dx": n camera turns of dx mouse units with a shot after each (sweep-<i>.png), to find the arena; otherwise a walk (Z, AZERTY forward).
        if($LookSweep){ $v=$LookSweep -split ',';for($i=1;$i -le [int]$v[0] -and (Alive);$i++){ P '-Action','look','-X',$v[1],'-Y','0';Start-Sleep 1;P '-Action','shot','-Path',"$o/sweep-$i.png" } }
        else{ for($k=1;$k -le 3 -and (Alive);$k++){ P '-Action','hold','-Vk','90','-Ms','2500'; Start-Sleep 1; P '-Action','shot','-Path',"$o/walk-$k.png" } } }
    for($t=0;$t -lt $ArenaWait -and (Alive);$t+=10){ Start-Sleep 10; P '-Action','shot','-Path',"$o/arena-$t.png" }
    P '-Action','key','-Vk','115'; Start-Sleep 3; P '-Action','shot','-Path',"$o/f4-off.png"
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" } finally {
    foreach($f in $flagPath,$revivePath){ if(Test-Path -LiteralPath $f){ Remove-Item -LiteralPath $f -Force } } }
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 6
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing: run Restore-StartupTrace.ps1 on the baseline' }
Select-String -LiteralPath $trace -Pattern '"event":"test_' | ForEach-Object { $_.Line }
