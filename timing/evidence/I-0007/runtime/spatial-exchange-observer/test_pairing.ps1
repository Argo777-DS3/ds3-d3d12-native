$ErrorActionPreference = 'Stop'

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$Obj = Join-Path $Here 'obj'
$Exe = Join-Path $Obj 'pairing_self_test.exe'
New-Item -ItemType Directory -Force -Path $Obj | Out-Null

$command = "cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\pairing_self_test.obj`" `"$Here\pairing_self_test.cpp`" /Fe:`"$Exe`""
$full = "call `"$VcVars`" >nul && $command"
& $env:ComSpec /d /s /c $full
if ($LASTEXITCODE -ne 0) { throw "Native pairing build failed with exit code $LASTEXITCODE" }

& $Exe
if ($LASTEXITCODE -ne 0) { throw "Native pairing test failed with exit code $LASTEXITCODE" }
