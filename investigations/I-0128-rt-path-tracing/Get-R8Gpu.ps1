# I-0128 R8a: the native_rt_gpu rows of runs (rt-gpu-measure.flag; Run-R7aGi.ps1's *gpu modes), one line a run: per zone the frames it
# was timed over the run's windows after the first (the first holds the load), its frames-weighted mean and its largest time, in
# microseconds; the render extent, the level, ReSTIR and the path vertices the rows name. -Windows prints every row. Reads every
# *.jsonl of each run folder under evidence/I-0066/artifacts (the module's journal rows).
param([Parameter(Mandatory)][string[]]$RunNames,[switch]$Windows,[int]$Skip=1)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$zones='frame','rt_scene','rt_sun','rt_cache','rt_reflect','rt_water','rt_lights','rt_gi','rr_guides','dlss','rt_probe','taa'   # I-0132 R11j: the TAA
foreach($run in @($RunNames | ForEach-Object { $_ -split ',' } | Where-Object { $_ })){
    $dir=Join-Path $repo "evidence/I-0066/artifacts/$run";if(-not (Test-Path -LiteralPath $dir)){"${run}: no folder";continue}
    $rows=foreach($f in Get-ChildItem -LiteralPath $dir -Filter *.jsonl -Recurse){foreach($l in [IO.File]::ReadLines($f.FullName)){
        if($l.Contains('"native_rt_gpu"') -and $l.Contains('"zones"')){$i=$l.IndexOf('{"event":"native_rt_gpu"');if($i -ge 0){try{$l.Substring($i)|ConvertFrom-Json}catch{}}}}}
    $rows=@($rows|Sort-Object serial -Unique)
    if(-not $rows.Count){"${run}: no native_rt_gpu row";continue}
    if($Windows){foreach($r in $rows){$cells=foreach($z in $zones){$c=$r.zones.$z;if($c -and $c[0]){'{0} {1:N0}' -f $z,$c[1]}};"$run serial $($r.serial) frames $($r.frames) $($r.width)x$($r.height): "+($cells -join ', ')}}
    $kept=@($rows|Select-Object -Skip $Skip);if(-not $kept.Count){"${run}: $($rows.Count) windows, none after the first $Skip";continue}
    $last=$kept[-1]
    $cells=foreach($z in $zones){
        $n=0;$sum=0.0;$max=0.0
        foreach($r in $kept){$c=$r.zones.$z;if(-not $c -or -not $c[0]){continue};$n+=[int64]$c[0];$sum+=[double]$c[0]*[double]$c[1];$max=[math]::Max($max,[double]$c[2])}
        if($n){'{0} {1:N0} us (max {2:N0}, {3} frames)' -f $z,($sum/$n),$max,$n}}
    $pt=($kept|ForEach-Object{[int64]$_.pt_frames}|Measure-Object -Sum).Sum;$all=($kept|ForEach-Object{[int64]$_.frames}|Measure-Object -Sum).Sum   # pt_frames: absent before the fixes' L4 (0)
    '{0}: {1} windows, {2}x{3}, level {4} ({9} of {10} frames traced at the PT level), restir {5}, bounces {6}, reference {7}, light grid {11}: {8}' -f $run,$kept.Count,$last.width,$last.height,$last.level,$last.restir,$last.bounces,$last.reference,($cells -join '; '),$pt,$all,$(if($null -eq $last.rt_light_grid){'absent'}else{$last.rt_light_grid})}   # I-0128 R8c: the row's grid (absent before R8c)
