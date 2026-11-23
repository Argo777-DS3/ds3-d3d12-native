# D3D12 review phase A (D3D12-PHASES-A-C-PREP 3.1): medians of the in-game pacing windows of native runs.
# Read-only. Keeps the windows with content draws, discards the first -Skip of them (the load-in transition),
# and prints one row per run with the medians of each field and the number of windows used.
param([Parameter(Mandatory)][string[]]$Runs,[int]$Skip=3)
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Median([double[]]$v){ if(-not $v.Count){ return $null }; $s=$v | Sort-Object; $n=$s.Count
    if($n%2){ return $s[($n-1)/2] }; return ($s[$n/2-1]+$s[$n/2])/2 }
$Runs=@($Runs -split '[,;\s]+' | Where-Object { $_ })
foreach($run in $Runs){
    $trace=Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl"
    $rows=@(Get-Content -LiteralPath $trace | Where-Object { $_ -like '*"native_present_pacing"*' } | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $_.content_draws -gt 0 })
    $use=@($rows | Select-Object -Skip $Skip)
    $out=[ordered]@{run=$run;windows=$use.Count}
    foreach($f in 'fps','slowest_ms','content_draws','collect_ms','prepare_ms','render_ms','record_ms','gpu_wait_ms','present_ms'){
        $out[$f]=Median @($use | Where-Object { $null -ne $_.$f } | ForEach-Object { [double]$_.$f }) }
    foreach($pair in @(@('ui_produce_drain_build_wait_diag_ms',@('ui_produce','ui_drain','ui_build','ui_wait','ui_diag')),
                       @('prep_validate_resources_draw_ms',@('prep_validate','prep_resources','prep_draw')),
                       @('queue_execute_drain_complete_per_frame',@('executes','drains','completes')),
                       @('memory_private_local_mb',@('private_mb','local_mb')))){
        for($k=0;$k -lt $pair[1].Count;++$k){ $vals=@($use | Where-Object { $null -ne $_.($pair[0]) } | ForEach-Object { [double]$_.($pair[0])[$k] })
            $out[$pair[1][$k]]=Median $vals } }
    [pscustomobject]$out
}
