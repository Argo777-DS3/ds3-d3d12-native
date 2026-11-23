param([string]$Run,[switch]$Reference)
# Announced run only: title -> Load Game list (portraits), screenshots, then window close (shutdown path).
$d='investigations/I-0066-independent-native-startup'; $o="evidence/I-0066/artifacts/$Run"; $trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace $(if($Reference){'none'}else{$trace}) -TimeoutSeconds 240 -MenuY 555 | Out-Null   # 1176: -Reference has no native frames
Start-Sleep 12; if(-not (Alive)){ 'dead after Load Game'; return }
P '-Action','shot','-Path',"$o/loadgame-1.jpg"; Start-Sleep 10; P '-Action','shot','-Path',"$o/loadgame-2.jpg"
if(-not (Alive)){ 'dead in Load Game'; return }
P '-Action','click','-X','1265','-Y','15'; Start-Sleep 25
"alive after close: $(Alive)"
