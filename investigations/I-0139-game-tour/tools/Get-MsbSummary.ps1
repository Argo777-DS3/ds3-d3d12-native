# I-0139 zones-bonfires: per MSB3 of a folder, counts of parts by kind and the player-start parts (c0000_*) with positions. Read-only.
param([Parameter(Mandatory)][string]$MsbDir,[Parameter(Mandatory)][string]$Out)
$ErrorActionPreference='Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$inv=[Globalization.CultureInfo]::InvariantCulture
$lines=@()
foreach($f in Get-ChildItem $MsbDir -Filter '*mapstudio_m*.msb.dcx'){
    $m=[SoulsFormats.MSB3]::Read($f.FullName); $map=[regex]::Match($f.Name,'m\d\d_\d\d_\d\d_\d\d').Value
    $lines+="$map`tcounts`tmapPieces=$($m.Parts.MapPieces.Count) objects=$($m.Parts.Objects.Count) enemies=$($m.Parts.Enemies.Count) players=$($m.Parts.Players.Count) collisions=$($m.Parts.Collisions.Count) regions=$($m.Regions.GetEntries().Count)"
    foreach($p in $m.Parts.Players){ $lines+="$map`tplayer`t$($p.Name)`t$($p.Position.X.ToString('F3',$inv))`t$($p.Position.Y.ToString('F3',$inv))`t$($p.Position.Z.ToString('F3',$inv))`t$($p.EventEntityID)" } }
[IO.File]::WriteAllLines($Out,$lines,[Text.UTF8Encoding]::new($false)); "$($lines.Count) lines"
