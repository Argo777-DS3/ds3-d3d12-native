[CmdletBinding()]
param([Parameter(Mandatory)][string]$RunName,[ValidateSet('Native','Reference')][string]$Mode='Native')
# I-0066 (1161): announced run pilot: title, wait in game, hold Esc (in-game menu), click the Status icon, capture,
# report whether the game survived and the exception stack RVAs if not, then close the window.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot); $out=Join-Path $root "evidence/I-0066/artifacts/$RunName"
$trace=if($Mode -eq 'Native'){ Join-Path $out 'startup.jsonl' }else{ 'none' }; $pilot=Join-Path $PSScriptRoot 'Pilot-Game.ps1'
pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Pilot-TitleContinue.ps1') -Trace $trace -TimeoutSeconds 240 | Out-Null
if($Mode -eq 'Native'){ $deadline=[DateTime]::UtcNow.AddSeconds(240)
    while(@(Select-String -LiteralPath $trace -Pattern '"native_present_pacing".*"content_draws":[1-9]').Count -lt 2 -and [DateTime]::UtcNow -lt $deadline){ Start-Sleep -Seconds 2 } }
else { Start-Sleep -Seconds 20 }
pwsh -NoProfile -File $pilot -Action hold -Vk 0x1B -Ms 2500 | Out-Null; Start-Sleep -Seconds 3
pwsh -NoProfile -File $pilot -Action shot -Path (Join-Path $out 'menu.jpg') | Out-Null
pwsh -NoProfile -File $pilot -Action click -X 395 -Y 390 | Out-Null; Start-Sleep -Seconds 6
$alive=[bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue); "alive after Status: $alive"
if($alive){ pwsh -NoProfile -File $pilot -Action shot -Path (Join-Path $out 'status.jpg') | Out-Null; pwsh -NoProfile -File $pilot -Action click -X 1265 -Y 15 | Out-Null }
elseif($Mode -eq 'Native'){ $base=$null
    foreach($l in Get-Content -LiteralPath $trace){ if($l -match '"event":"exception_module".*"base":(\d+)'){ $base=[int64]$Matches[1] } }
    foreach($l in Get-Content -LiteralPath $trace){ if($base -and $l -match '"event":"exception_stack","index":(\d+),"address":(\d+)' -and [int]$Matches[1] -lt 12){ "stack {0} 0x{1:X}" -f $Matches[1],([int64]$Matches[2]-$base) } } }
