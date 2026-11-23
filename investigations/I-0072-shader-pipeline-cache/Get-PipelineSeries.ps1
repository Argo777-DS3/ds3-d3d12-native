param([Parameter(Mandatory)][string[]]$Runs)
# I-0072: read-only summary of native runs (evidence/I-0066/artifacts/<run>/startup.jsonl) for the pipeline cache. Per run: the
# in-game windows (native_frame_times with content): frames over 34 ms, p99 median, worst frame; the pipelines compiled on the
# frame path (native_present_pacing.pso_created) and the frames over 34 ms of the windows that compiled some against the
# others; the slow frames carrying compilations (native_slow_frame); the cache's last counters (native_frame_audit
# pipeline_library [loaded, created, stored, saves] and pipeline_cache [memory hits, precompiled, listed, failed, waits,
# direct, precompile ms, done]) and its start row (native_pipeline_library). Historical: it reads the journals before I-0113
# (D-0241), whose rows are native_pipeline_list, pipeline_store and native_pipeline_list_close.
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Median([double[]]$v){ if(-not $v.Count){ return $null }; $s=$v | Sort-Object; $n=$s.Count; if($n%2){ return $s[($n-1)/2] }; ($s[$n/2-1]+$s[$n/2])/2 }
foreach($run in @($Runs -split '[,;\s]+' | Where-Object { $_ })){
    $trace=Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl"
    $times=@{};$pacing=@();$slow=@();$audit=$null;$start=$null;$close=$null
    foreach($line in [IO.File]::ReadLines($trace)){
        if($line.Contains('"native_frame_times"')){ $o=$line | ConvertFrom-Json;$times[$o.frame]=$o }
        elseif($line.Contains('"native_present_pacing"')){ $pacing+=($line | ConvertFrom-Json) }
        elseif($line.Contains('"native_slow_frame"')){ $slow+=($line | ConvertFrom-Json) }
        elseif($line.Contains('"native_frame_audit"')){ $audit=$line }
        elseif($line.Contains('"native_pipeline_library_close"')){ $close=$line }
        elseif($line.Contains('"native_pipeline_library"')){ $start=$line | ConvertFrom-Json } }
    $previous=0;$compiled=0;$windows=0;$overWith=0;$overWithout=0;$p99=@();$worst=0;$over34=0
    foreach($p in $pacing){ $d=$p.pso_created-$previous;$previous=$p.pso_created;$t=$times[$p.frame]
        if(-not $t -or -not $t.content){ continue }
        $over34+=$t.over_17_25_34_ms[2];$p99+=[double]$t.ms_p50_p99_max[1];$worst=[Math]::Max($worst,[double]$t.ms_p50_p99_max[2])
        if($d -gt 0){ $compiled+=$d;$windows++;$overWith+=$t.over_17_25_34_ms[2] }else{ $overWithout+=$t.over_17_25_34_ms[2] } }
    $slowCompiling=@($slow | Where-Object { $_.pso_created -gt 0 })
    [pscustomobject][ordered]@{run=$run;over34=$over34;p99_median=Median $p99;worst_ms=$worst
        frame_path_compiled=$compiled;compiling_windows=$windows;over34_compiling=$overWith;over34_other=$overWithout
        slow_compiling=$slowCompiling.Count;slow_compiling_ms=[int](($slowCompiling | Measure-Object -Property interval_ms -Sum).Sum)
        library=[regex]::Match("$audit",'"pipeline_library":\[[0-9,]*\]').Value;cache=[regex]::Match("$audit",'"pipeline_cache":\[[0-9,]*\]').Value
        start=$(if($start){"file_loaded=$($start.file_loaded) list_loaded=$($start.list_loaded) records=$($start.records) workers=$($start.precompile_workers)"})
        close=$(if($close){($close | ConvertFrom-Json | ForEach-Object { "precompiled=$($_.precompiled)/$($_.precompile_total) ms=$($_.precompile_ms) hits=$($_.memory_hits) waits=$($_.waits) direct=$($_.direct) records=$($_.records)" })})}
}
