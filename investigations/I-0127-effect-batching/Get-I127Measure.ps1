# I-0127: the native_effect_measure rows of runs (Run-I127Measure.ps1), one line a 600-frame window, then each run's summary
# over its windows with draws (the frames-weighted means, the windows' worst p99 and maximum). Calls and repeats by kind, in
# the module's order: heaps, pipeline, topology, targets, texture table, sampler table, constants, vertices, indices, views,
# draws. Reads every *.jsonl of each run folder under evidence/I-0066/artifacts (the module's journal rows).
param([Parameter(Mandatory)][string[]]$RunNames,[switch]$Windows)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$kinds='heaps','pipeline','topology','targets','textable','samptable','cbv','vb','ib','views','draws'
foreach($run in @($RunNames -split ",")){
    $dir=Join-Path $repo "evidence/I-0066/artifacts/$run";if(-not (Test-Path -LiteralPath $dir)){"${run}: no folder";continue}
    $rows=foreach($f in Get-ChildItem -LiteralPath $dir -Filter *.jsonl -Recurse){foreach($l in [IO.File]::ReadLines($f.FullName)){
        if($l.Contains('"native_effect_measure"')){$i=$l.IndexOf('{"event":"native_effect_measure"');if($i -ge 0){try{$l.Substring($i)|ConvertFrom-Json}catch{}}}}}
    $rows=@($rows|Sort-Object serial -Unique)
    if(-not $rows.Count){"${run}: no native_effect_measure row";continue}
    if($Windows){foreach($r in $rows){'{0} serial {1}: cpu {2:N1} us (p99 {3}, max {4:N0}), gpu {5:N1} us (p99 {6}, max {7:N0}), draws {8:N1} (max {9}), pipelines {10:N1}, calls {11:N0}, repeats {12:N0}' -f $run,$r.serial,$r.cpu_us.mean,$r.cpu_us.p99,$r.cpu_us.max,$r.gpu_us.mean,$r.gpu_us.p99,$r.gpu_us.max,$r.draws.mean,$r.draws.max,$r.pipeline_changes_mean,($r.calls|Measure-Object -Sum).Sum,($r.repeats|Measure-Object -Sum).Sum}}
    $busy=@($rows|Where-Object{$_.draws.mean -gt 0});if(-not $busy.Count){"${run}: $($rows.Count) windows, none with effect draws";continue}
    $frames=($busy|Measure-Object frames -Sum).Sum
    $w={param($get)($busy|ForEach-Object{(& $get $_)*$_.frames}|Measure-Object -Sum).Sum/$frames}
    $calls=foreach($k in 0..($kinds.Count-1)){'{0} {1:N1}/{2:N1}' -f $kinds[$k],(& $w {param($r)$r.calls[$k]}),(& $w {param($r)$r.repeats[$k]})}
    '{0}: {1} windows with draws ({2} frames): cpu {3:N1} us (worst p99 {4}, max {5:N0}), hooks {6:N1} us, gpu {7:N1} us (worst p99 {8}, max {9:N0}), draws {10:N1} (max {11}), pipeline changes {12:N1}, arena {13:N1} KB' -f `
        $run,$busy.Count,$frames,(& $w {param($r)$r.cpu_us.mean}),($busy|ForEach-Object{$_.cpu_us.p99}|Measure-Object -Maximum).Maximum,($busy|ForEach-Object{$_.cpu_us.max}|Measure-Object -Maximum).Maximum,
        (& $w {param($r)$r.hooks_us_mean}),(& $w {param($r)$r.gpu_us.mean}),($busy|ForEach-Object{$_.gpu_us.p99}|Measure-Object -Maximum).Maximum,($busy|ForEach-Object{$_.gpu_us.max}|Measure-Object -Maximum).Maximum,
        (& $w {param($r)$r.draws.mean}),($busy|ForEach-Object{$_.draws.max}|Measure-Object -Maximum).Maximum,(& $w {param($r)$r.pipeline_changes_mean}),(& $w {param($r)$r.arena_kb.mean})
    '  calls/repeats a frame: '+($calls -join ', ')}
