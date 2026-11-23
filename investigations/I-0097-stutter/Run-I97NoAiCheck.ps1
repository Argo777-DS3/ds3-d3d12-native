param([Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][string]$NotBefore,[ValidatePattern('^[a-z0-9]*$')][string]$Tag='')
# I-0097 tranche 5 (Thomas, 2026-10-25): the Undead Settlement path natively then in the original, on the ASUS, enemies without AI
# (test-no-ai.flag), with a picture of the screen every 2 s from the launch (the loading screen, the pop-in): one announced test.
$ErrorActionPreference='Stop';$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){ Start-Sleep 5 }
foreach($r in @(@("i97-t5-noai-settlement-n$Tag",'Measure','test-no-ai,chr-positions,frame-dump'),@("i97-t5-noai-settlement-o$Tag",'Reference','test-no-ai,frame-dump'))){
    "== $($r[0]) $(Get-Date -Format HH:mm:ss)"
    $cap=Start-Process pwsh -WindowStyle Hidden -PassThru -ArgumentList "-NoProfile -File `"$repo\scripts\Capture-ScreenSeries.ps1`" -Out `"$repo\evidence\I-0097\captures\$($r[0])`" -Seconds 230 -Every 2"
    & "$repo\scripts\Invoke-ReplayRun.ps1" -RunName $r[0] -Build $Build -Kind $r[1] -Screen '1920x1080@60' -PathFile "$repo\scripts\replay\settlement.path.json" -EffectFlags $r[2] 2>&1 | Select-Object -Last 2 | Out-String
    $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5
    if(-not $cap.HasExited){ Stop-Process -Id $cap.Id -ErrorAction SilentlyContinue } }
"done $(Get-Date -Format HH:mm:ss)"
