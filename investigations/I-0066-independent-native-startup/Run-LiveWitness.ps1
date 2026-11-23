[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[ValidateRange(60,900)][int]$Seconds=200,
    [ValidateSet('Native','Reference')][string]$BootstrapMode='Native',[string]$EffectFlags='',
    [Parameter(Mandatory)][string]$TestSave,[Parameter(Mandatory)][string]$TestSaveSha256,[ValidateRange(500,20000)][int]$ContinueDelayMs=1500,
    [switch]$Measure,[switch]$NoDebugLayer,[string]$TestLanguage='',[switch]$NoKProfile,[switch]$OneFrameSlot,[switch]$NoWaitablePresent,[string]$GraphicsLevels='',
    [string]$BuildFolder='')   # optimisation review: a session's own build folder (Build-GameBootstrap -Out)   # DF-4: a measurement run (no periodic readback, no debug layer: the frame rate of normal play)
# I-0066 W10 (thread « Suite W10 YEBIS », 2026-10-13): one announced witness run on a hash-recorded test save, as one command
# (so a session permission rule can name it): Run-StartupTrace.ps1 in a job (it restores every protected file by hash),
# Pilot-TitleContinue.ps1 in the foreground once the trace exists (-Trace none for a Reference run), then the job's tail.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$extra=@();if($BuildFolder){$extra+='-BuildFolder',$BuildFolder};if($EffectFlags){$extra+='-EffectFlags',$EffectFlags};if($Measure){$extra+='-Measure'};if($NoDebugLayer){$extra+='-NoDebugLayer'};if($NoKProfile){$extra+='-NoKProfile'};if($OneFrameSlot){$extra+='-OneFrameSlot'};if($NoWaitablePresent){$extra+='-NoWaitablePresent'};if($TestLanguage){$extra+='-TestLanguage',$TestLanguage};if($GraphicsLevels){$extra+='-GraphicsLevels',$GraphicsLevels}   # water (2026-10-18): e.g. ReflectionQuality=DISABLE
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$mode,$save,$hash,$extra)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode $mode -TestSave $save -TestSaveSha256 $hash @extra 2>&1 } -ArgumentList $PSScriptRoot,$RunName,$Seconds,$BootstrapMode,$TestSave,$TestSaveSha256,$extra
$trace="$repo/evidence/I-0066/artifacts/$RunName/startup.jsonl"
$deadline=(Get-Date).AddSeconds(120); while(-not (Test-Path -LiteralPath $trace) -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
$pilotTrace=if($BootstrapMode -eq 'Native' -and -not $Measure){$trace}else{'none'}   # a measurement trace drops the per-frame rows the pilot waits for
try { pwsh -NoProfile -File "$PSScriptRoot/Pilot-TitleContinue.ps1" -Trace $pilotTrace -ContinueDelayMs $ContinueDelayMs 2>&1 | Select-Object -Last 5 } catch { "pilot error: $_" }
# Review: never leave while the trace job runs (its end restores the protected files by hash, D-0116): wait, then report the
# restoration.
Wait-Job $job -Timeout ($Seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 12
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ 'game still alive' }
$restoration="$repo/evidence/I-0066/artifacts/$RunName/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
