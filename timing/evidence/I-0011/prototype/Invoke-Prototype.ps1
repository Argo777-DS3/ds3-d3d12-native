[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Inject', 'Show', 'Hide', 'FixOn', 'FixOff', 'Native',
                 'Unlimited', 'Status', 'Stop')]
    [string]$Action,

    [Parameter(Mandatory)]
    [ValidateRange(1, [int]::MaxValue)]
    [int]$ProcessId
)

$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Controller = Join-Path $Here 'bin\I0011RuntimeController.exe'
$Dll = Join-Path $Here 'bin\I0011RuntimeFpsOverlay.dll'

if (-not (Test-Path -LiteralPath $Controller -PathType Leaf)) {
    throw "Controller not found. Run build.ps1 first: $Controller"
}

if ($Action -eq 'Inject') {
    if (-not (Test-Path -LiteralPath $Dll -PathType Leaf)) {
        throw "Runtime DLL not found. Run build.ps1 first: $Dll"
    }
    & node (Join-Path $Here 'validate_exact_build.mjs') | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw 'Exact-build validation failed; injection refused.'
    }
    & $Controller inject $ProcessId $Dll
    exit $LASTEXITCODE
}

$Command = @{
    Show = 'show'
    Hide = 'hide'
    FixOn = 'fix-on'
    FixOff = 'fix-off'
    Native = 'native'
    Unlimited = 'unlimited'
    Status = 'status'
    Stop = 'stop'
}[$Action]

& $Controller $Command $ProcessId
exit $LASTEXITCODE
