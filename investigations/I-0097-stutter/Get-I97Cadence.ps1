param([Parameter(Mandatory)][string[]]$Run,[string]$Segment='sweep',[string]$Csv='')
# I-0097 tranche 5 step 3: one frame timeline per run from the cadence-trace.flag rows (both renderers: the screen's vertical
# blanks, DS3's game step at the world update gate and its dt; natively the engine's frame end and post, the worker's latency wait, pacer and Present; in the original its pacer D5CB0), all on the performance counter, with
# PresentMon's presents (QPCTime and msUntilDisplayed) on the same counter. Over one segment of the path (motion-marks.txt;
# -Segment all: the whole motion): the screen's period, the game steps, the gap DS3 spends after the post (its limiter and frame
# start), the presents' phase against the vertical blank (where in the refresh each present starts) and the holds the screen
# showed. -Csv writes the segment's presents (time, phase, the game step before them).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv=[Globalization.CultureInfo]::InvariantCulture
$Run=@($Run | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
function Q([double[]]$v,[double]$p){ if(-not $v -or -not $v.Count){ return [double]::NaN };$s=[double[]]$v.Clone();[Array]::Sort($s);$s[[math]::Min($s.Count-1,[int][math]::Floor($p*($s.Count-1)+0.5))] }
function F($x){ if([double]::IsNaN([double]$x)){ '-' }else{ ([double]$x).ToString('F2',$inv) } }
function Count([double[]]$v,[scriptblock]$test){ $n=0;foreach($x in $v){ if(& $test $x){ $n++ } };$n }
# The last tick at or before t (-1 if none), and the first tick after t (-1 if none), in a sorted array.
function AtOrBefore([double[]]$a,[double]$t){ $i=[Array]::BinarySearch($a,$t);if($i -lt 0){ $i=(-bnot $i)-1 };$i }
function After([double[]]$a,[double]$t){ $i=[Array]::BinarySearch($a,$t);$i=if($i -ge 0){ $i+1 }else{ -bnot $i };if($i -lt $a.Count){ $i }else{ -1 } }
foreach($n in $Run){
    $d=if(Test-Path -LiteralPath $n){ $n }else{ "$repo/evidence/I-0066/artifacts/$n" };$marks=@{}
    foreach($l in Get-Content -LiteralPath "$d/motion-marks.txt"){ $f=$l -split ' ';$marks[$f[0]]=[int64]$f[1] }
    $from=if($Segment -eq 'all'){ $marks['motion_start'] }else{ $marks["segment_$($Segment)_start"] };$to=if($Segment -eq 'all'){ $marks['motion_end'] }else{ $marks["segment_$($Segment)_end"] }
    if(-not $from -or -not $to){ throw "$n has no segment $Segment" }
    $lists=@{};$freq=0.0;$pair=$null;$dropped=0
    foreach($line in [IO.File]::ReadLines("$d/startup.jsonl")){ if(-not $line.Contains('"cadence_trace"')){ continue }
        $r=$line | ConvertFrom-Json;$freq=[double]$r.frequency;$dropped+=[int64]$r.dropped
        if(-not $lists.ContainsKey($r.kind)){ $lists[$r.kind]=[Collections.Generic.List[double]]::new() };$list=$lists[$r.kind]
        if(-not $pair -or [math]::Abs([double]$r.tick_ms-($from+$to)/2) -lt [math]::Abs($pair[0]-($from+$to)/2)){ $pair=@([double]$r.tick_ms,[double]$r.qpc) }
        if($r.kind -eq 'game_dt_us'){ $list.Add([double]$r.first);foreach($v in $r.d){ $list.Add([double]$v) };continue }
        $t=[double]$r.first;$list.Add($t);foreach($v in $r.d){ $t+=[double]$v;$list.Add($t) } }
    if(-not $freq){ "$n : no cadence_trace rows";continue }
    $a=@{};foreach($k in $lists.Keys){ $a[$k]=$lists[$k].ToArray() }
    # The segment on the counter's line (the tick count's 15.6 ms steps stay under a frame's half at the edges).
    $q0=$pair[1]+($from-$pair[0])*$freq/1000;$q1=$pair[1]+($to-$pair[0])*$freq/1000;$ms=1000.0/$freq
    $in={ param($k) $v=$a[$k];if(-not $v){ return ,([double[]]@()) };$i0=After $v ($q0-1);if($i0 -lt 0){ return ,([double[]]@()) };$i1=AtOrBefore $v $q1;if($i1 -lt $i0){ return ,([double[]]@()) }
        $o=[double[]]::new($i1-$i0+1);[Array]::Copy($v,$i0,$o,0,$o.Count);,$o }
    $vb=& $in 'vblank';$gs=& $in 'game_step';$vbAll=$a['vblank'];$native=$a.ContainsKey('present_start')
    $period=if($vb.Count -gt 2){ ($vb[-1]-$vb[0])/($vb.Count-1)*$ms }else{ [double]::NaN }
    $phase={ param([double]$t) if(-not $vbAll){ return [double]::NaN };$i=AtOrBefore $vbAll $t;if($i -lt 0){ [double]::NaN }else{ ($t-$vbAll[$i])*$ms } }
    $steps=[double[]]@(for($i=1;$i -lt $gs.Count;$i++){ ($gs[$i]-$gs[$i-1])*$ms })
    "== $([IO.Path]::GetFileName($d.TrimEnd('/','\'))) ($Segment, $([math]::Round(($to-$from)/1000.0,1)) s): screen period $(F $period) ms over $($vb.Count) vblanks; dropped marks $dropped"
    "game steps $($steps.Count): p50 $(F (Q $steps 0.5)), p01 $(F (Q $steps 0.01)), p99 $(F (Q $steps 0.99)); over 17 ms $(Count $steps { $args[0] -gt 17 }), over 18 $(Count $steps { $args[0] -gt 18 }), under 16 $(Count $steps { $args[0] -lt 16 }), over 25 $(Count $steps { $args[0] -gt 25 })"
    if($a['game_dt_us'] -and $a['game_step'] -and $a['game_dt_us'].Count -eq $a['game_step'].Count){ $g=$a['game_step'];$dt=$a['game_dt_us']
        $dts=[double[]]@(for($i=0;$i -lt $g.Count;$i++){ if($g[$i] -ge $q0 -and $g[$i] -lt $q1){ $dt[$i]/1000.0 } })
        "game dt: p50 $(F (Q $dts 0.5)) ms, p01 $(F (Q $dts 0.01)), p99 $(F (Q $dts 0.99)); distinct values (0.01 ms) $(@($dts | ForEach-Object { [math]::Round($_,2) } | Sort-Object -Unique).Count)" }
    $rows=[Collections.Generic.List[object]]::new()
    # Final review M4: both renderers' presents at the same point, Present returned (native present_end, the original's pacer
    # leaving after its Present), so their phases in the refresh compare.
    $presents=if($native){ & $in 'present_end' }else{ & $in 'original_pacer_leave' }
    if($native){
        $post=$a['engine_post_done'];$enter=$a['engine_end_enter'];$ps=$a['present_start'];$pe=$a['present_end'];$pace=$a['pace_start']
        $gaps=[Collections.Generic.List[double]]::new();$posts=[Collections.Generic.List[double]]::new()
        for($i=1;$i -lt $gs.Count;$i++){ $j=After $post $gs[$i-1];if($j -lt 0 -or $post[$j] -ge $gs[$i]){ continue };$e=After $enter $gs[$i-1]
            $gaps.Add(($gs[$i]-$post[$j])*$ms);if($e -ge 0 -and $enter[$e] -le $post[$j]){ $posts.Add(($post[$j]-$enter[$e])*$ms) } }
        $waits=[Collections.Generic.List[double]]::new();$calls=[Collections.Generic.List[double]]::new()
        foreach($t in (& $in 'present_start')){ $k=AtOrBefore $pace $t;if($k -ge 0 -and ($t-$pace[$k])*$ms -lt 50){ $waits.Add(($t-$pace[$k])*$ms) };$m=After $pe ($t-1);if($m -ge 0){ $calls.Add(($pe[$m]-$t)*$ms) } }
        "engine: end enter to post done p50 $(F (Q $posts.ToArray() 0.5)) p99 $(F (Q $posts.ToArray() 0.99)); after the stamp to the next game step (DS3's limiter and frame start) p50 $(F (Q $gaps.ToArray() 0.5)) p01 $(F (Q $gaps.ToArray() 0.01)) p99 $(F (Q $gaps.ToArray() 0.99))"
        $ls=$a['latency_wait_start'];$le=$a['latency_wait_end'];$lat=[double[]]@(if($ls -and $le){ for($i=0;$i -lt [math]::Min($ls.Count,$le.Count);$i++){ if($ls[$i] -ge $q0 -and $ls[$i] -lt $q1){ ($le[$i]-$ls[$i])*$ms } } })
        "worker: pacer wait p50 $(F (Q $waits.ToArray() 0.5)) p99 $(F (Q $waits.ToArray() 0.99)); latency wait p50 $(F (Q $lat 0.5)) p99 $(F (Q $lat 0.99)); Present call p50 $(F (Q $calls.ToArray() 0.5)) p99 $(F (Q $calls.ToArray() 0.99))"
    }elseif($a.ContainsKey('original_pacer_enter')){
        $en=$a['original_pacer_enter'];$lv=$a['original_pacer_leave']
        $inside=[double[]]@(foreach($t in $presents){ $k=AtOrBefore $en $t;if($k -ge 0){ ($t-$en[$k])*$ms } })
        "original: pacer (its wait and Present) p50 $(F (Q $inside 0.5)) p99 $(F (Q $inside 0.99))"
    }
    $phases=[double[]]@(foreach($t in $presents){ & $phase $t })
    $near=if(-not [double]::IsNaN($period)){ Count $phases { $args[0] -lt 1.0 -or $args[0] -gt $period-1.0 } }else{ 0 }
    "presents $($presents.Count): phase in the refresh p10 $(F (Q $phases 0.1)) p50 $(F (Q $phases 0.5)) p90 $(F (Q $phases 0.9)); within 1 ms of a vertical blank $near"
    foreach($t in $presents){ $k=AtOrBefore $gs $t;$rows.Add([pscustomobject]@{t_ms=[math]::Round(($t-$q0)*$ms,3);phase_ms=[math]::Round((& $phase $t),3);step_ms=if($k -ge 1){ [math]::Round(($gs[$k]-$gs[$k-1])*$ms,3) }else{ '' }}) }
    if(Test-Path -LiteralPath "$d/presentmon.csv"){
        $pm=@(Import-Csv -LiteralPath "$d/presentmon.csv" | Where-Object { $_.Application -eq 'DarkSoulsIII.exe' -and [double]$_.QPCTime -ge $q0 -and [double]$_.QPCTime -lt $q1 })
        $shown=@($pm | Where-Object { [double]$_.msUntilDisplayed -gt 0 });$holds=[double[]]@($shown | ForEach-Object { [double]$_.msBetweenDisplayChange } | Where-Object { $_ -gt 0 })
        # Final review L10: holds against the content's period (the median game step), not the screen's (279.96 Hz shows a
        # 60 frames/s frame over several refreshes; the vblank thread also misses some there).
        $p=if($steps.Count){ Q $steps 0.5 }else{ 16.67 }
        "presentmon: presents $($pm.Count), dropped $(@($pm | Where-Object { $_.Dropped -eq '1' }).Count); holds over 1.5 periods $(Count $holds { $args[0] -gt 1.5*$p }), under 0.5 $(Count $holds { $args[0] -lt 0.5*$p }); until displayed p50 $(F (Q ([double[]]@($shown | ForEach-Object { [double]$_.msUntilDisplayed })) 0.5))"
    }
    if($Csv -and $rows.Count){ $file=if($Run.Count -gt 1){ $Csv -replace '\.csv$',"-$([IO.Path]::GetFileName($d.TrimEnd('/','\'))).csv" }else{ $Csv };$rows | Export-Csv -LiteralPath $file -NoTypeInformation }
}
