param([Parameter(Mandatory)][string[]]$Runs,[int]$Skip=3)
# I-0091 (read-only): per run, I-0090's summary (frame and present p50/p99, the engine thread's stamps, the render worker's job)
# and the collect's frame store at the run's last native_frame_store row (blocks taken again, asked of the system, kept back,
# given back, held), per collect since the start.
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Runs=@($Runs -split '[,;\s]+' | Where-Object { $_ })
$base=@{};foreach($row in @(& "$root/investigations/I-0090-render-worker-frame-memory/Get-I90Summary.ps1" -Runs $Runs -Skip $Skip)){ $base[$row.run]=$row }
foreach($run in $Runs){
    $out=[ordered]@{run=$run};if($base.ContainsKey($run)){foreach($p in $base[$run].PSObject.Properties){ $out[$p.Name]=$p.Value }}
    $store=@(Get-Content -LiteralPath (Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl") | Where-Object { $_ -like '*"native_frame_store"*' } | ForEach-Object { $_ | ConvertFrom-Json })
    if($store.Count){ $s=$store[-1];$c=[double]$s.collects
        $out.store_reused=[math]::Round($s.reused/$c,2);$out.store_fresh=[math]::Round($s.fresh/$c,3);$out.store_freed=[math]::Round($s.freed/$c,3)
        $out.store_held_mb=$s.held_mb;$out.store_held_blocks=$s.held_blocks }
    [pscustomobject]$out
}
