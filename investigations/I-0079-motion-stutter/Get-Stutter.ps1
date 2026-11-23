param([Parameter(Mandatory)][string[]]$Runs,[switch]$Rows,[int]$TrimSeconds=2)
# I-0079: the frames over 34 ms of a Run-Stutter run inside its motion (motion-marks.txt: the pilot's TickCount64 at the motion's
# start and end; the journal's rows carry the module's GetTickCount64, the same clock), natively (native_slow_frame with content
# draws) or in the original (original_slow_frame), and the 120-frame windows ending inside the motion (native_frame_times or
# native_original_frames: frames over 17/25/34 ms). -Rows lists the slow rows.
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
foreach($run in @($Runs -split '[,;\s]+' | Where-Object { $_ })){
    $dir=Join-Path $root "evidence/I-0066/artifacts/$run"
    $marks=@{};foreach($line in Get-Content -LiteralPath "$dir/motion-marks.txt"){ $f=$line -split ' ';$marks[$f[0]]=[uint64]$f[1] }
    $from=$marks['motion_start']+1000*$TrimSeconds;$to=$marks['motion_end']
    $journal=@(Get-Content -LiteralPath "$dir/startup.jsonl" | Where-Object { $_ -like '*"native_slow_frame"*' -or $_ -like '*"original_slow_frame"*' -or $_ -like '*"native_frame_times"*' -or $_ -like '*"native_original_frames"*' } |
        ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $_.tick_ms -and [uint64]$_.tick_ms -ge $from -and [uint64]$_.tick_ms -le $to })
    $slow=@($journal | Where-Object { ($_.event -eq 'native_slow_frame' -and $_.content_draws -gt 0) -or $_.event -eq 'original_slow_frame' })
    $windows=@($journal | Where-Object { $_.event -in 'native_frame_times','native_original_frames' })
    $sum={ param($k) ($windows | ForEach-Object { [int]$_.over_17_25_34_ms[$k] } | Measure-Object -Sum).Sum }
    $sorted=@($slow | ForEach-Object { [double]$_.interval_ms } | Sort-Object)
    [pscustomobject][ordered]@{run=$run;seconds=[Math]::Round(($to-$from)/1000.0,1);windows=$windows.Count;frames=120*$windows.Count
        over17=& $sum 0;over25=& $sum 1;over34=& $sum 2;slow_rows=$slow.Count;worst_ms=$(if($sorted.Count){$sorted[-1]}else{0})
        slow_median_ms=$(if($sorted.Count){$sorted[[int](($sorted.Count-1)/2)]}else{0})}
    if($Rows){ $slow | Select-Object * -ExcludeProperty event | Format-Table -AutoSize | Out-String -Width 400 }
}
