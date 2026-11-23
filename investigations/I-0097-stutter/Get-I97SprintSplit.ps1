param([Parameter(Mandatory)][string[]]$Run,[string]$PathFile='',[ValidateRange(100,5000)][int]$BinMs=500)
# I-0097 tranche 5 study (the sprint's extra slow frames): reads replayed runs' journals only.
# 1. Each frame over 25 ms at the frame end placed in its cycle (ms since the cycle's start, bins of -BinMs), per segment, and
#    whether it follows another frame over 25 ms within 100 ms (a pair: one wait spread over two frames).
# 2. Native runs only: the 120-frame windows' collect and frame figures (native_present_pacing, native_collect_split,
#    native_program_analysis, matched to native_frame_times by their frame number), averaged per segment and split by
#    windows with and without a frame over 25 ms.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(-not $PathFile){ $PathFile="$repo/scripts/replay/firelink.path.json" }
$recording=Get-Content -LiteralPath $PathFile -Raw | ConvertFrom-Json
$Run=@($Run | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$inv=[Globalization.CultureInfo]::InvariantCulture
$bins=@{};$pairs=@{};$slow=@{};$win=@{}
function Acc($key,[double[]]$v){ if(-not $win.Contains($key)){ $win[$key]=[double[]]::new($v.Count+1) };$a=$win[$key];for($i=0;$i -lt $v.Count;$i++){ $a[$i]+=$v[$i] };$a[$v.Count]++ }
foreach($n in $Run){
    $d=if(Test-Path -LiteralPath $n){ $n }else{ "$repo/evidence/I-0066/artifacts/$n" };$marks=@{}
    foreach($l in Get-Content -LiteralPath "$d/motion-marks.txt"){ $f=$l -split ' ';$marks[$f[0]]=[int64]$f[1] }
    $at=[double]$marks['motion_start'];$cycles=@()
    foreach($s in $recording.segments){ for($k=0;$k -lt [int]$s.cycles;$k++){ $cycles+=,@($s.name,($at+$k*[double]$s.cycle_ms),($at+($k+1)*[double]$s.cycle_ms)) };$at+=[int]$s.cycles*[double]$s.cycle_ms }
    function CycleOf([double]$t){ foreach($c in $cycles){ if($t -ge $c[1] -and $t -lt $c[2]){ return $c } };$null }
    $frames=[Collections.Generic.List[double[]]]::new();$tickOf=@{};$pacing=@{};$split=@{};$analysis=@{}
    foreach($line in [IO.File]::ReadLines("$d/startup.jsonl")){
        if($line -match '"event":"(native_frame_times|native_original_frames)"'){
            $r=$line | ConvertFrom-Json;if($null -eq $r.tick_ms){continue};$tickOf[[int64]$r.frame]=[double]$r.tick_ms;$end=[double]$r.tick_ms
            for($i=$r.ms.Count-1;$i -ge 0;$i--){ $frames.Add(@($end,[double]$r.ms[$i]));$end-=[double]$r.ms[$i] } }
        elseif($line -match '"event":"native_present_pacing"'){ $r=$line | ConvertFrom-Json;$pacing[[int64]$r.frame]=$r }
        elseif($line -match '"event":"native_collect_split"'){ $r=$line | ConvertFrom-Json;$split[[int64]$r.frame]=$r }
        elseif($line -match '"event":"native_program_analysis"'){ $r=$line | ConvertFrom-Json;$analysis[[int64]$r.frame]=$r } }
    $frames.Sort([Comparison[double[]]]{ param($x,$y) $x[0].CompareTo($y[0]) })
    $previousSlow=-1e9
    foreach($x in $frames){ if($x[1] -le 25){ continue };$c=CycleOf $x[0];if($c){ $seg=$c[0]
            $b=[math]::Floor(($x[0]-$c[1])/$BinMs);$key="$seg|$b";$bins[$key]=1+$bins[$key];$slow[$seg]=1+$slow[$seg]
            if($x[0]-$x[1]-$previousSlow -lt 100){ $pairs[$seg]=1+$pairs[$seg] } }
        $previousSlow=$x[0] }
    $previousAnalysis=$null
    foreach($frame in ($tickOf.Keys | Sort-Object)){
        $t=$tickOf[$frame];$a=$analysis[$frame];$p=$pacing[$frame];$s=$split[$frame]
        $admit=if($a -and $previousAnalysis){ [double]$a.texture_admit_ms-[double]$previousAnalysis.texture_admit_ms }else{ 0 };if($a){ $previousAnalysis=$a }
        if(-not $p -or -not $s){ continue };$c=CycleOf $t;if(-not $c){ continue }
        $over=@($frames | Where-Object { $_[0] -gt $t-2000 -and $_[0] -le $t -and $_[1] -gt 25 }).Count
        $v=@([double]$p.collect_ms,[double]$p.prepare_ms,[double]$p.render_ms,[double]$s.stamp_ms[2],[double]$s.counts[2],[double]$s.counts[3],[double]$s.counts[12],[double]$s.counts[13],[double]$s.counts[7],[double]$s.reuse[2],[double]$s.self_ms[5],$admit,[double]$p.slowest_ms,[double]$p.content_draws)
        Acc "$($c[0])|all" $v;Acc "$($c[0])|$(if($over){'slow'}else{'none'})" $v } }
$k=$Run.Count
"runs=${k}: frames over 25 ms per segment (a run), of which within 100 ms of the previous one"
foreach($seg in $slow.Keys){ [string]::Format($inv,'  {0,-7} {1,6:F1}  pairs {2,5:F1}',$seg,$slow[$seg]/$k,[double]$pairs[$seg]/$k) }
"place in the cycle (ms from its start, bins of $BinMs), frames over 25 ms a run:"
foreach($seg in @($recording.segments.name)){ $row=foreach($b in 0..([int](([double]($recording.segments | Where-Object name -eq $seg).cycle_ms)/$BinMs)-1)){ [string]::Format($inv,'{0:F1}',[double]$bins["$seg|$b"]/$k) };"  {0,-7} {1}" -f $seg,($row -join ' ') }
if($win.Count){
    "windows (means): collect_ms prepare_ms render_ms update_ms admitted_models programs_built records_built records_invalid texture_lookups models_new mesh_programs_ms texture_admit_ms slowest_ms draws (windows)"
    foreach($e in ($win.GetEnumerator() | Sort-Object Name)){ $a=$e.Value;$m=$a[$a.Count-1];$cells=for($i=0;$i -lt $a.Count-1;$i++){ [string]::Format($inv,'{0:F2}',$a[$i]/$m) };"  {0,-13} {1} ({2})" -f $e.Name,($cells -join ' '),$m } }
