# I-0139 npcs-merchants: bonfire objects of the recovered MSBs (object model o000100... and the entity ids with 1950 patterns), position in the MSB frame. Read-only.
param([Parameter(Mandatory)][string]$MsbDir,[Parameter(Mandatory)][string]$Out,[string]$Model='^o0001',[string]$EntityRegex='')
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$ci=[Globalization.CultureInfo]::InvariantCulture
$l=[Collections.Generic.List[string]]::new(); $l.Add("file`tname`tmodel`tentity`tx`ty`tz")
foreach($f in Get-ChildItem $MsbDir -Filter *.msb.dcx){
  $m=[SoulsFormats.MSB3]::Read([IO.File]::ReadAllBytes($f.FullName))
  foreach($o in $m.Parts.Objects){ if($o.ModelName -match $Model -or ($EntityRegex -and "$($o.EventEntityID)" -match $EntityRegex)){
    $l.Add([string]::Format($ci,"{0}`t{1}`t{2}`t{3}`t{4:F2}`t{5:F2}`t{6:F2}",$f.Name,$o.Name,$o.ModelName,$o.EventEntityID,$o.Position.X,$o.Position.Y,$o.Position.Z)) } }
}
$l|Set-Content $Out -Encoding utf8; $l.Count
