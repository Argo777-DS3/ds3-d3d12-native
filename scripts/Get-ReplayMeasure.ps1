param([Parameter(Mandatory)][string]$Run,[int]$TrimSeconds=2)
# I-0083: one replayed-path run's numbers, per segment of the path (motion-marks.txt: segment_<name>_start/_end, written by
# Invoke-ReplayPath.ps1; a run with motion_start/motion_end only, as I-0079's, is one segment « motion »), the first
# -TrimSeconds of each segment left out. The same points as I-0079 and I-0081 (Get-Stutter.ps1, Get-StutterSummary.ps1):
# - frame_end: frames over 17/25/34 ms at the native frame end (native_frame_times) or the original's present pacer
#   (native_original_frames), windows of 120 frames whose end falls inside the segment;
# - game_step: the same counts at the world update gate (game_frame_times), the point both renderers share (the original's
#   since I-0083's probe hook);
# - display_repeats (I-0097): the refreshes on which the display repeated a frame (display_refreshes rows, both renderers: the
#   swap chain's frame statistics, refreshes scanned minus frames shown), and display_hitches, the statistics samples with a
#   repeat, windows whose end falls inside the segment; with display_presented and display_refreshes, the frames shown and the
#   refreshes scanned; display_windows, the windows found, and display_hz, the refresh rates they report (at 60 frames/s on a
#   120 Hz display every frame holds two refreshes);
# - held (runs before I-0097 only): frames the display held two refreshes or more (native_present_refreshes, native only),
#   bounded by the frame numbers of the segment's native_frame_times windows; biased low where stutter happens (I-0095 §4).
# Also the run's checks: its mode, measure mode and debug layer (baseline.json), the faults (crash reports beside the journal,
# native_failure, refusal and native_device_removed rows), the replay's status and lateness (replay.log), the restoration.
$ErrorActionPreference='Stop'
$dir=if(Test-Path -LiteralPath $Run){ (Resolve-Path -LiteralPath $Run).Path }else{ Join-Path (Split-Path -Parent $PSScriptRoot) "evidence/I-0066/artifacts/$Run" }
# Step review: a run that stopped before its motion (no marks: a fault while loading, the title not passed) still gives its
# checks and faults, with no segment.
$base=if(Test-Path -LiteralPath "$dir/baseline.json"){ Get-Content -LiteralPath "$dir/baseline.json" -Raw | ConvertFrom-Json }else{ [pscustomobject]@{} }
$marks=[ordered]@{};if(Test-Path -LiteralPath "$dir/motion-marks.txt"){ foreach($line in Get-Content -LiteralPath "$dir/motion-marks.txt"){ $f=$line -split ' ';$marks[$f[0]]=[uint64]$f[1] } }
$segments=@($marks.Keys | Where-Object { $_ -match '^segment_(.+)_start$' } | ForEach-Object { $name=$Matches[1]
    [pscustomobject]@{name=$name;from=$marks[$_];to=$marks["segment_${name}_end"]} } | Where-Object { $_.to })
if(-not $segments.Count -and $marks['motion_end']){ $segments=@([pscustomobject]@{name='motion';from=$marks['motion_start'];to=$marks['motion_end']}) }
$rows=@(Get-Content -LiteralPath "$dir/startup.jsonl" -ErrorAction SilentlyContinue | Where-Object { $_ -match '"event":"(native_frame_times|native_original_frames|game_frame_times|native_present_refreshes|display_refreshes|native_failure|refusal|native_device_removed)"' } |
    ForEach-Object { try{ $_ | ConvertFrom-Json }catch{ $null } } | Where-Object { $_ })
function Sum3($set){ $s=@(0,0,0);foreach($r in $set){ for($k=0;$k -lt 3;$k++){ $s[$k]+=[int]$r.over_17_25_34_ms[$k] } };,$s }
# I-0098 (Thomas, 2026-10-24: a gate independent of the screen): the run's frame period P, the median of the mean interval of
# the frame windows ending inside the segments (16.67 ms under DS3's 60 frames/s cap on any screen, the frame time itself once
# the rate is unlocked), and the game step's own. With frame-dump.flag (each window's frame times in order, "ms"): the frames
# over 1.5 P and 2.04 P (25 and 34 ms at 60 frames/s: the fixed counts' equivalents); and, from the display rows' holds (each
# statistics sample's frames and refreshes), the refreshes beyond the screen's cadence c (the refreshes a frame period spans,
# rounded up with a tenth of slack: 1 at 60 Hz, 5 at 279.96 Hz where 60 frames/s hold 4 or 5), in frame periods: at 60 Hz the
# display repeats. A single frame held outside the cadence's floor and c is counted apart (irregular), with the holds' histogram.
# Step review (known limit): a sample spanning several frames is allowed frames x c refreshes, so at 279.96 Hz a late frame
# (6 refreshes) read together with a quick one (4) is not counted; samples are one frame each almost always.
# Step review: with the window's frame times, its mean over the frames within 1.5 times of its median either way (a hitch and its
# catch-up left out, the 4/5 cadence of 60 frames/s at 279.96 Hz kept), so P stays the cadence's whatever the hitches; else the
# window's mean.
function WindowMean($r){ if($null -ne $r.ms){ $m=Median @($r.ms);if(-not $m){ return $null }
        $kept=@($r.ms | Where-Object { $_ -le 1.5*$m -and $_ -ge $m/1.5 });if($kept.Count){ return ($kept | Measure-Object -Average).Average } }
    if($null -ne $r.ms_mean_stdev){[double]$r.ms_mean_stdev[0]}elseif($null -ne $r.frame_ms_mean){[double]$r.frame_ms_mean}else{$null} }
function Median([double[]]$v){ if(-not $v.Count){ return $null };$s=@($v | Sort-Object);$n=$s.Count;if($n%2){ $s[($n-1)/2] }else{ ($s[$n/2-1]+$s[$n/2])/2 } }
function InSegments([string[]]$events){ @($rows | Where-Object { $_.event -in $events -and $_.tick_ms } | Where-Object { $t=[uint64]$_.tick_ms
    @($segments | Where-Object { $t -ge $_.from+[uint64](1000*$TrimSeconds) -and $t -le $_.to }).Count }) }
$period=Median @(InSegments 'native_frame_times','native_original_frames' | ForEach-Object { WindowMean $_ } | Where-Object { $null -ne $_ })
$stepPeriod=Median @(InSegments 'game_frame_times' | ForEach-Object { WindowMean $_ } | Where-Object { $null -ne $_ })
function Relative($set,$p){ if(-not $set.Count -or -not $p -or @($set | Where-Object { $null -eq $_.ms }).Count){ return $null }
    $a=0;$b=0;foreach($r in $set){ foreach($v in $r.ms){ if($v -gt 1.5*$p){ $a++ };if($v -gt 2.04*$p){ $b++ } } };,@($a,$b) }
function Late($set,$p){ if(-not $set.Count -or -not $p){ return $null }
    $late=0.0;$samples=0;$irregular=0;$singles=0;$hist=[ordered]@{};$cadence=@();$partial=$false
    foreach($r in $set){ $h=@($r.holds);$hz=[double]$r.refresh_hz;if(-not $hz){ return $null }
        $span=$hz*$p/1000.0;$c=[int][Math]::Ceiling($span-0.1);$floor=[int][Math]::Max(1,[Math]::Floor($span+0.1));$cadence+=$c
        # A row without its holds (no frame-dump, runs before I-0098) at a cadence of one: the repeats are the late refreshes.
        if($null -eq $r.holds -and $c -eq 1){ $late+=[double]$r.repeated*1000.0/$hz/$p;$samples+=[int]$r.hitches;$partial=$true;continue }
        if($null -eq $r.holds -or $h.Count -ne 2*[int]$r.samples){ return $null }
        for($i=0;$i -lt $h.Count;$i+=2){ $f=[int]$h[$i];$n=[int]$h[$i+1];$x=$n-$f*$c
            if($x -gt 0){ $late+=$x*1000.0/$hz/$p;$samples++ }
            if($f -eq 1){ $singles++;$hist["$n"]=1+[int]$hist["$n"];if($n -lt $floor -or $n -gt $c){ $irregular++ } } } }
    if($partial){ $singles=$null;$irregular=$null;$hist=$null }
    [ordered]@{late=[Math]::Round($late,2);late_samples=$samples;cadence=@($cadence | Sort-Object -Unique);singles=$singles;irregular=$irregular;holds=$hist} }
# I-0098: what the screen showed, from PresentMon (presentmon.csv, Invoke-ReplayRun -PresentMon), the same for both renderers:
# DXGI's frame statistics proved unreliable with two screens (i98-c1: on the 60 Hz screen they kept the 279.96 Hz screen's count
# and almost stopped; on the 279.96 Hz screen frames « held » 0 or 3 refreshes at 60 frames/s). Each frame displayed (Dropped 0)
# whose time falls in a segment (placed on the marks' tick count by presentmon-clock.txt's pair): its interval since the previous display
# change beyond the cadence (c refreshes of the screen, c as above; half a refresh of slack for timing) counts in frame periods
# (pm_late: a repeat at 60 Hz is one), and the intervals over 1.5 P and 2.04 P are counted (pm_over150, pm_over204).
$screen=if(Test-Path -LiteralPath "$dir/screen.json"){ Get-Content -LiteralPath "$dir/screen.json" -Raw | ConvertFrom-Json }else{ $null }
$pmFrames=$null;$pmModes=$null
if((Test-Path -LiteralPath "$dir/presentmon.csv") -and (Test-Path -LiteralPath "$dir/presentmon-clock.txt")){
    # Each frame's performance counter (QPCTime) placed on the tick count by the pair Invoke-ReplayRun sampled together.
    $clock=@((Get-Content -LiteralPath "$dir/presentmon-clock.txt" -Raw).Trim() -split ' ' | ForEach-Object { [double]$_ })
    $csv=@(Import-Csv -LiteralPath "$dir/presentmon.csv")
    if($csv.Count){
        $pmFrames=@($csv | ForEach-Object { [pscustomobject]@{tick=$clock[0]+([double]$_.QPCTime-$clock[1])*1000.0/$clock[2];dropped=$_.Dropped -ne '0'
            change=$(if($_.MsBetweenDisplayChange -match '^[\d.]+$'){[double]$_.MsBetweenDisplayChange}else{0.0})} })
        $pmModes=[ordered]@{};foreach($g in @($csv | Group-Object PresentMode)){ $pmModes[$(if($g.Name){$g.Name}else{'(none)'})]=$g.Count } } }
function PresentMonSegment($from,$to,$p){ if($null -eq $pmFrames -or -not $p -or -not $screen){ return $null }
    $hz=[double]$screen.hz;if($hz -eq 280){ $hz=279.96 }   # the OLED's exact rate (the desktop mode gives whole hertz)
    $r=1000.0/$hz;$c=[int][Math]::Ceiling($hz*$p/1000-0.1);$late=0.0;$a=0;$b=0;$n=0
    foreach($f in $pmFrames){ if($f.dropped -or $f.change -le 0 -or $f.tick -lt $from -or $f.tick -gt $to){ continue };$n++
        $x=$f.change-$c*$r;if($x -gt 0.5*$r){ $late+=$x/$p };if($f.change -gt 1.5*$p){ $a++ };if($f.change -gt 2.04*$p){ $b++ } }
    [ordered]@{displayed=$n;late=[Math]::Round($late,2);over150=$a;over204=$b} }
$out=foreach($s in $segments){
    $from=$s.from+[uint64](1000*$TrimSeconds);$to=$s.to
    $pm=PresentMonSegment $from $to $period
    $ends=@($rows | Where-Object { $_.event -in 'native_frame_times','native_original_frames' -and $_.tick_ms -and [uint64]$_.tick_ms -ge $from -and [uint64]$_.tick_ms -le $to })
    $steps=@($rows | Where-Object { $_.event -eq 'game_frame_times' -and [uint64]$_.tick_ms -ge $from -and [uint64]$_.tick_ms -le $to })
    $held=$null;$natives=@($ends | Where-Object { $_.event -eq 'native_frame_times' })
    if($natives.Count){ $first=($natives | Measure-Object frame -Minimum).Minimum;$last=($natives | Measure-Object frame -Maximum).Maximum;$held=0
        foreach($r in @($rows | Where-Object { $_.event -eq 'native_present_refreshes' -and $_.frame -gt $first -and $_.frame -le $last })){ $held+=[int]$r.refreshes_1_2_3_4plus[1]+[int]$r.refreshes_1_2_3_4plus[2]+[int]$r.refreshes_1_2_3_4plus[3] }
        if(-not @($rows | Where-Object event -eq 'native_present_refreshes').Count){ $held=$null } }
    $shown=@($rows | Where-Object { $_.event -eq 'display_refreshes' -and $_.tick_ms -and [uint64]$_.tick_ms -ge $from -and [uint64]$_.tick_ms -le $to })
    $display=if($shown.Count){ [ordered]@{presented=($shown | Measure-Object presented -Sum).Sum;refreshes=($shown | Measure-Object refreshes -Sum).Sum
        repeated=($shown | Measure-Object repeated -Sum).Sum;hitches=($shown | Measure-Object hitches -Sum).Sum;windows=$shown.Count
        hz=@($shown | ForEach-Object { [int]$_.refresh_hz } | Sort-Object -Unique)} }else{ $null }
    # Step review: the windows the segment should hold at the game's rate (the display's refresh, at most DS3's 60 frames/s) and
    # those found (a run whose rows stopped would count nothing).
    # I-0098: at the run's frame rate once known (its period), not at most 60 frames/s.
    $hz=if($period){1000.0/$period}else{[Math]::Min(60.0,$(if($natives.Count -and $natives[0].refresh_hz){[double]$natives[0].refresh_hz}else{60.0}))};$seconds=($to-$from)/1000.0
    $late=Late $shown $period
    [pscustomobject][ordered]@{name=$s.name;seconds=[Math]::Round($seconds,1);frames=120*$ends.Count;windows=$ends.Count;expected_windows=[Math]::Floor($seconds*$hz/120)
        frame_end=(Sum3 $ends);game_step=$(if($steps.Count){Sum3 $steps}else{$null});held=$held
        frame_hz=@($ends | Where-Object { $_.refresh_hz } | ForEach-Object { [int]$_.refresh_hz } | Sort-Object -Unique)   # I-0098: the screen the module saw
        frame_end_rel=(Relative $ends $period);game_step_rel=(Relative $steps $stepPeriod)
        display_late=$(if($late){$late.late}else{$null});display_late_samples=$(if($late){$late.late_samples}else{$null});display_cadence=$(if($late){$late.cadence}else{@()})
        display_irregular=$(if($late){$late.irregular}else{$null});display_singles=$(if($late){$late.singles}else{$null});display_holds=$(if($late){$late.holds}else{$null})
        pm_displayed=$(if($pm){$pm.displayed}else{$null});pm_late=$(if($pm){$pm.late}else{$null});pm_over150=$(if($pm){$pm.over150}else{$null});pm_over204=$(if($pm){$pm.over204}else{$null})
        display_repeats=$(if($display){$display.repeated}else{$null});display_hitches=$(if($display){$display.hitches}else{$null})
        display_presented=$(if($display){$display.presented}else{$null});display_refreshes=$(if($display){$display.refreshes}else{$null})
        display_windows=$(if($display){$display.windows}else{0});display_hz=$(if($display){$display.hz}else{@()})} }
$out=@($out)
$total=[pscustomobject][ordered]@{name='total';seconds=($out | Measure-Object seconds -Sum).Sum;frames=($out | Measure-Object frames -Sum).Sum
    frame_end=@(0,1,2 | ForEach-Object { $k=$_;($out | ForEach-Object { $_.frame_end[$k] } | Measure-Object -Sum).Sum })
    game_step=$(if(@($out | Where-Object { $_.game_step }).Count){@(0,1,2 | ForEach-Object { $k=$_;($out | Where-Object { $_.game_step } | ForEach-Object { $_.game_step[$k] } | Measure-Object -Sum).Sum })}else{$null})
    held=$(if(@($out | Where-Object { $null -ne $_.held }).Count){($out | Measure-Object held -Sum).Sum}else{$null})
    display_repeats=$(if(@($out | Where-Object { $null -ne $_.display_repeats }).Count){($out | Measure-Object display_repeats -Sum).Sum}else{$null})
    display_hitches=$(if(@($out | Where-Object { $null -ne $_.display_hitches }).Count){($out | Measure-Object display_hitches -Sum).Sum}else{$null})
    display_presented=$(if(@($out | Where-Object { $null -ne $_.display_presented }).Count){($out | Measure-Object display_presented -Sum).Sum}else{$null})
    display_refreshes=$(if(@($out | Where-Object { $null -ne $_.display_refreshes }).Count){($out | Measure-Object display_refreshes -Sum).Sum}else{$null})
    # I-0098: the relative counts, null unless every segment has them (a partial sum would pass short).
    period_ms=$(if($period){[Math]::Round($period,3)}else{$null});game_step_period_ms=$(if($stepPeriod){[Math]::Round($stepPeriod,3)}else{$null})
    frame_end_rel=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.frame_end_rel }).Count){@(0,1 | ForEach-Object { $k=$_;($out | ForEach-Object { $_.frame_end_rel[$k] } | Measure-Object -Sum).Sum })}else{$null})
    game_step_rel=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.game_step_rel }).Count){@(0,1 | ForEach-Object { $k=$_;($out | ForEach-Object { $_.game_step_rel[$k] } | Measure-Object -Sum).Sum })}else{$null})
    display_late=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.display_late }).Count){[Math]::Round(($out | Measure-Object display_late -Sum).Sum,2)}else{$null})
    display_late_samples=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.display_late_samples }).Count){($out | Measure-Object display_late_samples -Sum).Sum}else{$null})
    display_irregular=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.display_irregular }).Count){($out | Measure-Object display_irregular -Sum).Sum}else{$null})
    display_singles=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.display_singles }).Count){($out | Measure-Object display_singles -Sum).Sum}else{$null})
    display_cadence=@($out | ForEach-Object { $_.display_cadence } | Sort-Object -Unique)
    pm_modes=$pmModes
    pm_displayed=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.pm_late }).Count){($out | Measure-Object pm_displayed -Sum).Sum}else{$null})
    pm_late=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.pm_late }).Count){[Math]::Round(($out | Measure-Object pm_late -Sum).Sum,2)}else{$null})
    pm_over150=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.pm_late }).Count){($out | Measure-Object pm_over150 -Sum).Sum}else{$null})
    pm_over204=$(if($out.Count -and -not @($out | Where-Object { $null -eq $_.pm_late }).Count){($out | Measure-Object pm_over204 -Sum).Sum}else{$null})}
$replay=if(Test-Path -LiteralPath "$dir/replay.log"){ (Get-Content -LiteralPath "$dir/replay.log" | Select-Object -Last 1) }else{ '' }
$late=if($replay -match 'late_max_ms=([\d.]+) late_mean_ms=([\d.]+)'){ @([double]$Matches[1],[double]$Matches[2]) }else{ $null }
$crashes=@(Get-ChildItem -LiteralPath "$dir/crashes" -File -Filter '*.txt' -ErrorAction SilentlyContinue)
$restoration=if(Test-Path -LiteralPath "$dir/restoration.json"){ [bool](Get-Content -LiteralPath "$dir/restoration.json" -Raw | ConvertFrom-Json).ok }else{ $false }
[pscustomobject][ordered]@{run=Split-Path -Leaf $dir;mode=$base.bootstrap_mode;measure=[bool]$base.measure;debug_layer=-not [bool]$base.debug_layer_disabled
    replay=$(if($replay -match '^replay (\S+)'){$Matches[1]}else{'missing'});late_ms=$late
    faults=[pscustomobject][ordered]@{crash_reports=$crashes.Count;native_failure=@($rows | Where-Object event -eq 'native_failure').Count
        refusal=@($rows | Where-Object event -eq 'refusal').Count;device_removed=@($rows | Where-Object event -eq 'native_device_removed').Count}
    restoration_ok=$restoration;segments=$out;total=$total}
