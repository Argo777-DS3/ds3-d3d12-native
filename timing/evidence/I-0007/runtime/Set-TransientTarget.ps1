[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet(60, 165)]
    [int]$Target,
    [ValidatePattern('^[A-Za-z0-9._-]+\.dll$')]
    [string]$DllName = 'I0007TransientPatch.dll',
    [switch]$ElapsedRecurrence
)

$ErrorActionPreference = 'Stop'
if ($ElapsedRecurrence -and $DllName -ne 'I0007ElapsedRecurrence.dll') {
    throw 'Elapsed candidate requires its dedicated binary name.'
}
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Instrument = Resolve-Path -LiteralPath (Join-Path $Here 'transient-capture')
$Source = Join-Path $Here "configs/transient-$Target.txt"
$Destination = Join-Path $Instrument 'pass_c_config.txt'
$Dll = Join-Path $Instrument "bin/$DllName"
$Injector = Join-Path $Instrument 'bin/I0007TransientPatchInjector.exe'

foreach ($Artifact in @($Dll, $Injector)) {
    if (-not (Test-Path -LiteralPath $Artifact -PathType Leaf)) {
        throw "Required artifact not found: $Artifact"
    }
}

Copy-Item -LiteralPath $Source -Destination $Destination -Force
if ($ElapsedRecurrence) {
    Add-Content -LiteralPath $Destination -Value 'elapsed_recurrence_patch=1'
    $CandidateConfig = (Get-Content -LiteralPath $Destination -Raw).Replace("condition=i-0007-transient-$Target", "condition=i-0007-elapsed-$Target")
    [System.IO.File]::WriteAllText($Destination, $CandidateConfig)
}
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
    if ($Config -notmatch "(?m)^$([regex]::Escape($Line))\r?$") {
        throw "Active configuration is missing: $Line"
    }
}

Get-FileHash -Algorithm SHA256 -LiteralPath $Dll, $Injector |
    Select-Object Path, Hash
if ($ElapsedRecurrence) { Write-Output "Prepared the experimental elapsed-recurrence candidate for target $Target; not injected." } else { Write-Output "Prepared the unchanged distance-patch observer for target $Target; not injected." }
