param([Parameter(Mandatory)][string[]]$Run,[string]$PathFile='')
# I-0097 tranche 5 study: on the Firelink path DS3 reloads the same map parts once a 10 s cycle, and both renderers stall twice
# at each reload, about 6.5 frames apart. Per cycle (means over the runs): the first stall (the longest frame between 5 and 9 s
# into the cycle) and the second (the longest frame 3 to 10 frames after it), at the frame end; and, for native runs, the
# GPARAM bank publications of the cycle from native_program_analysis's cumulative counters (count, ms, ms a publication).
# Reads the journal only.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(-not $PathFile){ $PathFile="$repo/scripts/replay/firelink.path.json" }
$recording=Get-Content -LiteralPath $PathFile -Raw | ConvertFrom-Json
$Run=@($Run | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$inv=[Globalization.CultureInfo]::InvariantCulture;$table=[ordered]@{}
foreach($n in $Run){
    $d=if(Test-Path -LiteralPath $n){ $n }else{ "$repo/evidence/I-0066/artifacts/$n" };$marks=@{}
    foreach($l in Get-Content -LiteralPath "$d/motion-marks.txt"){ $f=$l -split ' ';$marks[$f[0]]=[int64]$f[1] }
    $at=[double]$marks['motion_start'];$cycles=@();$i=0
    foreach($s in $recording.segments){ for($k=0;$k -lt [int]$s.cycles;$k++){ $i++;if($s.name -notin 'turn','sweep'){ $cycles+=[pscustomobject]@{name=('{0:D2} {1}' -f $i,$s.name);from=$at+$k*[double]$s.cycle_ms;to=$at+($k+1)*[double]$s.cycle_ms} } };$at+=[int]$s.cycles*[double]$s.cycle_ms }
    $fe=[Collections.Generic.List[double[]]]::new();$tick=@{};$analysis=[Collections.Generic.List[object]]::new()
    foreach($line in [IO.File]::ReadLines("$d/startup.jsonl")){
        if($line -match '"event":"(native_frame_times|native_original_frames)"'){
            $r=$line | ConvertFrom-Json;if($null -eq $r.tick_ms){continue};$end=[double]$r.tick_ms;if($r.frame){ $tick[[int64]$r.frame]=$end }
            for($j=$r.ms.Count-1;$j -ge 0;$j--){ $fe.Add(@($end,[double]$r.ms[$j]));$end-=[double]$r.ms[$j] } }
        elseif($line -match '"event":"native_program_analysis"'){ $analysis.Add(($line | ConvertFrom-Json)) } }
    $fe.Sort([Comparison[double[]]]{ param($x,$y) $x[0].CompareTo($y[0]) })
    foreach($c in $cycles){
        if(-not $table.Contains($c.name)){ $table[$c.name]=[pscustomobject]@{first=[Collections.Generic.List[double]]::new();second=[Collections.Generic.List[double]]::new();pubs=0.0;ms=0.0;runs=0} }
        $row=$table[$c.name];$first=-1
        for($j=0;$j -lt $fe.Count;$j++){ if($fe[$j][0] -ge $c.from+5000 -and $fe[$j][0] -lt $c.from+9000 -and ($first -lt 0 -or $fe[$j][1] -gt $fe[$first][1])){ $first=$j } }
        if($first -ge 0){ $second=[math]::Min($first+3,$fe.Count-1);for($j=$first+3;$j -le [math]::Min($first+10,$fe.Count-1);$j++){ if($fe[$j][1] -gt $fe[$second][1]){ $second=$j } }
            $row.first.Add($fe[$first][1]);$row.second.Add($fe[$second][1]) }
        $previous=$null
        foreach($a in $analysis){ $t=$tick[[int64]$a.frame];if($t -and $previous -and $t -gt $c.from -and $t -le $c.to){ $row.pubs+=[double]$a.gparam_publishes-[double]$previous.gparam_publishes;$row.ms+=[double]$a.gparam_publish_ms-[double]$previous.gparam_publish_ms }
            if($t){ $previous=$a } }
        $row.runs++ } }
"runs=$($Run.Count): cycle, first stall ms (mean), second stall ms (mean, min-max), GPARAM publications, their ms, ms a publication"
foreach($e in $table.GetEnumerator()){ $r=$e.Value;if(-not $r.second.Count){ continue }
    $s=$r.second | Measure-Object -Average -Minimum -Maximum
    [string]::Format($inv,'{0,-10} {1,6:F1} {2,6:F1} ({3:F1}-{4:F1}) {5,6:F1} {6,7:F2} {7,7:F3}',$e.Key,($r.first | Measure-Object -Average).Average,$s.Average,$s.Minimum,$s.Maximum,$r.pubs/$r.runs,$r.ms/$r.runs,$(if($r.pubs){$r.ms/$r.pubs}else{0})) }
