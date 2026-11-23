# I-0139 zones-bonfires: finds, in every MSB3 of a folder, the object parts whose EntityID is a BonfireWarpParam bonfireEntityId. Read-only.
# Usage: pwsh -NoProfile -File Find-BonfireObjects.ps1 -MsbDir <dir of *.msb.dcx>  -Out <tsv>
param([Parameter(Mandatory)][string]$MsbDir,[Parameter(Mandatory)][string]$IdsFile,[Parameter(Mandatory)][string]$Out)
$ErrorActionPreference='Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$set=@{};foreach($i in ((Get-Content $IdsFile) -split ",")|Where-Object{$_}){$set[[int]$i]=1}
$lines=@("map`tentity`tpart`tmodel`tx`ty`tz")
foreach($f in Get-ChildItem $MsbDir -Filter '*mapstudio_m*.msb.dcx'){
    $m=[SoulsFormats.MSB3]::Read($f.FullName)
    $mapName=[regex]::Match($f.Name,'m\d\d_\d\d_\d\d_\d\d').Value
    foreach($p in $m.Parts.Objects){ if($set.ContainsKey([int]$p.EventEntityID)){ $lines+="{0}`t{1}`t{2}`t{3}`t{4:F3}`t{5:F3}`t{6:F3}" -f $mapName,$p.EventEntityID,$p.Name,$p.ModelName,$p.Position.X,$p.Position.Y,$p.Position.Z } } }
[IO.File]::WriteAllLines($Out,$lines,[Text.UTF8Encoding]::new($false)); "$($lines.Count-1) found"
