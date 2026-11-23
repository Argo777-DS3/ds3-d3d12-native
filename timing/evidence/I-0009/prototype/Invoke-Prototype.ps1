[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('inject', 'prepare', 'enable', 'disable', 'status', 'stop')]
    [string]$Action,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, 2147483647)]
    [int]$ProcessId
)

$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Bin = Join-Path $Here 'bin'
$Controller = Join-Path $Bin 'I0009PrototypeController.exe'
$Dll = Join-Path $Bin 'I0009VisibleLocomotionPrototype.dll'
$ValidationPath = Join-Path $Here 'BUILD-VALIDATION.json'
$ExpectedExe = 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'

foreach ($Artifact in @($Controller, $Dll, $ValidationPath)) {
    if (-not (Test-Path -LiteralPath $Artifact -PathType Leaf)) {
        throw "Required prototype artifact not found: $Artifact"
    }
}

$Process = Get-Process -Id $ProcessId -ErrorAction Stop
if ($Process.ProcessName -ne 'DarkSoulsIII') {
    throw "PID $ProcessId is not DarkSoulsIII.exe."
}
$ExecutablePath = $Process.Path
$ExecutableHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $ExecutablePath).Hash
if ($ExecutableHash -ne $ExpectedExe) {
    throw "Exact-build check failed for ${ExecutablePath}: $ExecutableHash"
}

$Validation = Get-Content -LiteralPath $ValidationPath -Raw | ConvertFrom-Json
foreach ($Binary in $Validation.binaries) {
    $Resolved = Resolve-Path -LiteralPath (Join-Path $Here $Binary.path)
    $Actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $Resolved).Hash
    if ($Actual -ne $Binary.sha256) {
        throw "Prototype binary hash mismatch: $Resolved"
    }
}

if ($Action -eq 'inject') {
    & $Controller inject $ProcessId $Dll
} else {
    & $Controller $Action $ProcessId
}
if ($LASTEXITCODE -ne 0) {
    throw "Prototype controller action '$Action' failed with exit code $LASTEXITCODE"
}
