[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^I-[0-9]{4}$')]
    [string]$Id,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[a-z0-9]+(?:-[a-z0-9]+)*$')]
    [string]$Slug,

    [Parameter(Mandatory = $true)]
    [string]$Title,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^C[1-9][0-9]*$')]
    [string]$Capability,

    [string]$Owner = 'unassigned',

    [ValidateRange(1, 100)]
    [int]$Budget = 3
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$folderName = "$Id-$Slug"
$investigationPath = Join-Path $repoRoot "investigations/$folderName"
$evidencePath = Join-Path $repoRoot "evidence/$Id"

if (Test-Path -LiteralPath $investigationPath) {
    throw "Investigation already exists: $investigationPath"
}

$duplicate = Get-ChildItem -LiteralPath (Join-Path $repoRoot 'investigations') -Directory |
    Where-Object { $_.Name -like "$Id-*" }
if ($duplicate) {
    throw "Investigation ID already exists: $($duplicate.FullName)"
}

New-Item -ItemType Directory -Path $investigationPath | Out-Null
New-Item -ItemType Directory -Path (Join-Path $evidencePath 'artifacts') -Force | Out-Null

$contractTemplate = Get-Content -LiteralPath (Join-Path $repoRoot 'investigations/_template/CONTRACT.md') -Raw -Encoding utf8
$contract = $contractTemplate -replace '(?m)^# I-NNNN .+$', "# $Id - $Title"
$contract = $contract.Replace('Capability: C?', "Capability: $Capability")
Set-Content -LiteralPath (Join-Path $investigationPath 'CONTRACT.md') -Value $contract -Encoding utf8

Copy-Item -LiteralPath (Join-Path $repoRoot 'investigations/_template/RESULT.md') -Destination (Join-Path $investigationPath 'RESULT.md')

$now = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
$status = [ordered]@{
    schema_version = 1
    id = $Id
    title = $Title
    capability = $Capability
    state = 'planned'
    owner = $Owner
    created_at = $now
    updated_at = $now
    budget = [ordered]@{
        iterations_max = $Budget
        iterations_used = 0
    }
    stop_reason = $null
}
$status | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $investigationPath 'status.json') -Encoding utf8

$manifest = [ordered]@{
    schema_version = 1
    investigation_id = $Id
    subject = $Title
    game_build = [ordered]@{
        label = 'unknown'
        executable_sha256 = $null
    }
    artifacts = @()
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $evidencePath 'manifest.json') -Encoding utf8
New-Item -ItemType File -Path (Join-Path $evidencePath 'artifacts/.gitkeep') | Out-Null

Write-Output "Created $folderName and evidence/$Id"
