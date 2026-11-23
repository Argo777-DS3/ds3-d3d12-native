param([Parameter(Mandatory)][string]$Run,[Parameter(Mandatory)][string]$BuildFolder,[double]$SlowMs=25,[double]$FastMs=17.5,[int]$TrimSeconds=2,[string]$SlowName="slow",[switch]$Original)
# I-0097 (I-0095 §8 item 5): where the engine thread's time goes in the frames of the 25-34 ms band, against the ordinary
# frames, on a replayed-path run made with frame-dump.flag and the sampler (sample-profile-start.txt, Run-I97Profile.ps1).
# Each native_frame_times row inside a segment of the path carries its 120 frame times in order; the row's frame number is the
# presented count at its end, so its j-th time (0-based) is the interval that ended at presented frame - 119 + j, whose samples
# carry the mark presented - 1 (Get-SampleProfile.ps1). Writes frames-slow.txt (intervals over -SlowMs) and frames-fast.txt
# (under -FastMs) beside the run, then sample-profile-slow-report.txt and sample-profile-fast-report.txt (sample_symbolize from
# the build the run loaded, frames:FILE). Tranche 4: -Original reads an original run (Run-I97Profile.ps1 -Original): its rows are
# native_original_frames, whose call is the pacer calls finished before the window's last interval began, so its j-th time ended at
# call - 119 + j and its samples carry that mark.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$dir="$repo/evidence/I-0066/artifacts/$Run"
$marks=[ordered]@{};foreach($line in Get-Content -LiteralPath "$dir/motion-marks.txt"){ $f=$line -split ' ';$marks[$f[0]]=[uint64]$f[1] }
$segments=@($marks.Keys | Where-Object { $_ -match '^segment_(.+)_start$' } | ForEach-Object { $n=$Matches[1];[pscustomobject]@{from=$marks[$_]+[uint64](1000*$TrimSeconds);to=$marks["segment_${n}_end"]} } | Where-Object { $_.to })
$slow=[Collections.Generic.List[int64]]::new();$fast=[Collections.Generic.List[int64]]::new()
foreach($r in @(Get-Content -LiteralPath "$dir/startup.jsonl" | Where-Object { $_ -match $(if($Original){'"event":"native_original_frames".*"ms":\['}else{'"event":"native_frame_times".*"ms":\['}) } | ForEach-Object { $_ | ConvertFrom-Json })){
    if(-not @($segments | Where-Object { [uint64]$r.tick_ms -ge $_.from -and [uint64]$r.tick_ms -le $_.to }).Count){ continue }
    $n=@($r.ms).Count;for($j=0;$j -lt $n;$j++){ $mark=if($Original){[int64]$r.call-($n-1)+$j}else{[int64]$r.frame-($n-1)+$j-1};$v=[double]$r.ms[$j]
        if($v -gt $SlowMs){ $slow.Add($mark) }elseif($v -lt $FastMs){ $fast.Add($mark) } } }
$slow | Set-Content -LiteralPath "$dir/frames-$SlowName.txt";$fast | Set-Content -LiteralPath "$dir/frames-fast.txt"
$tool="$BuildFolder/sample_symbolize.exe"
foreach($k in $SlowName,"fast"){ & $tool "$dir/sample-profile.txt" "$dir/sample-profile-$k-report.txt" game_bootstrap.dll "frames:$dir/frames-$k.txt"
    if($LASTEXITCODE){ throw "sample_symbolize ($k) failed: $LASTEXITCODE" } }
"$SlowName frames $($slow.Count), fast frames $($fast.Count): $dir/sample-profile-$SlowName-report.txt, $dir/sample-profile-fast-report.txt"
