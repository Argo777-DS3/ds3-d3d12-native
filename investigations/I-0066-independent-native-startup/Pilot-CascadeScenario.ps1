param([Parameter(Mandatory)][string]$Trace,[Parameter(Mandatory)][string]$Event,[ValidateRange(10,300)][int]$StillSeconds=60,
    [ValidateRange(0,300)][int]$EndStillSeconds=40,[ValidateRange(60,900)][int]$WaitSeconds=400,[ValidateRange(0,120)][int]$SettleSeconds=15)
# Cascade cache (SUN-SHADOWS-WAVE1 7): the camera scenario of an announced live run, beside Run-LiveWitness.ps1. Once the trace
# holds the first $Event row (the world is drawn), the character stands still $StillSeconds, then walks forward (Z, AZERTY) and
# turns the camera twice, then stands still $EndStillSeconds. Each phase's start is written to the trace's folder (scenario.txt)
# so the rows can be split by phase. Input goes only to a foreground DarkSoulsIII window (Pilot-Game.ps1's guard).
$ErrorActionPreference='Stop'
$log=Join-Path (Split-Path -Parent $Trace) 'scenario.txt'
$deadline=(Get-Date).AddSeconds($WaitSeconds)
while((Get-Date) -lt $deadline){
    if((Test-Path -LiteralPath $Trace) -and (Select-String -LiteralPath $Trace -SimpleMatch "`"$Event`"" -Quiet)){break}
    Start-Sleep 2}
if((Get-Date) -ge $deadline){"no $Event row before the deadline";exit 1}
Start-Sleep -Seconds $SettleSeconds   # the first row can precede the end of the loading screen
function Phase([string]$name){"$((Get-Date).ToString('HH:mm:ss.fff')) $name" | Add-Content -LiteralPath $log;"phase $name"}
function P([string[]]$a){pwsh -NoProfile -File "$PSScriptRoot/Pilot-Game.ps1" @a}
Phase 'still'
Start-Sleep -Seconds $StillSeconds
Phase 'move'
P '-Action','hold','-Vk','90','-Ms','3000'
P '-Action','look','-X','600'
P '-Action','hold','-Vk','90','-Ms','3000'
P '-Action','look','-X','-900'
P '-Action','hold','-Vk','90','-Ms','2000'
Phase 'end-still'
Start-Sleep -Seconds $EndStillSeconds
Phase 'done'
