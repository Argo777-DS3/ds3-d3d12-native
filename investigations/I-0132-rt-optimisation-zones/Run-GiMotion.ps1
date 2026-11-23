# I-0132, ROADMAP workstream 2 step 4 (D-0273: the GI quality row measured in motion, removed if it gains nothing). One announced
# series on the PC entirely free (WORKFLOW « Live game rule »), the ASUS by default, offline, each run restored by hash: Firelink's save
# (m40, Run-R7aGi's), natively at the PT level with NVIDIA DLAA and RR, the frame rate held at 30 (unlock-30.flag: every mode holds it,
# so each world frame is one game step and the camera turns the same angle a frame in every mode), a 1280x720 window, the debug layer
# off. For each run the module names its burst window (world-burst-after-900.flag: 32 consecutive lossless pictures from the 900th frame
# after the first world frame, the row world_burst_window), and a watcher started beside the walk turns the camera from that row on at a
# constant rate (Pilot-Game -Action sweep: -SweepUnits over -SweepSeconds), the turn begun -LeadFrames frames before the burst at 30
# frames/s, so the burst falls inside the turn even if the game ran slower. Modes: `gm1` (rt-gi-paths-1, the row's Low), `gm4`
# (rt-gi-paths-4, its Ultra); -Modes repeats them (gm1,gm4,gm1,gm4: the run-to-run spread). The figures: Get-GiMotion.py on the
# bursts (the frame-to-frame change left once the turn's sideways shift is aligned, the character's column out; a run whose turn did not
# cover its burst marked INVALID), the same motion in every run.
# Usage: pwsh -NoProfile -File Run-GiMotion.ps1 -Prefix i132-gim1 -BuildFolder <build> [-NotBefore HH:mm] [-Log <file>]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string[]]$Modes=@('gm1','gm4','gm1','gm4'),[string]$NotBefore='',[string]$Log='',[string]$Screen='1920x1080@60',
    [int]$SweepUnits=-5000,[ValidateRange(5,60)][int]$SweepSeconds=20,[ValidateRange(30,800)][int]$LeadFrames=240,[ValidateRange(120,600)][int]$Seconds=200)
$ErrorActionPreference='Stop'
$Modes=@($Modes | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(Get-ChildItem -LiteralPath "$repo/evidence/I-0066/artifacts" -Directory -Filter "$Prefix-*" -ErrorAction SilentlyContinue){throw "runs named $Prefix-* exist: another -Prefix"}   # the step review's L7
$inv="$repo/investigations/I-0066-independent-native-startup"
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$hash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$base='test-no-ai,world-pictures,world-burst,world-burst-after-900,unlock-30,rt-fail-magenta,rt-pt,dlss-dlaa'
$modeFlags=@{gm1="$base,rt-gi-paths-1";gm4="$base,rt-gi-paths-4"}
$burstAfter=900
$say={param($t) if($Log){$t | Add-Content $Log}else{$t}}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    $i=0;$runs=@()
    foreach($mode in $Modes){$i++
        if(-not $modeFlags.ContainsKey($mode)){& $say "$mode skipped: no such mode";continue}
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$mode skipped: DarkSoulsIII already runs";continue}
        $run="$Prefix-$i-$mode";$trace="$repo/evidence/I-0066/artifacts/$run/startup.jsonl";& $say "$(Get-Date -Format HH:mm:ss) $run"
        # The watcher: the burst's row, then the turn from LeadFrames before it (at 30 frames/s), bounded by the run's length.
        $watcher=Start-Job -ScriptBlock { param($trace,$inv,$after,$lead,$units,$seconds,$limit,$screen)
            $env:DS3_PILOT_SCREEN=$screen;$deadline=(Get-Date).AddSeconds($limit)
            while((Get-Date) -lt $deadline -and -not ((Test-Path -LiteralPath $trace) -and (Select-String -LiteralPath $trace -Pattern '"world_burst_window"' -SimpleMatch -Quiet))){Start-Sleep -Milliseconds 250}
            if((Get-Date) -ge $deadline){return 'watcher: no burst window row'}
            $seen=Get-Date;Start-Sleep -Milliseconds ([int](1000*[Math]::Max(0,$after-$lead)/30.0))
            "watcher: row at $($seen.ToString('HH:mm:ss.fff')), turn from $((Get-Date).ToString('HH:mm:ss.fff'))"
            pwsh -NoProfile -File "$inv/Pilot-Game.ps1" -Action sweep -X $units -SweepSeconds $seconds 2>&1 | Select-Object -Last 2   # a stop line before the last (the step review's L1)
            "watcher: turn ended $((Get-Date).ToString('HH:mm:ss.fff'))" } -ArgumentList $trace,$inv,$burstAfter,$LeadFrames,$SweepUnits,$SweepSeconds,($Seconds+60),$device
        try{ & "$inv/Run-ZoneWalk.ps1" -RunName $run -TestSave $save -TestSaveSha256 $hash -Seconds $Seconds -SettleSeconds 20 -Look '' -Shots 0 `
            -StaySeconds ([int](($burstAfter+60)/30.0)+$SweepSeconds) -NoDebugLayer -BuildFolder $BuildFolder -WindowWidth 1280 -WindowHeight 720 `
            -EffectFlags $modeFlags[$mode] *>&1 | ForEach-Object { & $say "$_" }
        }finally{ Wait-Job $watcher -Timeout 120 | Out-Null;Receive-Job $watcher | ForEach-Object { & $say "$_" };Remove-Job $watcher -Force }   # the final review's L4
        & $say "$(Get-Date -Format HH:mm:ss) $run done";$runs+=@("$repo/evidence/I-0066/artifacts/$run")}
    if($runs.Count){python -I "$PSScriptRoot/Get-GiMotion.py" @runs --size 1280x720 2>&1 | ForEach-Object { & $say "$_" }}
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
