param([Parameter(Mandatory)][string[]]$Builds,[ValidateRange(1,20)][int]$Rounds=4,[string]$Prefix='i96-title',[string]$NotBefore='')
# I-0096: does the title accept the pilot's input? (Five runs of the branch's builds stayed on « Press any button » on
# 2026-10-23, none of 475 earlier runs.) Announced live runs (D-0116): each round launches each build in turn on the Shebang
# zone save (Alpha untouched), as the replay runs do (Run-StartupTrace -SteamLaunch, measure mode), runs Pilot-TitleContinue
# -MenuY 0 (the two offline notices, then the main menu) with a 150 s bound, writes whether the title was reached, then closes
# the window and waits for the restoration. One line a launch: run, build, outcome, seconds.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
$save='C:/Users/User/.ds3-i0039-backups/shebang-zones-final-20261013-045344/DS30000.sl2';$hash='78948904F13ADD2F1AC79015F9F01F171D5B4F0F01A0A716CE12F884B94265B8'
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null); while((Get-Date) -lt $at){ Start-Sleep 5 } }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
$n=0
for($round=1;$round -le $Rounds;$round++){ foreach($build in $Builds){ $n++
    $run="$Prefix-$n";$o="$repo/evidence/I-0066/artifacts/$run";$start=Get-Date
    $job=Start-Job -ScriptBlock { param($inv,$run,$save,$hash,$build)
        pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds 240 -SteamLaunch -TestSave $save -TestSaveSha256 $hash -BuildFolder $build -BootstrapMode Native -Measure -NoDebugLayer 2>&1 } -ArgumentList $inv,$run,$save,$hash,$build
    $deadline=(Get-Date).AddSeconds(120)
    while(-not (Test-Path -LiteralPath "$o/startup.jsonl") -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 2 }
    $outcome=try { pwsh -NoProfile -File "$inv/Pilot-TitleContinue.ps1" -Trace 'none' -MenuY 0 -TimeoutSeconds 150 2>&1 | Select-Object -Last 1 } catch { "error $_" }
    $seconds=[int]((Get-Date)-$start).TotalSeconds
    if(Alive){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" -Action shot -Path "$o/title.jpg" | Out-Null; pwsh -NoProfile -File "$inv/Pilot-Game.ps1" -Action click -X 1265 -Y 15 | Out-Null }
    Wait-Job $job -Timeout 400 | Out-Null
    while($job.State -eq 'Running'){ Wait-Job $job -Timeout 60 | Out-Null }
    $ok=if(Test-Path -LiteralPath "$o/restoration.json"){ (Get-Content -LiteralPath "$o/restoration.json" -Raw | ConvertFrom-Json).ok }else{ 'missing' }
    "$run build=$(Split-Path -Leaf $build) outcome=$(if("$outcome" -like 'title reached*'){"reached $(([regex]::Match("$outcome",'notice_retries=\d+')).Value)"}else{"stuck ($outcome)"}) seconds=$seconds restoration=$ok" } }
