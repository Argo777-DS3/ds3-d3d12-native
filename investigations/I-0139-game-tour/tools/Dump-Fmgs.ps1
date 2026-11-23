# I-0139 npcs-merchants: lists the FMGs of a msgbnd and dumps chosen ones (id<TAB>text). Read-only on extracted bytes.
param([Parameter(Mandatory)][string]$Bnd,[string]$OutDir='',[string]$Match='')
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$b=[SoulsFormats.BND4]::Read([SoulsFormats.DCX]::Decompress([IO.File]::ReadAllBytes($Bnd)))
foreach($f in $b.Files){
  $fmg=[SoulsFormats.FMG]::Read($f.Bytes); $n=[IO.Path]::GetFileName($f.Name)
  "{0}`t{1} entries" -f $n,$fmg.Entries.Count
  if($OutDir -and $n -match $Match){ New-Item -ItemType Directory -Force $OutDir|Out-Null
    ($fmg.Entries|%{ "{0}`t{1}" -f $_.ID,(($_.Text -replace "`r?`n",' / ')) }) | Set-Content (Join-Path $OutDir ($n+'.tsv')) -Encoding utf8 }
}
