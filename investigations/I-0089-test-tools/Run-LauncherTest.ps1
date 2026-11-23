[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildFolder,[string]$Bosses='iudex')
# I-0089, one announced live run of the TEST LAUNCHER itself (tools/Launch-DS3-DX12-Test.ps1 -Shebang -Bosses ...), as one process:
# the launcher runs as a job, this script plays the title (the two offline notices, Continue), presses F4 and F5 in the world,
# closes the window, then checks that the saves are byte-identical to what they were before (SHA-256 of every file), that the
# pending-restore marker is gone and what the module journaled (test_*). No screenshot is taken (they show the desktop).
$ErrorActionPreference='Stop'
$inv=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup';$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ throw 'game already running' }
$saves=Join-Path $env:APPDATA 'DarkSoulsIII';$marker=Join-Path $env:USERPROFILE '.ds3-test-backups\PENDING.json'
$before=@{};foreach($f in Get-ChildItem -LiteralPath $saves -Recurse -File -Force){ $before[$f.FullName]=(Get-FileHash -LiteralPath $f.FullName).Hash }
$play=Join-Path $env:LOCALAPPDATA 'DS3-DX12-Test';$started=Get-Date
$job=Start-Job -ScriptBlock { param($repo,$build,$bosses) pwsh -NoProfile -File "$repo/tools/Launch-DS3-DX12-Test.ps1" -Build $build -Shebang -Bosses $bosses 2>&1 } -ArgumentList $repo,$BuildFolder,$Bosses
function P([string[]]$a){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
function Journal{ Get-ChildItem -LiteralPath $play -Filter 'test-*.jsonl' -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $started.AddSeconds(-5) } | Sort-Object LastWriteTime -Descending | Select-Object -First 1 }
function Seen([string]$pattern){ $j=Journal; $j -and [bool](Select-String -LiteralPath $j.FullName -Pattern $pattern -Quiet) }
"marker during setup (expected after the launcher began): wait"
try {
    $deadline=(Get-Date).AddSeconds(150); while(-not (Journal) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    $deadline=(Get-Date).AddSeconds(240); while(-not (Seen '"name":"05_000_Title"') -and (Get-Date) -lt $deadline){ Start-Sleep 1 }
    "marker present while playing: $(Test-Path -LiteralPath $marker)"
    function Closed{ $j=Journal; if(-not $j){return 0}; @(Select-String -LiteralPath $j.FullName -Pattern '"event":"menu_window_close"').Count }
    Start-Sleep 8; $base=Closed; P '-Action','click','-X','600','-Y','15'; Start-Sleep 1
    for($k=0;$k -lt 6 -and (Closed) -lt $base+1;$k++){ P '-Action','press','-X','648','-Y','427','-Ms','1200'; Start-Sleep 4 }
    for($k=0;$k -lt 6 -and (Closed) -lt $base+2;$k++){ P '-Action','press','-X','648','-Y','450','-Ms','1200'; Start-Sleep 4 }
    $loaded='"title_step_state".*"state":4,'
    for($k=1;$k -le 4 -and -not (Seen $loaded);$k++){ P '-Action','press','-X','648','-Y','525','-Ms','1200'
        $until=(Get-Date).AddSeconds(25); while(-not (Seen $loaded) -and (Get-Date) -lt $until){ Start-Sleep 1 } }
    "loaded=$(Seen $loaded)"
    Start-Sleep 40
    P '-Action','key','-Vk','115'; Start-Sleep 3   # F4
    P '-Action','key','-Vk','116'; Start-Sleep 5   # F5
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout 300 | Out-Null
while($job.State -eq 'Running'){ 'launcher still running'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 12
"marker after: $(Test-Path -LiteralPath $marker)"
$changed=@();foreach($f in $before.Keys){ if(-not (Test-Path -LiteralPath $f) -or (Get-FileHash -LiteralPath $f).Hash -ne $before[$f]){ $changed+=$f } }
$extra=@(Get-ChildItem -LiteralPath $saves -Recurse -File -Force | Where-Object { -not $before.ContainsKey($_.FullName) })
"saves identical to before: $($changed.Count -eq 0 -and $extra.Count -eq 0) (changed $($changed.Count), new $($extra.Count))"
$j=Journal; if($j){ Select-String -LiteralPath $j.FullName -Pattern '"event":"test_' | ForEach-Object { $_.Line } }
