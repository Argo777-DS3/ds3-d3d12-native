# I-0139 npcs-merchants: every MSB3 Enemy and DummyEnemy part of the recovered MSBs (map, name, model, entity id, NPC param id, talk id, position). Read-only.
param([Parameter(Mandatory)][string]$MsbDir,[Parameter(Mandatory)][string]$Out)
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$l=[Collections.Generic.List[string]]::new(); $l.Add("file`tkind`tname`tmodel`tentity`tnpcparam`ttalk`tcharainit`tx`ty`tz`trotY`tmapstudio")
foreach($f in Get-ChildItem $MsbDir -Filter *.msb.dcx){
  $m=[SoulsFormats.MSB3]::Read([IO.File]::ReadAllBytes($f.FullName))
  foreach($e in $m.Parts.Enemies){
    $l.Add(([string]::Format([Globalization.CultureInfo]::InvariantCulture,"{0}`tEnemy`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7:F2}`t{8:F2}`t{9:F2}`t{10:F1}",$f.Name,$e.Name,$e.ModelName,$e.EventEntityID,$e.NPCParamID,$e.TalkID,$e.CharaInitID,$e.Position.X,$e.Position.Y,$e.Position.Z,$e.Rotation.Y)))
  }
}
$l | Set-Content $Out -Encoding utf8
$l.Count
