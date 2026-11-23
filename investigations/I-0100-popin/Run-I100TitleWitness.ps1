param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$Build,
    [string]$Screen='1920x1080@60',[int]$CaptureSeconds=150,[double]$Every=1.0,[string]$EffectFlags='')
# I-0100 step 1 (C5, POPIN-STUDY 2.3 M4): the title and the first loading page of one native run, pictured on -Screen every
# -Every seconds from this call (scripts/Capture-ScreenSeries.ps1, hidden, D-0199), the window placed on that screen before its
# swap chain (-LaunchOnScreen); the run is Invoke-ReplayRun's on the Settlement's turn-only path (Beta m31, a 3-minute announced
# run under D-0116). The pictures go to evidence/I-0100/captures/<run>; the journal to evidence/I-0066/artifacts/<run>.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$shots="$repo/evidence/I-0100/captures/$RunName"
$capture=Start-Process pwsh -WindowStyle Hidden -PassThru -ArgumentList "-NoProfile -File `"$repo/scripts/Capture-ScreenSeries.ps1`" -Out `"$shots`" -Seconds $CaptureSeconds -Every $Every -Screen $Screen"
try {
    & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $RunName -Build $Build -Kind Measure -PathFile "$repo/scripts/replay/settlement-turn.path.json" -Screen $Screen -LaunchOnScreen -EffectFlags $EffectFlags
} finally { if(-not $capture.HasExited -and -not $capture.WaitForExit(($CaptureSeconds+30)*1000)){ Stop-Process -Id $capture.Id -Force -ErrorAction SilentlyContinue } }
"pictures: $(@(Get-ChildItem -LiteralPath $shots -Filter 'cap-*.jpg' -ErrorAction SilentlyContinue).Count) in $shots"
