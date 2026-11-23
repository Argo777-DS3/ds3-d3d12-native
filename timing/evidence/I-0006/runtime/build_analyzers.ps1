$ErrorActionPreference = 'Stop'

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$Bin = Join-Path $Here 'bin'
$Obj = Join-Path $Here 'obj'
New-Item -ItemType Directory -Force -Path $Bin, $Obj | Out-Null

$Source = Join-Path $Here 'reconstruct_writer_mirror.cpp'
$Object = Join-Path $Obj 'reconstruct_writer_mirror.obj'
$Executable = Join-Path $Bin 'reconstruct_writer_mirror.exe'
$commands = @(
    "cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /fp:strict /Fo`"$Object`" `"$Source`"",
    "link /nologo /OUT:`"$Executable`" `"$Object`""
)

foreach ($command in $commands) {
    $full = "call `"$VcVars`" >nul && $command"
    & $env:ComSpec /d /s /c $full
    if ($LASTEXITCODE -ne 0) {
        throw "Analyzer build failed with exit code $LASTEXITCODE"
    }
}

Get-FileHash -Algorithm SHA256 -LiteralPath $Executable | Select-Object Path, Hash
