# I-0113 tranche 21, gap 22 (Thomas, 2026-11-06 19:31: « la barrière magique à l'entrée d'Irithyll buggait, comme si elle
# saccadait », seen in R's endurance full-1106-night-asus2, visit 2: Gamma of the final Shebang save at its m37 bonfire, the bridge
# and its far arch in view, shots/v2-zone.jpg). A filmed pair from that very place: one announced slot, the endurance tour itself
# (Run-Endurance.ps1 with -OnlyZones from -Zones and -Visits, one process, restored by hash) natively and in the original (-Original
# -Probe: the original's D3D11 renderer, the module in Reference mode for the journal the tour waits on), the game window filmed with
# ffmpeg's gdigrab at 60 frames/s for -FilmSeconds from -FilmAfter seconds after the tour's « in world » line of the zone -FilmZone (a missed load renumbers the visits: i113-t21-bridge-nofix-native filmed Beta)
# (barrier.mp4 in the run's folder). The tour's own sequence, Beta then Gamma, Gamma's visit filmed: tranche 21's first version
# (Run-AlphaCast -Continue on the m37 zone save) loaded Gamma at Central Irithyll, i113-t21-barrier-*; its second (the tour with Gamma
# alone) another place, i113-t21-bridge-* (INFERRED: the Load Game rows the tour clicks hold other characters before a first quit).
# -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateSet('native','original')][string[]]$Runs=@('native','original'),[string]$NotBefore='',[ValidateRange(10,120)][int]$FilmSeconds=40,
    [ValidateRange(0,60)][int]$FilmAfter=8,[string]$Zones='Beta m31,Gamma m37',[ValidateRange(1,5)][int]$Visits=2,[string]$FilmZone='Gamma m37',[string]$Screen='1920x1080@60',[string]$EffectFlags='')   # -Screen: a scripts/Get-Screens.ps1 key, for the tour and the shot alike; -EffectFlags: passed to the tour (tranche 22: a flagged copy of a build folder loses its flags)
$ErrorActionPreference='Stop'
# the camera turns every 8 s during the stay (Pilot-EnduranceTour): analyse a window between two turns
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$endurance=Join-Path $repo 'investigations/I-0085-night-endurance/Run-Endurance.ps1';$pilot=Join-Path $repo 'investigations/I-0066-independent-native-startup/Pilot-Game.ps1'
$ff=(Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
if(-not $ff){$ff=(Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe -ErrorAction SilentlyContinue | Select-Object -First 1).FullName}
if(-not $ff){throw 'no ffmpeg'}
$device=(& (Join-Path $repo 'scripts/Get-Screens.ps1') -Key $Screen).device   # resolved once, before any run (the fix review's L4)
if(-not $device){throw "no screen for $Screen"}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($r in $Runs){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$r skipped: DarkSoulsIII already runs";break}
    $name="$Prefix-$r";$o=Join-Path $repo "evidence/I-0066/artifacts/$name";"$(Get-Date -Format HH:mm:ss) start $name"
    $mode=if($r -eq 'native'){@{Build=$BuildFolder}}else{@{Original=$true;Probe=$true;Build=$BuildFolder}}   # a hashtable: an array's '-Build' binds as a positional value to a script
    $job=Start-Job -ScriptBlock {param($script,$run,$mode,$zones,$visits,$screen,$flags) if($flags){$mode.EffectFlags=$flags}; & $script -RunName $run -OnlyZones $zones -Visits $visits -StaySeconds 90 -Screen $screen @mode 2>&1} -ArgumentList $endurance,$name,$mode,$Zones,$Visits,$Screen,$EffectFlags
    $recorder=$null;$previous=$env:DS3_PILOT_SCREEN;$go=Join-Path $BuildFolder 'ffx-series-go.txt';Remove-Item -LiteralPath $go -ErrorAction SilentlyContinue
    try{
        $until=(Get-Date).AddMinutes(4+3*$Visits);$tour=Join-Path $o 'tour.txt'
        while((Get-Date) -lt $until -and $job.State -eq 'Running' -and -not ((Test-Path -LiteralPath $tour) -and (Select-String -LiteralPath $tour -Pattern "zone $FilmZone in world" -Quiet))){Start-Sleep 1}
        $seen=(Test-Path -LiteralPath $tour) -and (Select-String -LiteralPath $tour -Pattern "zone $FilmZone in world" -Quiet)
        if(-not $seen){if($job.State -ne 'Running'){'the tour ended before the world'}else{'no « in world » line: not filmed'}}
        else{Start-Sleep $FilmAfter
            $env:DS3_PILOT_SCREEN=$device;$info=pwsh -NoProfile -File $pilot -Action shot -Path "$o/film-start.png" | Select-Object -Last 1
            if("$info" -match 'rect=(-?\d+),(-?\d+),(-?\d+),(-?\d+)' -and [int]$Matches[3] -gt [int]$Matches[1]){$L=[int]$Matches[1];$T=[int]$Matches[2];$W=([int]$Matches[3]-$L) -band -2;$H=([int]$Matches[4]-$T) -band -2
                $psi=[Diagnostics.ProcessStartInfo]::new($ff,"-hide_banner -loglevel error -y -f gdigrab -framerate 60 -draw_mouse 0 -offset_x $L -offset_y $T -video_size ${W}x$H -i desktop -t $FilmSeconds -c:v libx264 -preset ultrafast -crf 16 -pix_fmt yuv420p `"$o/barrier.mp4`"")
                $psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardInput=$true   # D-0199: hidden; -t bounds it
                $recorder=[Diagnostics.Process]::Start($psi);if($EffectFlags -match 'ffx-series'){Set-Content -LiteralPath $go -Value $name}   # tranche 23: the module's 3 s distortion series (ffx_distortion_series) starts with the film
                "film start $((Get-Date).ToUniversalTime().ToString('o')) rect=$L,$T,${W}x$H"
                if(-not $recorder.WaitForExit(($FilmSeconds+20)*1000)){try{$recorder.StandardInput.Write('q');$recorder.StandardInput.Close()}catch{};$recorder.WaitForExit(10000) | Out-Null}}   # the file closed before any kill
            else{"no window rectangle ($info)"}}}
    catch{"film error: $_"}
    finally{$env:DS3_PILOT_SCREEN=$previous;Remove-Item -LiteralPath $go -ErrorAction SilentlyContinue;if($recorder -and -not $recorder.HasExited){try{$recorder.Kill()}catch{}}
        Receive-Job $job -Wait -AutoRemoveJob | Where-Object {"$_" -match 'restoration|exit|tour end|error|fault'}}   # the tour's own restoration awaited
    "$(Get-Date -Format HH:mm:ss) end $name"}
"SERIES-DONE $(Get-Date -Format HH:mm:ss)"
