[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet(60, 165)]
    [int]$Target
)

$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Instrument = Resolve-Path -LiteralPath (Join-Path $Here 'causal-patch')
$Source = Join-Path $Here "configs/causal-patch-$Target.txt"
$Destination = Join-Path $Instrument 'pass_c_config.txt'
$Dll = Join-Path $Instrument 'bin/I0007CausalPatchV3.dll'
$Injector = Join-Path $Instrument 'bin/I0007CausalPatchV3Injector.exe'

foreach ($Artifact in @($Dll, $Injector)) {
    if (-not (Test-Path -LiteralPath $Artifact -PathType Leaf)) {
        throw "Required artifact not found: $Artifact"
    }
}

Copy-Item -LiteralPath $Source -Destination $Destination -Force
$Config = Get-Content -LiteralPath $Destination -Raw
$Required = @(
    "target_fps=$Target",
    'enable_timing_override=1',
    'locomotion_trials=1',
    'locomotion_phase_rca=1',
    'locomotion_write_watch=1',
    'locomotion_writer_mirror=0',
    'causal_distance_patch=1'
    'auto_stop_seconds=900'
)
foreach ($Line in $Required) {
    if ($Config -notmatch "(?m)^$([regex]::Escape($Line))$") {
        throw "Active configuration is missing: $Line"
    }
}

Get-FileHash -Algorithm SHA256 -LiteralPath $Dll, $Injector |
    Select-Object Path, Hash
Write-Output "Prepared the causal distance patch for target $Target. DS3 was not launched."
