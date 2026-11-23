# I-0139 zones-bonfires: dumps every FMG entry (id, text) of a BND4 msgbnd, read-only. Usage: pwsh -NoProfile -File Dump-Fmg.ps1 -Msgbnd <dcx> -Out <tsv>
param([Parameter(Mandatory)][string]$Msgbnd,[Parameter(Mandatory)][string]$Out)
$ErrorActionPreference='Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$b=[SoulsFormats.BND4]::Read($Msgbnd)
$lines=foreach($f in $b.Files){ try{ $fmg=[SoulsFormats.FMG]::Read($f.Bytes) }catch{ continue }
    foreach($e in $fmg.Entries){ if($e.Text){ "{0}`t{1}`t{2}" -f ($f.Name -replace '^.*[\\/]',''),$e.ID,($e.Text -replace "[\r\n]+",' / ') } } }
[IO.File]::WriteAllLines($Out,$lines,[Text.UTF8Encoding]::new($false))
"$($lines.Count) entries"
