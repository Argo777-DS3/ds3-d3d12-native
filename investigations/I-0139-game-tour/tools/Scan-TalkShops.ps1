# I-0139 npcs-merchants: for every ESD of the recovered talkesdbnd files, the integer constants (ESD expression opcode 0x82 + int32) that fall in ShopLineupParam id ranges, and in the event-flag/menu ranges used by service calls. Heuristic scan, read-only.
param([Parameter(Mandatory)][string]$TalkDir,[Parameter(Mandatory)][string]$Out)
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$l=[Collections.Generic.List[string]]::new()
foreach($f in Get-ChildItem $TalkDir -Filter *.talkesdbnd.dcx){
  $b=[SoulsFormats.BND4]::Read([SoulsFormats.DCX]::Decompress([IO.File]::ReadAllBytes($f.FullName)))
  foreach($e in $b.Files){
    $d=[byte[]]$e.Bytes; $vals=@{}
    for($i=0;$i -lt $d.Length-5;$i++){ if($d[$i] -eq 0x82){ $v=[BitConverter]::ToInt32($d,$i+1); $vals[$v]=1+$vals[$v] } }
    $shop=@($vals.Keys|?{ ($_ -ge 1000 -and $_ -le 2100) -or ($_ -ge 30000 -and $_ -le 30300) -or ($_ -ge 40000 -and $_ -le 40010) -or ($_ -ge 50000 -and $_ -le 50200) -or ($_ -ge 100000 -and $_ -le 200400) -or ($_ -ge 760000 -and $_ -le 760010) -or ($_ -ge 900000 -and $_ -le 900100)}|sort)
    $l.Add($f.Name+"`t"+[IO.Path]::GetFileName($e.Name)+"`t"+$d.Length+"`t"+($shop -join ' '))
  }
}
$l|Set-Content $Out -Encoding utf8; $l.Count
