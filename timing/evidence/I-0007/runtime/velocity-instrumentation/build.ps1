[CmdletBinding()]
param(
    [ValidatePattern('^[A-Za-z0-9._-]+\.dll$')]
    [string]$DllName = 'I0007VelocityMirror.dll'
)

$ErrorActionPreference = 'Stop'

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$MinHook = 'C:\Users\User\Tools\DS3-Decompilation\third-party\minhook-v1.3.4'
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'

if (-not (Test-Path -LiteralPath $MinHook -PathType Container)) {
    throw "MinHook source not found: $MinHook"
}
if (-not (Test-Path -LiteralPath $VcVars -PathType Leaf)) {
    throw "MSVC environment script not found: $VcVars"
}

$ExpectedMinHookCommit = 'c3fcafdc10146beb5919319d0683e44e3c30d537'
$ActualMinHookCommit = (& git -C $MinHook rev-parse HEAD).Trim()
if ($ActualMinHookCommit -ne $ExpectedMinHookCommit) {
    throw "Unexpected MinHook revision: $ActualMinHookCommit"
}

$Obj = Join-Path $Here 'obj'
$Bin = Join-Path $Here 'bin'
$DllPath = Join-Path $Bin $DllName
$DllPdb = [System.IO.Path]::ChangeExtension($DllPath, '.pdb')
New-Item -ItemType Directory -Force -Path $Obj, $Bin | Out-Null

$Include = Join-Path $MinHook 'include'
$Source = Join-Path $MinHook 'src'
$Hde = Join-Path $Source 'hde'

$commands = @(
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hook.obj`" `"$Source\hook.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\buffer.obj`" `"$Source\buffer.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\trampoline.obj`" `"$Source\trampoline.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hde64.obj`" `"$Hde\hde64.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
    "cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\pass_c_probe.obj`" `"$Here\pass_c_probe.cpp`" /I`"$Include`"",
    "link /nologo /DLL /OUT:`"$DllPath`" /PDB:`"$DllPdb`" `"$Obj\pass_c_probe.obj`" `"$Obj\hook.obj`" `"$Obj\buffer.obj`" `"$Obj\trampoline.obj`" `"$Obj\hde64.obj`" kernel32.lib user32.lib",
    "cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\injector.obj`" `"$Here\injector.cpp`" /Fe:`"$Bin\I0007VelocityMirrorInjector.exe`" /link /PDB:`"$Bin\I0007VelocityMirrorInjector.pdb`" kernel32.lib"
)

foreach ($command in $commands) {
    $full = "call `"$VcVars`" >nul && $command"
    & $env:ComSpec /d /s /c $full
    if ($LASTEXITCODE -ne 0) {
        throw "Build command failed with exit code $LASTEXITCODE"
    }
}

Get-FileHash -Algorithm SHA256 -LiteralPath $DllPath, (Join-Path $Bin 'I0007VelocityMirrorInjector.exe') |
    Select-Object Path, Hash
