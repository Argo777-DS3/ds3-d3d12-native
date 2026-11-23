param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$Build,
    [ValidateSet('Measure','Validation','Reference')][string]$Kind='Measure',[string]$PathFile='',
    # Experiments only (never a gate record): Run-StartupTrace switches for a known-worse configuration, e.g. InlineRender.
    [ValidateSet('InlineRender','InlinePrepare','NoPersistentDraws','NoModelRecords','GpuValidation','NoWaitablePresent','OneFrameSlot')][string[]]$ExtraSwitch=@(),
    # I-0084: Run-StartupTrace effect flags, e.g. no-pipeline-library or sample-profile (experiments only, never a gate record);
    # I-0098: frame-dump is the gate's own (its measure runs' relative counts).
    [string]$EffectFlags='',[ValidateRange(0,65536)][int]$VideoMemoryBudgetMb=0,
    # I-0098: the screen the game runs on, a base key (« 1920x1080@60 », scripts/Get-Screens.ps1) or a device name; the primary
    # screen without it. The pilots place the window there (DS3_PILOT_SCREEN, Pilot-Game.ps1); screen.json records it.
    [string]$Screen='',[switch]$PresentMon,
    # I-0097 tranche 3 (experiment): with -Screen, the native module places the window on that screen before making its swap chain
    # (display-screen.txt next to the module, removed after the run), instead of the pilot moving it at the title.
    [switch]$LaunchOnScreen,
    # I-0097 tranche 3 (experiment, never a gate record): Run-StartupTrace -GraphicsLevels for the run, e.g. the screen mode
    # « ScreenMode=FULLSCREEN;Resolution-FullScreenWidth=1920;Resolution-FullScreenHeight=1080 » (restored by hash afterwards).
    [string]$GraphicsLevels='')
# I-0083: one run of the replayed path (D-0200) under D-0116 (an announced live run: offline, protected files restored by hash,
# nothing installed in the game folder). The save named by the path file is installed as the test save; the title is passed
# (Pilot-TitleContinue), the character settles for the path's settle_seconds, the standstill picture is taken (start.png, the
# gate's picture, with the game's client rectangle and whether it was in front in start-client.txt), Invoke-ReplayPath plays
# the path, a picture after it (end.png), then the game is closed. Kinds: Measure (measure mode, no debug layer: the gate's
# numbers), Validation (the debug layer: a D3D12 error ends the native run), Reference (the original with original-probe.flag:
# its present pacer and, since I-0083, its game step at the world update gate).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot;$inv="$repo/investigations/I-0066-independent-native-startup"
if(-not $PathFile){ $PathFile="$PSScriptRoot/replay/firelink.path.json" }
$recording=Get-Content -LiteralPath $PathFile -Raw | ConvertFrom-Json
$motion=0;foreach($s in $recording.segments){ $motion+=[int]$s.cycles*[int]$s.cycle_ms };$motion=[int][Math]::Ceiling($motion/1000.0)
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ throw 'DarkSoulsIII is already running' }
$previousScreen=$env:DS3_PILOT_SCREEN   # step review: restored at the end (a caller's shell keeps no offset)
$target=if($Screen){ & "$PSScriptRoot/Get-Screens.ps1" -Key $Screen }else{ @(& "$PSScriptRoot/Get-Screens.ps1" | Where-Object primary)[0] }
if($Screen){ $env:DS3_PILOT_SCREEN=$target.device }else{ Remove-Item Env:DS3_PILOT_SCREEN -ErrorAction SilentlyContinue }
Add-Type @'
using System;using System.Runtime.InteropServices;
public static class ReplayWindow{
 [StructLayout(LayoutKind.Sequential)]public struct RECT{public int L,T,R,B;}
 [StructLayout(LayoutKind.Sequential)]public struct PT{public int X,Y;}
 [DllImport("user32.dll")]static extern bool GetClientRect(IntPtr h,out RECT r);
 [DllImport("user32.dll")]static extern bool ClientToScreen(IntPtr h,ref PT p);
 [DllImport("user32.dll")]static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")]static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 // "x y width height front" of the game's client area on the screen.
 public static string Client(IntPtr h,uint gamePid){RECT r;PT p=new PT();if(h==IntPtr.Zero||!GetClientRect(h,out r)||!ClientToScreen(h,ref p))return "";
   uint pid;GetWindowThreadProcessId(GetForegroundWindow(),out pid);return p.X+" "+p.Y+" "+r.R+" "+r.B+" "+(pid==gamePid?"front":"behind");}
}
'@
# The trace's budget: start-up, the settle, the motion and 150 s for the title, the pictures and the close (step review L6).
$seconds=60+[int]$recording.settle_seconds+$motion+150
# I-0097: a Reference run keeps its own effect flag (original-probe), plus frame-dump (every frame's time in the timing rows) only;
# tranche 4 (experiments, never a gate record): the sampler on the original's threads too (sample-profile, sample-ds3-threads).
# Tranche 5 (D-0213, Thomas): test-no-ai (the study paths' enemies without AI, both renderers; never a gate record; I-0114 tranche 8: test-freeze-ai, their moves and AI step stopped too); step 3: cadence-trace (the frame timeline, diagnostics only). I-0100 T0: popin-observe (per-object rows of what each renderer draws, diagnostics only); tranche 2: light-observe (the instance ambient rows, diagnostics only); tranche 5: caster-place (the shadow passes' casters by place, diagnostics only).
$referenceFlags=@($EffectFlags -split '[;,]' | Where-Object { $_ })
if($Kind -eq 'Reference' -and (@($referenceFlags | Where-Object { $_ -notin 'frame-dump','sample-profile','sample-ds3-threads','test-no-ai','test-freeze-ai','cadence-trace','popin-observe','light-observe','caster-place' }).Count -or $VideoMemoryBudgetMb)){ throw 'a Reference run keeps its own effect flag (original-probe) and at most frame-dump, the sampler, test-no-ai, test-freeze-ai, cadence-trace, popin-observe, light-observe and caster-place, and no ballast' }
if($Kind -eq 'Measure' -and $ExtraSwitch -contains 'GpuValidation'){ throw 'GPU-based validation needs the debug layer: a Validation run' }
# I-0097 tranche 4 (Thomas, 2026-10-25 00:52: a launch check before every announced test): the module and its controller in
# the build folder (i97-t4-final: Windows Defender removed a gate's module between two runs), the test save at its recorded hash;
# after the settle, the module loaded in the game and, for a native run, the game on the target screen (its presenter's refresh
# rate), else the run stops before the path (launch-check.json, the replay « launch_check_failed »).
$check=[ordered]@{module=$true;controller=$true;save=$true}
$check.module=Test-Path -LiteralPath (Join-Path $Build 'game_bootstrap.dll');$check.controller=Test-Path -LiteralPath (Join-Path $Build 'game_bootstrap_ctl.exe')
$check.save=(Test-Path -LiteralPath $recording.save.path) -and (Get-FileHash -LiteralPath $recording.save.path).Hash -eq $recording.save.sha256
if(-not ($check.module -and $check.controller -and $check.save)){ throw "launch check before $RunName failed: $(($check.GetEnumerator() | Where-Object { -not $_.Value } | ForEach-Object Key) -join ', ') missing or changed" }
$launchScreen=Join-Path $Build 'display-screen.txt'
# Step review: a file left by an interrupted run never moves a later one.
Remove-Item -LiteralPath $launchScreen -ErrorAction SilentlyContinue
if($LaunchOnScreen){ if(-not $Screen -or $Kind -eq 'Reference'){ throw '-LaunchOnScreen needs -Screen and the native module' };Set-Content -LiteralPath $launchScreen -Value $target.device -Encoding ascii }
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$save,$hash,$build,$kind,$extra,$flags,$budget,$levels)
    $a=@{};if($kind -eq 'Reference'){ $a.BootstrapMode='Reference';$a.EffectFlags='original-probe' }else{ $a.BootstrapMode='Native' }
    if($kind -eq 'Measure'){ $a.Measure=$true;$a.NoDebugLayer=$true }
    if($flags){ $a.EffectFlags=$(if($kind -eq 'Reference'){"original-probe;$flags"}else{$flags}) }
    if($budget){ $a.VideoMemoryBudgetMb=$budget }
    if($levels){ $a.GraphicsLevels=$levels }
    foreach($x in @($extra | Where-Object { $_ })){ $a[$x]=$true }
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -TestSave $save -TestSaveSha256 $hash -BuildFolder $build @a 2>&1 } `
    -ArgumentList $inv,$RunName,$seconds,$recording.save.path,$recording.save.sha256,$Build,$Kind,$ExtraSwitch,$EffectFlags,$VideoMemoryBudgetMb,$GraphicsLevels
$o="$repo/evidence/I-0066/artifacts/$RunName";$deadline=(Get-Date).AddSeconds(120)
function P([string[]]$a){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
$replay='not_started'
try {
    while(-not (Test-Path -LiteralPath "$o/startup.jsonl") -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    if(Test-Path -LiteralPath $o){ [ordered]@{requested=$Screen;key=$target.key;device=$target.device;primary=$target.primary;x=$target.x;y=$target.y
        width=$target.width;height=$target.height;hz=$target.hz} | ConvertTo-Json | Set-Content -LiteralPath "$o/screen.json" -Encoding utf8 }
    # I-0098: -PresentMon, what the screen showed (presentmon.csv: Intel's PresentMon 2.6.0 in C:/Users/User/Tools/PresentMon,
    # its 1.x columns with each frame's performance counter, QPCTime), from the title to the game's exit, read by
    # Get-ReplayMeasure for both renderers alike; presentmon-clock.txt pairs the tick count (the marks' clock) with the counter
    # (its ticks, its frequency). Its ETW session needs the Performance Log Users group (Thomas, 2026-10-24) or elevation;
    # without them it leaves no CSV and its messages in presentmon.log. NVIDIA FrameView's copy does not run outside its service
    # (i98-c1), and PresentMon's --date_time is not local time (two hours ahead on 2026-10-24).
    if($PresentMon -and (Test-Path -LiteralPath $o)){ $pm='C:/Users/User/Tools/PresentMon/PresentMon-2.6.0-x64.exe'
        Set-Content -LiteralPath "$o/presentmon-clock.txt" -Value "$([Environment]::TickCount64) $([Diagnostics.Stopwatch]::GetTimestamp()) $([Diagnostics.Stopwatch]::Frequency)"
        $pmProcess=Start-Process -PassThru -FilePath $pm -NoNewWindow -RedirectStandardOutput "$o/presentmon.log" -RedirectStandardError "$o/presentmon-errors.log" -ArgumentList `
            "--process_name DarkSoulsIII.exe --output_file `"$o\presentmon.csv`" --v1_metrics --qpc_time --no_console_stats --terminate_on_proc_exit --session_name DS3Replay --stop_existing_session" }
    pwsh -NoProfile -File "$inv/Pilot-TitleContinue.ps1" -Trace 'none' 2>&1 | Select-Object -Last 1
    Start-Sleep ([int]$recording.settle_seconds)
    # I-0098: the client rectangle relative to the captured screen (the game's), as the picture is.
    if(Alive){ P '-Action','shot','-Path',"$o/start.png";$g=Get-Process DarkSoulsIII | Select-Object -First 1
        $c=([ReplayWindow]::Client($g.MainWindowHandle,[uint32]$g.Id) -split ' ')
        # Step review: a client outside the captured screen (the window not placed) is recorded empty, so the gate is inconclusive.
        $x=if($c.Count -ge 5){ [int]$c[0]-$target.x }else{ -1 };$y=if($c.Count -ge 5){ [int]$c[1]-$target.y }else{ -1 }
        Set-Content -LiteralPath "$o/start-client.txt" -Value $(if($x -ge 0 -and $y -ge 0 -and $x+[int]$c[2] -le $target.width -and $y+[int]$c[3] -le $target.height){ "$x $y $($c[2]) $($c[3]) $($c[4])" }else{ '' }) }
    # The launch check after the settle (above): the module in the game's process; a native run's presenter at the target
    # screen's rate (its latest display_refreshes row, within 1 Hz).
    $launched=$true
    if(Alive){ $g=Get-Process DarkSoulsIII | Select-Object -First 1;$loaded=$false;try{ $loaded=[bool](@($g.Modules | Where-Object { $_.ModuleName -eq 'game_bootstrap.dll' }).Count) }catch{}
        $hz=$null;if($Kind -ne 'Reference' -and (Test-Path -LiteralPath "$o/startup.jsonl")){ $row=@(Select-String -LiteralPath "$o/startup.jsonl" -Pattern '"event":"display_refreshes","renderer":"native".*"refresh_hz":(\d+)' | Select-Object -Last 1)
            if($row.Count){ $hz=[int]$row[0].Matches[0].Groups[1].Value } }
        $onScreen=$Kind -eq 'Reference' -or ($null -ne $hz -and [Math]::Abs($hz-[int]$target.hz) -le 1)
        [ordered]@{module_loaded=$loaded;presenter_hz=$hz;target_hz=$target.hz;on_screen=$onScreen} | ConvertTo-Json | Set-Content -LiteralPath "$o/launch-check.json" -Encoding utf8
        if(-not ($loaded -and $onScreen)){ $launched=$false;$replay='launch_check_failed';"launch check after the settle failed: module loaded $loaded, presenter $hz Hz for a $($target.hz) Hz screen" } }
    if($launched -and (Alive)){ $r=pwsh -NoProfile -File "$PSScriptRoot/Invoke-ReplayPath.ps1" -Path $PathFile -Marks "$o/motion-marks.txt" -Log "$o/replay.log" 2>&1;$replay=($r | Select-Object -Last 1) }
    if(Alive){ P '-Action','shot','-Path',"$o/end.png" }
    if(Alive){ P '-Action','click','-X','1265','-Y','15' }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout ($seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 3
# i98-c2-o60: PresentMon outlived the game once; it is stopped here (its CSV is written as it goes).
if($pmProcess -and -not $pmProcess.HasExited){ if(-not $pmProcess.WaitForExit(15000)){ Stop-Process -Id $pmProcess.Id -Force -ErrorAction SilentlyContinue } }
if($LaunchOnScreen){ Remove-Item -LiteralPath $launchScreen -ErrorAction SilentlyContinue }
$restoration="$o/restoration.json"
"$replay; restoration $(if(Test-Path -LiteralPath $restoration){(Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok}else{'missing'})"
if($previousScreen){ $env:DS3_PILOT_SCREEN=$previousScreen }else{ Remove-Item Env:DS3_PILOT_SCREEN -ErrorAction SilentlyContinue }
