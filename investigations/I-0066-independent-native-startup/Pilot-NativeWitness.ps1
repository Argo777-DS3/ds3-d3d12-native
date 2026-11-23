[CmdletBinding()]
param([Parameter(Mandatory)][string]$RunName,[ValidateRange(1,10)][int]$InGameWindows=2,[ValidateRange(60,900)][int]$TimeoutSeconds=420)
# I-0066 (1125): pilot of an announced native witness run already started by Run-StartupTrace.ps1: title sequence,
# wait for $InGameWindows in-game native_present_pacing windows, capture the game area, read the menu gate state,
# then close the game window (the runner restores by hash). Prints the last pacing row and the character families.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$out=Join-Path $root "evidence/I-0066/artifacts/$RunName"; $trace=Join-Path $out 'startup.jsonl'
pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Pilot-TitleContinue.ps1') -Trace $trace -TimeoutSeconds 240
$deadline=[DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
while($true){
    $rows=@(Select-String -LiteralPath $trace -Pattern '"native_present_pacing".*"content_draws":[1-9]')
    if($rows.Count -ge $InGameWindows){ break }
    if(Select-String -LiteralPath $trace -Pattern '"event":"refusal"' -Quiet){ throw 'native refusal before the in-game windows' }
    if([DateTime]::UtcNow -gt $deadline){ throw 'timeout waiting for in-game frames' }
    Start-Sleep -Seconds 2 }
Add-Type -AssemblyName System.Drawing
pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Pilot-Game.ps1') -Action shot -Path "$env:TEMP\witness-full.jpg" | Out-Null
$b=[Drawing.Bitmap]::FromFile("$env:TEMP\witness-full.jpg"); $c=$b.Clone((New-Object Drawing.Rectangle 0,30,1290,720),$b.PixelFormat)
$c.Save((Join-Path $out 'native-ingame.jpg'),[Drawing.Imaging.ImageFormat]::Jpeg); $b.Dispose(); $c.Dispose()
$game=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1
if($game){ pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Read-MenuGateState.ps1') -TargetProcessId $game.Id -Label 'native-ingame' -Output (Join-Path $out 'menu-gate-state.jsonl') | Out-Null
    pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Read-SssState.ps1') -TargetProcessId $game.Id -Label 'native-ingame' -Output (Join-Path $out 'sss-state.jsonl') | Out-Null
    pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Read-EnvRegionState.ps1') -TargetProcessId $game.Id -Label 'native-ingame' -Output (Join-Path $out 'env-region-state.jsonl') | Out-Null
    pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Read-LightState.ps1') -TargetProcessId $game.Id -Label 'native-ingame' -Output (Join-Path $out 'light-state.jsonl') | Out-Null }
pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Pilot-Game.ps1') -Action click -X 1265 -Y 15 | Out-Null
(Select-String -LiteralPath $trace -Pattern '"native_present_pacing"' | Select-Object -Last 1).Line
$outcome=Select-String -LiteralPath $trace -Pattern '"native_asm_part_outcomes"' | Select-Object -Last 1
if($outcome){ ($outcome.Line | ConvertFrom-Json).families.PSObject.Properties | ForEach-Object { "$($_.Name) gates=$(($_.Value.gates | ConvertTo-Json -Compress)) draw=$($_.Value.draw -join ',')" } }
