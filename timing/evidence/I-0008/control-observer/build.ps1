$ErrorActionPreference='Stop'
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Out=Join-Path $Here '../artifacts/control-observer-build'
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
    capture='PASS generator/control entry/exit/player-ring fields and rejection cases';
    native_execute='PASS timing 30/0 to165/1 to30/0, two hardware-breakpoint arms, all sites and exact debug restoration';
    analyzer='PASS exact chained values; rejects mismatch/restoration/truncation and distinguishes variable delta from fixed60';
    live_status='NOT RUN. DLL load, DS3 runtime coverage and overhead remain unqualified. No game was opened by these tests.'
}|ConvertTo-Json -Depth 8|Set-Content (Join-Path $Out 'validation.json')
