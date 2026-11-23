$ErrorActionPreference='Stop'
$Repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Out=Join-Path $PSScriptRoot 'artifacts/static-control-step'
$Image='C:/Users/User/Desktop/DS3 Decompilation/evidence/I-0002/artifacts/staging/DarkSoulsIII.exe'
if((Get-FileHash $Image -Algorithm SHA256).Hash -ne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'){throw 'Wrong build'}
node (Join-Path $PSScriptRoot 'extract_control_kernel.mjs') $Image (Join-Path $Out 'control-kernel.bin')
if($LASTEXITCODE){throw 'Extraction failed'}
$Vc='C:/Program Files/Microsoft Visual Studio/18/Insiders/VC/Auxiliary/Build/vcvars64.bat'
$Source=Join-Path $PSScriptRoot 'control_formula_test.cpp'
$Command="call `"$Vc`" >nul && cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /fp:strict /Fo`"$Out/control_formula_test.obj`" `"$Source`" /Fe:`"$Out/control_formula_test.exe`" /link kernel32.lib"
& $env:ComSpec /d /s /c $Command
if($LASTEXITCODE){throw 'Compile failed'}
& "$Out/control_formula_test.exe" "$Out/control-kernel.bin"
if($LASTEXITCODE){throw 'Native formula comparison failed'}
