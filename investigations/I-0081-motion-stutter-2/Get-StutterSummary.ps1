param([Parameter(Mandatory)][string[]]$Runs)
# I-0081: one row per Run-Stutter run: its mode checked in baseline.json, then inside the motion (motion-marks.txt, the first
# 2 s left out) the frames over 17 / 25 / 34 ms at the native frame end or the original's pacer (Get-Stutter.ps1), the game step
# at the world update gate under both renderers (game_frame_times, absent from the original's probe runs) and the frames the
# display held two refreshes or more (native_present_refreshes, from SyncRefreshCount; native only).
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
foreach($run in @($Runs -split '[,;\s]+' | Where-Object { $_ })){
    $dir=Join-Path $root "evidence/I-0066/artifacts/$run"
    $base=Get-Content -LiteralPath "$dir/baseline.json" -Raw | ConvertFrom-Json
    $marks=@{};foreach($line in Get-Content -LiteralPath "$dir/motion-marks.txt"){ $f=$line -split ' ';$marks[$f[0]]=[uint64]$f[1] }
    $from=$marks['motion_start']+2000;$to=$marks['motion_end']
    $stutter=& "$root/investigations/I-0079-motion-stutter/Get-Stutter.ps1" -Runs $run
    $rows=@(Get-Content -LiteralPath "$dir/startup.jsonl" | Where-Object { $_ -like '*"game_frame_times"*' -or $_ -like '*"native_present_refreshes"*' -or $_ -like '*"native_frame_times"*' } | ForEach-Object { $_ | ConvertFrom-Json })
    $game=@($rows | Where-Object { $_.event -eq 'game_frame_times' -and $_.tick_ms -ge $from -and $_.tick_ms -le $to })
    $g=@(0,0,0);foreach($r in $game){for($k=0;$k -lt 3;$k++){$g[$k]+=$r.over_17_25_34_ms[$k]}}
    # The refresh rows carry no tick: their frame numbers are bounded by the native_frame_times rows inside the motion.
    $times=@($rows | Where-Object { $_.event -eq 'native_frame_times' -and $_.tick_ms -ge $from -and $_.tick_ms -le $to })
    $held='';if($times.Count){$first=($times | Measure-Object frame -Minimum).Minimum;$last=($times | Measure-Object frame -Maximum).Maximum
        $c=@(0,0,0,0);$n=0;foreach($r in @($rows | Where-Object { $_.event -eq 'native_present_refreshes' -and $_.frame -gt $first -and $_.frame -le $last })){$n+=$r.frames;for($k=0;$k -lt 4;$k++){$c[$k]+=$r.refreshes_1_2_3_4plus[$k]}}
        $held="$($c[1]+$c[2]+$c[3]) of $n"}
    [pscustomobject][ordered]@{run=$run;mode=$base.bootstrap_mode;measure=$base.measure;no_debug=$base.debug_layer_disabled;no_kprofile=$base.no_kprofile
        over17_25_34="$($stutter.over17)/$($stutter.over25)/$($stutter.over34)";worst_ms=$stutter.worst_ms
        game_step_17_25_34=$(if($game.Count){$g -join '/'}else{''});held_2plus=$held}
}
