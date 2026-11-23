param([Parameter(Mandatory)][string[]]$Runs)
# I-0100 tranche 7: per run of a replayed path made with crossing-observe.flag, the layouts' programme sets built over the whole
# run and in play (native_programs_build: sets, those taken from a kept recipe set, their pool time by part), the collects whose
# admission took 3 ms or more in play (native_admission_slow), and the frames over 17, 25 and 34 ms of the native_frame_times
# windows ending in the walk and in the turn (the path's motion marks; a window ending after the turn is left out: at Archdragon
# Peak the game closes then). « Play » starts at the path's motion start, mapped to the module's frame serial by the first
# native_frame_times window ending after it (final review H1: a fixed frame 1400 counted Archdragon's load as play in some runs
# and not in others). Read-only.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
foreach($run in $Runs){
    $d="$repo/evidence/I-0066/artifacts/$run";$journal="$d/startup.jsonl"
    $marks=Get-Content -LiteralPath "$d/motion-marks.txt"
    $tick={param($name)[int64](($marks | Where-Object {$_ -like "$name *"}) -split ' ')[1]}
    $walk0=& $tick 'segment_walk_start';$turn0=& $tick 'segment_sweep_start';$turn1=& $tick 'segment_sweep_end'
    $windows=@(Select-String -LiteralPath $journal -Pattern '"event":"native_frame_times"' | ForEach-Object {$_.Line | ConvertFrom-Json})
    $playFrame=($windows | Where-Object {$_.tick_ms -ge $walk0} | Select-Object -First 1).frame
    $rows=@(Select-String -LiteralPath $journal -Pattern '"event":"native_programs_build"' | ForEach-Object {$_.Line | ConvertFrom-Json})
    $start=$rows | Where-Object {$_.frame -le $playFrame} | Select-Object -Last 1;$last=$rows[-1]
    $parts=for($k=0;$k -lt $last.parts_ms.Count;$k++){[math]::Round($last.parts_ms[$k]-$start.parts_ms[$k],1)}
    $slow=@(Select-String -LiteralPath $journal -Pattern '"event":"native_admission_slow"' | ForEach-Object {$_.Line | ConvertFrom-Json} | Where-Object {$_.serial -ge $playFrame})
    $count={param($set,$i)($set | ForEach-Object {$_.over_17_25_34_ms[$i]} | Measure-Object -Sum).Sum}
    $walk=@($windows | Where-Object {$_.tick_ms -gt $walk0 -and $_.tick_ms -le $turn0});$turn=@($windows | Where-Object {$_.tick_ms -gt $turn0 -and $_.tick_ms -le $turn1})
    [ordered]@{run=$run;play_from_frame=$playFrame;run_built=$last.built;run_taken=$last.recipe_sets_taken;run_ms=[math]::Round(($last.parts_ms | Measure-Object -Sum).Sum,1)
        play_built=$last.built-$start.built;play_taken=$last.recipe_sets_taken-$start.recipe_sets_taken;play_ms=[math]::Round(($parts | Measure-Object -Sum).Sum,1);play_parts_ms=$parts
        slow_collects=$slow.Count;longest_ms=($slow.wall_ms | Measure-Object -Maximum).Maximum
        walk_over_25_34=@((& $count $walk 1),(& $count $walk 2));turn_over_17_25_34=@((& $count $turn 0),(& $count $turn 1),(& $count $turn 2))
        turn_worst_ms=($turn | ForEach-Object {$_.ms_p50_p99_max[2]} | Measure-Object -Maximum).Maximum} | ConvertTo-Json -Compress
}
