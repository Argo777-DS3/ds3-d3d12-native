$ErrorActionPreference='Stop'
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Out=Join-Path $Here '../artifacts/observer-build'
New-Item -ItemType Directory -Force $Out|Out-Null
$VcVars='C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
foreach($name in @('capture_test','execute_test','observer','controller')) {
    $flags=if($name -eq 'observer'){'/LD'}else{''}
    $extension=if($name -eq 'observer'){'dll'}else{'exe'}
    $command="call `"$VcVars`" >nul && cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 $flags /Fo`"$Out\$name.obj`" `"$Here\$name.cpp`" /Fe:`"$Out\$name.$extension`" /link kernel32.lib"
    & $env:ComSpec /d /s /c $command
    if($LASTEXITCODE -ne 0){throw "Build failed: $name"}
}
foreach($name in @('capture_test','execute_test')) {
    & (Join-Path $Out "$name.exe")
    if($LASTEXITCODE -ne 0){throw "Test failed: $name ($LASTEXITCODE)"}
}
node (Join-Path $Here 'analyzer_test.mjs')
if($LASTEXITCODE -ne 0){throw 'Analyzer tests failed'}
$Binaries=@('capture_test.exe','execute_test.exe','observer.dll','controller.exe')|ForEach-Object {
    [ordered]@{name=$_;sha256=(Get-FileHash (Join-Path $Out $_) -Algorithm SHA256).Hash}
}
$Sources=Get-ChildItem -LiteralPath $Here -File|ForEach-Object {
    [ordered]@{name=$_.Name;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}
}
[ordered]@{schema_version=1;built_at=[DateTime]::UtcNow.ToString('o');binaries=@($Binaries);sources=@($Sources);
    native_capture='PASS ten ring positions/wrap, bit-exact values, unchanged context/memory and rejection cases';
    native_execute='PASS actual DLL controller/VEH, two4x256 arms,1024 exact getter samples, caller reads, RF resume and verified debug restoration';
    analyzer='PASS paired slots/wrap; rejects mismatch, overflow, failed restoration and truncation';
    live_status='NOT RUN. DLL load/control/logging against DS3 and runtime overhead remain unqualified. No game was opened by these tests.'
}|ConvertTo-Json -Depth 8|Set-Content (Join-Path $Out 'validation.json')

