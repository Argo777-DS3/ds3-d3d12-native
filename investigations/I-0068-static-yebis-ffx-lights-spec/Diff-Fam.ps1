param([string]$Tsv,[string]$Base)
$rows=Import-Csv $Tsv -Delimiter "`t"
function Toks($s){ ($s -split ' ') | Where-Object {$_} }
function Cb($s){ # name[size]{vars}
  $h=@{}; foreach($m in [regex]::Matches($s,'(\w+)\[(\d+)\]\{([^}]*)\}')){ $h[$m.Groups[1].Value]=@{size=$m.Groups[2].Value;vars=$m.Groups[3].Value} }; $h }
$prog=@('Gbuf.ppo','GBuf_Skin.vpo','Gbuf_Skin.vpo','Fwd.ppo','Fwd_Skin.vpo')
$fams=$rows.family | Select-Object -Unique
foreach($p in $prog){
  $b=$rows | Where-Object {$_.family -eq $Base -and $_.program -eq $p}
  foreach($f in $fams){
    if($f -eq $Base){continue}
    $r=$rows | Where-Object {$_.family -eq $f -and $_.program -eq $p}
    if(-not $r){continue}
    $out=@()
    $bt=if($b){Toks $b.resources}else{@()}; $rt=Toks $r.resources
    $add=$rt | Where-Object {$_ -notin $bt}; $del=$bt | Where-Object {$_ -notin $rt}
    if($add){$out+="  +res: "+($add -join ' ')}; if($del){$out+="  -res: "+($del -join ' ')}
    $bc=if($b){Cb $b.used_constants}else{@{}}; $rc=Cb $r.used_constants
    foreach($k in $rc.Keys){ if(-not $bc.ContainsKey($k)){ $out+="  +cb $k[$($rc[$k].size)]{$($rc[$k].vars)}" } elseif($bc[$k].vars -ne $rc[$k].vars -or $bc[$k].size -ne $rc[$k].size){ $out+="  ~cb $k[$($bc[$k].size)->$($rc[$k].size)] {$($bc[$k].vars)} -> {$($rc[$k].vars)}" } }
    foreach($k in $bc.Keys){ if(-not $rc.ContainsKey($k)){ $out+="  -cb $k" } }
    "[$p] $f ($($r.bytes) B vs $(if($b){$b.bytes}) B)"; $out
  }
}
