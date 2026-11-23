[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $Here '..\..\..')).Path
$MinHook = 'C:\Users\User\Tools\DS3-Decompilation\third-party\minhook-v1.3.4'
$ReShade = 'C:\Users\User\Tools\DS3-Decompilation\third-party\reshade-6.8.0'
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$ExpectedMinHookCommit = 'c3fcafdc10146beb5919319d0683e44e3c30d537'
$ExpectedReShadeCommit = '18deaa52de0c425a78b329e9cb3c497281cd00ec'
$ExpectedImGuiCommit = '3912b3d9a9c1b3f17431aebafd86d2f40ee6e59c'

foreach ($Dependency in @($MinHook, $ReShade)) {
    if (-not (Test-Path -LiteralPath $Dependency -PathType Container)) {
        throw "Dependency source not found: $Dependency"
    }
}
if (-not (Test-Path -LiteralPath $VcVars -PathType Leaf)) {
    throw "MSVC environment script not found: $VcVars"
}
$ActualMinHookCommit = (& git -C $MinHook rev-parse HEAD).Trim()
$ActualReShadeCommit = (& git -C $ReShade rev-parse HEAD).Trim()
$ActualImGuiCommit = (& git -C (Join-Path $ReShade 'deps\imgui') rev-parse HEAD).Trim()
if ($ActualMinHookCommit -ne $ExpectedMinHookCommit) {
    throw "Unexpected MinHook revision: $ActualMinHookCommit"
}
if ($ActualReShadeCommit -ne $ExpectedReShadeCommit) {
    throw "Unexpected ReShade revision: $ActualReShadeCommit"
}
if ($ActualImGuiCommit -ne $ExpectedImGuiCommit) {
    throw "Unexpected ImGui revision: $ActualImGuiCommit"
}

$Obj = Join-Path $Here 'obj'
$Bin = Join-Path $Here 'bin'
New-Item -ItemType Directory -Force -Path $Obj, $Bin | Out-Null
$MinHookInclude = Join-Path $MinHook 'include'
$MinHookSource = Join-Path $MinHook 'src'
$Hde = Join-Path $MinHookSource 'hde'
$ReShadeInclude = Join-Path $ReShade 'include'
$ImGuiInclude = Join-Path $ReShade 'deps\imgui'
$Addon = Join-Path $Bin 'DS3TerrainCapture.addon64'
$SelfTest = Join-Path $Obj 'I0013CoreSelfTest.exe'

$Commands = @(
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hook.obj`" `"$MinHookSource\hook.c`" /I`"$MinHookInclude`" /I`"$MinHookSource`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\buffer.obj`" `"$MinHookSource\buffer.c`" /I`"$MinHookInclude`" /I`"$MinHookSource`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\trampoline.obj`" `"$MinHookSource\trampoline.c`" /I`"$MinHookInclude`" /I`"$MinHookSource`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hde64.obj`" `"$Hde\hde64.c`" /I`"$MinHookInclude`" /I`"$MinHookSource`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\timing_core.obj`" `"$Here\timing_core.cpp`" /I`"$MinHookInclude`"",
    "cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\reshade_addon.obj`" `"$Here\reshade_addon.cpp`" /I`"$ReShadeInclude`" /I`"$ImGuiInclude`"",
    "link /nologo /DLL /OUT:`"$Addon`" /PDB:`"$Bin\DS3TerrainCapture.pdb`" `"$Obj\timing_core.obj`" `"$Obj\reshade_addon.obj`" `"$Obj\hook.obj`" `"$Obj\buffer.obj`" `"$Obj\trampoline.obj`" `"$Obj\hde64.obj`" kernel32.lib user32.lib bcrypt.lib",
    "cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\runtime_self_test.obj`" `"$Here\runtime_self_test.cpp`" /Fe:`"$SelfTest`" /link kernel32.lib"
)

foreach ($Command in $Commands) {
    $Full = "call `"$VcVars`" >nul && $Command"
    & $env:ComSpec /d /s /c $Full
    if ($LASTEXITCODE -ne 0) {
        throw "Build command failed with exit code $LASTEXITCODE"
    }
}

$DumpCommand = "call `"$VcVars`" >nul && dumpbin /nologo /headers /exports /dependents `"$Addon`""
$Dump = (& $env:ComSpec /d /s /c $DumpCommand 2>&1) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw 'PE inspection failed.'
}
foreach ($Required in @(
    'machine \(x64\)', 'AddonUninit', 'DESCRIPTION', 'NAME',
    'KERNEL32.dll', 'bcrypt.dll')) {
    if ($Dump -notmatch $Required) {
        throw "Required add-on PE property is missing: $Required"
    }
}
if ($Dump -match '(?m)^\s+ReShade[^\r\n]*\.dll\s*$') {
    throw 'The add-on must resolve ReShade dynamically, not import a host DLL.'
}
$PeValidation = [ordered]@{
    schema_version = 1
    investigation_id = 'I-0013'
    validated_at = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    machine = 'x64'
    extension = '.addon64'
    exports = @('AddonUninit', 'DESCRIPTION', 'NAME')
    dependencies = @('KERNEL32.dll', 'bcrypt.dll')
    reshade_import = $false
    relation = 'ReShade API and ImGui function table are resolved through the official header at runtime'
}
$PeValidationPath = Join-Path $Here 'PE-VALIDATION.json'
$PeValidation | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath $PeValidationPath -Encoding utf8

$SelfTestRuns = @()
for ($Run = 1; $Run -le 10; $Run++) {
    $Output = & $SelfTest 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Runtime self-test run $Run failed: $Output"
    }
    $SelfTestRuns += "run=$Run $($Output -join ' ')"
}
$SelfTestRuns | Set-Content -LiteralPath (Join-Path $Here 'NATIVE-RESTORATION-STRESS.txt') -Encoding utf8
$SelfTestRuns[0] | Set-Content -LiteralPath (Join-Path $Here 'native-test-output.txt') -Encoding utf8

$AnalyzerOutput = & node (Join-Path $Here 'analyze_capture_test.mjs') 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "Trajectory analyzer tests failed: $AnalyzerOutput"
}
$AnalyzerOutput | Set-Content -LiteralPath (Join-Path $Here 'ANALYZER-TEST.json') -Encoding utf8

$DerivationOutput = & node (Join-Path $Here 'validate_derivation.mjs') 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "Derivation validation failed: $DerivationOutput"
}
$DerivationPath = Join-Path $Here 'DERIVATION-VALIDATION.json'
$Derivation = Get-Content -LiteralPath $DerivationPath -Raw | ConvertFrom-Json

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

$SourceFiles = @(
    'timing_core.cpp', 'timing_core.hpp', 'control_core.hpp', 'capture_core.hpp',
    'reshade_addon.cpp', 'runtime_self_test.cpp', 'build.ps1',
    'validate_derivation.mjs', 'validate_exact_build.mjs',
    'analyze_capture.mjs', 'analyze_capture_test.mjs',
    'Manage-Addon.ps1', 'README.md', 'LIVE-PROTOCOL.md'
) | ForEach-Object { Join-Path $Here $_ }
$UpstreamFiles = @(
    (Join-Path $Here '..\..\I-0010\prototype\prototype_core.hpp'),
    (Join-Path $Here '..\..\I-0010\prototype\debug_config.hpp'),
    (Join-Path $Here '..\..\I-0012\prototype\control_core.hpp')
) | ForEach-Object { (Resolve-Path -LiteralPath $_).Path }
$ReShadeHeader = Join-Path $ReShade 'include\reshade.hpp'
$ReShadeLicense = Join-Path $ReShade 'LICENSE.md'

$Validation = [ordered]@{
    schema_version = 1
    investigation_id = 'I-0013'
    built_at = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    target = 'DarkSoulsIII.exe 1.15.2.0 / BuildID 10167187'
    target_executable_sha256 = 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'
    start_state = 'STANDBY'
    start_inert = $true
    architecture = 'I-0012 cadence/correction core plus bounded broad-update trajectory capture, statically linked into the same ReShade-only UI host'
    minhook_commit = $ActualMinHookCommit
    reshade = [ordered]@{
        version = '6.8.0'
        api_version = 20
        commit = $ActualReShadeCommit
        imgui_commit = $ActualImGuiCommit
        reshade_hpp_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $ReShadeHeader).Hash
        license_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $ReShadeLicense).Hash
    }
    unlimited = 'unsupported; no proven limiter-off timing representation'
    derivation = [ordered]@{
        passed = [bool]$Derivation.passed
        validation_path = Get-RepoRelativePath $DerivationPath
        validation_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $DerivationPath).Hash
    }
    pe_validation = [ordered]@{
        passed = $true
        validation_path = Get-RepoRelativePath $PeValidationPath
        validation_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $PeValidationPath).Hash
    }
    source_files = @($SourceFiles | ForEach-Object {
        $Hash = Get-FileHash -Algorithm SHA256 -LiteralPath $_
        [ordered]@{ path = Get-RepoRelativePath $_; sha256 = $Hash.Hash }
    })
    accepted_upstream_files = @($UpstreamFiles | ForEach-Object {
        $Hash = Get-FileHash -Algorithm SHA256 -LiteralPath $_
        [ordered]@{ path = Get-RepoRelativePath $_; sha256 = $Hash.Hash }
    })
    binaries = @($Addon | ForEach-Object {
        $Hash = Get-FileHash -Algorithm SHA256 -LiteralPath $_
        [ordered]@{ path = Get-PrototypeRelativePath $_; sha256 = $Hash.Hash; bytes = (Get-Item -LiteralPath $_).Length }
    })
    native_test = [ordered]@{
        passed = $true
        runs = 10
        output = @($SelfTestRuns[0])
    }
    analyzer_test = [ordered]@{
        passed = $true
        output_path = 'ANALYZER-TEST.json'
    }
}
$Validation | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath (Join-Path $Here 'BUILD-VALIDATION.json') -Encoding utf8

$ExactOutput = & node (Join-Path $Here 'validate_exact_build.mjs') 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "Exact-build validation failed: $ExactOutput"
}

Get-FileHash -Algorithm SHA256 -LiteralPath $Addon | Select-Object Path, Hash
Write-Output 'PASS: I-0013 terrain capture add-on and offline safety/analyzer suites passed.'
