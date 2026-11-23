[CmdletBinding()]
param([Parameter(Mandatory)][string]$Label,[Parameter(Mandatory)][string]$EvidenceDir,[switch]$AtQuitTab)
# I-0066 (1124): during an announced Reference run on a test save, Quit Game (which saves and returns to the title),
# then copy the active save as a named test save (read-only copy; the runner still restores Thomas's save by hash).
$ErrorActionPreference='Stop'
$pilot=Join-Path $PSScriptRoot 'Pilot-Game.ps1'
function P([string[]]$a){ pwsh -NoProfile -File $pilot @a | Out-Null }
if(-not $AtQuitTab){
P '-Action','key','-Vk','27'; Start-Sleep -Milliseconds 1500
P '-Action','click','-X','525','-Y','390'; Start-Sleep -Milliseconds 1500
P '-Action','click','-X','742','-Y','191'; Start-Sleep -Milliseconds 1500 }
P '-Action','click','-X','648','-Y','556'; Start-Sleep -Milliseconds 1500
P '-Action','click','-X','501','-Y','428'; Start-Sleep -Seconds 8
$save=@(Get-ChildItem (Join-Path $env:APPDATA 'DarkSoulsIII') -Recurse -Filter 'DS30000.sl2')[0].FullName
$target=Join-Path $env:USERPROFILE ".ds3-i0039-backups\$Label-$(Get-Date -Format yyyyMMdd-HHmmss)"
New-Item -ItemType Directory -Path $target | Out-Null
Copy-Item -LiteralPath $save -Destination $target
$hash=(Get-FileHash -LiteralPath (Join-Path $target 'DS30000.sl2')).Hash
$record=[ordered]@{label=$Label;created_utc=[DateTime]::UtcNow.ToString('o');source=$save;copy=(Join-Path $target 'DS30000.sl2');sha256=$hash}
$record | ConvertTo-Json | Set-Content (Join-Path $target 'record.json') -Encoding utf8
($record | ConvertTo-Json -Compress) | Add-Content (Join-Path $EvidenceDir 'kept-saves.jsonl') -Encoding utf8
$record | ConvertTo-Json -Compress
