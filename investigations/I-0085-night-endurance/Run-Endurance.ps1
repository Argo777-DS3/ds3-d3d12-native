param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[string]$Build='',
    [ValidateRange(1,110)][int]$Minutes=95,[ValidateRange(20,300)][int]$StaySeconds=45,[switch]$GpuValidation,[switch]$Original,[switch]$Probe,[switch]$Once,[string]$NotBefore='',
    [string]$OnlyZones='',[ValidateRange(0,200)][int]$Visits=0,[ValidateRange(0,300)][int]$TitleSeconds=0,[string]$Screen='',[string]$GraphicsLevels='',[string]$EffectFlags='')   # -EffectFlags: Run-StartupTrace's (an A/B diagnostic; it removes stale flag files from the build folder, so a flagged copy of a build folder loses its flag, I-0113 tranche 22)
# I-0098: -Screen (a base key « 1920x1080@60 » or a device name, scripts/Get-Screens.ps1): the game placed on that screen
# (DS3_PILOT_SCREEN, Pilot-Game.ps1), recorded in screen.json; the primary screen without it.
# -OnlyZones and -Visits go to the tour (one zone loaded -Visits times: the replayed path gate's memory leg, scripts/Invoke-ReplayGate.ps1).
# I-0085: the night endurance run (an announced live run under D-0116). One process (the trace job dies with the shell that
# started it): Run-StartupTrace.ps1 in a job on the Shebang zone save (Alpha untouched), the title reached
# (Pilot-TitleContinue -MenuY 0), Pilot-EnduranceTour.ps1 (the five zones in a loop for -Minutes, or once each with -Once, every
# load checked in the journal), then the window closed and the job's restoration awaited (every protected file restored by
# hash). Measure mode without the debug layer (a fault at the quit dumped like any other since I-0099's step 4); or
# -GpuValidation (the debug layer with GPU-based validation, I-0084's deferred check at the witness zones; slow, so the tour's
# waits are stretched); or -Original (the original game, D3D11 and uninstrumented, the reference: no journal, so the tour runs
# -Blind). Every run samples the process from outside every 5 s into memory.csv (private bytes, working set, handles,
# threads), the same for both renderers. The trace's bound is the tour's time plus the start and the close.
# I-0097 tranche 2: -Original -Probe -Build <folder> runs the original with the module in Reference mode and original-probe.flag (the
# original's D3D11 renderer, only its pacer and world update gate observed), so its journal dates each load's end as the native one
# does (game_step_resumed); the tour stays -Blind (no native pacing rows). Get-I97LoadTimes.ps1 reads both.
# I-0113 tranche 23: with ffx-draw-observe in -EffectFlags, no original-probe.flag (the module's original-probe setup returns before
# the effect witnesses): plain Reference mode, the original's renderer with those witnesses, no game_step_resumed rows.
$ErrorActionPreference='Stop'
if($Original -and $GpuValidation){ throw '-Original takes no validation layer' }
if(-not $Original -and -not $Build){ throw 'a native run needs -Build' }
if($Probe -and -not ($Original -and $Build)){ throw '-Probe goes with -Original and -Build (the module''s Reference mode)' }
if($EffectFlags -and $Original -and -not $Probe){ throw '-EffectFlags needs the module: a native run or -Original -Probe' }
# -NotBefore HH:mm: today, or tomorrow when that time is more than twelve hours past (00:30 given at 23:00).
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null); if($at -lt (Get-Date).AddHours(-12)){ $at=$at.AddDays(1) }; while((Get-Date) -lt $at){ Start-Sleep 5 } }
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
$save='C:/Users/User/.ds3-i0039-backups/shebang-zones-final-20261013-045344/DS30000.sl2';$hash='78948904F13ADD2F1AC79015F9F01F171D5B4F0F01A0A716CE12F884B94265B8'
$previousScreen=$env:DS3_PILOT_SCREEN   # step review: restored at the end (a caller's shell keeps no offset)
$target=if($Screen){ & "$repo/scripts/Get-Screens.ps1" -Key $Screen }else{ @(& "$repo/scripts/Get-Screens.ps1" | Where-Object primary)[0] }
if($Screen){ $env:DS3_PILOT_SCREEN=$target.device }else{ Remove-Item Env:DS3_PILOT_SCREEN -ErrorAction SilentlyContinue }
$scale=if($GpuValidation){4}else{1}
# -Once: five cycles of the stay plus about 120 s each (scaled); otherwise the tour's minutes. Start, a slow last cycle and the
# close: 10 minutes (scaled), so the controller's bound does not end the game before its close.
$tour=if($Visits){ [int]($Visits*($StaySeconds+120+$TitleSeconds)*$scale) }elseif($Once){ [int](5*($StaySeconds+120)*$scale) }else{ $Minutes*60 }
$seconds=$tour+[int](600*$scale)+$(if($Visits){300}else{0})
if($seconds -gt 7200){ throw "the run needs about $seconds s, over the controller's 7200 s: lower -Minutes" }
$tourMinutes=[int][Math]::Ceiling($tour/60)+$(if($Visits){5}else{0})   # -Visits: the count ends the tour, the deadline only bounds it
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$save,$hash,$build,$gbv,$orig,$probe,$levels,$flags)
    $mode=@(if($orig -and $probe){ '-BootstrapMode','Reference','-BuildFolder',$build,'-EffectFlags',$(if($flags -match 'ffx-draw-observe'){$flags}elseif($flags){"original-probe,$flags"}else{'original-probe'}) }elseif($orig){ '-BootstrapMode','Original' }elseif($gbv){ '-BootstrapMode','Native','-BuildFolder',$build,'-GpuValidation' }else{ '-BootstrapMode','Native','-BuildFolder',$build,'-Measure','-NoDebugLayer' })
    if($flags -and -not $orig){ $mode+='-EffectFlags',$flags }   # with -Original, only -Probe takes them (checked above)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -TestSave $save -TestSaveSha256 $hash @mode @(if($levels){'-GraphicsLevels',$levels}) 2>&1 } -ArgumentList $inv,$RunName,$seconds,$save,$hash,$Build,($GpuValidation.IsPresent),($Original.IsPresent),($Probe.IsPresent),$GraphicsLevels,$EffectFlags
$o="$repo/evidence/I-0066/artifacts/$RunName";$deadline=(Get-Date).AddSeconds(120)
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
"run $RunName start $([DateTime]::UtcNow.ToString('o')) seconds=$seconds tour_minutes=$tourMinutes gbv=$([bool]$GpuValidation) original=$([bool]$Original)"
$sampler=$null
try {
    # The native run's journal, or the original's game process (Run-StartupTrace has made the run folder by then).
    while(-not ($(if($Original){ (Alive) -and (Test-Path -LiteralPath $o) }else{ Test-Path -LiteralPath "$o/startup.jsonl" })) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    if(-not (Alive) -or -not (Test-Path -LiteralPath $o)){ throw "no game: $((Receive-Job $job -Keep | Select-Object -Last 3) -join ' | ')" }
    [ordered]@{requested=$Screen;key=$target.key;device=$target.device;primary=$target.primary;x=$target.x;y=$target.y
        width=$target.width;height=$target.height;hz=$target.hz} | ConvertTo-Json | Set-Content -LiteralPath "$o/screen.json" -Encoding utf8
    # I-0096: and the process's GPU memory counters (MB, the same for both renderers): dedicated and shared usage, local and
    # non-local usage, total committed (video memory is charged to the private bytes on this PC, I-0096 O1). A query takes
    # about 1.4 s, so a row comes about every 6.5 s.
    $sampler=Start-ThreadJob -ArgumentList "$o/memory.csv" -ScriptBlock { param($path)
        'utc,tick,private_mb,working_mb,handles,threads,gpu_dedicated_mb,gpu_shared_mb,gpu_local_mb,gpu_nonlocal_mb,gpu_committed_mb' | Set-Content -LiteralPath $path
        $names='Dedicated Usage','Shared Usage','Local Usage','Non Local Usage','Total Committed'
        while($p=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1){
            $samples=(Get-Counter -Counter ($names | ForEach-Object { "\GPU Process Memory(pid_$($p.Id)_*)\$_" }) -ErrorAction SilentlyContinue).CounterSamples
            $gpu=foreach($n in $names){ $s=$samples | Where-Object { $_.Path -like "*\$n" }
                if($s){ [int](($s | Measure-Object CookedValue -Sum).Sum/1MB) }else{ '' } }
            "$([DateTime]::UtcNow.ToString('o')),$([Environment]::TickCount64),$([int]($p.PrivateMemorySize64/1MB)),$([int]($p.WorkingSet64/1MB)),$($p.HandleCount),$($p.Threads.Count),$($gpu -join ',')" | Add-Content -LiteralPath $path
            Start-Sleep -Seconds 5 } }
    $delay=if($Original){ 6000 }else{ [int](3000*$scale) }   # the original's main menu comes later (Pilot-TitleContinue)
    pwsh -NoProfile -File "$inv/Pilot-TitleContinue.ps1" -Trace 'none' -MenuY 0 -TimeoutSeconds ([int][Math]::Min(600,240*$scale)) -ContinueDelayMs $delay 2>&1 | Select-Object -Last 1
    New-Item -ItemType Directory -Force -Path "$o/shots" | Out-Null
    $a=@{Minutes=$tourMinutes;StaySeconds=$StaySeconds;ShotDir="$o/shots";TimeScale=$scale}
    if($Original){ $a.Blind=$true }else{ $a.Journal="$o/startup.jsonl" }
    if($Once){ $a.Once=$true }
    if($OnlyZones){ $a.OnlyZones=$OnlyZones };if($Visits){ $a.Visits=$Visits };if($TitleSeconds){ $a.TitleSeconds=$TitleSeconds }
    & "$PSScriptRoot/Pilot-EnduranceTour.ps1" @a 2>&1 | Tee-Object -FilePath "$o/tour.txt"
} catch { "pilot error: $_" }
finally {
    # The window's close box, after the tour or a pilot error alike (a fault at the quit, if it comes, is dumped like any other):
    # posted as WM_SYSCOMMAND SC_CLOSE, as the start check does, at any size or mode (-GraphicsLevels; WORKFLOW, 2026-11-06: the
    # OLED's tests in a 2560x1440 window; the tour itself not yet tried at that size, its inputs were off: the open-gaps pilot's
    # scaled Pilot-Game, 3722fcdc, then the tour's pixel tests; this close not yet run). close.txt: whether the game was still there for its close
    # (the gate's memory leg fails on a game gone before it).
    if(Alive){ Add-Type -Namespace Endurance -Name User32 -MemberDefinition '[DllImport("user32.dll")] public static extern bool PostMessage(System.IntPtr h,uint m,System.IntPtr w,System.IntPtr l);'
        $game=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1
        $posted=$game -and $game.MainWindowHandle -ne [IntPtr]::Zero -and [Endurance.User32]::PostMessage($game.MainWindowHandle,0x0112,[IntPtr]0xF060,[IntPtr]::Zero)
        $line="$(if($posted){'close clicked (SC_CLOSE posted)'}else{'close not posted'}) $([DateTime]::UtcNow.ToString('o'))" }   # the gate's memory leg reads « close clicked »
    else{ $line="game gone before the close $([DateTime]::UtcNow.ToString('o'))" }
    $line;if(Test-Path -LiteralPath $o){ Set-Content -LiteralPath "$o/close.txt" -Value $line } }
Wait-Job $job -Timeout ($seconds+300) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 3
if($sampler){ Wait-Job $sampler -Timeout 30 | Out-Null; Remove-Job $sampler -Force }
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
if($previousScreen){ $env:DS3_PILOT_SCREEN=$previousScreen }else{ Remove-Item Env:DS3_PILOT_SCREEN -ErrorAction SilentlyContinue }
