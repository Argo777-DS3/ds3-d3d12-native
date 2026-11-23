param([Parameter(Mandatory)][string]$Series,[int]$TrimSeconds=2)
# I-0097 tranche 2: the area-crossing frame of each run of an A/B series (Run-I97Series.ps1), from the native journal's
# native_slow_frame rows inside the path's motion segments (motion-marks.txt, the first -TrimSeconds of each trimmed as the gate
# does): class A, 34-50 ms (I-0095 §3: one a cycle, at the crossing), their count, mean interval and mean collect, prepare and
# record; class B, 50 ms and over. Then per build letter the means of the run values (the difference in standard errors of the
# first build's, as Get-I97Series.ps1).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$runs=@(Get-ChildItem "$repo/evidence/I-0066/artifacts" -Directory | Where-Object { $_.Name -match "^$([regex]::Escape($Series))-\d+-([a-h])(-r)?$" } | Sort-Object Name)
$rows=foreach($r in $runs){ $null=$r.Name -match '-\d+-([a-h])(-r)?$';$letter=$Matches[1];$dir=$r.FullName
    if(-not (Test-Path -LiteralPath "$dir/motion-marks.txt")){ continue }
    if($r.Name -notmatch '-r$' -and (Test-Path -LiteralPath "$dir-r")){ continue }   # step review: a relaunched run counts once, as its -r
    $marks=@{};foreach($line in Get-Content -LiteralPath "$dir/motion-marks.txt"){ $f=$line -split ' ';$marks[$f[0]]=[uint64]$f[1] }
    $segments=@($marks.Keys | Where-Object { $_ -match '^segment_(.+)_start$' } | ForEach-Object { $n=$Matches[1];[pscustomobject]@{from=$marks[$_]+[uint64](1000*$TrimSeconds);to=$marks["segment_${n}_end"]} } | Where-Object { $_.to })
    $slow=@(Select-String -LiteralPath "$dir/startup.jsonl" -Pattern '"event":"native_slow_frame".*"interval_ms":([\d.]+),"collect_ms":([\d.]+),"prepare_ms":([\d.]+),"render_ms":[\d.]+,"record_ms":([\d.]+).*"tick_ms":(\d+)' | ForEach-Object { $g=$_.Matches[0].Groups
        [pscustomobject]@{ms=[double]$g[1].Value;collect=[double]$g[2].Value;prepare=[double]$g[3].Value;record=[double]$g[4].Value;tick=[uint64]$g[5].Value} } |
        Where-Object { $t=$_.tick;@($segments | Where-Object { $t -ge $_.from -and $t -le $_.to }).Count })
    $a=@($slow | Where-Object { $_.ms -gt 34 -and $_.ms -lt 50 });$b=@($slow | Where-Object { $_.ms -ge 50 })
    $avg={ param($v,$k) if($v.Count){ ($v | Measure-Object $k -Average).Average }else{ 0 } }
    [pscustomobject]@{run=$r.Name;build=$letter;a=$a.Count;a_ms=(& $avg $a 'ms');a_collect=(& $avg $a 'collect');a_prepare=(& $avg $a 'prepare');a_record=(& $avg $a 'record');b=$b.Count;b_worst=$(if($b.Count){($b | Measure-Object ms -Maximum).Maximum}else{0})} }
$rows | Format-Table run,build,a,@{n='a_ms';e={'{0:0.0}' -f $_.a_ms}},@{n='a_collect';e={'{0:0.00}' -f $_.a_collect}},@{n='a_prepare';e={'{0:0.00}' -f $_.a_prepare}},@{n='a_record';e={'{0:0.00}' -f $_.a_record}},b,b_worst -AutoSize | Out-String -Width 160
function Stat($v){ $n=$v.Count;$mean=($v | Measure-Object -Average).Average;$sd=if($n -gt 1){[Math]::Sqrt((($v | ForEach-Object { ($_-$mean)*($_-$mean) } | Measure-Object -Sum).Sum)/($n-1))}else{0};[pscustomobject]@{n=$n;mean=$mean;sd=$sd} }
$builds=@($rows.build | Sort-Object -Unique);$metrics='a','a_ms','a_collect','b'
$base=@{};foreach($k in $metrics){ $base[$k]=Stat @($rows | Where-Object build -eq $builds[0] | ForEach-Object { [double]$_.$k }) }
foreach($bl in $builds){ $line="build $bl ($(@($rows | Where-Object build -eq $bl).Count) runs):"
    foreach($k in $metrics){ $s=Stat @($rows | Where-Object build -eq $bl | ForEach-Object { [double]$_.$k });$z=''
        if($bl -ne $builds[0]){ $se=[Math]::Sqrt($s.sd*$s.sd/[Math]::Max(1,$s.n)+$base[$k].sd*$base[$k].sd/[Math]::Max(1,$base[$k].n));$z=if($se){' ({0:+0.0;-0.0} se)' -f (($s.mean-$base[$k].mean)/$se)}else{''} }
        $line+=" $k {0:0.00} sd {1:0.00}$z;" -f $s.mean,$s.sd }
    $line }
