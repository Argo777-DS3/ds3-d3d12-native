[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$out=Join-Path $root 'evidence/I-0066/artifacts/game-bootstrap-build'
$vcvars='C:/Program Files/Microsoft Visual Studio/18/Insiders/VC/Auxiliary/Build/vcvars64.bat'
$dependencies=@('backend','content','acquire')
foreach($unit in $dependencies){
    if(-not (Test-Path -LiteralPath "$out/$unit.obj")){throw 'Build the native bootstrap before this control'}
}
$commands=@('@echo off',"call `"$vcvars`" >nul || exit /b 1")
foreach($unit in @('game_model_selection','game_model_selection_test')){
    $commands+="cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Brepro /Gy /Fo`"$out/$unit.obj`" `"$PSScriptRoot/$unit.cpp`" || exit /b 1"
}
$objects=@('game_model_selection_test','game_model_selection')+$dependencies
$links=($objects | ForEach-Object {"`"$out/$_.obj`""}) -join ' '
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_model_selection_test.exe`" $links d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
Set-Content -LiteralPath "$out/model-selection-test.cmd" -Value $commands -Encoding ascii
& cmd /c "`"$out/model-selection-test.cmd`""
if($LASTEXITCODE){throw "Native model selection control build failed: $LASTEXITCODE"}
foreach($profile in @('warp','hardware')){
    & "$out/game_model_selection_test.exe" $profile *> "$out/model-selection-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/model-selection-$profile.txt";throw "Native model selection control failed: $profile"}
    Get-Content -LiteralPath "$out/model-selection-$profile.txt"
}
