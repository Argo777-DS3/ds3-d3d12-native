param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$Build,
    [ValidateSet('Native','Reference')][string]$Kind='Native',[string]$EffectFlags='',[string]$Screen='1920x1080@60',
    [Parameter(Mandatory)][string]$Commands,[ValidateRange(300,1800)][int]$Seconds=900,[ValidateRange(10,120)][int]$SettleSeconds=40,
    [string]$SaveFile='C:/Users/User/.ds3-i0039-backups/shebang-zones-gamma-m37-anorlondo-20261026-123818/DS30000.sl2',
    [string]$SaveSha256='E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411')
# I-0115 (Thomas, 2026-11-02: the enemies that flicker, the original against the native, « screenshots à volonté »): one announced
# live run (D-0116: offline, protected files restored by hash by Run-StartupTrace's job) steered from outside while it runs. As
# Invoke-ReplayRun: the test save installed, the title passed, the settle, start.png; then the commands appended to -Commands
# (one a line, read as they appear, each logged with its time in commands.log) are played in order until « end », the trace's
# budget or the game's exit; then the game is closed and the restoration awaited. All in this one process (LIVE-RUN-NOTES: a
# job dies with the pwsh that started it). Commands: « shot <name> » (<name>.png, the game's screen), « key <vk> », « hold <vk>
# <ms> », « click <x> <y> » / « rclick <x> <y> » (screen pixels), « look <dx> <dy> », « wait <s> », « path <path file> »
# (Invoke-ReplayPath, its marks and log numbered), « video start <name> » / « video stop » (ffmpeg gdigrab of the window's
# rectangle at 60 frames/s into <name>.mp4, as Run-ZoneWalk's -Video; its start's tick in commands.log), « end ».
# Reference runs keep original-probe (its pacer and world gate, the popin rows) and Invoke-ReplayRun's allowed flags.
# Native runs use Invoke-ReplayRun's Measure kind (measure mode, no debug layer; nothing is measured). Start it in the background
# (run_in_background or a hidden pwsh), never as a foreground tool call: the restoration lives in the job, which dies with this
# pwsh, and a foreground call is capped at 10 minutes while the trace's budget is -Seconds (step review L5).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ throw 'DarkSoulsIII is already running' }
# Final review L4: a reused run name would read the former run's journal and restoration.
if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$RunName"){ throw "run $RunName already exists" }
if(-not ((Test-Path -LiteralPath (Join-Path $Build 'game_bootstrap.dll')) -and (Test-Path -LiteralPath (Join-Path $Build 'game_bootstrap_ctl.exe')))){ throw "no module in $Build" }
if(-not ((Test-Path -LiteralPath $SaveFile) -and (Get-FileHash -LiteralPath $SaveFile).Hash -eq $SaveSha256)){ throw 'the test save is missing or changed' }
$flags=@($EffectFlags -split '[;,]' | Where-Object { $_ })
if($Kind -eq 'Reference' -and @($flags | Where-Object { $_ -notin 'frame-dump','test-no-ai','test-freeze-ai','cadence-trace','popin-observe','light-observe','caster-place' }).Count){ throw 'a Reference run keeps original-probe and at most frame-dump, test-no-ai, test-freeze-ai, cadence-trace, popin-observe, light-observe and caster-place' }
$target=& "$repo/scripts/Get-Screens.ps1" -Key $Screen;$previousScreen=$env:DS3_PILOT_SCREEN;$env:DS3_PILOT_SCREEN=$target.device
if(-not (Test-Path -LiteralPath $Commands)){ Set-Content -LiteralPath $Commands -Value '' -Encoding utf8 }
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$save,$hash,$build,$kind,$flags)
    $a=@{};if($kind -eq 'Reference'){ $a.BootstrapMode='Reference';$a.EffectFlags='original-probe' }else{ $a.BootstrapMode='Native';$a.Measure=$true;$a.NoDebugLayer=$true }   # Invoke-ReplayRun's Measure kind
    if($flags){ $a.EffectFlags=$(if($kind -eq 'Reference'){"original-probe;$flags"}else{$flags}) }
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -TestSave $save -TestSaveSha256 $hash -BuildFolder $build @a 2>&1 } `
    -ArgumentList $inv,$RunName,$Seconds,$SaveFile,$SaveSha256,$Build,$Kind,$EffectFlags
$o="$repo/evidence/I-0066/artifacts/$RunName";$log="$o/commands.log"
function P([string[]]$a){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" @a | Select-Object -Last 1 }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
function Note([string]$t){ $line="$(Get-Date -Format HH:mm:ss.fff) tick=$([Environment]::TickCount64) $t";$line;if(Test-Path -LiteralPath $o){ Add-Content -LiteralPath $log -Value $line } }
$recorder=$null
function Stop-Recorder{ try{ if($script:recorder -and -not $script:recorder.HasExited){ try{ $script:recorder.StandardInput.Write('q');$script:recorder.StandardInput.Close() }catch{}
    if(-not $script:recorder.WaitForExit(20000)){ $script:recorder.Kill();[void]$script:recorder.WaitForExit(5000);Note 'video killed (unreadable)' }else{ Note "video stopped exit=$($script:recorder.ExitCode)" } }
    $script:recorder=$null }catch{ Note "video stop error: $_" } }
$deadline=(Get-Date).AddSeconds($Seconds-60);$paths=0
try {
    $until=(Get-Date).AddSeconds(120);while(-not (Test-Path -LiteralPath "$o/startup.jsonl") -and (Get-Date) -lt $until -and $job.State -eq 'Running'){ Start-Sleep 2 }
    pwsh -NoProfile -File "$inv/Pilot-TitleContinue.ps1" -Trace 'none' 2>&1 | Select-Object -Last 1
    Start-Sleep $SettleSeconds
    if(Alive){ Note "shot start $(P '-Action','shot','-Path',"$o/start.png")" }
    $done=0
    while((Alive) -and (Get-Date) -lt $deadline){
        $lines=@(Get-Content -LiteralPath $Commands -ErrorAction SilentlyContinue | Where-Object { $_.Trim() })
        if($lines.Count -le $done){ Start-Sleep -Milliseconds 500;continue }
        $c=$lines[$done].Trim();$done++;$w=$c -split '\s+';Note "> $c"
        try{ switch($w[0]){
            'shot'{ Note (P '-Action','shot','-Path',"$o/$($w[1]).png") }
            'key'{ P '-Action','key','-Vk',$w[1] | Out-Null }
            'hold'{ P '-Action','hold','-Vk',$w[1],'-Ms',$w[2] | Out-Null }
            'click'{ P '-Action','click','-X',$w[1],'-Y',$w[2] | Out-Null }
            'rclick'{ P '-Action','rclick','-X',$w[1],'-Y',$w[2] | Out-Null }
            'look'{ P '-Action','look','-X',$w[1],'-Y',$w[2] | Out-Null }
            'wait'{ $until=(Get-Date).AddMilliseconds([double]$w[1]*1000);while((Get-Date) -lt $until -and (Get-Date) -lt $deadline -and (Alive)){ Start-Sleep -Milliseconds 100 } }
            'path'{ $paths++;$r=pwsh -NoProfile -File "$repo/scripts/Invoke-ReplayPath.ps1" -Path $c.Substring(5).Trim() -Marks "$o/motion-marks-$paths.txt" -Log "$o/replay-$paths.log" 2>&1;Note "path $paths $($r | Select-Object -Last 1)" }
            'video'{ if($w[1] -eq 'stop'){ Stop-Recorder }elseif(-not $recorder){ $info=P '-Action','shot','-Path',"$o/$($w[2])-first.png"
                $ff=(Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
                if(-not $ff){ $ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages/Gyan.FFmpeg_*/ffmpeg-*/bin/ffmpeg.exe" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName }
                if("$info" -match 'rect=(-?\d+),(-?\d+),(-?\d+),(-?\d+)' -and [int]$Matches[3] -gt [int]$Matches[1] -and $ff){ $L=[int]$Matches[1];$T=[int]$Matches[2];$vw=([int]$Matches[3]-$L) -band -2;$vh=([int]$Matches[4]-$T) -band -2
                    $psi=[Diagnostics.ProcessStartInfo]::new($ff,"-hide_banner -loglevel error -y -f gdigrab -framerate 60 -draw_mouse 0 -offset_x $L -offset_y $T -video_size ${vw}x$vh -i desktop -t 300 -c:v libx264 -preset ultrafast -crf 16 -pix_fmt yuv420p `"$o/$($w[2]).mp4`"")
                    $psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardInput=$true
                    $recorder=[Diagnostics.Process]::Start($psi);Note "video start $($w[2]) rect=$L,$T,${vw}x$vh" }else{ Note "video: no window rectangle or no ffmpeg ($info)" } } }
            'end'{ $deadline=Get-Date }
            default{ Note "unknown command $($w[0])" } } }catch{ Note "command error: $_" } }
    Stop-Recorder
    if(Alive){ Note "shot end $(P '-Action','shot','-Path',"$o/end.png")";P '-Action','click','-X','1265','-Y','15' | Out-Null }
} catch { "pilot error: $_" }
Stop-Recorder
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 3
$restoration="$o/restoration.json"
"done; restoration $(if(Test-Path -LiteralPath $restoration){(Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok}else{'missing'})"
if($previousScreen){ $env:DS3_PILOT_SCREEN=$previousScreen }else{ Remove-Item Env:DS3_PILOT_SCREEN -ErrorAction SilentlyContinue }
