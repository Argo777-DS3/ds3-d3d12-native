param([Parameter(Mandatory)][string[]]$Runs,[int]$Skip=3)
# I-0078: medians of the native_original_frames rows (original-probe.flag) of reference runs: the frame time between two calls
# of the original's present pacer, the time inside it (wait and present) and their difference (the presenting thread's work).
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Median([double[]]$v){ if(-not $v.Count){ return $null }; $s=$v | Sort-Object; $n=$s.Count
    if($n%2){ return $s[($n-1)/2] }; return ($s[$n/2-1]+$s[$n/2])/2 }
foreach($run in @($Runs -split '[,;\s]+' | Where-Object { $_ })){
    $rows=@(Get-Content -LiteralPath (Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl") | Where-Object { $_ -like '*"native_original_frames"*' } | ForEach-Object { $_ | ConvertFrom-Json })
    $use=@($rows | Select-Object -Skip $Skip)
    [pscustomobject][ordered]@{run=$run;windows=$use.Count;threads=(@($use | ForEach-Object { $_.thread } | Sort-Object -Unique) -join ',');unlocked=($use | Select-Object -First 1).unlocked
        frame_ms=Median @($use | ForEach-Object { [double]$_.frame_ms_mean });frame_p50=Median @($use | ForEach-Object { [double]$_.frame_ms_p50 })
        frame_p99=Median @($use | ForEach-Object { [double]$_.frame_ms_p99 });pacer_ms=Median @($use | ForEach-Object { [double]$_.pacer_ms_mean })
        work_ms=Median @($use | ForEach-Object { [double]$_.work_ms_mean });fps=$(if($use.Count){[Math]::Round(1000/(Median @($use | ForEach-Object { [double]$_.frame_ms_mean })),1)}else{$null})}
}
