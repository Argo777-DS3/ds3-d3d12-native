param([Parameter(Mandatory)][string]$Run,[string[]]$Needles=@('509BD0','D5209D','9FFF6','A02B9','E6E5DB'))
# I-0097 tranche 4: in one pass over a run's sample-profile.txt, the samples of every thread whose stack holds each game return address
# (raw: the report's RVA + 1; -Run is the run's name under evidence/I-0066/artifacts). Defaults: DS3's joined loading step 509BCF, its texture path D5209C, its model CPU load A0170
# (9FFF5) and storage admission (A02B8), the FXR registration E6E5DA. Works for native and original (Reference) runs alike.
$want=@{};foreach($n in $Needles){$want[[Convert]::ToInt64($n,16)]=$n}
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$count=@{};foreach($n in $Needles){$count[$n]=0}
$r=[System.IO.StreamReader]::new("$repo/evidence/I-0066/artifacts/$Run/sample-profile.txt");$base=$null;$size=0;$total=0
while(($line=$r.ReadLine()) -ne $null){
  if($line.StartsWith('module')){if($base -eq $null){$p=$line.Split(' ');$base=[Convert]::ToInt64($p[1],16);$size=[Convert]::ToInt64($p[2],16)};continue}
  if(-not $line.StartsWith('s ')){continue}
  $total++;$p=$line.Split(' ');$seen=@{}
  for($i=3;$i -lt $p.Length;$i++){$a=[Convert]::ToInt64($p[$i],16)-$base;if($a -ge 0 -and $a -lt $size -and $want.ContainsKey($a) -and -not $seen[$a]){$seen[$a]=1;$count[$want[$a]]++}}}
"{0}: {1}" -f $Run,(($Needles|%{"$_=$($count[$_])"}) -join ' ')
