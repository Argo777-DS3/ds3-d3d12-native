param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$Build,
    [ValidateRange(1,6)][int]$Loops=2,[ValidateRange(20,300)][int]$StaySeconds=40,[string]$NotBefore='')
# I-0081: the process's local video memory over zone travels. One process (the trace job dies with the shell that started it):
# Run-StartupTrace.ps1 in a job on the Shebang zone save (shebang-zones-final: Beta m31, Gamma m37, Delta m50, Epsilon m45,
# Zeta m51; Alpha untouched), measure mode without the debug layer, the title reached (Pilot-TitleContinue -MenuY 0), then
# Pilot-ZoneTour -Loops (each zone loaded from the title, -StaySeconds there, quit to the title) with a screenshot before
# every navigation click, then the window closed and the job's restoration awaited (every protected file restored by hash).
# The journal's native_present_pacing rows carry the private and local memory every 120 frames, native_streamed_memory the
# placed heaps every 600; zone-tour.txt the zones' UTC times. Known limit (i81-19): after a quit to the title the next zone's
# notices are sometimes not up when they are clicked, and that zone stays at the notice (its screenshot shows it): in i81-19
# five of the ten loads happened (m31, m50, m51, then m37 and m45), every other one.
$ErrorActionPreference='Stop'
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null); while((Get-Date) -lt $at){ Start-Sleep 5 } }
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
$save='C:/Users/User/.ds3-i0039-backups/shebang-zones-final-20261013-045344/DS30000.sl2';$hash='78948904F13ADD2F1AC79015F9F01F171D5B4F0F01A0A716CE12F884B94265B8'
# A zone's cycle took StaySeconds + 45 s in i81-19 (the notices, the menus, the screenshots; 804 s for ten) and the title
# about 50 s; the trace is bounded to 900 s.
$seconds=60+$Loops*5*($StaySeconds+46)
if($seconds -gt 900){ throw "the tour needs about $seconds s, over the trace's 900 s: lower -Loops or -StaySeconds" }
$job=Start-Job -ScriptBlock { param($inv,$run,$s,$save,$hash,$build)
    pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds $s -SteamLaunch -BootstrapMode Native -TestSave $save -TestSaveSha256 $hash -BuildFolder $build -Measure -NoDebugLayer 2>&1 } -ArgumentList $inv,$RunName,$seconds,$save,$hash,$Build
$o="$repo/evidence/I-0066/artifacts/$RunName";$deadline=(Get-Date).AddSeconds(120)
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
try {
    while(-not (Test-Path -LiteralPath "$o/startup.jsonl") -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    pwsh -NoProfile -File "$inv/Pilot-TitleContinue.ps1" -Trace 'none' -MenuY 0 2>&1 | Select-Object -Last 1
    New-Item -ItemType Directory -Force -Path "$o/shots" | Out-Null
    pwsh -NoProfile -File "$inv/Pilot-ZoneTour.ps1" -StaySeconds $StaySeconds -Loops $Loops -ShotDir "$o/shots" 2>&1 | Tee-Object -FilePath "$o/zone-tour.txt"
    if(Alive){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" -Action click -X 1265 -Y 15 | Out-Null }
} catch { "pilot error: $_" }
Wait-Job $job -Timeout ($seconds+240) | Out-Null
while($job.State -eq 'Running'){ 'trace job still running: waiting for its restoration'; Wait-Job $job -Timeout 60 | Out-Null }
Receive-Job $job | Select-Object -Last 3
$restoration="$o/restoration.json"
if(Test-Path -LiteralPath $restoration){ "restoration ok: $((Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok)" }else{ 'restoration.json missing' }
