[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$target = 'C:\Users\User\Desktop\DS3_evidences'
$link = Join-Path $repoRoot 'evidence'
if (-not (Test-Path -LiteralPath $target -PathType Container)) {
    throw "External Evidence Store is missing: $target"
}
$existing = Get-Item -LiteralPath $link -Force -ErrorAction SilentlyContinue
if ($null -ne $existing) {
    if ($existing.LinkType -ne 'Junction' -or $existing.Target -ne $target) {
        throw "Refusing to replace existing evidence content at $link. Preserve it before connecting the external store."
    }
    Write-Output "Evidence Store already connected: $link -> $target"
    return
}
New-Item -ItemType Junction -Path $link -Target $target | Out-Null
Write-Output "Evidence Store connected: $link -> $target"
