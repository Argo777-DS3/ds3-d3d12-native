param([Parameter(Mandatory)][string]$Series)
# I-0097: an A/B series of Run-I97Series.ps1 read per build letter: each run's counts (Get-ReplayMeasure.ps1: frames over 25 and 34
# ms at the frame end and at the game step, the display's repeated refreshes), their mean and standard deviation per build, the
# difference against the first build in units of its standard error (a difference is called beyond 2), and the pooled lows of
# each build (Get-I97Lows.ps1, every frame of its runs). Runs not complete, not restored, faulted or refused are left out (listed).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$runs=@(Get-ChildItem "$repo/evidence/I-0066/artifacts" -Directory | Where-Object { $_.Name -match "^$([regex]::Escape($Series))-\d+-([a-h])(-r)?$" } | Sort-Object Name)
$rows=foreach($r in $runs){ $null=$r.Name -match '-\d+-([a-h])(-r)?$';$letter=$Matches[1];$m=& "$repo/scripts/Get-ReplayMeasure.ps1" -Run $r.Name
    $f=$m.faults;$ok=$m.replay -eq 'complete' -and $m.restoration_ok -and -not ($f.crash_reports+$f.native_failure+$f.refusal+$f.device_removed) -and $m.total.game_step -and $null -ne $m.total.display_repeats
    [pscustomobject]@{run=$r.Name;build=$letter;ok=$ok;fe25=$m.total.frame_end[1];fe34=$m.total.frame_end[2];gs25=$(if($m.total.game_step){$m.total.game_step[1]});gs34=$(if($m.total.game_step){$m.total.game_step[2]});repeats=$m.total.display_repeats} }
$rows | Format-Table -AutoSize | Out-String -Width 160
$good=@($rows | Where-Object ok)
function Stat($v){ $n=$v.Count;$mean=($v | Measure-Object -Average).Average;$sd=if($n -gt 1){[Math]::Sqrt((($v | ForEach-Object { ($_-$mean)*($_-$mean) } | Measure-Object -Sum).Sum)/($n-1))}else{0};[pscustomobject]@{n=$n;mean=$mean;sd=$sd} }
$builds=@($good.build | Sort-Object -Unique);$metrics='fe25','fe34','gs25','gs34','repeats'
$base=@{};foreach($k in $metrics){ $base[$k]=Stat @($good | Where-Object build -eq $builds[0] | ForEach-Object { [double]$_.$k }) }
foreach($b in $builds){ $line="build $b ($(@($good | Where-Object build -eq $b).Count) runs):"
    foreach($k in $metrics){ $s=Stat @($good | Where-Object build -eq $b | ForEach-Object { [double]$_.$k });$z=''
        if($b -ne $builds[0]){ $se=[Math]::Sqrt($s.sd*$s.sd/[Math]::Max(1,$s.n)+$base[$k].sd*$base[$k].sd/[Math]::Max(1,$base[$k].n));$z=if($se){' ({0:+0.0;-0.0} se)' -f (($s.mean-$base[$k].mean)/$se)}else{''} }
        $line+=" $k {0:0.0} sd {1:0.0}$z;" -f $s.mean,$s.sd }
    $line }
foreach($b in $builds){ $names=@($good | Where-Object build -eq $b | ForEach-Object run);"lows, build $b"
    & "$PSScriptRoot/Get-I97Lows.ps1" -Run $names | Where-Object run -eq 'pooled' | Format-Table point,frames,low1_ms,low1_fps,low01_ms,low01_fps,worst_ms,over25,over34 -AutoSize | Out-String -Width 160 }
