[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$out=Join-Path $root 'evidence/I-0066/artifacts/game-bootstrap-build'
$vcvars='C:/Program Files/Microsoft Visual Studio/18/Insiders/VC/Auxiliary/Build/vcvars64.bat'
foreach($unit in @('backend','content','acquire','scene_view','streamline_host')) {
    if(-not (Test-Path -LiteralPath "$out/$unit.obj")){throw 'Build native bootstrap before this control'}
}
$commands=@('@echo off',"call `"$vcvars`" >nul || exit /b 1")
foreach($unit in @('game_model_selection','scene_ingress','game_model_preparation_test')) {
    $commands+="cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Brepro /Gy /Fo`"$out/$unit.obj`" `"$PSScriptRoot/$unit.cpp`" || exit /b 1"
}
$commands+="cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Brepro /Gy /Fo`"$out/model_programs.obj`" `"$root/native/content/model_programs.cpp`" || exit /b 1"
$commands+="cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Brepro /Gy /Fo`"$out/model_parameters.obj`" `"$root/native/content/model_parameters.cpp`" || exit /b 1"
$commands+="cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Brepro /Gy /Fo`"$out/model_textures.obj`" `"$root/native/content/model_textures.cpp`" || exit /b 1"
$commands+="cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Brepro /Gy /Fo`"$out/producers.obj`" `"$root/native/world/producers.cpp`" || exit /b 1"
# 1186: model_textures calls ProduceTextureTiling (texture_tiling.cpp).
$commands+="cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Brepro /Gy /Fo`"$out/texture_tiling.obj`" `"$root/native/content/texture_tiling.cpp`" || exit /b 1"
$objects=@('game_model_preparation_test','game_model_selection','scene_ingress','scene_view','model_programs','model_parameters','model_textures','texture_tiling','producers','acquire','backend','content','streamline_host')   # I-0115: backend's presenter calls Streamline's host (I-0111)
$links=($objects | ForEach-Object {"`"$out/$_.obj`""}) -join ' '
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_model_preparation_test.exe`" $links d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
Set-Content -LiteralPath "$out/model-preparation-test.cmd" -Value $commands -Encoding ascii
& cmd /c "`"$out/model-preparation-test.cmd`""
if($LASTEXITCODE){throw "Native model preparation control build failed: $LASTEXITCODE"}
& "$out/game_model_preparation_test.exe" *> "$out/model-preparation.txt"
Get-Content -LiteralPath "$out/model-preparation.txt"
if($LASTEXITCODE){throw 'Native model preparation control failed'}
