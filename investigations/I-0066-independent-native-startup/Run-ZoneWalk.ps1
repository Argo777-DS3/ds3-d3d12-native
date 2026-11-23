[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[ValidateRange(60,900)][int]$Seconds=240,
    [ValidateSet('Native','Reference')][string]$BootstrapMode='Native',[string]$EffectFlags='',
    [Parameter(Mandatory)][string]$TestSave,[Parameter(Mandatory)][string]$TestSaveSha256,[string]$GraphicsLevels='',
    [ValidatePattern('^([A-Z][0-9]{2,4})?(,[A-Z][0-9]{2,4})*$')][string]$KeyPath='',[string]$Look='',[string]$Teleport='',[ValidateRange(20,300)][int]$SettleSeconds=40,
    [ValidateRange(0,10)][int]$Shots=3,[string]$HideTextures='',
    # Fable review: the observation window (Run-StartupTrace -WindowWidth/-WindowHeight) and -LoadShots n shots 2 s apart right
    # after Continue (load-<n>.png: the loading screen).
    [ValidateRange(640,3840)][int]$WindowWidth=1280,[ValidateRange(360,2160)][int]$WindowHeight=720,[ValidateRange(0,10)][int]$LoadShots=0,
    # Optimisation review: a measurement walk (Run-StartupTrace -Measure -NoDebugLayer -NoKProfile: the launcher's play configuration).
    [switch]$Measure,[switch]$NoDebugLayer,[switch]$NoKProfile,[ValidateRange(0,600)][int]$StaySeconds=0,   # -StaySeconds: wait before closing the window (a steady measurement after the walk)
    # Optimisation review: a session's own build folder (Build-GameBootstrap -Out), passed to Run-StartupTrace; the witness files go next to that module.
    [string]$BuildFolder='',
    # I-0113 (a held item seen frame by frame, the Estus Flask): -Video records the game's window with ffmpeg (as Run-AlphaCast's,
    # gdigrab over the window's rectangle, hidden) from before -KeyPath to the end of the shots into walk.mp4, each key step's
    # offset in the video printed ("step k ... video_ms=").
    [switch]$Video,[ValidateRange(10,60)][int]$VideoFps=60)
# Native water (thread « Sang sur persos, eau, siège », 2026-10-18): one announced run on a hash-recorded zone save, as one process:
# Run-StartupTrace.ps1 in a job (it restores every protected file by hash), Pilot-TitleContinue.ps1 (Continue: the save's last
# character), -SettleSeconds in game, then the optional -Teleport "x y z [hold]" (witness-teleport.txt, both modes), -Look "dx,dy[;dx,dy...]"
# (mouse) and -KeyPath "Q1500,Z800" (keys held in order: AZERTY Z forward, Q left, S back, D right; J/L/I/K the camera), a shot
# after each step and -Shots more 4 s apart (walk-<n>.png), then -HideTextures "frag1,frag2" (each written to hide-texture.txt in
# turn, a shot hide-<i>.png 4 s later, the file removed at the end and a shot unhidden.png), then the window closed and the job's
# restoration awaited.
$ErrorActionPreference='Stop'
$d=$PSScriptRoot;$repo=Split-Path -Parent (Split-Path -Parent $d)
$extra=@();if($EffectFlags){$extra+='-EffectFlags',$EffectFlags};if($GraphicsLevels){$extra+='-GraphicsLevels',$GraphicsLevels}
if($WindowWidth -ne 1280 -or $WindowHeight -ne 720){$extra+='-WindowWidth',"$WindowWidth",'-WindowHeight',"$WindowHeight"}
if($Measure){$extra+='-Measure'};if($NoDebugLayer){$extra+='-NoDebugLayer'};if($NoKProfile){$extra+='-NoKProfile'}
$build=if($BuildFolder){$extra+='-BuildFolder',$BuildFolder;$BuildFolder}else{Join-Path $repo 'evidence/I-0066/artifacts/game-bootstrap-build'}
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$mode,$save,$hash,$extra)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode $mode -TestSave $save -TestSaveSha256 $hash @extra 2>&1 } -ArgumentList $d,$RunName,$Seconds,$BootstrapMode,$TestSave,$TestSaveSha256,$extra
$o="$repo/evidence/I-0066/artifacts/$RunName";$trace="$o/startup.jsonl"
function P([string[]]$a){ pwsh -NoProfile -File "$d/Pilot-Game.ps1" @a | Out-Null }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
# -Video's recorder (Run-AlphaCast's): 'q' on its input closes the file; killed after 20 s. Never throws (the restoration must run).
$recorder=$null;$v0=$null;$recorderReported=$false
function Stop-Recorder{ try{ if($script:recorder -and -not $script:recorder.HasExited){ try{ $script:recorder.StandardInput.Write('q');$script:recorder.StandardInput.Close() }catch{}
    if(-not $script:recorder.WaitForExit(20000)){ $script:recorder.Kill();[void]$script:recorder.WaitForExit(5000);'video killed: walk.mp4 unreadable' }else{ "video stopped exit=$($script:recorder.ExitCode)" } }
    elseif($script:recorder -and -not $script:recorderReported){ "video had already ended exit=$($script:recorder.ExitCode)" }
    if($script:recorder){ $script:recorderReported=$true } }catch{ "video stop error: $_" } }
try {
    $deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    $pilotTrace=if($BootstrapMode -eq 'Native' -and -not $Measure){$trace}else{'none'}   # a measurement trace drops the per-frame rows the pilot waits for
    pwsh -NoProfile -File "$d/Pilot-TitleContinue.ps1" -Trace $pilotTrace 2>&1 | Select-Object -Last 2
    for($k=1;$k -le $LoadShots -and (Alive);$k++){ Start-Sleep 2; P '-Action','shot','-Path',"$o/load-$k.png" }
    Start-Sleep ([Math]::Max(0,$SettleSeconds-2*$LoadShots)); if(Alive){ P '-Action','shot','-Path',"$o/start.png" }
    $n=0
    if($Teleport -and (Alive)){ $file=Join-Path $build 'witness-teleport.txt'; Set-Content -LiteralPath $file -Value $Teleport -NoNewline -Encoding ascii
        Start-Sleep 6; if(Test-Path -LiteralPath $file){ Remove-Item -LiteralPath $file -Force; 'teleport file not taken' }; $n++; P '-Action','shot','-Path',"$o/walk-$n.png" }
    if($Video -and (Alive)){ $info=pwsh -NoProfile -File "$d/Pilot-Game.ps1" -Action shot -Path "$o/video-start.png" | Select-Object -Last 1
        $ff=(Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
        if(-not $ff){$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages/Gyan.FFmpeg_*/ffmpeg-*/bin/ffmpeg.exe" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName}
        if("$info" -match 'rect=(-?\d+),(-?\d+),(-?\d+),(-?\d+)' -and [int]$Matches[3] -gt [int]$Matches[1] -and $ff){ $L=[int]$Matches[1];$T=[int]$Matches[2];$W=([int]$Matches[3]-$L) -band -2;$H=([int]$Matches[4]-$T) -band -2
            $holds=0;foreach($step in $KeyPath -split ','){ if($step){ $holds+=[int]$step.Substring(1) } }   # -t bounds the file if this script is killed
            $psi=[Diagnostics.ProcessStartInfo]::new($ff,"-hide_banner -loglevel error -y -f gdigrab -framerate $VideoFps -draw_mouse 0 -offset_x $L -offset_y $T -video_size ${W}x$H -i desktop -t $([int]($holds/1000)+5*(($KeyPath -split ',').Count+$Shots)+60) -c:v libx264 -preset ultrafast -crf 16 -pix_fmt yuv420p `"$o/walk.mp4`"")
            $psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardInput=$true
            try{ $recorder=[Diagnostics.Process]::Start($psi);$v0=Get-Date;Start-Sleep 1;"video start rect=$L,$T,${W}x$H" }catch{ "video start error: $_" } }
        else{ "video: no window rectangle or no ffmpeg ($info)" } }
    # I-0132 R11j: the looks after the recorder's start, so a film shows the camera turning (the TAA's ghosting).
    if($Look){ foreach($move in $Look -split ';'){ if(-not (Alive)){ break }; $xy=$move -split ','; P '-Action','look','-X',$xy[0],'-Y',$(if($xy.Count -gt 1){$xy[1]}else{'0'}); Start-Sleep 2; $n++; P '-Action','shot','-Path',"$o/walk-$n.png" } }
    if($KeyPath){ $k=0; foreach($step in $KeyPath -split ','){ if(-not (Alive)){ break }; $k++; $t0=Get-Date; P '-Action','hold','-Vk',"$([int][char]$step[0])",'-Ms',"$([int]$step.Substring(1))"
        if($v0){ "step $k $step video_ms=$([int]($t0-$v0).TotalMilliseconds)" }; Start-Sleep 1; $n++; P '-Action','shot','-Path',"$o/walk-$n.png" } }
    for($k=0;$k -lt $Shots -and (Alive);$k++){ Start-Sleep 4; $n++; P '-Action','shot','-Path',"$o/walk-$n.png" }
    Stop-Recorder
    if($HideTextures){ $hide=Join-Path $build 'hide-texture.txt'
        try{ $i=0; foreach($fragment in $HideTextures -split ','){ if(-not (Alive)){ break }; $i++; Set-Content -LiteralPath $hide -Value $fragment -NoNewline -Encoding ascii; Start-Sleep 4; P '-Action','shot','-Path',"$o/hide-$i.png" } }
        finally{ Remove-Item -LiteralPath $hide -Force -ErrorAction SilentlyContinue }
        if(Alive){ Start-Sleep 4; P '-Action','shot','-Path',"$o/unhidden.png" } }
    for($k=0;$k -lt $StaySeconds -and (Alive);$k+=5){ Start-Sleep 5 }
    if(Alive){ P '-Action','click','-X',"$($WindowWidth-15)",'-Y','15' }
} catch { "pilot error: $_" }
Stop-Recorder
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 4
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
