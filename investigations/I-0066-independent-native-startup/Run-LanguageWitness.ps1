[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[ValidateSet('Native','Reference')][string]$BootstrapMode='Native',
    [Parameter(Mandatory)][string]$TestLanguage,[string]$EffectFlags='',[ValidateRange(120,900)][int]$Seconds=360,[ValidateRange(200,3000)][int]$PressMs=1200,
    [int]$OfflineOkY=439,[switch]$ReadFadeGate)
# Menu tests FL5-FL7 (FONTS-LANGUAGES-NATIVE 6, Thomas's agreement 2026-10-13): one announced run, as one process (the trace job dies
# with the shell that started it): Run-StartupTrace.ps1 with -TestLanguage on the Eta save (m40, Firelink), the two offline notices
# (OK at 427, then -OfflineOkY: 439 in French, 450 in English), the main menu, Continue, then the in-game menu pages: Esc (the menu),
# Equipment, Esc, Esc; a screenshot of each (title.png, notice-*.png, menu.png, ingame.png, esc.png, equipment.png), then the window
# closed and the job's restoration awaited (it restores every protected file by hash).
$ErrorActionPreference='Stop'
$d=$PSScriptRoot;$repo=Split-Path -Parent (Split-Path -Parent $d)
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$hash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$extra=@('-TestLanguage',$TestLanguage);if($EffectFlags){$extra+='-EffectFlags',$EffectFlags}
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$mode,$save,$hash,$extra)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode $mode -TestSave $save -TestSaveSha256 $hash @extra 2>&1 } -ArgumentList $d,$RunName,$Seconds,$BootstrapMode,$save,$hash,$extra
$o="$repo/evidence/I-0066/artifacts/$RunName";$trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
function Seen([string]$pattern){ (Test-Path -LiteralPath $trace) -and [bool](Select-String -LiteralPath $trace -Pattern $pattern -Quiet) }
try {
    $deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    $deadline=(Get-Date).AddSeconds(240); while(-not (Seen '"name":"05_000_Title"') -and (Get-Date) -lt $deadline){ Start-Sleep 1 }
    # Each notice closes with a menu_window_close row: press its OK until it closed; the second notice's OK height depends on the
    # language (French 439, English 450, others not measured), so the presses cycle -OfflineOkY, 450, 445, 433.
    function Closed{ if(-not (Test-Path -LiteralPath $trace)){return 0}; @(Select-String -LiteralPath $trace -Pattern '"event":"menu_window_close"').Count }
    Start-Sleep 8; $base=Closed; P '-Action','click','-X','600','-Y','15'; Start-Sleep 1; P '-Action','shot','-Path',"$o/notice-1.png"
    for($k=0;$k -lt 6 -and (Closed) -lt $base+1;$k++){ P '-Action','press','-X','648','-Y','427','-Ms',"$PressMs"; Start-Sleep 4 }; P '-Action','shot','-Path',"$o/notice-2.png"
    $heights=@($OfflineOkY,450,445,433)
    for($k=0;$k -lt 8 -and (Closed) -lt $base+2;$k++){ P '-Action','press','-X','648','-Y',"$($heights[$k % 4])",'-Ms',"$PressMs"; Start-Sleep 4 }; Start-Sleep 2; P '-Action','shot','-Path',"$o/menu.png"
    $loaded='"title_step_state".*"state":4,'
    for($k=1;$k -le 3 -and -not (Seen $loaded);$k++){ P '-Action','press','-X','648','-Y','524','-Ms',"$PressMs"; $until=(Get-Date).AddSeconds(20); while(-not (Seen $loaded) -and (Get-Date) -lt $until){ Start-Sleep 1 } }
    "loaded=$(Seen $loaded)"
    Start-Sleep 45; P '-Action','shot','-Path',"$o/ingame.png"
    # -ReadFadeGate (FADE-PLATES-NATIVE F4): the cached NewMenuSystem byte [1447632B0] and its init bit [1447632B4], read-only.
    if($ReadFadeGate){ try { & "$d/Read-LoadedCode.ps1" -TargetProcessId (Get-Process DarkSoulsIII).Id -Rva 0x47632B0 -Bytes 16 -Output "$o/fade-gate.json" } catch { "fade gate read: $_" } }
    P '-Action','key','-Vk','27'; Start-Sleep 4; P '-Action','shot','-Path',"$o/esc.png"
    P '-Action','click','-X','263','-Y','390'; Start-Sleep 5; P '-Action','shot','-Path',"$o/equipment.png"
    P '-Action','key','-Vk','27'; Start-Sleep 3; P '-Action','key','-Vk','27'; Start-Sleep 3
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 6
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing: run Restore-StartupTrace.ps1 on the baseline' }
