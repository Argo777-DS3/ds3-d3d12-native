# I-0139: every MSB3 region and part with a non-zero event entity id (name, id, position), to resolve EMEVD entity ids to the teleport frame (MSB frame).
param([string]$MsbDir='C:\Users\User\.codex\I0066-msb',[string]$Out)
[System.Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$lines=@('map`tkind`tname`tentity`tx`ty`tz`tmodel')
foreach($f in Get-ChildItem $MsbDir -Filter *.msb.dcx){
  $map=($f.Name -replace '.*MapStudio_','' -replace '\.msb\.dcx','')
  try{ $m=[SoulsFormats.MSB3]::Read($f.FullName) } catch { $lines+="$map`tERROR`t$($_.Exception.Message)"; continue }
  foreach($r in $m.Regions.GetEntries()){ $id=$r.EventEntityID; if($id -ne 0 -or $r.Name -like 'c0000_*' -or $r.Name -like 'SpawnPoint*'){ $lines+=("{0}`tregion`t{1}`t{2}`t{3:F1}`t{4:F1}`t{5:F1}`t" -f $map,$r.Name,$id,$r.Position.X,$r.Position.Y,$r.Position.Z) } }
  foreach($p in $m.Parts.GetEntries()){ $id=$p.EventEntityID; if($id -ne 0 -or $p.Name -like 'c0000_*'){ $lines+=("{0}`tpart`t{1}`t{2}`t{3:F1}`t{4:F1}`t{5:F1}`t{6}" -f $map,$p.Name,$id,$p.Position.X,$p.Position.Y,$p.Position.Z,$p.ModelName) } }
}
$lines | Set-Content -LiteralPath $Out -Encoding utf8
