# Read-only decoder of the FD4 frame timeline tables (I-0086). Usage:
#   Get-TimelineSteps -Exe <image> -Start <file offset of the first 0x20-byte step record>  (ER: 0x2C03B20, DS3 1.15.2: 0x29B8B80)
param([string]$Exe,[long]$Start,[int]$Max=200)
$b=[IO.File]::ReadAllBytes($Exe)
$pe=[BitConverter]::ToInt32($b,0x3c); $ns=[BitConverter]::ToUInt16($b,$pe+6); $opt=[BitConverter]::ToUInt16($b,$pe+20)
$base=[BitConverter]::ToUInt64($b,$pe+24+24); $sec=$pe+24+$opt
$secs=@(); for($i=0;$i -lt $ns;$i++){ $o=$sec+40*$i; $secs+=[pscustomobject]@{va=[BitConverter]::ToUInt32($b,$o+12); vs=[BitConverter]::ToUInt32($b,$o+8); raw=[BitConverter]::ToUInt32($b,$o+20); rs=[BitConverter]::ToUInt32($b,$o+16)} }
function F([uint64]$va){ $r=$va-$base; $s=$secs|?{ $r -ge $_.va -and $r -lt $_.va+$_.rs }; if(!$s){return -1}; return [long]($s.raw+($r-$s.va)) }
for($k=0;$k -lt $Max;$k++){ $o=$Start+0x20*$k; $id=[BitConverter]::ToInt32($b,$o); $np=[BitConverter]::ToUInt64($b,$o+8); $m=[BitConverter]::ToUInt64($b,$o+0x10); $c=[BitConverter]::ToUInt32($b,$o+0x18); $d=[BitConverter]::ToUInt32($b,$o+0x1C)
 $f=F $np; if($f -lt 0){ "end at $k"; break }
 $e=$f; while($b[$e] -ne 0 -or $b[$e+1] -ne 0){$e+=2}; $n=[Text.Encoding]::Unicode.GetString($b,$f,$e-$f)
 "{0,3} 0x{0:X2} {1,-46} mask={2:X3} c={3:X} d={4:X}" -f $id,$n,$m,$c,$d }
