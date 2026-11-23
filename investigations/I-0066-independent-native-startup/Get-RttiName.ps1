param([string]$Exe,[string[]]$Rva)
# Read-only: the MSVC RTTI class name of a vtable at an image RVA (vtable[-1] -> COL -> type descriptor name).
$b=[IO.File]::ReadAllBytes($Exe); $pe=[BitConverter]::ToInt32($b,0x3C); $n=[BitConverter]::ToUInt16($b,$pe+6); $opt=[BitConverter]::ToUInt16($b,$pe+20)
$base=[BitConverter]::ToUInt64($b,$pe+24+24); $sec=$pe+24+$opt; $secs=@()
for($i=0;$i -lt $n;$i++){$o=$sec+40*$i; $secs+=[pscustomobject]@{va=[BitConverter]::ToUInt32($b,$o+12);vs=[BitConverter]::ToUInt32($b,$o+8);raw=[BitConverter]::ToUInt32($b,$o+20);rs=[BitConverter]::ToUInt32($b,$o+16)}}
function Off([uint64]$rva){foreach($s in $secs){if($rva -ge $s.va -and $rva -lt $s.va+[Math]::Max($s.vs,$s.rs)){return [int64]($rva-$s.va+$s.raw)}};return -1}
foreach($r in $Rva){$v=[Convert]::ToUInt64($r,16); $o=Off ($v-8); if($o -lt 0){"$r ?";continue}
  $col=[BitConverter]::ToUInt64($b,$o)-$base; $oc=Off $col; if($oc -lt 0){"$r col?";continue}
  $td=[BitConverter]::ToUInt32($b,$oc+12); $ot=Off $td; if($ot -lt 0){"$r td?";continue}
  $s=$ot+16; $e=$s; while($b[$e] -ne 0 -and $e-$s -lt 200){$e++}; "$r " + [Text.Encoding]::ASCII.GetString($b,$s,$e-$s)}
