[CmdletBinding()]
param([Parameter(Mandatory)][string]$RunName,[Parameter(Mandatory)][ValidateSet('Native','Reference')][string]$Mode)
# I-0066 (1155): announced run pilot for the in-game menu-button test: title, wait in game, then three read-only
# snapshots of Read-MenuButtonState.ps1 (idle, while Esc is held through Pilot-Game, after release), a capture, and
# close the game window (the runner restores by hash).
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot); $out=Join-Path $root "evidence/I-0066/artifacts/$RunName"
$trace=if($Mode -eq 'Native'){ Join-Path $out 'startup.jsonl' }else{ 'none' }
pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Pilot-TitleContinue.ps1') -Trace $trace -TimeoutSeconds 240 | Out-Null
if($Mode -eq 'Native'){ $deadline=[DateTime]::UtcNow.AddSeconds(300)
    while(@(Select-String -LiteralPath $trace -Pattern '"native_present_pacing".*"content_draws":[1-9]').Count -lt 2){
        if([DateTime]::UtcNow -gt $deadline){ throw 'timeout waiting for in-game frames' }; Start-Sleep -Seconds 2 } }
else { Start-Sleep -Seconds 20 }
$game=Get-Process DarkSoulsIII | Select-Object -First 1; $state=Join-Path $out 'menu-button-state.jsonl'
$read={param($label) pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Read-MenuButtonState.ps1') -TargetProcessId $game.Id -Label $label -Output $state | Out-Null}
& $read "$($Mode.ToLower())-idle"
$hold=Start-Job -ScriptBlock { param($p) pwsh -NoProfile -File $p -Action hold -Vk 0x1B -Ms 2500 } -ArgumentList (Join-Path $PSScriptRoot 'Pilot-Game.ps1')
Start-Sleep -Milliseconds 1200; & $read "$($Mode.ToLower())-esc-held"; Receive-Job -Job $hold -Wait | Out-Null; Remove-Job $hold
Start-Sleep -Seconds 2; & $read "$($Mode.ToLower())-after-esc"
pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Pilot-Game.ps1') -Action shot -Path (Join-Path $out "$($Mode.ToLower())-after-esc.jpg") | Out-Null
pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Pilot-Game.ps1') -Action click -X 1265 -Y 15 | Out-Null
Get-Content -LiteralPath $state
