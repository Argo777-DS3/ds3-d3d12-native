param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Run,[ValidateRange(0,120)][int]$SettleSeconds=15)
# I-0085: one row per zone visit and per title stay of an endurance run (Run-Endurance.ps1), written to visits.csv beside the
# journal, and the trends as JSON (summary.json, also printed). The journal is split by its native_present_pacing rows (every
# 120 frames): a world stay is a run of rows whose last frame drew 200 content draws or more, named by the tour's "in world"
# frame inside it; a title stay a run of rows at 0, ended at the next load's click (the loading screen draws nothing too). A
# world stay leaves out its first -SettleSeconds (the load's own stutter), a title stay its first 10 s (the quit's unloading). Per stay: the private, local and non-local memory (median and peak, MB), the CPU descriptors, pending
# retirements and pipelines created, the streamed heaps, placed and staging memory (native_streamed_memory, every 600 frames),
# and in a world stay the frames over 25 and 34 ms and the p99 (native_frame_times). Trends: the least-squares slope per visit
# of the title levels (the memory left behind by each visit) and, per zone, of the world levels and the slow frames.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$dir=Join-Path $root "evidence/I-0066/artifacts/$Run"
$pacing=[Collections.Generic.List[object]]::new();$streamed=[Collections.Generic.List[object]]::new();$times=[Collections.Generic.List[object]]::new()
$registry=[Collections.Generic.List[object]]::new()   # native_model_registry (I-0085's fix build on), every 600 frames
$faults=[Collections.Generic.List[string]]::new();$dumps=[Collections.Generic.List[string]]::new();$counts=[ordered]@{over_budget=0;slow_frame=0;crash_dump=0;budget_released=0}
# Shared for writing: a run still in progress can be read.
$reader=[IO.StreamReader]::new([IO.FileStream]::new("$dir/startup.jsonl",[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite))
$lines=while($null -ne ($l=$reader.ReadLine())){ $l };$reader.Dispose()
foreach($line in $lines){
    if($line.StartsWith('{"event":"native_present_pacing"')){ $r=$line | ConvertFrom-Json
        $pacing.Add([pscustomobject]@{frame=[long]$r.frame;fps=$r.fps;slowest=$r.slowest_ms;draws=[int]$r.content_draws;private=[int]$r.memory_private_local_mb[0];local=[int]$r.memory_private_local_mb[1]
            budget=[int]$r.local_budget_mb;nonlocal=$(if($r.nonlocal_usage_budget_mb){[int]$r.nonlocal_usage_budget_mb[0]}else{0});descriptors=[int]$r.cpu_descriptors;retirements=[int]$r.retirements_pending;pso=[long]$r.pso_created}) }
    elseif($line.StartsWith('{"event":"native_streamed_memory"')){ $r=$line | ConvertFrom-Json
        $streamed.Add([pscustomobject]@{frame=[long]$r.frame;heaps=[int]$r.heaps;placed=[int]$r.placed;placed_mb=[double]$r.placed_live_mb;staging_mb=[double]$r.staging_mb}) }
    elseif($line.StartsWith('{"event":"native_frame_times"')){ $r=$line | ConvertFrom-Json
        $times.Add([pscustomobject]@{frame=[long]$r.frame;content=[bool]$r.content;n=[int]$r.frames;p99=[double]$r.ms_p50_p99_max[1];max=[double]$r.ms_p50_p99_max[2];o25=[int]$r.over_17_25_34_ms[1];o34=[int]$r.over_17_25_34_ms[2];tick=[uint64]$r.tick_ms}) }
    elseif($line.StartsWith('{"event":"native_model_registry"')){ $r=$line | ConvertFrom-Json
        $registry.Add([pscustomobject]@{frame=[long]$r.frame;materials=[int]$r.materials;layouts=[int]$r.layouts;retired=[long]$r.layouts_retired;spares=[int]$r.spare_prefixes
            textures=$(if($null -ne $r.texture_records){[int]$r.texture_records}else{$null});targets=$(if($null -ne $r.render_targets){[int]$r.render_targets}else{$null})}) }   # I-0087: the ingress census
    elseif($line.StartsWith('{"event":"native_video_memory_over_budget"')){ $counts.over_budget++;if($line -match '"released":(\d+)'){ $counts.budget_released+=[int]$Matches[1] } }
    elseif($line.StartsWith('{"event":"native_slow_frame"')){ $counts.slow_frame++ }
    elseif($line.StartsWith('{"event":"native_crash_dump"')){ $counts.crash_dump++;$dumps.Add($line) }   # in play; one at the quit leaves the controller's row only (controller_crash)
    elseif($line -match '^\{"event":"(native_failure|refusal|native_device_removed)"'){ $faults.Add($line) } }
if(-not $pacing.Count){ throw "no pacing rows in $Run" }
# The tour's log: the frame each zone was seen in the world at ("in world ... frame=N"), and each load's click ("loading": the
# last pacing frame before the click as before=N, else its GetTickCount64, matched to the native_frame_times rows' tick_ms).
$inWorld=@();$cuts=[Collections.Generic.List[long]]::new()
$tickOf=@{};foreach($t in $times){ $tickOf[$t.frame]=$t.tick }
if(Test-Path -LiteralPath "$dir/tour.txt"){ foreach($l in Get-Content -LiteralPath "$dir/tour.txt"){
    if($l -match '^zone (.+) in world visit=\d+ frame=(\d+) '){ $inWorld+=[pscustomobject]@{zone=$Matches[1];frame=[long]$Matches[2]} }
    elseif($l -match '^zone .+ loading .*tick=(\d+)$'){ $tick=[uint64]$Matches[1]
        if($l -match ' before=(\d+) '){ $cuts.Add([long]$Matches[1]) }
        else{$at=@($pacing | Where-Object { $tickOf.ContainsKey($_.frame) -and $tickOf[$_.frame] -le $tick } | Select-Object -Last 1)
            if($at.Count){ $cuts.Add($at[0].frame) } } } } }
# Stays: runs of consecutive pacing rows of one kind (world: 200 draws or more; title: 0; between: fades at a few draws). A title
# stay ends at the next load's click: the loading screen draws nothing either, and its rows are the next zone's load.
$stays=[Collections.Generic.List[object]]::new();$cur=$null
foreach($p in $pacing){ $kind=if($p.draws -ge 200){'world'}elseif($p.draws -eq 0){'title'}else{'other'}
    if($kind -eq 'title' -and $cur -and $cur.kind -eq 'title' -and @($cuts | Where-Object { $_ -ge $cur.rows[0].frame -and $_ -lt $p.frame }).Count){ $kind='loading' }
    if($kind -eq 'title' -and $cur -and $cur.kind -eq 'loading'){ $kind='loading' }
    # A row of a few draws inside a world stay (a fade, a view without the zone) does not split it (i85-long-2: 23 fragments).
    if($kind -eq 'other' -and $cur -and $cur.kind -eq 'world'){ $kind='world' }
    if(-not $cur -or $cur.kind -ne $kind){ $cur=[pscustomobject]@{kind=$kind;rows=[Collections.Generic.List[object]]::new()};$stays.Add($cur) }
    $cur.rows.Add($p) }
function Median($v){ $s=@($v | Sort-Object);if(-not $s.Count){ return $null };$s[[int][Math]::Floor(($s.Count-1)/2)] }
function Slope($ys){ $n=$ys.Count;if($n -lt 3){ return $null };$mx=($n-1)/2.0;$my=($ys | Measure-Object -Average).Average;$sxy=0.0;$sxx=0.0
    for($i=0;$i -lt $n;$i++){ $sxy+=($i-$mx)*($ys[$i]-$my);$sxx+=($i-$mx)*($i-$mx) };[Math]::Round($sxy/$sxx,3) }
$rows=[Collections.Generic.List[object]]::new();$world=0
foreach($s in $stays){ if($s.kind -notin 'world','title'){ continue }
    $first=$s.rows[0].frame-120;$last=$s.rows[$s.rows.Count-1].frame
    # A world stay is named by the tour's "in world" frame inside it; a fragment without one (a split by a fade) stays '?'.
    $zone='title';if($s.kind -eq 'world'){ $world++;$hit=@($inWorld | Where-Object { $_.frame -ge $s.rows[0].frame -and $_.frame -le $last });$zone=if($hit.Count){ $hit[0].zone }else{ '?' } }
    # The settle time left out, by the rows' own wall time (120 frames at the row's rate): -SettleSeconds in a world stay (the
    # load's stutter), 10 s in a title stay (the quit's unloading).
    $settle=if($s.kind -eq 'world'){ $SettleSeconds }else{ 10 };$skip=0;$acc=0.0
    while($skip -lt $s.rows.Count -and $acc -lt $settle){ $acc+=120.0/[Math]::Max(1.0,[double]$s.rows[$skip].fps);$skip++ }
    $kept=@($s.rows | Select-Object -Skip $skip);if(-not $kept.Count){ continue }
    # The private memory over one fixed window of every stay, 10 to 20 s from its start (the gate's memory leg: each title stay
    # measured alike, whatever its length, and never the close's release at the end of the last one).
    $at=0.0;$window=@(foreach($r in $s.rows){ $at+=120.0/[Math]::Max(1.0,[double]$r.fps);if($at -gt 10 -and $at -le 22){ $r.private } })
    $from=$kept[0].frame-120
    $st=@($streamed | Where-Object { $_.frame -gt $from -and $_.frame -le $last })
    $ft=@($times | Where-Object { $_.frame -gt $from -and $_.frame -le $last -and $_.content })
    $rg=@($registry | Where-Object { $_.frame -gt $from -and $_.frame -le $last })
    $frames=($ft | Measure-Object n -Sum).Sum
    $rows.Add([pscustomobject][ordered]@{stay=$rows.Count+1;kind=$s.kind;zone=$zone;visit=$(if($s.kind -eq 'world'){$world}else{$null});from_frame=$first;to_frame=$last;seconds=[int](($s.rows | ForEach-Object { 120.0/[Math]::Max(1.0,[double]$_.fps) } | Measure-Object -Sum).Sum)
        private_med=(Median $kept.private);private_window=(Median $window);private_max=($kept.private | Measure-Object -Maximum).Maximum;local_med=(Median $kept.local);local_max=($kept.local | Measure-Object -Maximum).Maximum
        nonlocal_med=(Median $kept.nonlocal);budget=(Median $kept.budget);descriptors_last=$kept[-1].descriptors;retirements_max=($kept.retirements | Measure-Object -Maximum).Maximum;pso_last=$kept[-1].pso
        heaps_max=$(if($st.Count){($st.heaps | Measure-Object -Maximum).Maximum}else{$null});placed_mb_max=$(if($st.Count){($st.placed_mb | Measure-Object -Maximum).Maximum}else{$null});staging_mb_max=$(if($st.Count){($st.staging_mb | Measure-Object -Maximum).Maximum}else{$null})
        frames=$frames;over25=($ft | Measure-Object o25 -Sum).Sum;over34=($ft | Measure-Object o34 -Sum).Sum;p99_med=(Median $ft.p99);max_ms=$(if($ft.Count){($ft.max | Measure-Object -Maximum).Maximum}else{$null})
        fps_med=(Median $kept.fps);layouts_last=$(if($rg.Count){$rg[-1].layouts}else{$null});materials_last=$(if($rg.Count){$rg[-1].materials}else{$null})
        texture_records_last=$(if($rg.Count){$rg[-1].textures}else{$null});render_targets_last=$(if($rg.Count){$rg[-1].targets}else{$null})}) }
$rows | Export-Csv -LiteralPath "$dir/visits.csv" -NoTypeInformation -Encoding utf8
# Title levels after the first world visit (the memory each visit leaves behind), and per zone the world levels in visit order.
$titles=@($rows | Where-Object { $_.kind -eq 'title' -and $_.from_frame -gt (@($rows | Where-Object kind -eq 'world')[0].from_frame) })
$trend=[ordered]@{title_stays=$titles.Count;private_mb_per_visit=(Slope @($titles.private_med));local_mb_per_visit=(Slope @($titles.local_med));nonlocal_mb_per_visit=(Slope @($titles.nonlocal_med))
    descriptors_per_visit=(Slope @($titles.descriptors_last));heaps_per_visit=(Slope @($titles | Where-Object { $null -ne $_.heaps_max } | ForEach-Object heaps_max))
    title_private_first_last=@($titles[0].private_med,$titles[-1].private_med);title_local_first_last=@($titles[0].local_med,$titles[-1].local_med)
    title_descriptors_first_last=@($titles[0].descriptors_last,$titles[-1].descriptors_last);pso_first_last=@($rows[0].pso_last,$rows[-1].pso_last)}
$perZone=[ordered]@{};foreach($g in @($rows | Where-Object kind -eq 'world' | Group-Object zone)){ $v=@($g.Group)
    $rate=@($v | ForEach-Object { if($_.frames){ [Math]::Round(1000.0*$_.over25/$_.frames,2) }else{ 0 } })
    $perZone[$g.Name]=[ordered]@{visits=$v.Count;local_med=@($v.local_med);local_mb_per_visit=(Slope @($v.local_med));private_mb_per_visit=(Slope @($v.private_med))
        over25_per_1000=$rate;over25_rate_per_visit=(Slope $rate);over34=@($v.over34);p99=@($v.p99_med);fps=@($v.fps_med)} }
$last=$pacing[$pacing.Count-1]
$wall=($pacing | ForEach-Object { 120.0/[Math]::Max(1.0,[double]$_.fps) } | Measure-Object -Sum).Sum
# The controller's crash record (final review M1 of I-0099 step 4): a fault at the quit is dumped by the controller, but the game's
# own native_crash_dump row is lost with its thread when the process ends; the controller's row (crash_kind) is what remains.
$controllerCrash='';if(Test-Path -LiteralPath "$dir/controller.jsonl"){foreach($row in Get-Content -LiteralPath "$dir/controller.jsonl"){
    if($row -match '"crash_kind":"([^"]*)","crash_code":"([^"]*)"' -and $Matches[1] -ne 'none'){$controllerCrash="$($Matches[1]) $($Matches[2])"}}}
$summary=[ordered]@{run=$Run;controller_crash=$controllerCrash;pacing_rows=$pacing.Count;frames=$last.frame;minutes=[Math]::Round($wall/60.0,1);world_stays=$world;tour_loads=$inWorld.Count
    unnamed_world_stays=@($rows | Where-Object zone -eq '?').Count;faults=@($faults);crash_dumps=@($dumps);counts=$counts;peak_local_mb=($pacing.local | Measure-Object -Maximum).Maximum;peak_private_mb=($pacing.private | Measure-Object -Maximum).Maximum
    budget_mb=(Median $pacing.budget);trend=$trend;zones=$perZone}
$summary | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$dir/summary.json" -Encoding utf8
$summary | ConvertTo-Json -Depth 5
