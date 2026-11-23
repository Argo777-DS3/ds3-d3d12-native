[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $Here '..\..\..')).Path
$MinHook = 'C:\Users\User\Tools\DS3-Decompilation\third-party\minhook-v1.3.4'
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$ExpectedMinHookCommit = 'c3fcafdc10146beb5919319d0683e44e3c30d537'

if (-not (Test-Path -LiteralPath $MinHook -PathType Container)) {
    throw "MinHook source not found: $MinHook"
}
if (-not (Test-Path -LiteralPath $VcVars -PathType Leaf)) {
    throw "MSVC environment script not found: $VcVars"
}
$ActualMinHookCommit = (& git -C $MinHook rev-parse HEAD).Trim()
if ($ActualMinHookCommit -ne $ExpectedMinHookCommit) {
    throw "Unexpected MinHook revision: $ActualMinHookCommit"
}

$Config = Get-Content -LiteralPath (Join-Path $Here 'prototype-config.txt') -Raw
foreach ($Required in @('target_fps=240', 'start_enabled=0')) {
    if ($Config -notmatch "(?m)^$([regex]::Escape($Required))$") {
        throw "Disabled-by-default config is missing: $Required"
    }
}

$Obj = Join-Path $Here 'obj'
$Bin = Join-Path $Here 'bin'
New-Item -ItemType Directory -Force -Path $Obj, $Bin | Out-Null
$Include = Join-Path $MinHook 'include'
$Source = Join-Path $MinHook 'src'
$Hde = Join-Path $Source 'hde'
$Dll = Join-Path $Bin 'I0010Locomotion240Prototype.dll'
$Controller = Join-Path $Bin 'I0010PrototypeController.exe'
$SelfTest = Join-Path $Obj 'I0010PrototypeSelfTest.exe'

$Commands = @(
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hook.obj`" `"$Source\hook.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\buffer.obj`" `"$Source\buffer.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\trampoline.obj`" `"$Source\trampoline.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hde64.obj`" `"$Hde\hde64.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\prototype.obj`" `"$Here\prototype.cpp`" /I`"$Include`"",
    "link /nologo /DLL /OUT:`"$Dll`" /PDB:`"$Bin\I0010Locomotion240Prototype.pdb`" `"$Obj\prototype.obj`" `"$Obj\hook.obj`" `"$Obj\buffer.obj`" `"$Obj\trampoline.obj`" `"$Obj\hde64.obj`" kernel32.lib user32.lib",
    "cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\injector.obj`" `"$Here\injector.cpp`" /Fe:`"$Controller`" /link /PDB:`"$Bin\I0010PrototypeController.pdb`" kernel32.lib",
    "cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\prototype_self_test.obj`" `"$Here\prototype_self_test.cpp`" /Fe:`"$SelfTest`" /link kernel32.lib user32.lib"
)

foreach ($Command in $Commands) {
    $Full = "call `"$VcVars`" >nul && $Command"
    & $env:ComSpec /d /s /c $Full
    if ($LASTEXITCODE -ne 0) {
        throw "Build command failed with exit code $LASTEXITCODE"
    }
}

$SelfTestOutput = & $SelfTest 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "Prototype self-test failed: $SelfTestOutput"
}
$SelfTestOutput | Set-Content -LiteralPath (Join-Path $Here 'native-test-output.txt') -Encoding utf8

$DerivationOutput = & node (Join-Path $Here 'validate_derivation.mjs') 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "I-0009 derivation audit failed: $DerivationOutput"
}
$DerivationValidationPath = Join-Path $Here 'DERIVATION-VALIDATION.json'
$DerivationValidation = Get-Content -LiteralPath $DerivationValidationPath -Raw |
    ConvertFrom-Json

$SourceFiles = @(
    'prototype.cpp', 'injector.cpp', 'prototype_core.hpp', 'debug_config.hpp',
    'public_status.hpp', 'prototype_self_test.cpp', 'prototype-config.txt',
    'build.ps1', 'validate_derivation.mjs'
) | ForEach-Object { Join-Path $Here $_ }
$UpstreamFiles = @(
    (Join-Path $Here '..\..\I-0007\runtime\causal-patch\distance_normalization.hpp'),
    (Join-Path $Here '..\..\I-0007\runtime\causal-patch\patch_debug_config.hpp'),
    (Join-Path $Here '..\..\I-0007\runtime\causal-patch\pass_c_probe.cpp')
) | ForEach-Object { (Resolve-Path -LiteralPath $_).Path }

function Get-RepoRelativePath([string]$Path) {
    $Resolved = (Resolve-Path -LiteralPath $Path).Path
    if (-not $Resolved.StartsWith($RepoRoot + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Artifact is outside repository: $Resolved"
    }
    return $Resolved.Substring($RepoRoot.Length + 1).Replace('\', '/')
}

function Get-PrototypeRelativePath([string]$Path) {
    $Resolved = (Resolve-Path -LiteralPath $Path).Path
    if (-not $Resolved.StartsWith($Here + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Binary is outside prototype directory: $Resolved"
    }
    return $Resolved.Substring($Here.Length + 1).Replace('\', '/')
}

$Validation = [ordered]@{
    schema_version = 1
    investigation_id = 'I-0010'
    built_at = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    target = 'DarkSoulsIII.exe 1.15.2.0 / BuildID 10167187 / 240 FPS only'
    target_executable_sha256 = 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'
    start_state = 'STANDBY'
    start_enabled = $false
    minhook_commit = $ActualMinHookCommit
    i0009_derivation = [ordered]@{
        passed = [bool]$DerivationValidation.passed
        base_commit = $DerivationValidation.base_commit
        validation_path = Get-RepoRelativePath $DerivationValidationPath
        validation_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $DerivationValidationPath).Hash
    }
    source_files = @($SourceFiles | ForEach-Object {
        $Hash = Get-FileHash -Algorithm SHA256 -LiteralPath $_
        [ordered]@{ path = Get-RepoRelativePath $_; sha256 = $Hash.Hash }
    })
    accepted_upstream_files = @($UpstreamFiles | ForEach-Object {
        $Hash = Get-FileHash -Algorithm SHA256 -LiteralPath $_
        [ordered]@{ path = Get-RepoRelativePath $_; sha256 = $Hash.Hash }
    })
    binaries = @($Dll, $Controller | ForEach-Object {
        $Hash = Get-FileHash -Algorithm SHA256 -LiteralPath $_
        [ordered]@{ path = Get-PrototypeRelativePath $_; sha256 = $Hash.Hash; bytes = (Get-Item -LiteralPath $_).Length }
    })
    native_test = [ordered]@{
        passed = $true
        output = @($SelfTestOutput)
    }
}
$Validation | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath (Join-Path $Here 'BUILD-VALIDATION.json') -Encoding utf8

Get-FileHash -Algorithm SHA256 -LiteralPath $Dll, $Controller |
    Select-Object Path, Hash
Write-Output 'PASS: prototype built in STANDBY configuration and native safety tests passed.'
