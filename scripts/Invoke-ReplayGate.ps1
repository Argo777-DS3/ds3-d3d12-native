param([string]$Tag='',[string]$BuildFolder='',[ValidateSet('InlineRender','InlinePrepare','NoPersistentDraws','NoModelRecords')][string[]]$ExtraSwitch=@(),
    [string]$NotBefore='',[switch]$Calibrate,[ValidateRange(3,8)][int]$CalibrationRuns=4,[string]$AllowRaise='',[switch]$NewReference,
    [string[]]$Evaluate=@(),[string]$RecordOriginal='',[switch]$NoValidation,[switch]$Memory,[string]$MemoryRun='',[string[]]$Screens=@())
# I-0083 (Thomas, D-0200): the replayed path's blocking gate at every merge into main. Announced live runs only (D-0116): measure
# runs and one validation run of scripts/replay/firelink.path.json (Invoke-ReplayRun.ps1), built from this checkout's committed
# native/ tree (Build-GameBootstrap -NoControls -Out; the unit controls belong to the workstream's own build), then, with -Memory, the memory leg.
# I-0098 (Thomas, 2026-10-24: « dans l'idéal ce serait sur une base illimitée ... j'ai deux écrans bien différents »): the gate is
# independent of the screen. .project/REPLAY-GATE.json holds one base per screen, keyed « <width>x<height>@<hz> »
# (scripts/Get-Screens.ps1), each with its thresholds, their calibration, its reference picture, the original's numbers and its
# last pass; bases are never mixed. A gate places its runs on each screen of -Screens (all of required_bases by default): two
# measure runs a screen, judged by that screen's base; the validation run (the debug layer) and the memory leg (-Memory), which do not
# depend on the screen, once, on the first. A merge needs a pass on every required base (scripts/Test-Project.ps1; D-0218: the ASUS
# alone; the OLED is added with -Screens 1920x1080@60,2560x1440@280, never instead of it).
# The measures (scripts/Get-ReplayMeasure.ps1, measure runs with frame-dump.flag) are relative to the run's frame period P (the
# median of its 120-frame windows' mean interval: 16.67 ms under DS3's 60 frames/s cap on any screen, the frame time once the
# rate is unlocked): frames over 2.04 P and 1.5 P at the native frame end (34 and 25 ms at 60 frames/s), game steps over 2.04 P
# of the game step's own period, and the display's refreshes beyond the screen's cadence in frame periods (display_late: the
# display repeats at 60 Hz; at 279.96 Hz the 60 frames/s cap's 4/5 cadence is not counted); P itself (period_ms) guards against
# a uniformly slower run, which relative counts alone would not see.
# Verdict:
# - FAIL when a run faults (a crash report, native_failure, refusal or removed device; the validation run's D3D12 error ends the
#   native run, so it faults), when the mean of a screen's two measure runs exceeds one of its base's thresholds, when a
#   standstill picture departs from its base's reference picture beyond its threshold, or when the memory leg faults or rises;
# - INCONCLUSIVE (run the gate again) when a replay did not complete (the foreground lost, an error), a protected file was not
#   restored, a measure run is not in measure mode without the debug layer, a segment holds fewer than three quarters of the
#   120-frame windows its length gives at the run's rate, a run displayed at another refresh rate than its base's (the window not
#   on its screen), a relative measure is missing (no frame-dump), the thresholds were calibrated on another path, a run's module
#   is not the gate's build, a base has no threshold for a metric yet, or a standstill picture is missing or was taken with the
#   game behind another window;
# - PASS otherwise: each judged base's record names the native/ tree that passed, which scripts/Test-Project.ps1 requires on main
#   for every required base.
# Experiments (-BuildFolder, -ExtraSwitch, -NoValidation, -Evaluate of runs already made) print and file their verdict,
# never a record. -Calibrate: N measure runs a screen (and the validation run) of the committed tree set that screen's base's
# thresholds, filed with their numbers: per count max(mean + 3 sd / sqrt(2), mean + 3 sqrt(max(mean,1)/2), highest run), rounded
# up; the period max(mean + 3 sd / sqrt(2), highest run) and half a percent, to the hundredth above; judged against the mean of
# two runs; a threshold only goes down (and only when the new runs are lower beyond twice their combined standard error) unless
# -AllowRaise names Thomas's decision in decisions/LOG.md; a base's first calibration sets them. The reference picture is kept
# (its thresholds too) unless the base has none or -NewReference is given (a visual change, after D-0181's side-by-side check).
# A calibration never writes a pass. -RecordOriginal <run>: files an original (Reference) run's numbers in its screen's base.
# The memory leg (I-0085, D-0204, D-0206): one zone (Beta m31, the Shebang zone save) loaded from the title eight times in one
# process (Run-Endurance.ps1 -OnlyZones -Visits 8, measure mode); the Theil-Sen slope (I-0092) of the private memory at the title
# over the visits after the first, against memory_mb_per_visit (filed, not calibrated; a leak of 151 MB a visit, i85-long-2,
# passed the 15-minute path unseen). D-0218 (Thomas, 2026-10-26): run only with -Memory, when the branch touches the memory, the
# loadings or the streaming; -MemoryRun judges a leg already run (with -Evaluate).
# Exit codes: 0 PASS, 1 FAIL, 2 INCONCLUSIVE, 3 the gate itself failed.
$ErrorActionPreference='Stop'
trap { "replay gate error: $_";exit 3 }
$repo=Split-Path -Parent $PSScriptRoot;$gateFile="$repo/.project/REPLAY-GATE.json"
$gate=Get-Content -LiteralPath $gateFile -Raw | ConvertFrom-Json -AsHashtable
if($gate.schema -ne 2 -or -not $gate.bases){ throw 'REPLAY-GATE.json is not schema 2 (one base per screen, I-0098)' }
$Screens=@($Screens -split '[,;\s]+' | Where-Object { $_ })   # step review: split once, for every use
# The path by its Git object (the committed file, whatever the working copy's line endings).
$pathHash=(git -C $repo rev-parse "HEAD:$($gate.path)").Trim()
# The display's count comes from PresentMon (display_source « presentmon »: each measure run under PresentMon, Get-ReplayMeasure's
# pm_late): DXGI's frame statistics proved unreliable with two screens (i98-c1). « none » judges no display count; « pending »
# (Thomas's choice between the two not made yet) judges none either and leaves every launched gate INCONCLUSIVE.
$displaySource=if($gate.display_source){ $gate.display_source }else{ 'pending' }
$metricKeys=@('period_ms','frame_end_over204','frame_end_over150')+@(if($displaySource -eq 'presentmon'){ 'display_late' })+@('game_step_over204')
Add-Type -AssemblyName System.Drawing
# The game's client area (start-client.txt beside the picture, relative to the captured screen; else 1280x720 at (8,31),
# Pilot-TitleContinue's layout) scaled to 160x90; the mean absolute difference of the RGB channels (0-255) and the share of cells
# whose summed channel difference exceeds 120 (40 a channel).
function PictureCells([string]$file){ $src=[Drawing.Bitmap]::new($file);$dst=[Drawing.Bitmap]::new(160,90);$g=[Drawing.Graphics]::FromImage($dst)
    try{ $g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $client=Join-Path (Split-Path -Parent $file) ((Split-Path -LeafBase $file)+'-client.txt');$c=if(Test-Path -LiteralPath $client){ @((Get-Content -LiteralPath $client -Raw).Trim() -split ' ') }else{ @() }
        $from=if($c.Count -ge 4){[Drawing.Rectangle]::new([int]$c[0],[int]$c[1],[int]$c[2],[int]$c[3])}elseif($src.Width -ge 1288 -and $src.Height -ge 751){[Drawing.Rectangle]::new(8,31,1280,720)}else{[Drawing.Rectangle]::new(0,0,$src.Width,$src.Height)}
        $g.DrawImage($src,[Drawing.Rectangle]::new(0,0,160,90),$from,[Drawing.GraphicsUnit]::Pixel);$b=[int[]]::new(160*90*3)
        for($y=0;$y -lt 90;$y++){ for($x=0;$x -lt 160;$x++){ $px=$dst.GetPixel($x,$y);$i=($y*160+$x)*3;$b[$i]=$px.R;$b[$i+1]=$px.G;$b[$i+2]=$px.B } };,$b }
    finally{ $g.Dispose();$dst.Dispose();$src.Dispose() } }
function ComparePicture([string]$a,[string]$b){ $p=PictureCells $a;$q=PictureCells $b;$sum=0.0;$over=0
    for($i=0;$i -lt $p.Length;$i+=3){ $d=[Math]::Abs($p[$i]-$q[$i])+[Math]::Abs($p[$i+1]-$q[$i+1])+[Math]::Abs($p[$i+2]-$q[$i+2]);$sum+=$d;if($d -gt 120){$over++} }
    ,@(($sum/$p.Length),($over/($p.Length/3))) }
function RunDir([string]$name){ "$repo/evidence/I-0066/artifacts/$name" }
function MeasureRun([string]$name){ & "$PSScriptRoot/Get-ReplayMeasure.ps1" -Run $name }
function Metrics($m){ $t=$m.total;[ordered]@{period_ms=$t.period_ms
    frame_end_over204=$(if($t.frame_end_rel){[int]$t.frame_end_rel[1]}else{$null});frame_end_over150=$(if($t.frame_end_rel){[int]$t.frame_end_rel[0]}else{$null})
    display_late=$t.pm_late;game_step_over204=$(if($t.game_step_rel){[int]$t.game_step_rel[1]}else{$null})} }
# A run's screen: its screen.json (Invoke-ReplayRun -Screen), else the only screen named by -Screens (runs made before I-0098).
function RunBase([string]$name){ $f="$(RunDir $name)/screen.json"
    if(Test-Path -LiteralPath $f){ return (Get-Content -LiteralPath $f -Raw | ConvertFrom-Json).key }
    if($Screens.Count -eq 1){ return $Screens[0] };throw "run $name has no screen.json: name its base with -Screens <key>" }
function BaseHz([string]$key){ [int]($key -replace '^.*@','') }
$tree=(git -C $repo rev-parse HEAD:native).Trim();$commit=(git -C $repo rev-parse --short HEAD).Trim()
# The record names what ran: the native code, the path and the scripts that drive and read it (step review L4).
$dirty=@(git -C $repo status --porcelain -- native scripts/replay scripts/Invoke-ReplayRun.ps1 scripts/Invoke-ReplayPath.ps1 scripts/Get-ReplayMeasure.ps1 scripts/Invoke-ReplayGate.ps1 scripts/Get-Screens.ps1 investigations/I-0085-night-endurance investigations/I-0066-independent-native-startup/Pilot-Game.ps1 investigations/I-0066-independent-native-startup/Pilot-TitleContinue.ps1)
if($RecordOriginal){
    $m=MeasureRun $RecordOriginal;$key=RunBase $RecordOriginal
    $f=$m.faults;if($m.mode -ne 'Reference' -or $m.replay -ne 'complete' -or -not $m.restoration_ok -or ($f.crash_reports+$f.native_failure+$f.refusal+$f.device_removed)){ throw "$RecordOriginal is not a complete, restored, fault-free Reference run of the path" }
    if(-not $gate.bases[$key]){ $gate.bases[$key]=[ordered]@{} }
    $gate.bases[$key].original=[ordered]@{run=$RecordOriginal;date=(Get-Date -Format 'yyyy-MM-dd HH:mm');path_object=$pathHash
        segments=@($m.segments | ForEach-Object { [ordered]@{name=$_.name;frame_end_17_25_34=$_.frame_end;game_step_17_25_34=$_.game_step;frame_end_rel=$_.frame_end_rel;display_late=$_.pm_late;display_late_dxgi=$_.display_late} })
        total=[ordered]@{period_ms=$m.total.period_ms;frame_end_17_25_34=$m.total.frame_end;game_step_17_25_34=$m.total.game_step;frame_end_rel=$m.total.frame_end_rel;game_step_rel=$m.total.game_step_rel
            display_repeats=$m.total.display_repeats;display_late=$m.total.pm_late;display_late_dxgi=$m.total.display_late;display_hitches=$m.total.display_hitches;pm_modes=$m.total.pm_modes}}
    $gate | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $gateFile -Encoding utf8;"original filed in $key`: $RecordOriginal";exit 0 }
# Step review (i98-cal): a calibration may judge again its own runs of the committed tree (-Evaluate), their module checked.
$experiment=[bool]($BuildFolder -or $ExtraSwitch.Count -or $NoValidation -or ($Evaluate.Count -and -not $Calibrate))
# Step review L5: the calibration's conditions checked before any live run.
if($Calibrate){
    if($experiment -or $dirty.Count){ throw 'calibration needs the committed tree of this checkout (no experiment switch, nothing uncommitted)' }
    if($Evaluate.Count){ $BuildFolder="$repo/evidence/I-0083/gates/build-$($tree.Substring(0,12))"
        if(-not (Test-Path -LiteralPath "$BuildFolder/tree.txt") -or (Get-Content -LiteralPath "$BuildFolder/tree.txt" -Raw).Trim() -ne $tree){ throw 'a calibration of runs already made needs the build of the committed tree' } }
    # Second review N5: a rise needs one of Thomas's decisions, by its index line.
    if($AllowRaise -and -not (Select-String -LiteralPath "$repo/decisions/LOG.md" -SimpleMatch "- **$AllowRaise** (Thomas" -Quiet)){ throw "-AllowRaise $AllowRaise is not one of Thomas's decisions in decisions/LOG.md" } }
elseif($AllowRaise -or $NewReference){ throw '-AllowRaise and -NewReference belong to a calibration (-Calibrate)' }
if($Memory -and $Evaluate.Count){ throw '-Memory runs a new memory leg; with -Evaluate judge one already run with -MemoryRun' }   # final review L5
if(-not $Tag){ $Tag="gate-$($tree.Substring(0,8))-$(Get-Date -Format 'MMddHHmm')" }
# The screens, resolved now (each must be attached): -Screens, else the required bases.
$targets=@()
if(-not $Evaluate.Count){ $keys=if($Screens.Count){ $Screens }else{ @($gate.required_bases) }
    $targets=@($keys | ForEach-Object { & "$PSScriptRoot/Get-Screens.ps1" -Key $_ }) }
# Runs: a name and its base (screen key).
$runs=[Collections.Generic.List[object]]::new()
if($Evaluate.Count){ foreach($n in @($Evaluate -split '[,;\s]+' | Where-Object { $_ })){ $runs.Add([pscustomobject]@{name=$n;base=$(if((MeasureRun $n).mode -eq 'Native' -and -not (MeasureRun $n).debug_layer){ RunBase $n }else{ '' })}) } }
else{
    if(-not $experiment -and $dirty.Count){ throw "uncommitted changes in native/ or the replay scripts: commit them first (the record names the committed tree)" }
    if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);if($at -lt (Get-Date).AddHours(-12)){ $at=$at.AddDays(1) };while((Get-Date) -lt $at){ Start-Sleep 5 } }
    if(-not $BuildFolder){ $BuildFolder="$repo/evidence/I-0083/gates/build-$($tree.Substring(0,12))"
        if(-not (Test-Path -LiteralPath "$BuildFolder/tree.txt") -or (Get-Content -LiteralPath "$BuildFolder/tree.txt" -Raw).Trim() -ne $tree){
            "== build $(Get-Date -Format HH:mm:ss) $BuildFolder"
            & "$repo/native/frame/Build-GameBootstrap.ps1" -NoControls -Out $BuildFolder 2>&1 | Select-Object -Last 2
            if(-not (Test-Path -LiteralPath "$BuildFolder/game_bootstrap.dll")){ throw "the build left no module in $BuildFolder" }
            Set-Content -LiteralPath "$BuildFolder/tree.txt" -Value $tree -NoNewline } } }
function RunPlan($plan){
    # Final review M2: a run name already used would be measured again from its old folder (the trace refuses it inside its job).
    foreach($p in $plan){ if(Test-Path -LiteralPath (RunDir $p.name)){ throw "run $($p.name) already exists: give the gate a new -Tag" } }
    # I-0097: a launch the module refused before any of its code ran (prefix_refused, I-0091 i91-20, I-0097 i97-cal-m1) is
    # launched once more under the name <run>-r, in its place.
    foreach($p in $plan){ $name=$p.name
        for($try=0;$try -lt 2;$try++){ "== $name $(Get-Date -Format HH:mm:ss) on $($p.screen.key)"
            # I-0098: measure runs with frame-dump.flag (every frame's time, every display sample's holds: the relative measures).
            & "$PSScriptRoot/Invoke-ReplayRun.ps1" -RunName $name -Build $BuildFolder -Kind $p.kind -ExtraSwitch $ExtraSwitch -Screen $p.screen.key -EffectFlags $(if($p.kind -eq 'Measure'){'frame-dump'}else{''}) -PresentMon:$($p.kind -eq 'Measure' -and $displaySource -eq 'presentmon') 2>&1 | Select-Object -Last 2
            $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5
            if($try -or -not (Select-String -LiteralPath "$(RunDir $name)/startup.jsonl" -SimpleMatch '"event":"prefix_refused"' -Quiet -ErrorAction SilentlyContinue)){ break }
            "   refused at launch (prefix_refused): once more as $name-r";$name="$($p.name)-r"
            if(Test-Path -LiteralPath (RunDir $name)){ throw "run $name already exists: give the gate a new -Tag" } }
        $script:runs.Add([pscustomobject]@{name=$name;base=$(if($p.kind -eq 'Measure'){$p.screen.key}else{''})}) } }
# Final review: PresentMon checked before any live run (its file, and its trace session allowed: a two-second capture).
if(-not $Evaluate.Count -and $displaySource -eq 'presentmon'){ $pmExe='C:/Users/User/Tools/PresentMon/PresentMon-2.6.0-x64.exe'
    if(-not (Test-Path -LiteralPath $pmExe)){ throw "PresentMon is not at $pmExe" }
    $probe=Start-Process -FilePath $pmExe -ArgumentList '--no_csv --no_console_stats --timed 2 --terminate_after_timed --session_name DS3GateProbe --stop_existing_session' -NoNewWindow -PassThru -RedirectStandardOutput "$env:TEMP/ds3-gate-presentmon.txt" -RedirectStandardError "$env:TEMP/ds3-gate-presentmon-errors.txt"
    if(-not $probe.WaitForExit(30000) -or $probe.ExitCode){ throw "PresentMon could not trace (exit $($probe.ExitCode)): the Performance Log Users group or elevation is needed" } }if(-not $Evaluate.Count){ $plan=[Collections.Generic.List[object]]::new()
    foreach($s in $targets){ for($i=1;$i -le $(if($Calibrate){$CalibrationRuns}else{2});$i++){ $plan.Add([pscustomobject]@{name="$Tag-$($s.key -replace '@','-')-m$i";kind='Measure';screen=$s}) } }
    if(-not $NoValidation){ $plan.Add([pscustomobject]@{name="$Tag-v1";kind='Validation';screen=$targets[0]}) }
    RunPlan $plan }
# The memory leg (I-0085), after the replay runs, on the first screen, with -Memory (D-0218); its threshold is filed.
if($MemoryRun -and -not $Evaluate.Count){ throw '-MemoryRun judges a leg already run, with -Evaluate' }
$memoryLeg=$MemoryRun
if(-not $Evaluate.Count -and $Memory){ $memoryLeg="$Tag-z1"
    if(Test-Path -LiteralPath (RunDir $memoryLeg)){ throw "run $memoryLeg already exists: give the gate a new -Tag" }
    "== $memoryLeg $(Get-Date -Format HH:mm:ss) on $($targets[0].key)"
    & "$repo/investigations/I-0085-night-endurance/Run-Endurance.ps1" -RunName $memoryLeg -Build $BuildFolder -OnlyZones 'Beta m31' -Visits 8 -StaySeconds 20 -Screen $targets[0].key 2>&1 | Select-Object -Last 2
    $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5 }
# Its value: the title stays that follow a world visit (Get-EnduranceSummary), each over the same window 10-22 s after the quit
# (final review M1: never the close's release). A fault row, a crash dump or the controller's crash record (the heap fault at the
# quit included since I-0099's STUDY step 4), or the game gone before its close (a crash), fails. I-0092 (Thomas, 2026-10-23 01:51): the title's private memory toggles by about 100 MB between
# visits, so the slope is the median of the pairwise slopes (Theil-Sen) of the titles after visits 2 to 8: 0 false failures in
# 200 simulated flat legs with that state, a 60 MB a visit growth failing 196.
function TheilSen([double[]]$y){ $s=@(for($i=0;$i -lt $y.Count;$i++){ for($j=$i+1;$j -lt $y.Count;$j++){ ($y[$j]-$y[$i])/($j-$i) } }) | Sort-Object
    $n=$s.Count; if($n%2){ $s[($n-1)/2] }else{ ($s[$n/2-1]+$s[$n/2])/2 } }
function MemoryLegValue([string]$run){ $dir=RunDir $run
    try{ $null=& "$repo/investigations/I-0085-night-endurance/Get-EnduranceSummary.ps1" -Run $run
        $s=Get-Content -LiteralPath "$dir/summary.json" -Raw | ConvertFrom-Json;$rows=@(Import-Csv -LiteralPath "$dir/visits.csv")
        $after=@(for($i=1;$i -lt $rows.Count;$i++){ if($rows[$i].kind -eq 'title' -and $rows[$i-1].kind -eq 'world' -and $rows[$i].private_window){ [double]$rows[$i].private_window } })
        $restored=(Test-Path -LiteralPath "$dir/restoration.json") -and (Get-Content -LiteralPath "$dir/restoration.json" -Raw | ConvertFrom-Json).ok
        $closed=(Test-Path -LiteralPath "$dir/close.txt") -and (Get-Content -LiteralPath "$dir/close.txt" -Raw) -match '^close clicked'
        $b=Get-Content -LiteralPath "$dir/baseline.json" -Raw -ErrorAction SilentlyContinue | ConvertFrom-Json
        [ordered]@{run=$run;error='';faults=@($s.faults).Count+@($s.crash_dumps).Count+$(if($s.controller_crash){1}else{0});closed=[bool]$closed;loads=[int]$s.tour_loads;titles_after_visits=$after;restored=[bool]$restored
            module_sha256=$(if($b){$b.module_sha256}else{$null});mb_per_visit=$(if($after.Count -ge 8){ [Math]::Round((TheilSen $after[1..($after.Count-1)]),1) }else{ $null })} }
    catch{ [ordered]@{run=$run;error="$_";mb_per_visit=$null} } }
# I-0100: not $memory, which is the -Memory switch (PowerShell names are case-blind: the first -Memory gate, i100-t1, failed here).
$legValue=if($memoryLeg){ MemoryLegValue $memoryLeg }else{ $null }
function Variance($v,$m){ (($v | ForEach-Object { ($_-$m)*($_-$m) } | Measure-Object -Sum).Sum)/[Math]::Max(1,$v.Count-1) }
# Second review N1: a FAIL on a base's measure thresholds alone (no fault, no picture, nothing inconclusive) is confirmed by two
# more measure runs on that screen before it blocks, the four judged by their mean (a launched gate only, not a calibration).
for($round=0;$round -lt 2;$round++){
$moduleHash=if($BuildFolder -and (Test-Path -LiteralPath "$BuildFolder/game_bootstrap.dll")){ (Get-FileHash -LiteralPath "$BuildFolder/game_bootstrap.dll").Hash }else{ $null }
$measured=@($runs | ForEach-Object { $m=MeasureRun $_.name;$m | Add-Member -NotePropertyName base -NotePropertyValue $_.base -Force -PassThru })
$validations=@($measured | Where-Object { $_.mode -eq 'Native' -and $_.debug_layer })
$reasons=[Collections.Generic.List[string]]::new();$verdict='PASS';$thresholdFails=[ordered]@{}
function Inconclusive([string]$why){ $reasons.Add("inconclusive: $why");if($script:verdict -eq 'PASS'){ $script:verdict='INCONCLUSIVE' } }
function Fail([string]$why){ $reasons.Add("fail: $why");$script:verdict='FAIL' }
foreach($m in $measured){
    $f=$m.faults;if($f.crash_reports+$f.native_failure+$f.refusal+$f.device_removed){ Fail "$($m.run): fault (crash reports $($f.crash_reports), native_failure $($f.native_failure), refusal $($f.refusal), device removed $($f.device_removed))" }
    if($m.replay -ne 'complete'){ Inconclusive "$($m.run): replay $($m.replay)" }
    if(-not $m.restoration_ok){ Inconclusive "$($m.run): protected files not restored by hash" }
    # I-0097 tranche 5 step review: a run with test-no-ai.flag (enemies without AI, a study tool) is never judged by the gate.
    if(Select-String -LiteralPath "$(RunDir $m.run)/startup.jsonl" -SimpleMatch '"event":"test_no_ai"' -Quiet -ErrorAction SilentlyContinue){ Inconclusive "$($m.run): run with test-no-ai.flag" }
    # I-0097 tranche 5 final review M2: nor a run traced by cadence-trace.flag (its threads and journal rows, a diagnostic).
    if(Select-String -LiteralPath "$(RunDir $m.run)/startup.jsonl" -SimpleMatch '"event":"cadence_trace_started"' -Quiet -ErrorAction SilentlyContinue){ Inconclusive "$($m.run): run with cadence-trace.flag" }
    # Final review M2: a launched gate judges only runs of its own build.
    if((-not $Evaluate.Count -or $Calibrate) -and $moduleHash){ $b=Get-Content -LiteralPath "$(RunDir $m.run)/baseline.json" -Raw -ErrorAction SilentlyContinue | ConvertFrom-Json
        if(-not $b -or $b.module_sha256 -ne $moduleHash){ Inconclusive "$($m.run): its module is not this gate's build" } } }
if(-not $NoValidation -and -not $validations.Count){ Inconclusive 'no validation run (debug layer)' }
if($displaySource -notin 'presentmon','none' -and -not $Calibrate){ Inconclusive "the display count's source is not chosen (display_source ${displaySource}: PresentMon or none, Thomas's choice)" }
$bases=@($runs | Where-Object { $_.base } | ForEach-Object { $_.base } | Select-Object -Unique)
if(-not $bases.Count){ Inconclusive 'no measure run (measure mode, no debug layer)' }
$values=[ordered]@{};$means=[ordered]@{};$calibrations=[ordered]@{};$pictureDiffs=[ordered]@{}
foreach($key in $bases){
    $base=$gate.bases[$key];if(-not $base){ $base=[ordered]@{};$gate.bases[$key]=$base }
    $measures=@($measured | Where-Object { $_.base -eq $key -and $_.mode -eq 'Native' -and $_.measure -and -not $_.debug_layer })
    foreach($m in $measures){
        # Step review H2: a run whose frame rows stopped would count nothing and pass; a quarter of slack (a slow build has fewer
        # frames, but its period then fails).
        foreach($s in @($m.segments)){ if($s.windows -lt [Math]::Floor(0.75*$s.expected_windows)){ Inconclusive "$($m.run): segment $($s.name) has $($s.windows) frame windows of $($s.expected_windows)" }
            # I-0098: the frames PresentMon saw displayed (DXGI's display windows are unreliable with two screens, i98-c1), at
            # least three quarters of the segment's frames at the run's rate; and the window on its base's screen (the refresh
            # rate the module read for the frame rows, within 1 Hz).
            if($displaySource -eq 'presentmon' -and $s.pm_displayed -lt [Math]::Floor(0.75*120*$s.expected_windows)){ Inconclusive "$($m.run): segment $($s.name) has $($s.pm_displayed) frames displayed (PresentMon) of $(120*$s.expected_windows)" }
            if(-not @($s.frame_hz).Count -or @($s.frame_hz | Where-Object { [Math]::Abs($_-(BaseHz $key)) -gt 1 }).Count){ Inconclusive "$($m.run): segment $($s.name) ran at $(@($s.frame_hz) -join '/') Hz, its base $key" } }
        $mv=Metrics $m;foreach($k in $metricKeys){ if($null -eq $mv[$k]){ Inconclusive "$($m.run): no $k (measure runs need frame-dump.flag, and PresentMon for the display)" } } }
    # Final review M3: thresholds and reference measured on another path judge nothing.
    if(-not $Calibrate -and $base.calibration -and $base.calibration.path_object -ne $pathHash){ Inconclusive "$key`: the thresholds were calibrated on another path: recalibrate (-Calibrate)" }
    if($measures.Count -lt 2){ Inconclusive "$key`: two measure runs (measure mode, no debug layer) are needed, $($measures.Count) found" }
    # Final review: a calibration from runs already made takes as many as a launched one would (at least three a base).
    if($Calibrate -and $Evaluate.Count -and $measures.Count -lt [Math]::Max(3,$CalibrationRuns)){ Inconclusive "${key}: a calibration needs $([Math]::Max(3,$CalibrationRuns)) measure runs, $($measures.Count) given" }
    $values[$key]=[ordered]@{};foreach($k in $metricKeys){ $values[$key][$k]=@($measures | ForEach-Object { (Metrics $_)[$k] }) }
    $pictures=@($measures | ForEach-Object { "$(RunDir $_.run)/start.png" })
    foreach($p in $pictures){ $client="$(Split-Path -Parent $p)/start-client.txt"
        if(-not (Test-Path -LiteralPath $p)){ Inconclusive "standstill picture missing: $p" }
        elseif((Test-Path -LiteralPath $client) -and -not (Get-Content -LiteralPath $client -Raw).Trim()){ Inconclusive "no client rectangle recorded with the standstill picture: $p" }
        elseif((Test-Path -LiteralPath $client) -and (Get-Content -LiteralPath $client -Raw) -notmatch 'front'){ Inconclusive "standstill picture taken with the game behind another window: $p" } }
    if($Calibrate){
        if(@($reasons | Where-Object { $_ -match '^(fail|inconclusive):' }).Count){   # step review: not the notes of a base calibrated before
            $reasons.Add("$key`: calibration not applied: every calibration run must be complete, restored and fault-free, with every measure") }else{
            $old=$base.thresholds;$new=[ordered]@{};$pd=@()
            foreach($k in $metricKeys){ $v=@($values[$key][$k] | ForEach-Object { [double]$_ });$mean=($v | Measure-Object -Average).Average;$sd=[Math]::Sqrt((Variance $v $mean))
                $top=($v | Measure-Object -Maximum).Maximum
                $new[$k]=if($k -eq 'period_ms'){ [Math]::Ceiling(100*1.005*[Math]::Max($mean+3*$sd/[Math]::Sqrt(2),$top))/100 }
                    else{ [int][Math]::Ceiling([Math]::Max([Math]::Max($mean+3*$sd/[Math]::Sqrt(2),$mean+3*[Math]::Sqrt([Math]::Max($mean,1.0)/2)),$top)) } }
            if($NewReference -or -not $base.reference_picture){ $refDir="$repo/evidence/I-0083/reference";New-Item -ItemType Directory -Force $refDir | Out-Null
                $refHash=(Get-FileHash -LiteralPath $pictures[0]).Hash;$ref="$refDir/firelink-start-$($refHash.Substring(0,12)).png"
                Copy-Item -LiteralPath $pictures[0] -Destination $ref -Force;Copy-Item -LiteralPath "$(Split-Path -Parent $pictures[0])/start-client.txt" -Destination ($ref -replace '\.png$','-client.txt') -Force
                $base.reference_picture=[ordered]@{path="evidence/I-0083/reference/$(Split-Path -Leaf $ref)";sha256=$refHash;from_run=$measures[0].run;native_tree=$tree}
                $pd=@($pictures | Select-Object -Skip 1 | ForEach-Object { ComparePicture $ref $_ })
                $new.picture_mean_diff=[Math]::Round([Math]::Max(1.5,3*($pd | ForEach-Object { $_[0] } | Measure-Object -Maximum).Maximum),2)
                $new.picture_cells_over=[Math]::Round([Math]::Max(0.005,3*($pd | ForEach-Object { $_[1] } | Measure-Object -Maximum).Maximum),4) }
            else{ $new.picture_mean_diff=$old.picture_mean_diff;$new.picture_cells_over=$old.picture_cells_over }
            # Final review L3: the picture thresholds under the same rule (a rise is Thomas's), the measures also only down beyond noise.
            if($old){ foreach($k in @($metricKeys+'picture_mean_diff','picture_cells_over')){ if($null -eq $old[$k]){ continue }
                if($new[$k] -gt $old[$k] -and -not $AllowRaise){
                    $reasons.Add("$key`: threshold $k would rise $($old[$k]) -> $($new[$k]): kept (a rise is Thomas's decision, -AllowRaise D-NNNN)");$new[$k]=$old[$k] }
                elseif($new[$k] -lt $old[$k] -and $k -notlike 'picture_*' -and $base.calibration -and $base.calibration.values[$k]){
                    # Second review N1: lowered only when the new runs are lower beyond their combined standard error (twice), so the
                    # threshold does not drift down to the luckiest of several noisy calibrations.
                    $p=@($base.calibration.values[$k] | ForEach-Object { [double]$_ });$q=@($values[$key][$k] | ForEach-Object { [double]$_ })
                    $mp=($p | Measure-Object -Average).Average;$mq=($q | Measure-Object -Average).Average
                    if($mp-$mq -le 2*[Math]::Sqrt((Variance $p $mp)/$p.Count+(Variance $q $mq)/$q.Count)){ $reasons.Add("$key`: threshold $k kept at $($old[$k]): the new mean $mq is not significantly under $mp");$new[$k]=$old[$k] } } } }
            $base.thresholds=$new
            $calibration=[ordered]@{date=(Get-Date -Format 'yyyy-MM-dd HH:mm');native_tree=$tree;commit=$commit;path_object=$pathHash;runs=@($measures.run)
                values=[ordered]@{};picture_diffs=@($pd | ForEach-Object { ,@([Math]::Round($_[0],2),[Math]::Round($_[1],4)) });raise_decision=$AllowRaise
                formula='per count: max(mean + 3 sd / sqrt(2), mean + 3 sqrt(max(mean,1) / 2), highest run), rounded up; period: max(mean + 3 sd / sqrt(2), highest run) x 1.005, to the hundredth above; against the mean of two measure runs; pictures: three times the largest difference to the reference (at least 1.5 and 0.005)'}
            # Final review M1: each threshold keeps the numbers it comes from; a kept threshold keeps its earlier runs.
            $calibration.sources=[ordered]@{}
            foreach($k in $metricKeys){ if($old -and $null -ne $old[$k] -and $new[$k] -eq $old[$k] -and $base.calibration -and $base.calibration.values[$k]){
                    $calibration.values[$k]=@($base.calibration.values[$k]);$calibration.sources[$k]=$(if($base.calibration.sources -and $base.calibration.sources[$k]){$base.calibration.sources[$k]}else{[ordered]@{date=$base.calibration.date;runs=@($base.calibration.runs)}}) }
                else{ $calibration.values[$k]=@($values[$key][$k]);$calibration.sources[$k]=[ordered]@{date=$calibration.date;runs=@($measures.run)} } }   # copies (step review M2)
            $base.calibration=$calibration;$calibrations[$key]=$calibration } }
    $th=$base.thresholds;$means[$key]=[ordered]@{};$thresholdFails[$key]=0
    if(-not $th){ Inconclusive "$key`: no thresholds yet (run -Calibrate on that screen)" }elseif($measures.Count -ge 2){
        foreach($k in $metricKeys){ $v=@($values[$key][$k] | Where-Object { $null -ne $_ });if($v.Count -lt 2){ continue }
            if($null -eq $th[$k]){ Inconclusive "$key`: no threshold for $k yet (run -Calibrate)";continue }   # I-0097: a new metric judges nothing before its calibration
            $means[$key][$k]=($v | Measure-Object -Average).Average;if($means[$key][$k] -gt $th[$k]){ $thresholdFails[$key]++;Fail "$key`: $k mean $($means[$key][$k]) over its threshold $($th[$k]) (runs $($v -join ', '))" } }
        $ref=if($base.reference_picture){ "$repo/$($base.reference_picture.path)" }else{ '' }
        if(-not $ref -or -not (Test-Path -LiteralPath $ref) -or (Get-FileHash -LiteralPath $ref).Hash -ne $base.reference_picture.sha256){ Inconclusive "$key`: reference picture missing or changed" }
        else{ foreach($p in @($pictures | Where-Object { Test-Path -LiteralPath $_ })){ $d=ComparePicture $ref $p;$means[$key]["picture $(Split-Path -Leaf (Split-Path -Parent $p))"]=@([Math]::Round($d[0],2),[Math]::Round($d[1],4))
            if($d[0] -gt $th.picture_mean_diff -or $d[1] -gt $th.picture_cells_over){ Fail "$key`: picture $p departs from the reference (mean $([Math]::Round($d[0],2)) > $($th.picture_mean_diff) or cells $([Math]::Round($d[1],4)) > $($th.picture_cells_over))" } } } } }
# The memory leg (I-0085): a fault fails; an incomplete leg is inconclusive; a rise per visit over its threshold fails.
if($legValue){ $means['memory_mb_per_visit']=$legValue.mb_per_visit
    if($legValue.error){ Inconclusive "$($legValue.run): the memory leg could not be read ($($legValue.error))" }
    elseif($legValue.faults){ Fail "$($legValue.run): fault in the memory leg ($($legValue.faults))" }
    elseif(-not $legValue.closed){ Fail "$($legValue.run): the game was gone before its close (a crash in the memory leg)" }
    elseif((-not $Evaluate.Count -or $Calibrate) -and $moduleHash -and $legValue.module_sha256 -ne $moduleHash){ Inconclusive "$($legValue.run): its module is not this gate's build" }
    elseif(-not $legValue.restored){ Inconclusive "$($legValue.run): protected files not restored by hash" }
    elseif($null -eq $legValue.mb_per_visit){ Inconclusive "$($legValue.run): $($legValue.loads) loads and $(@($legValue.titles_after_visits).Count) title stays after a visit, eight needed" }
    elseif($null -eq $gate.memory_mb_per_visit){ Inconclusive 'no memory_mb_per_visit threshold' }
    elseif($legValue.mb_per_visit -gt $gate.memory_mb_per_visit){ Fail "$($legValue.run): private memory $($legValue.mb_per_visit) MB a visit over its threshold $($gate.memory_mb_per_visit) (titles after the visits $(@($legValue.titles_after_visits) -join ', '))" } }
$failing=@($thresholdFails.Keys | Where-Object { $thresholdFails[$_] })
if($round -eq 0 -and $verdict -eq 'FAIL' -and ($thresholdFails.Values | Measure-Object -Sum).Sum -eq @($reasons).Count -and -not $Evaluate.Count -and -not $Calibrate){
    "== confirmation: the measure thresholds alone failed on $($failing -join ', '); two more measure runs there"
    $more=[Collections.Generic.List[object]]::new();foreach($s in @($targets | Where-Object { $_.key -in $failing })){ foreach($i in 3,4){ $more.Add([pscustomobject]@{name="$Tag-$($s.key -replace '@','-')-m$i";kind='Measure';screen=$s}) } }
    RunPlan $more;continue }
break }
$result=[ordered]@{tag=$Tag;date=(Get-Date -Format 'yyyy-MM-dd HH:mm');verdict=$verdict;experiment=$experiment;calibration=[bool]$Calibrate;native_tree=$tree;commit=$commit;dirty=[bool]$dirty.Count
    build=$BuildFolder;extra=$ExtraSwitch;path_object=$pathHash;bases=$bases;runs=@(@($runs | ForEach-Object { $_.name })+@($memoryLeg | Where-Object { $_ }));values=$values;means=$means
    thresholds=[ordered]@{};reasons=@($reasons);measured=$measured;memory=$legValue}
foreach($key in $bases){ $result.thresholds[$key]=$gate.bases[$key].thresholds }
New-Item -ItemType Directory -Force "$repo/evidence/I-0083/gates" | Out-Null
$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$repo/evidence/I-0083/gates/$Tag.json" -Encoding utf8
$record=$verdict -eq 'PASS' -and -not $experiment -and -not $Calibrate -and -not $dirty.Count
if($record){ foreach($key in $bases){ $gate.bases[$key].last_pass=[ordered]@{date=$result.date;native_tree=$tree;commit=$commit;path_object=$pathHash
        runs=@(@($runs | Where-Object { $_.base -eq $key -or -not $_.base } | ForEach-Object { $_.name })+@($memoryLeg | Where-Object { $_ }));means=$means[$key];memory_mb_per_visit=$(if($legValue){$legValue.mb_per_visit}else{$null})} }
    $gate.path_object=$pathHash }
# Final review L4: a calibration whose own runs then fail (a picture) is not filed.
if($record -or ($calibrations.Count -and $verdict -eq 'PASS')){ $gate | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $gateFile -Encoding utf8 }
"replay gate $verdict ($Tag$(if($experiment){', experiment'})$(if($Calibrate){', calibration'})): " + (($bases | ForEach-Object { $k=$_;"$k " + (($means[$k].GetEnumerator() | ForEach-Object { "$($_.Key) $($_.Value -join '/')" }) -join '; ') }) -join ' | ') + $(if($legValue){" | memory $($legValue.mb_per_visit) MB a visit"}else{''})
$reasons
exit $(switch($verdict){ 'PASS'{0} 'FAIL'{1} default{2} })
