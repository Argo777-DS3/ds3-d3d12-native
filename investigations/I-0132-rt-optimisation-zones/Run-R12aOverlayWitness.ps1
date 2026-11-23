# I-0132 R12a: one announced witness run (D-0116: offline, the saves restored by hash, Run-LiveWitness's protocol) of the overlay's
# test screen (rekindled-overlay-test.flag) on a build folder of this branch, with a rekindled-settings.ini beside its module (R12b's
# final review M1: the module's reading of the file witnessed; the file removed after the run). The Firelink path's save, the ASUS by
# its key. A job takes a screen picture of the game's screen every 4 s (JPEG) into the run's evidence folder while the pilot carries
# the game past the title into the world. No measure.
param([Parameter(Mandatory)][string]$BuildFolder,[Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[int]$Seconds=100,
    [string]$Settings="dlss=dlaa`nsharpen=0.25`n",[string]$Screen='1920x1080@60',
    [switch]$DebugLayer)   # R12a's final review M3: the D3D12 debug layer on (its errors fault the run)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$recording=Get-Content -LiteralPath "$repo/scripts/replay/firelink.path.json" -Raw | ConvertFrom-Json
$ini=Join-Path $BuildFolder 'rekindled-settings.ini'
if(Test-Path -LiteralPath $ini){throw "a settings file is already beside the module: $ini"}
$shots="$repo/evidence/I-0132/artifacts/$RunName-shots";if(Test-Path -LiteralPath $shots){throw "Run $RunName exists"}
New-Item -ItemType Directory -Force $shots|Out-Null
$device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
[IO.File]::WriteAllText($ini,$Settings,[Text.UTF8Encoding]::new($false))
$layer=@(if(-not $DebugLayer){'-NoDebugLayer'})
$previous=$env:DS3_PILOT_SCREEN;$env:DS3_PILOT_SCREEN=$device
$shooter=Start-Job -ScriptBlock { param($pilot,$shots,$device,$seconds)
    $env:DS3_PILOT_SCREEN=$device;$end=(Get-Date).AddSeconds($seconds+20);$n=0
    while((Get-Date) -lt $end){Start-Sleep -Seconds 4
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){$n++;try{& $pilot -Action shot -Path (Join-Path $shots ('shot-{0:D3}.jpg' -f $n)) | Out-Null}catch{}}}
} -ArgumentList "$repo/investigations/I-0066-independent-native-startup/Pilot-Game.ps1",$shots,$device,$Seconds
try{
    pwsh -NoProfile -File "$repo/investigations/I-0066-independent-native-startup/Run-LiveWitness.ps1" -RunName $RunName -Seconds $Seconds `
        -TestSave $recording.save.path -TestSaveSha256 $recording.save.sha256 -BuildFolder $BuildFolder @layer -EffectFlags rekindled-overlay-test 2>&1 | Select-Object -Last 6
    $failed=$LASTEXITCODE   # the step review's L6: a failed run said, after the cleanup
}finally{
    Remove-Item -LiteralPath $ini -ErrorAction SilentlyContinue   # the run's own file, never Thomas's
    Wait-Job $shooter -Timeout 60 | Out-Null;Remove-Job $shooter -Force
    if($null -eq $previous){Remove-Item Env:DS3_PILOT_SCREEN -ErrorAction SilentlyContinue}else{$env:DS3_PILOT_SCREEN=$previous}}
$trace="$repo/evidence/I-0066/artifacts/$RunName/startup.jsonl"
if(Test-Path -LiteralPath $trace){Select-String -LiteralPath $trace -Pattern '"native_settings"|"native_dlss_mode"|"native_overlay_font"|"native_overlay"|native_taa_frames' | Select-Object -First 14 | ForEach-Object {$_.Line.Substring(0,[Math]::Min(400,$_.Line.Length))}}
Write-Host "shots: $((Get-ChildItem -LiteralPath $shots).Count) in $shots"
if($failed){throw "the witness run failed (exit $failed)"}
