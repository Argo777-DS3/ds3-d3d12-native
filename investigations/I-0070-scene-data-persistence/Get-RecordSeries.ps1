param([Parameter(Mandatory)][string[]]$Runs,[switch]$Windows)
# I-0070: read-only summary of native runs (evidence/I-0066/artifacts/<run>/startup.jsonl). Per run: the in-game pacing windows
# (native_frame_times: p99 and frames over 34 ms), the collect counters per collect (native_collect_split, kprofile runs: fresh
# admissions = records built + invalid + volatile, texture lookups, collect ms) and the record outcomes over the run
# (the last native_model_records row). -Windows prints every window.
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Median([double[]]$v){ if(-not $v.Count){ return $null }; $s=$v | Sort-Object; $n=$s.Count; if($n%2){ return $s[($n-1)/2] }; ($s[$n/2-1]+$s[$n/2])/2 }
foreach($run in @($Runs -split '[,;\s]+' | Where-Object { $_ })){
    $trace=Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl"
    $times=@();$splits=@();$records=$null;$pacing=@()
    foreach($line in [IO.File]::ReadLines($trace)){
        if($line.Contains('"native_frame_times"')){ $times+=($line | ConvertFrom-Json) }
        elseif($line.Contains('"native_collect_split"')){ $splits+=($line | ConvertFrom-Json) }
        elseif($line.Contains('"native_model_records"')){ $records=$line | ConvertFrom-Json }
        elseif($line.Contains('"native_present_pacing"')){ $pacing+=($line | ConvertFrom-Json) } }
    $content=@($times | Where-Object { $_.content })
    $splitsIn=@($splits | Where-Object { $_.collects -gt 0 })
    if($Windows){ foreach($w in $splitsIn){ $t=$content | Where-Object { $_.frame -eq $w.frame } | Select-Object -First 1
        '{0} {1,6} ms={2,5:N2} built={3,6:N1} invalid={4,6:N1} volatile={5,5:N1} lookups={6,6:N1} models={7,6:N1} p99={8} over34={9}' -f $run,$w.frame,$w.collect_ms,$w.counts[12],$w.counts[13],$w.counts[14],$w.counts[7],$w.counts[2],$(if($t){$t.ms_p50_p99_max[1]}),$(if($t){$t.over_17_25_34_ms[2]}) } }
    $over34=($content | Measure-Object -Property { $_.over_17_25_34_ms[2] } -Sum).Sum
    $over25=($content | Measure-Object -Property { $_.over_17_25_34_ms[1] } -Sum).Sum
    $frames=($content | Measure-Object -Property frames -Sum).Sum
    $use=@($pacing | Where-Object { $_.content_draws -gt 0 } | Select-Object -Skip 3)
    [pscustomobject][ordered]@{run=$run;windows=$content.Count;frames=$frames;over25=$over25;over34=$over34
        p99_median=Median @($content | ForEach-Object { [double]$_.ms_p50_p99_max[1] });p99_max=($content | ForEach-Object { [double]$_.ms_p50_p99_max[1] } | Measure-Object -Maximum).Maximum
        worst_ms=($content | ForEach-Object { [double]$_.ms_p50_p99_max[2] } | Measure-Object -Maximum).Maximum
        fps=Median @($use | ForEach-Object { [double]$_.fps });collect_ms=Median @($use | ForEach-Object { [double]$_.collect_ms })
        fresh_per_collect=$(if($splitsIn.Count){ ($splitsIn | ForEach-Object { $_.counts[12]+$_.counts[13]+$_.counts[14] } | Measure-Object -Maximum).Maximum })
        lookups_max=$(if($splitsIn.Count){ ($splitsIn | ForEach-Object { $_.counts[7] } | Measure-Object -Maximum).Maximum })
        outcomes=$(if($records){ ($records.outcomes.PSObject.Properties | Where-Object { $_.Value } | ForEach-Object { "$($_.Name)=$($_.Value)" }) -join ' ' })
        collects=$(if($records){$records.collects})}
}
