$ErrorActionPreference = 'Stop'

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$Obj = Join-Path $Here 'obj'
$Exe = Join-Path $Obj 'writer_mirror_self_test.exe'
New-Item -ItemType Directory -Force -Path $Obj | Out-Null

$CppObj = Join-Path $Obj 'writer_mirror_self_test.obj'
$AsmObj = Join-Path $Obj 'writer_mirror_self_test_asm.obj'
$commands = @(
    "ml64 /nologo /c /Fo`"$AsmObj`" `"$Here\writer_mirror_self_test.asm`"",
    "cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$CppObj`" `"$Here\writer_mirror_self_test.cpp`"",
    "link /nologo /OUT:`"$Exe`" `"$CppObj`" `"$AsmObj`" kernel32.lib"
)

foreach ($command in $commands) {
    $full = "call `"$VcVars`" >nul && $command"
    & $env:ComSpec /d /s /c $full
    if ($LASTEXITCODE -ne 0) {
        throw "Writer-mirror self-test build failed with exit code $LASTEXITCODE"
    }
}

& $Exe
if ($LASTEXITCODE -ne 0) {
    throw "Writer-mirror self-test failed with exit code $LASTEXITCODE"
}
