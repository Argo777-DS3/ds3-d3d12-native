[CmdletBinding()]
param([ValidateRange(20,300)][int]$StaySeconds=60,[string]$ShotDir='',[ValidateRange(1,6)][int]$Loops=1)
# W5 zone witnesses (D-0174): during an announced run on the Shebang zone save (shebang-zones-final, Beta Undead Settlement,
# Gamma Irithyll, Delta Dreg Heap, Epsilon Ariandel, Zeta Ringed City; Alpha untouched), started at the title (after
# Pilot-TitleContinue.ps1 -MenuY 0): load each character by Load Game, stay $StaySeconds in the zone, quit to the title
# (Quit Game saves; the runner restores Thomas's save by hash), dismiss the two offline notices, next character. Prints one
# line per zone with its UTC time, so the trace rows can be split by zone. Inputs go through Pilot-Game.ps1.
# I-0081 (the video memory over zone travels): -Loops tours the five zones that many times; with -ShotDir a screenshot is taken
# before each navigation click (nav-<loop>-<zone>-<step>.jpg), so every click of the run can be checked against what was on screen.
$ErrorActionPreference='Stop'
$pilot=Join-Path $PSScriptRoot 'Pilot-Game.ps1'
function P([string[]]$a){ pwsh -NoProfile -File $pilot @a | Out-Null }
$zones=@(@{name='Beta m31';y=285;scroll=0},@{name='Gamma m37';y=389;scroll=0},@{name='Delta m50';y=493;scroll=0},@{name='Epsilon m45';y=597;scroll=0},@{name='Zeta m51';y=493;scroll=1})
$first=$true
for($loop=1;$loop -le $Loops;$loop++){ $zi=0
foreach($z in $zones){ $zi++;$step=0
    function Look{ if($ShotDir){ $script:step++; P '-Action','shot','-Path',(Join-Path $ShotDir "nav-$loop-$zi-$($script:step).jpg") } }
    if(-not $first){
        Look; P '-Action','key','-Vk','69'; Start-Sleep -Seconds 3                      # PRESS ANY BUTTON
        Look; P '-Action','click','-X','648','-Y','428'; Start-Sleep -Seconds 2         # network notice
        Look; P '-Action','click','-X','648','-Y','451'; Start-Sleep -Seconds 2 }       # offline notice
    $first=$false
    Look; P '-Action','click','-X','648','-Y','555'; Start-Sleep -Seconds 3             # Load Game
    if($z.scroll){ P '-Action','wheel','-X','640','-Y','400','-Notches','-1'; Start-Sleep -Seconds 1 }
    Look; P '-Action','click','-X','640','-Y',"$($z.y)"; Start-Sleep -Seconds 2            # the character
    Look; P '-Action','click','-X','501','-Y','428'                                     # OK
    "zone $($z.name) loading $([DateTime]::UtcNow.ToString('o'))"
    Start-Sleep -Seconds $StaySeconds
    if($ShotDir){ P '-Action','shot','-Path',(Join-Path $ShotDir ("zone-$loop-"+($z.name -replace ' ','-')+'.jpg')) }
    P '-Action','key','-Vk','27'; Start-Sleep -Milliseconds 1500                   # Quit Game (Pilot-QuitAndKeepSave's clicks)
    Look; P '-Action','click','-X','525','-Y','390'; Start-Sleep -Milliseconds 1500
    Look; P '-Action','click','-X','742','-Y','191'; Start-Sleep -Milliseconds 1500
    Look; P '-Action','click','-X','648','-Y','556'; Start-Sleep -Milliseconds 1500
    Look; P '-Action','click','-X','501','-Y','428'; Start-Sleep -Seconds 8
    "zone $($z.name) left $([DateTime]::UtcNow.ToString('o'))"
}}
