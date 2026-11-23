param([string]$GameData,[string]$Out)
# I-0139 bosses sheet: dumps EMEVDs (I-0067 Get-EmevdDump) and MSB3 parts/regions (name, model, EventEntityID, position, NPCParamID) for the boss maps.
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$dump='C:\Users\User\Desktop\DS3 Decompilation\.claude\worktrees\kind-antonelli-18b0e1\investigations\I-0067-static-menus-subsystems-map\Get-EmevdDump.ps1'
foreach($f in Get-ChildItem $GameData -Filter 'event_*.emevd.dcx'){ & pwsh -NoProfile -File $dump -Path $f.FullName -Out (Join-Path $Out (($f.BaseName -replace '\.emevd$','')+'.emevd.txt')) }
function P($p){ '{0:F1} {1:F1} {2:F1}' -f $p.Position.X,$p.Position.Y,$p.Position.Z }
foreach($f in Get-ChildItem $GameData -Filter '*mapstudio_m*.msb.dcx'){
  $m=[SoulsFormats.MSB3]::Read($f.FullName); $L=@()
  foreach($p in $m.Parts.Enemies){ $L+="ENEMY`t$($p.Name)`t$($p.ModelName)`t$($p.EventEntityID)`t$(P $p)`tnpc=$($p.NPCParamID)`ttalk=$($p.TalkID)" }
  foreach($p in $m.Parts.Objects){ $L+="OBJ`t$($p.Name)`t$($p.ModelName)`t$($p.EventEntityID)`t$(P $p)" }
  foreach($p in $m.Parts.Players){ $L+="PLAYER`t$($p.Name)`t$($p.ModelName)`t$($p.EventEntityID)`t$(P $p)" }
  foreach($r in $m.Regions.GetEntries()){ $L+="REGION`t$($r.Name)`t$($r.GetType().Name)`t$($r.EventEntityID)`t$(P $r)" }
  $L | Set-Content (Join-Path $Out (($f.BaseName -replace '\.msb$','')+'.msb.txt')) -Encoding utf8
}
