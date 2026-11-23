param([Parameter(Mandatory)][string[]]$Runs,[int]$Skip=3)
# I-0090 (read-only): per run, I-0088's summary (frame and present p50/p99, the engine thread's stamps) and the render worker's
# rows: the medians over the in-game frame audits (the first -Skip dropped) of the worker's job and its slot release per frame,
# and the kept draws' last outcome row per 600 prepares (hits, model changed, the rebuilt rows that took their state).
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Median([double[]]$v){ if(-not $v.Count){ return $null }; $s=$v | Sort-Object; $n=$s.Count
    if($n%2){ return $s[($n-1)/2] }; return ($s[$n/2-1]+$s[$n/2])/2 }
$Runs=@($Runs -split '[,;\s]+' | Where-Object { $_ })
$base=@{};foreach($row in @(& "$root/investigations/I-0088-engine-thread-120-c/Get-I88Summary.ps1" -Runs $Runs -Skip $Skip)){ $base[$row.run]=$row }
foreach($run in $Runs){
    $lines=Get-Content -LiteralPath (Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl")
    $out=[ordered]@{};foreach($p in $base[$run].PSObject.Properties){ $out[$p.Name]=$p.Value }
    $audits=@($lines | Where-Object { $_ -like '*"native_frame_audit"*' } | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $_.worker_job_ms -gt 1 } | Select-Object -Skip $Skip)
    $out.audit_windows=$audits.Count
    $out.worker_job_ms=Median @($audits | ForEach-Object { [double]$_.worker_job_ms })
    $out.slot_release_ms=Median @($audits | ForEach-Object { [double]$_.slot_release_ms })
    if($audits.Count -and $null -ne $audits[0].PSObject.Properties['release_thread_ms']){ $out.release_thread_ms=Median @($audits | ForEach-Object { [double]$_.release_thread_ms }) }
    $kept=@($lines | Where-Object { $_ -like '*"native_persistent_draws"*' } | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $_.prepares -ge 600 })
    if($kept.Count){ $k=$kept[-1];$out.kept_hits=[math]::Round($k.hits/600,1);$out.model_changed=[math]::Round($k.model_changed/600,1)
        $out.state_reused=if($null -ne $k.PSObject.Properties['state_reused']){[math]::Round($k.state_reused/600,1)}else{$null} }
    [pscustomobject]$out
}
