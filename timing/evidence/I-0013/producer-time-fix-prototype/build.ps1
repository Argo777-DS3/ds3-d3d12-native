[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Repository=(Resolve-Path (Join-Path $Here '..\..\..')).Path
$MinHook='C:\Users\User\Tools\DS3-Decompilation\third-party\minhook-v1.3.4'
$VcVars='C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$Game='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$InjectorBase=Join-Path $Repository 'evidence\I-0007\runtime\spatial-exchange-observer\injector.cpp'
$ExpectedInjectorHash='F96D65B143D7040FABC6E585856F174AF234598911D4A12D5AAE8B8FF8A0E14D'
$ExpectedGameHash='EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'
$ExpectedMinHookCommit='c3fcafdc10146beb5919319d0683e44e3c30d537'
foreach($Path in @($MinHook,$VcVars,$Game,$InjectorBase)){if(-not(Test-Path -LiteralPath $Path)){throw "Missing dependency: $Path"}}
if((Get-FileHash -Algorithm SHA256 -LiteralPath $Game).Hash -ne $ExpectedGameHash){throw 'Unexpected game hash.'}
if((Get-FileHash -Algorithm SHA256 -LiteralPath $InjectorBase).Hash -ne $ExpectedInjectorHash){throw 'Unexpected injector source hash.'}
if((& git -C $MinHook rev-parse HEAD).Trim() -ne $ExpectedMinHookCommit){throw 'Unexpected MinHook revision.'}

$SourceFile=Join-Path $Here 'producer_time_fix.cpp'
$Text=[IO.File]::ReadAllText($SourceFile)
foreach($Forbidden in @('AddVectoredExceptionHandler','SetThreadContext','Dr0','Dr1','Dr2','Dr3','Dr7')){if($Text.Contains($Forbidden)){throw "Forbidden mechanism: $Forbidden"}}
if([regex]::Matches($Text,'MH_CreateHook\(').Count -ne 1){throw 'Exactly one hook is required.'}
foreach($Required in @('kRvaProducer = 0x005E6580','kReferenceDelta = 0.0166666005f',
    'native - sample.state_before) * ratio','std::pow','native == 0.0f','std::fabs(native) == 0.5f',
    'g_correction_enabled{false}','RestoreTiming()')){if(-not $Text.Contains($Required)){throw "Missing invariant: $Required"}}
& node (Join-Path $Here 'validate_exact_build.mjs') | Set-Content -LiteralPath (Join-Path $Here 'EXACT-BUILD-VALIDATION.json') -Encoding utf8
if($LASTEXITCODE -ne 0){throw 'Exact-build validation failed.'}
& node (Join-Path $Here 'analyze_fix_capture_test.mjs') | Set-Content -LiteralPath (Join-Path $Here 'ANALYZER-TEST.json') -Encoding utf8
if($LASTEXITCODE -ne 0){throw 'Analyzer test failed.'}

$Injector=[IO.File]::ReadAllText($InjectorBase)
$Injector=$Injector.Replace('DS3_I0007_SPATIAL_OBSERVER_','DS3_I0013_TIMEFIX_')
$Injector=$Injector.Replace('I0007SpatialExchangeObserverInjector.exe','I0013ProducerTimeFixInjector.exe')
$Injector=$Injector.Replace('<60|165>','<60|240>')
$Injector=$Injector.Replace(' | flush <pid>',' | enable <pid> | disable <pid>')
$Injector=$Injector.Replace('    if (_wcsicmp(argv[1], L"flush") == 0 && argc == 3) return Control(pid, L"FLUSH");',
    '    if (_wcsicmp(argv[1], L"enable") == 0 && argc == 3) return Control(pid, L"ENABLE");' + "`r`n" +
    '    if (_wcsicmp(argv[1], L"disable") == 0 && argc == 3) return Control(pid, L"DISABLE");')
$Injector=$Injector.Replace('L"165") == 0) return Control(pid, L"TARGET165")','L"240") == 0) return Control(pid, L"TARGET240")')
$Generated=Join-Path $Here 'injector.generated.cpp'
$Obj=Join-Path $Here 'obj';$Bin=Join-Path $Here 'bin';New-Item -ItemType Directory -Force -Path $Obj,$Bin|Out-Null
$Dll=Join-Path $Bin 'I0013ProducerTimeFix.dll';$InjectorExe=Join-Path $Bin 'I0013ProducerTimeFixInjector.exe'
$Include=Join-Path $MinHook 'include';$Source=Join-Path $MinHook 'src';$Hde=Join-Path $Source 'hde'
try{
    [IO.File]::WriteAllText($Generated,$Injector,[Text.UTF8Encoding]::new($false))
    $Commands=@(
      "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hook.obj`" `"$Source\hook.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
      "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\buffer.obj`" `"$Source\buffer.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
      "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\trampoline.obj`" `"$Source\trampoline.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
      "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hde64.obj`" `"$Hde\hde64.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
      "cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\fix.obj`" `"$SourceFile`" /I`"$Include`"",
      "link /nologo /DLL /OUT:`"$Dll`" /PDB:`"$Bin\I0013ProducerTimeFix.pdb`" `"$Obj\fix.obj`" `"$Obj\hook.obj`" `"$Obj\buffer.obj`" `"$Obj\trampoline.obj`" `"$Obj\hde64.obj`" kernel32.lib bcrypt.lib user32.lib",
      "cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\injector.obj`" `"$Generated`" /Fe:`"$InjectorExe`" /link /PDB:`"$Bin\I0013ProducerTimeFixInjector.pdb`" kernel32.lib")
    foreach($Command in $Commands){& $env:ComSpec /d /s /c "call `"$VcVars`" >nul && $Command";if($LASTEXITCODE -ne 0){throw "Build failed: $LASTEXITCODE"}}
}finally{Remove-Item -LiteralPath $Generated -Force -ErrorAction SilentlyContinue}
$Dumpbin=& $env:ComSpec /d /s /c "call `"$VcVars`" >nul && dumpbin /imports `"$Dll`""
$Imports=$Dumpbin|Select-String -Pattern '^\s+[A-Z0-9_.-]+\.dll\s*$'|ForEach-Object{$_.Line.Trim().ToUpperInvariant()}
$Expected=@('BCRYPT.DLL','KERNEL32.DLL','USER32.DLL')
if((@($Imports|Sort-Object)-join '|') -ne (@($Expected|Sort-Object)-join '|')){throw "Unexpected imports: $($Imports-join ', ')"}
$Validation=[ordered]@{schema_version=1;investigation_id='I-0013';status='pass';exact_game_sha256=$ExpectedGameHash;
 hook_rva='0x005E6580';hooks=1;starts_enabled=$false;debug_registers=$false;vectored_exception_handler=$false;
 engine_calls_added=0;writes=@('current-player owner+0x2DC only when explicitly enabled and dt differs from native reference','reversible central cadence pair');
 additive_law='s + (native_s - s) * dt / dt60';decay_law='s * retention^(dt / dt60)';native60_passthrough='bit-exact, no state write';
 reset_and_clamp_passthrough=$true;maximum_samples=4096;capture_seconds=3;imports=@($Imports);minhook_commit=$ExpectedMinHookCommit;
 outputs=@($Dll,$InjectorExe|ForEach-Object{$h=Get-FileHash -Algorithm SHA256 -LiteralPath $_;[ordered]@{file=[IO.Path]::GetFileName($_);sha256=$h.Hash;size=(Get-Item -LiteralPath $_).Length}});
 limit='Offline qualification only. Live causal and visual validation remains required.'}
$Validation|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $Here 'BUILD-VALIDATION.json') -Encoding utf8
$Validation|ConvertTo-Json -Depth 5
