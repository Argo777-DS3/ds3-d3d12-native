param([Parameter(Mandatory)][string[]]$Runs,[string]$Out='')
# I-0100 LOD scale study (question 1): reads Run-I100LodScaleSeries.ps1's runs (evidence/I-0066/artifacts/<run>/startup.jsonl,
# evidence/I-0100/lodscale/<run>-marks.txt and -presentmon.csv). Windows on the journal's clock (TickCount64 ms): « before » the
# 40 s ending at the death row's mark (the launch scale in both A and B), « still » from 4 s after the reload's mark to the
# sweep's start (the respawn's view; its first 4-6 s are still the load's: final review M1), « sweep » the camera turns. Per
# window: the native_present_pacing rows (every 2 s, placed on the clock by native_frame_times' frame and tick): content draws,
# collect, prepare and record ms, the GPU wait, private and local video memory; native_frame_times' frames in the module's buckets
# over_17_25_34_ms; native_current_view_selected objects; native_model_stream_initialized rows (two per model); PresentMon's GPU busy and frame interval (its QPC put on the tick clock by
# -presentmon-clock.txt). Also the run's checks (measure mode, debug layer, the A rows, faults).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Median([double[]]$v){ if(-not $v.Count){ return $null };$s=@($v | Sort-Object);$n=$s.Count;if($n%2){ $s[($n-1)/2] }else{ ($s[$n/2-1]+$s[$n/2])/2 } }
function Mean([double[]]$v){ if(-not $v.Count){ return $null };($v | Measure-Object -Average).Average }
$results=@()
foreach($run in $Runs){
    $dir="$repo/evidence/I-0066/artifacts/$run";$lod="$repo/evidence/I-0100/lodscale"
    $base=Get-Content -LiteralPath "$dir/baseline.json" -Raw | ConvertFrom-Json
    $marks=@{};if(Test-Path -LiteralPath "$lod/$run-marks.txt"){ foreach($l in Get-Content -LiteralPath "$lod/$run-marks.txt"){ $f=$l -split ' ';$marks[$f[0]]=[double]$f[1] } }
    $times=[System.Collections.Generic.List[object]]::new();$pacing=[System.Collections.Generic.List[object]]::new();$views=[System.Collections.Generic.List[object]]::new()
    $streams=[System.Collections.Generic.List[int]]::new();$kept=0;$faults=0;$deathLine=-1;$n=0
    foreach($line in [IO.File]::ReadLines("$dir/startup.jsonl")){ $n++
        if($line.StartsWith('{"event":"native_frame_times"')){ $r=$line | ConvertFrom-Json;$times.Add([pscustomobject]@{frame=[double]$r.frame;tick=[double]$r.tick_ms;over=$r.over_17_25_34_ms;line=$n}) }
        elseif($line.StartsWith('{"event":"native_present_pacing"')){ $r=$line | ConvertFrom-Json;$r | Add-Member line $n;$pacing.Add($r) }
        elseif($line.StartsWith('{"event":"native_current_view_selected"')){ $r=$line | ConvertFrom-Json;$views.Add([pscustomobject]@{objects=[int]$r.objects;intents=[int]$r.model_intents;line=$n}) }
        elseif($line.StartsWith('{"event":"native_model_stream_initialized"')){ $streams.Add($n) }
        elseif($line.Contains('"event":"native_lod_launch_scale_kept"') -or $line.Contains('"event":"native_lod_reference_kept"')){ $kept++ }   # A's row before and since build t5c
        elseif($line.Contains('"event":"test_death"')){ $deathLine=$n }
        elseif($line.Contains('"event":"native_failure"') -or $line.Contains('"event":"native_device_removed"')){ $faults++ } }
    # Journal lines to the tick clock: the native_frame_times rows (line, tick); a row between two gets the later one's tick.
    $tickOf={ param([int]$ln) foreach($t in $times){ if($t.line -ge $ln){ return $t.tick } };$null }
    $deathTick=if($marks.ContainsKey('death_seen')){ $marks['death_seen'] }else{ & $tickOf $deathLine }
    $windows=[ordered]@{before=@(($deathTick-40000),$deathTick);still=@(($marks['reload_seen']+4000),$marks['sweep_start']);sweep=@($marks['sweep_start'],$marks['sweep_end'])}
    $pm=@();$pmFile="$lod/$run-presentmon.csv";$clock=if(Test-Path -LiteralPath "$lod/$run-presentmon-clock.txt"){ (Get-Content -LiteralPath "$lod/$run-presentmon-clock.txt") -split ' ' }else{ $null }
    if((Test-Path -LiteralPath $pmFile) -and $clock){ $t0=[double]$clock[0];$q0=[double]$clock[1];$freq=[double]$clock[2]
        $pm=@(Import-Csv -LiteralPath $pmFile | Where-Object { $_.Application -eq 'DarkSoulsIII.exe' } | ForEach-Object {
            $q=if($_.PSObject.Properties['CPUStartQPC']){ [double]$_.CPUStartQPC }elseif($_.PSObject.Properties['TimeInQPC']){ [double]$_.TimeInQPC }else{ $null }
            if($null -ne $q){ [pscustomobject]@{tick=$t0+($q-$q0)*1000.0/$freq;busy=$(if($_.MsGPUBusy -match '^[0-9.]+$'){ [double]$_.MsGPUBusy }else{ $null });interval=$(if($_.MsBetweenPresents -match '^[0-9.]+$'){ [double]$_.MsBetweenPresents }else{ $null })} } }) }
    # i100-ls-b5 on: Windows' « GPU Engine » 3D utilisation of the game's process once a second (tick, percent, a French decimal comma).
    $gpu=@();if(Test-Path -LiteralPath "$lod/$run-gpu.txt"){ $gpu=@(Get-Content -LiteralPath "$lod/$run-gpu.txt" | ForEach-Object { $f=$_ -split ' ';if($f.Count -eq 2 -and $f[1]){ [pscustomobject]@{tick=[double]$f[0];pct=[double]($f[1] -replace ',','.')} } }) }
    $row=[ordered]@{run=$run;measure=$base.measure;debug_layer_disabled=$base.debug_layer_disabled;flags=$base.effect_flags;module=$base.module_sha256;kept_rows=$kept;faults=$faults;marks=$marks;windows=[ordered]@{}}
    foreach($w in $windows.Keys){ $a=[double]$windows[$w][0];$b=[double]$windows[$w][1];if(-not ($b -gt $a)){ continue }
        $inT=@($times | Where-Object { $_.tick -gt $a -and $_.tick -le $b })
        $lines=@($inT | ForEach-Object line);$lo=if($lines.Count){ ($lines | Measure-Object -Minimum).Minimum }else{ 0 };$hi=if($lines.Count){ ($lines | Measure-Object -Maximum).Maximum }else{ 0 }
        $p=@($pacing | Where-Object { $_.line -ge $lo -and $_.line -le $hi });$v=@($views | Where-Object { $_.line -ge $lo -and $_.line -le $hi })
        $over=@(0,0,0);foreach($t in $inT){ for($k=0;$k -lt 3;$k++){ $over[$k]+=[int]$t.over[$k] } }
        $pw=@($pm | Where-Object { $_.tick -gt $a -and $_.tick -le $b })
        $row.windows[$w]=[ordered]@{seconds=[Math]::Round(($b-$a)/1000,1);pacing_rows=$p.Count;
            content_draws=[Math]::Round((Mean @($p | ForEach-Object { [double]$_.content_draws })),1);
            collect_ms=[Math]::Round((Mean @($p | ForEach-Object { [double]$_.collect_ms })),3);prepare_ms=[Math]::Round((Mean @($p | ForEach-Object { [double]$_.prepare_ms })),3);
            record_ms=[Math]::Round((Mean @($p | ForEach-Object { [double]$_.record_ms })),3);gpu_wait_ms=[Math]::Round((Mean @($p | ForEach-Object { [double]$_.gpu_wait_ms })),3);
            private_mb=$(if($p.Count){ [int]$p[-1].memory_private_local_mb[0] }else{ $null });local_mb=$(if($p.Count){ [int]$p[-1].memory_private_local_mb[1] }else{ $null });
            frames_over_17_25_34=$over;view_objects=[Math]::Round((Mean @($v | ForEach-Object { [double]$_.objects })),1);view_intents=[Math]::Round((Mean @($v | ForEach-Object { [double]$_.intents })),1);
            stream_rows=@($streams | Where-Object { $_ -ge $lo -and $_ -le $hi }).Count;
            gpu_busy_ms_median_p95=@($(Median @($pw | Where-Object { $null -ne $_.busy } | ForEach-Object busy)),$(if($pw.Count){ $s=@($pw | Where-Object { $null -ne $_.busy } | ForEach-Object busy | Sort-Object);if($s.Count){ $s[[int][Math]::Floor(0.95*($s.Count-1))] }else{ $null } }else{ $null }));
            present_interval_ms_median=(Median @($pw | Where-Object { $null -ne $_.interval } | ForEach-Object interval));presents=$pw.Count;
            gpu_3d_pct_mean=$(if(($g=@($gpu | Where-Object { $_.tick -gt $a -and $_.tick -le $b })).Count){ [Math]::Round((Mean @($g | ForEach-Object pct)),1) }else{ $null })} }
    $results+=$row
    Write-Host ("== {0}: measure {1}, debug layer off {2}, flags {3}, A rows {4}, faults {5}" -f $run,$row.measure,$row.debug_layer_disabled,$row.flags,$kept,$faults)
    foreach($w in $row.windows.Keys){ $x=$row.windows[$w]
        Write-Host ("   {0,-6} {1,5}s draws {2,6} objects {3,6} collect {4} prepare {5} record {6} gpu_wait {7} | over17/25/34 {8} | stream rows {9} | private {10} local {11} | GPU 3D {14} % | GPU busy med/p95 {12} | interval {13}" -f $w,$x.seconds,$x.content_draws,$x.view_objects,$x.collect_ms,$x.prepare_ms,$x.record_ms,$x.gpu_wait_ms,($x.frames_over_17_25_34 -join '/'),$x.stream_rows,$x.private_mb,$x.local_mb,(($x.gpu_busy_ms_median_p95 | ForEach-Object { if($null -ne $_){ '{0:N2}' -f $_ }else{ '-' } }) -join '/'),$(if($null -ne $x.present_interval_ms_median){ '{0:N2}' -f $x.present_interval_ms_median }else{ '-' }),$(if($null -ne $x.gpu_3d_pct_mean){ $x.gpu_3d_pct_mean }else{ '-' })) } }
if($Out){ $results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $Out -Encoding utf8 }
