[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$BuildFolder,
    [string]$Screen='\\.\DISPLAY1',[ValidateRange(200,900)][int]$Seconds=420,[ValidateRange(60,600)][int]$StartAfterSeconds=200)
# I-0101 (STUDY §3.6 test): one announced run (D-0116: offline, the saves restored by hash) of the native module in exclusive
# full screen on one screen (display-screen.txt next to the module places the window before its swap chain; the pilot reads that
# screen), the Firelink path's save, and Invoke-I101FocusLoss.ps1's three Alt-Tabs with returns once the title is passed, then a close as by
# the close box (a quit must end without a refusal).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ throw 'DarkSoulsIII is already running' }
$recording=Get-Content -LiteralPath "$repo/scripts/replay/firelink.path.json" -Raw | ConvertFrom-Json
$screenFile=Join-Path $BuildFolder 'display-screen.txt'
$focus="$repo/evidence/I-0101/$RunName-focus.json";New-Item -ItemType Directory -Force -Path (Split-Path $focus) | Out-Null
Set-Content -LiteralPath $screenFile -Value $Screen -Encoding ascii
$previous=$env:DS3_PILOT_SCREEN;$env:DS3_PILOT_SCREEN=$Screen
$helper=Start-Job -ScriptBlock { param($script,$out,$after) pwsh -NoProfile -File $script -Out $out -StartAfterSeconds $after -Close 2>&1 } -ArgumentList "$PSScriptRoot/Invoke-I101FocusLoss.ps1",$focus,$StartAfterSeconds
try{
    pwsh -NoProfile -File "$repo/investigations/I-0066-independent-native-startup/Run-LiveWitness.ps1" -RunName $RunName -Seconds $Seconds `
        -TestSave $recording.save.path -TestSaveSha256 $recording.save.sha256 -BuildFolder $BuildFolder -NoDebugLayer `
        -GraphicsLevels 'ScreenMode=FULLSCREEN;Resolution-FullScreenWidth=1920;Resolution-FullScreenHeight=1080' 2>&1 | Select-Object -Last 14
}finally{
    Remove-Item -LiteralPath $screenFile -ErrorAction SilentlyContinue
    if($null -eq $previous){Remove-Item Env:DS3_PILOT_SCREEN -ErrorAction SilentlyContinue}else{$env:DS3_PILOT_SCREEN=$previous}
    Wait-Job $helper -Timeout 60 | Out-Null;Receive-Job $helper | Select-Object -Last 4;Remove-Job $helper -Force}
"focus: $focus"
