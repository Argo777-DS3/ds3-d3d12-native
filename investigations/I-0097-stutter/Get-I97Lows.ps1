param([Parameter(Mandatory)][string[]]$Run,[int]$TrimSeconds=2)
# I-0097 (I-0095 §8): the lows of replayed-path runs made with frame-dump.flag, from every frame's time. For each run and each
# timing point (frame_end: native_frame_times or the original's native_original_frames; present: native_present_times, native
# only; game_step: game_frame_times), the frames of the windows whose end falls inside a segment of the path (motion-marks.txt,
# the first -TrimSeconds of each left out, as Get-ReplayMeasure.ps1), then the 1 % and 0.1 % lows (the 99th and 99.9th
# percentile frame time, in ms and frames/s), the worst frame and the counts over 25 and 34 ms; with the display rows' repeats
# (display_refreshes) over the same segments. One line per run and point, then the pooled lows per renderer (baseline.json mode).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Pct([double[]]$sorted,[double]$q){ if(-not $sorted.Count){ return $null };$sorted[[Math]::Min($sorted.Count-1,[int][Math]::Floor($q*$sorted.Count))] }
$pool=@{}
$lines=foreach($name in $Run){
    $dir=if(Test-Path -LiteralPath $name){ (Resolve-Path -LiteralPath $name).Path }else{ "$repo/evidence/I-0066/artifacts/$name" }
    $marks=[ordered]@{};foreach($line in Get-Content -LiteralPath "$dir/motion-marks.txt"){ $f=$line -split ' ';$marks[$f[0]]=[uint64]$f[1] }
    $segments=@($marks.Keys | Where-Object { $_ -match '^segment_(.+)_start$' } | ForEach-Object { $n=$Matches[1];[pscustomobject]@{from=$marks[$_]+[uint64](1000*$TrimSeconds);to=$marks["segment_${n}_end"]} } | Where-Object { $_.to })
    $rows=@(Get-Content -LiteralPath "$dir/startup.jsonl" | Where-Object { $_ -match '"event":"(native_frame_times|native_original_frames|native_present_times|game_frame_times|display_refreshes)"' } | ForEach-Object { $_ | ConvertFrom-Json } |
        Where-Object { $r=$_;$r.tick_ms -and @($segments | Where-Object { [uint64]$r.tick_ms -ge $_.from -and [uint64]$r.tick_ms -le $_.to }).Count })
    $mode=(Get-Content -LiteralPath "$dir/baseline.json" -Raw | ConvertFrom-Json).bootstrap_mode
    $display=@($rows | Where-Object event -eq 'display_refreshes');$repeats=if($display.Count){ ($display | Measure-Object repeated -Sum).Sum }else{ $null }
    foreach($point in @(@('frame_end','native_frame_times','native_original_frames'),@('present','native_present_times'),@('game_step','game_frame_times'))){
        $ms=[Collections.Generic.List[double]]::new();foreach($r in @($rows | Where-Object { $_.event -in $point[1..2] -and $_.ms })){ foreach($v in $r.ms){ $ms.Add([double]$v) } }
        if(-not $ms.Count){ continue };$sorted=[double[]]($ms | Sort-Object);$key="$mode $($point[0])";if(-not $pool[$key]){ $pool[$key]=[Collections.Generic.List[double]]::new() };$pool[$key].AddRange($sorted)
        $p99=Pct $sorted 0.99;$p999=Pct $sorted 0.999
        [pscustomobject][ordered]@{run=Split-Path -Leaf $dir;mode=$mode;point=$point[0];frames=$sorted.Count;low1_ms=[Math]::Round($p99,2);low1_fps=[Math]::Round(1000/$p99,1)
            low01_ms=[Math]::Round($p999,2);low01_fps=[Math]::Round(1000/$p999,1);worst_ms=[Math]::Round($sorted[-1],2)
            over25=@($sorted | Where-Object { $_ -gt 25 }).Count;over34=@($sorted | Where-Object { $_ -gt 34 }).Count;display_repeats=$repeats} } }
$lines
if($Run.Count -gt 1){ foreach($k in $pool.Keys){ $s=[double[]]($pool[$k] | Sort-Object);$p99=Pct $s 0.99;$p999=Pct $s 0.999
    $kk=$k -split ' ';[pscustomobject][ordered]@{run='pooled';mode=$kk[0];point=$kk[1];frames=$s.Count;low1_ms=[Math]::Round($p99,2);low1_fps=[Math]::Round(1000/$p99,1);low01_ms=[Math]::Round($p999,2);low01_fps=[Math]::Round(1000/$p999,1);worst_ms=[Math]::Round($s[-1],2)
        over25=@($s | Where-Object { $_ -gt 25 }).Count;over34=@($s | Where-Object { $_ -gt 34 }).Count;display_repeats=$null} } }
