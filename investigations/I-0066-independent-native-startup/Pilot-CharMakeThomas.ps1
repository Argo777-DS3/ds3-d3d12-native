param([string]$Run)
# Announced run only, Thomas's save (restored by hash by the runner): press-any-button, the quit notice OK,
# New Game (no confirmation of the creation), screenshots, window close.
$d='investigations/I-0066-independent-native-startup'; $o="evidence/I-0066/artifacts/$Run"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
Start-Sleep 20
P '-Action','click','-X','648','-Y','555'; Start-Sleep 8; P '-Action','shot','-Path',"$o/c1.jpg"
P '-Action','click','-X','648','-Y','472'; Start-Sleep 8; P '-Action','shot','-Path',"$o/c2.jpg"
P '-Action','click','-X','648','-Y','586'; Start-Sleep 15; if(-not (Alive)){ 'dead after New Game'; return }
P '-Action','shot','-Path',"$o/c3.jpg"; Start-Sleep 15; P '-Action','shot','-Path',"$o/c4.jpg"; Start-Sleep 15; P '-Action','shot','-Path',"$o/c5.jpg"
if(-not (Alive)){ 'dead in New Game'; return }
P '-Action','click','-X','1265','-Y','15'; Start-Sleep 25
"alive after close: $(Alive)"
