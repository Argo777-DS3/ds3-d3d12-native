$ErrorActionPreference = 'Stop'

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$Obj = Join-Path $Here 'obj'
$Exe = Join-Path $Obj 'four_execute_self_test.exe'
New-Item -ItemType Directory -Force -Path $Obj | Out-Null

$command = "cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\four_execute_self_test.obj`" `"$Here\four_execute_self_test.cpp`" /Fe:`"$Exe`" /link kernel32.lib"
$full = "call `"$VcVars`" >nul && $command"
& $env:ComSpec /d /s /c $full
if ($LASTEXITCODE -ne 0) { throw "Four-execute build failed with exit code $LASTEXITCODE" }

& $Exe
if ($LASTEXITCODE -ne 0) { throw "Four-execute test failed with exit code $LASTEXITCODE" }
