param([Parameter(Mandatory)][string[]]$Runs,[int]$Skip=3)
# I-0088 (read-only): per run, the medians over the in-game windows (the first -Skip dropped) of the engine thread's frame
# intervals (native_frame_times p50, p99, over 17/25/34 ms) and the present intervals, then Get-CollectSplit's fields (frames/s,
# collect, the engine thread's stamps: to_begin is DS3's own work before the frame's native hooks, ui the UI stamp).
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Median([double[]]$v){ if(-not $v.Count){ return $null }; $s=$v | Sort-Object; $n=$s.Count
    if($n%2){ return $s[($n-1)/2] }; return ($s[$n/2-1]+$s[$n/2])/2 }
$Runs=@($Runs -split '[,;\s]+' | Where-Object { $_ })
$split=@{};foreach($row in @(& "$root/investigations/I-0066-independent-native-startup/Get-CollectSplit.ps1" -Runs $Runs -Skip $Skip | Where-Object { $null -ne $_.windows })){ $split[$row.run]=$row }
foreach($run in $Runs){
    $lines=Get-Content -LiteralPath (Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl")
    $out=[ordered]@{run=$run}
    foreach($event in 'native_frame_times','native_present_times'){
        $rows=@($lines | Where-Object { $_ -like "*`"$event`"*" } | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $_.content } | Select-Object -Skip $Skip)
        $short=if($event -eq 'native_frame_times'){'frame'}else{'present'}
        $out["${short}_windows"]=$rows.Count
        $out["${short}_p50"]=Median @($rows | ForEach-Object { [double]$_.ms_p50_p99_max[0] })
        $out["${short}_p99"]=Median @($rows | ForEach-Object { [double]$_.ms_p50_p99_max[1] })
        $out["${short}_over_25"]=($rows | ForEach-Object { [double]$_.over_17_25_34_ms[1] } | Measure-Object -Sum).Sum
        $out["${short}_over_34"]=($rows | ForEach-Object { [double]$_.over_17_25_34_ms[2] } | Measure-Object -Sum).Sum }
    if($split.ContainsKey($run)){$s=$split[$run];foreach($f in 'fps','collect_ms','stamp_to_begin','stamp_to_update','stamp_update','stamp_ui','stamp_to_end_frame','stamp_clear'){$out[$f]=$s.$f}
        $out.native_ms=[double]$s.stamp_update+[double]$s.stamp_ui+[double]$s.stamp_clear}
    [pscustomobject]$out
}
