# I-0139 npcs-merchants: all distinct int32 constants (0x82 opcode) >= Min of the chosen ESDs of a talkesdbnd. Read-only.
param([string]$Bnd,[string[]]$Esd,[int]$Min=100000,[int]$Max=2000000)
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$b=[SoulsFormats.BND4]::Read([SoulsFormats.DCX]::Decompress([IO.File]::ReadAllBytes($Bnd)))
foreach($e in $b.Files){ $n=[IO.Path]::GetFileName($e.Name); if($Esd -notcontains $n){continue}
  $d=[byte[]]$e.Bytes; $v=@{}
  for($i=0;$i -lt $d.Length-5;$i++){ if($d[$i] -eq 0x82){ $x=[BitConverter]::ToInt32($d,$i+1); if($x -ge $Min -and $x -le $Max){$v[$x]=1} } }
  $n+': '+(($v.Keys|sort) -join ' ') }
