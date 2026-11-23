[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Here = $PSScriptRoot
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$Compile = 'call "' + $VcVars + '" >nul && cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /D_CRT_SECURE_NO_WARNINGS /Fo"' + $Here + '\obj\elapsed_recurrence_test.obj" "' + $Here + '\elapsed_recurrence_test.cpp" /Fe:"' + $Here + '\bin\elapsed_recurrence_test.exe"'
& $env:ComSpec /d /s /c $Compile
if ($LASTEXITCODE -ne 0) { throw 'Elapsed recurrence test build failed.' }
& (Join-Path $Here 'bin/elapsed_recurrence_test.exe')
if ($LASTEXITCODE -ne 0) { throw 'Elapsed recurrence native tests failed.' }
& node (Join-Path $Here '../test_continuous_recurrence.mjs')
if ($LASTEXITCODE -ne 0) { throw 'Native and JS capture replay comparison failed.' }
