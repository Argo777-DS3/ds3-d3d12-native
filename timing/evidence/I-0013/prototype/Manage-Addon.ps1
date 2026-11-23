[CmdletBinding()]
param(
    [ValidateSet('Status', 'Install', 'Uninstall')]
    [string]$Action = 'Status',
    [string]$GameDirectory = 'C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game'
)

$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Source = Join-Path $Here 'bin\DS3TerrainCapture.addon64'
$Target = Join-Path $GameDirectory 'DS3TerrainCapture.addon64'
$Game = Join-Path $GameDirectory 'DarkSoulsIII.exe'
$ExpectedGameHash = 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'

if (-not (Test-Path -LiteralPath $Game -PathType Leaf)) {
    throw "DarkSoulsIII.exe not found at the exact target: $Game"
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $Game).Hash -ne $ExpectedGameHash) {
    throw 'DarkSoulsIII.exe does not match BuildID 10167187 / 1.15.2.0.'
}

$ProcessCount = @(Get-Process -Name DarkSoulsIII -ErrorAction SilentlyContinue).Count
$Installed = Test-Path -LiteralPath $Target -PathType Leaf
$SourceHash = if (Test-Path -LiteralPath $Source -PathType Leaf) {
    (Get-FileHash -Algorithm SHA256 -LiteralPath $Source).Hash
} else { $null }
$TargetHash = if ($Installed) {
    (Get-FileHash -Algorithm SHA256 -LiteralPath $Target).Hash
} else { $null }

if ($Action -eq 'Status') {
    [pscustomobject]@{
        GameRunning = $ProcessCount -ne 0
        ReShadeVersion = (Get-Item -LiteralPath (Join-Path $GameDirectory 'dxgi.dll')).VersionInfo.FileVersion
        AddonInstalled = $Installed
        InstalledHash = $TargetHash
        BundledHash = $SourceHash
        ExactMatch = $Installed -and $SourceHash -and $TargetHash -eq $SourceHash
    }
    return
}

if ($ProcessCount -ne 0) {
    throw 'Dark Souls III is running. Close it before changing the add-on file.'
}
if (-not $SourceHash) {
    throw "Built add-on is unavailable: $Source"
}

if ($Action -eq 'Install') {
    if ($Installed -and $TargetHash -ne $SourceHash) {
        throw "Refusing to overwrite a different file: $Target"
    }
    if (-not $Installed) {
        Copy-Item -LiteralPath $Source -Destination $Target
    }
    $After = (Get-FileHash -Algorithm SHA256 -LiteralPath $Target).Hash
    if ($After -ne $SourceHash) {
        throw 'Installed add-on hash does not match the validated build.'
    }
    Write-Output "Installed one reversible file: $Target"
    return
}

if (-not $Installed) {
    Write-Output 'Add-on is already absent.'
    return
}
if ($TargetHash -ne $SourceHash) {
    throw "Refusing to remove a file that differs from this validated build: $Target"
}
Remove-Item -LiteralPath $Target
Write-Output "Removed: $Target"
