# Workstream K: medians of the native_collect_split rows of native measurement runs (read-only). Keeps the windows whose
# pacing row (same frame) has content draws and 120 collects, drops the first -Skip of them, and prints per run the
# medians of collect_ms, the 14 self times, the counters, the C-0b stamp buckets and the C-5 reuse counts.
param([Parameter(Mandatory)][string[]]$Runs,[int]$Skip=3)
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Median([double[]]$v){ if(-not $v.Count){ return $null }; $s=$v | Sort-Object; $n=$s.Count
    if($n%2){ return $s[($n-1)/2] }; return ($s[$n/2-1]+$s[$n/2])/2 }
$phases='setup','traversal','filter','model','lookups','programs','textures','shader_tex','matdyn','blocks','passes','lights','offscreen','teardown'
$counters='pairs','meshes','models','programs_built','shared_built','numeric_misses','shader_tex_misses','texture_lookups',
    'fallbacks_kept','scene_tables_carried','instance_tables_carried',   # P4c, C7-1c
    'records_reused','records_built','records_invalid','records_volatile','records_revalidated','record_mismatches'   # C7-2
$stamps='to_begin','to_update','update','to_render_step','to_ui','ui','ui_to_step_end','to_end_frame','to_clear','clear'
$Runs=@($Runs -split '[,;\s]+' | Where-Object { $_ })
foreach($run in $Runs){
    $trace=Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl"
    $lines=Get-Content -LiteralPath $trace
    $drawn=@{};foreach($row in @($lines | Where-Object { $_ -like '*"native_present_pacing"*' } | ForEach-Object { $_ | ConvertFrom-Json })){ if($row.content_draws -gt 0){ $drawn[[int64]$row.frame]=$row } }
    $rows=@($lines | Where-Object { $_ -like '*"native_collect_split"*' } | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $drawn.ContainsKey([int64]$_.frame) -and $_.collects -ge 120 })
    $use=@($rows | Select-Object -Skip $Skip)
    $out=[ordered]@{run=$run;windows=$use.Count;fps=(Median @($use | ForEach-Object { [double]$drawn[[int64]$_.frame].fps }));
        collect_ms=(Median @($use | ForEach-Object { [double]$_.collect_ms }));prepare_ms=(Median @($use | ForEach-Object { [double]$drawn[[int64]$_.frame].prepare_ms }))}
    if($use.Count -and $null -ne $use[0].self_ms){for($k=0;$k -lt $phases.Count;++$k){$out[$phases[$k]]=Median @($use | ForEach-Object { [double]$_.self_ms[$k] })}}
    if($use.Count -and $null -ne $use[0].counts){for($k=0;$k -lt [Math]::Min($counters.Count,$use[0].counts.Count);++$k){$out[$counters[$k]]=Median @($use | ForEach-Object { [double]$_.counts[$k] })}}
    if($use.Count -and $null -ne $use[0].stamp_ms){for($k=0;$k -lt $stamps.Count;++$k){$out["stamp_$($stamps[$k])"]=Median @($use | ForEach-Object { [double]$_.stamp_ms[$k] })}}
    if($use.Count -and $null -ne $use[0].reuse){$out.reuse_unchanged=Median @($use | ForEach-Object { [double]$_.reuse[0] });
        $out.reuse_changed=Median @($use | ForEach-Object { [double]$_.reuse[1] });$out.reuse_new=Median @($use | ForEach-Object { [double]$_.reuse[2] })}
    [pscustomobject]$out
}
# PREPARE-REDUCTION cP0: medians of native_prepare_split (main view prepare, self ms per prepare) over the same windows.
$prepPhases='row_setup','palette_check','forward_light','stored','matdyn','model_params','palettes','frame_blocks','other_blocks','samplers_srvs','draw_programs','draw_validate','draw_pipeline','draw_samplers','draw_constants','draw_srvs','frame_tail','kept'
$prepCounters='rows','draws','constant_blocks','constant_bytes','srvs','palette_checks','palette_in_range','dry_rows','dry_identity_hits','dry_content_hits','dry_hit_draw_us','kept_hits','kept_new','kept_model','kept_row','kept_shadow','kept_frame_refusals'
foreach($run in $Runs){
    $trace=Join-Path $root "evidence/I-0066/artifacts/$run/startup.jsonl"
    $lines=Get-Content -LiteralPath $trace
    $drawn=@{};foreach($row in @($lines | Where-Object { $_ -like '*"native_present_pacing"*' } | ForEach-Object { $_ | ConvertFrom-Json })){ if($row.content_draws -gt 0){ $drawn[[int64]$row.frame]=$row } }
    $rows=@($lines | Where-Object { $_ -like '*"native_prepare_split"*' } | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $drawn.ContainsKey([int64]$_.frame) -and $_.prepares -ge 120 })
    if(-not $rows.Count){ continue }
    $use=@($rows | Select-Object -Skip $Skip)
    $collect=@($lines | Where-Object { $_ -like '*"native_collect_split"*' } | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $drawn.ContainsKey([int64]$_.frame) -and $_.collects -ge 120 } | Select-Object -Skip $Skip)
    $out=[ordered]@{run=$run;prepare_windows=$use.Count;prepare_ms=(Median @($use | ForEach-Object { [double]$_.prepare_ms }));split_sum_ms=(Median @($use | ForEach-Object { [double]$_.split_sum_ms }))}
    for($k=0;$k -lt $prepPhases.Count;++$k){$out["prep_$($prepPhases[$k])"]=Median @($use | ForEach-Object { [double]$_.self_ms[$k] })}
    # The prepare row carries its counters per prepare since the pre-merge review; earlier runs had them per collect at 8 to 14.
    # PD-0 counters exist from Native1429 on (older rows have fewer counts); dry_hit_draw_us sums whole microseconds per row (a lower bound).
    if($null -ne $use[0].counts){for($k=0;$k -lt [Math]::Min($prepCounters.Count,$use[0].counts.Count);++$k){$out["prep_$($prepCounters[$k])"]=Median @($use | ForEach-Object { [double]$_.counts[$k] })}}
    elseif($collect.Count -and $collect[0].counts.Count -ge 15){for($k=0;$k -lt [Math]::Min($prepCounters.Count,$collect[0].counts.Count-8);++$k){$out["prep_$($prepCounters[$k])"]=Median @($collect | ForEach-Object { [double]$_.counts[8+$k] })}}
    [pscustomobject]$out
}
