param([string]$Run,[switch]$Reference)
# Announced run only: Status open twice (renderer retirement and re-creation), screenshots, then window close.
$d='investigations/I-0066-independent-native-startup'; $o="evidence/I-0066/artifacts/$Run"; $trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
# 1174: -Reference (original renderer): no native frames to wait for; the title pilot runs without a trace and the load gets a fixed 45 s.
if($Reference){ pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace none -TimeoutSeconds 240 | Out-Null; Start-Sleep 45 }else{
pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace $trace -TimeoutSeconds 240 | Out-Null
$deadline=[DateTime]::UtcNow.AddSeconds(240)
while(@(Select-String -LiteralPath $trace -Pattern '"native_present_pacing".*"content_draws":[1-9]').Count -lt 2 -and [DateTime]::UtcNow -lt $deadline){ Start-Sleep -Seconds 2 } }
P '-Action','hold','-Vk','27','-Ms','2500'; Start-Sleep 3
P '-Action','click','-X','395','-Y','390'; Start-Sleep 8
if(-not (Alive)){ 'dead after first Status'; return }
P '-Action','shot','-Path',"$o/status-1a.jpg"; $base=(Get-Item "$o/status-1a.jpg").Length; $switched=$false
foreach($attempt in 1..4){ foreach($vk in @('82','70')){ P '-Action','hold','-Vk',$vk,'-Ms','600'; Start-Sleep 4; $p="$o/status-try$attempt-$vk.jpg"; P '-Action','shot','-Path',$p
    if([math]::Abs((Get-Item $p).Length-$base) -gt 12000){ "switched by $vk (attempt $attempt)"; $switched=$true; break } }; if($switched){break} }
Start-Sleep 6; P '-Action','shot','-Path',"$o/status-page.jpg"
P '-Action','hold','-Vk','27','-Ms','300'; Start-Sleep 4
if(-not (Alive)){ 'dead after leaving Status'; return }
P '-Action','shot','-Path',"$o/menu-back.jpg"
P '-Action','click','-X','395','-Y','390'; Start-Sleep 8
if(-not (Alive)){ 'dead after second Status'; return }
P '-Action','shot','-Path',"$o/status-2.jpg"
P '-Action','click','-X','1265','-Y','15'
'cycle complete'
