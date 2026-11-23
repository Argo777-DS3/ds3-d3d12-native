# D-0230: the full control of 2026-11-12 (main 08a47552, I-0113 tranche 40 after I-0132 R12e), the launcher's module, run from this script's
# checkout (the gate builds this checkout's committed native/ tree and writes .project/REPLAY-GATE.json here, not in the main
# checkout another pilot merges in). A copy of Run-FullControl-1109.ps1 with the tag as a parameter and one step added:
# -NewReference retakes the gate's ASUS standstill reference (2026-11-09: both measure runs departed from the 2026-10-21 reference by
# 21.47 for a threshold of 1.64, the light inside the window being the rays' since D-0253 and R11d). The control's own gate verdict
# is the one judged against the old reference; then one more measure run of the gate's build (m3) and a calibration of m1, m2 and m3
# (Invoke-ReplayGate -Calibrate -Evaluate -NewReference): the reference becomes m1's picture, the picture thresholds three times the
# largest difference of m2 and m3 to it (never above the old ones without -AllowRaise, which this script never passes), the
# cadence thresholds lowered only beyond noise. Stated in I-0132 RESULT (D-0268). Then the control sizes (1920x1080 full screen, a
# 1500x844 window, 1280x720 windowed) and the night endurance on -Screen (the ASUS). One process, sequential; every step's output in
# the log, an end marker in finally. Later controls reuse it with their own -Tag. Unlike the 2026-11-09 runner: no -AlsoOled (the ASUS
# base alone, D-0218), and $repo is this script's own checkout.
param([Parameter(Mandatory)][string]$Log,[string]$Tag='full-1112',[string]$NotBefore='',[string]$Screen='1920x1080@60',[switch]$NewReference)
$repo=(Resolve-Path "$PSScriptRoot/../..").Path -replace '\\','/'
$launcher="$repo/evidence/I-0066/artifacts/game-bootstrap-build"
$say={param($t) "$(Get-Date -Format HH:mm:ss) $t" | Out-File -FilePath $Log -Append -Encoding utf8}
try{
    Set-Location $repo
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);if($at -lt (Get-Date).AddHours(-12)){$at=$at.AddDays(1)};while((Get-Date) -lt $at){Start-Sleep 5}}
    & $say "gate start ($repo)"
    & ./scripts/Invoke-ReplayGate.ps1 -Tag "$Tag-gate" -Memory -Screens $Screen *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
    & $say "gate end, exit $LASTEXITCODE"
    # After the 2026-11-12 control's reviews (L1, L2): the reference is retaken only when every reason of the gate is a picture
    # departure (a fault, an inconclusive run or a cadence failure keeps the old one), from at least three measure runs (a refused
    # launch's -r preferred), and in its own try, so a failure there never skips the sizes and the endurance. The 2026-11-12 control ran
    # the earlier form (commit 95633af7), without that check. The new reference stays provisional until Thomas sees the stills (D-0181).
    if($NewReference -and -not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){ try{
        $reasons=@((Get-Content -LiteralPath "$repo/evidence/I-0083/gates/$Tag-gate.json" -Raw | ConvertFrom-Json).reasons)
        if(-not $reasons.Count -or @($reasons | Where-Object { $_ -notmatch '^fail: .*picture .* departs from the reference' }).Count){ & $say "reference: kept (the gate's reasons are not picture departures alone)" }
        else{
            $tree=(git -C $repo rev-parse HEAD:native).Trim();$gateBuild="$repo/evidence/I-0083/gates/build-$($tree.Substring(0,12))"
            $base="$Tag-gate-$($Screen -replace '@','-')";$art="$repo/evidence/I-0066/artifacts"
            if(-not (Test-Path -LiteralPath "$art/$base-m3")){
                & $say "reference: $base-m3 start"
                & ./scripts/Invoke-ReplayRun.ps1 -RunName "$base-m3" -Build $gateBuild -Kind Measure -Screen $Screen -EffectFlags frame-dump -PresentMon *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
                $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5}
            $pick={param($n) if(Test-Path -LiteralPath "$art/$n-r"){"$n-r"}elseif(Test-Path -LiteralPath "$art/$n"){$n}}
            $cal=@(1..4 | ForEach-Object { & $pick "$base-m$_" } | Where-Object { $_ });$v=& $pick "$Tag-gate-v1"
            if($cal.Count -lt 3 -or -not $v){ & $say "reference: kept ($($cal.Count) measure runs, validation '$v')" }
            else{
                & $say "reference: calibration of $($cal -join ', ')"
                & ./scripts/Invoke-ReplayGate.ps1 -Tag "$Tag-cal" -Calibrate -CalibrationRuns $cal.Count -NewReference -Evaluate (@($cal)+$v) -Screens $Screen *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
                & $say "reference end, exit $LASTEXITCODE"} } }
        catch{ & $say "reference: THROW $_ (the control goes on)" } }
    foreach($size in @(@("$Tag-1080fs",'ScreenMode=FULLSCREEN;Resolution-FullScreenWidth=1920;Resolution-FullScreenHeight=1080'),
                       @("$Tag-1500w",'ScreenMode=WINDOW;Resolution-WindowScreenWidth=1500;Resolution-WindowScreenHeight=844'),
                       @("$Tag-720w",'ScreenMode=WINDOW;Resolution-WindowScreenWidth=1280;Resolution-WindowScreenHeight=720'))){   # WORKFLOW: 720p, 1080p and one arbitrary size
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$($size[0]) skipped: DarkSoulsIII runs";continue}
        & $say "$($size[0]) start"
        & ./scripts/Invoke-ReplayRun.ps1 -RunName $size[0] -Build $launcher -Screen $Screen -GraphicsLevels $size[1] *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
        & $say "$($size[0]) end"}
    if(-not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){
        & $say 'night start'
        & ./investigations/I-0085-night-endurance/Run-Endurance.ps1 -RunName "$Tag-night" -Build $launcher -Screen $Screen *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
        & $say 'night end'}
}catch{& $say "THROW $_"}
finally{& $say 'EXIT'}
