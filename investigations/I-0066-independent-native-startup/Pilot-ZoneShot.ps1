param([Parameter(Mandatory)][string]$Trace,[ValidateRange(30,600)][int]$AfterSeconds=140,[Parameter(Mandatory)][string]$Path)
# Side-by-side witness (D-0181): a screenshot of the primary screen $AfterSeconds after the run's trace appears (the launch), the
# same moment for the Reference and the Native run of a zone save whose camera does not move. Pilot-Game.ps1 takes the shot.
$ErrorActionPreference='Stop'
$deadline=(Get-Date).AddSeconds(150)
while(-not (Test-Path -LiteralPath $Trace) -and (Get-Date) -lt $deadline){Start-Sleep 1}
if(-not (Test-Path -LiteralPath $Trace)){'no trace';exit 1}
Start-Sleep -Seconds $AfterSeconds
pwsh -NoProfile -File "$PSScriptRoot/Pilot-Game.ps1" -Action shot -Path $Path
