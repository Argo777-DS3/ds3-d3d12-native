[CmdletBinding()]
param([string]$Out='')
# Writes compile_commands.json at the repository root (ignored by Git: it names this machine's paths) for clangd: one entry per
# native C++ file, compiled as the native builds do (C++20, MinHook includes, the generated headers of the module build), with
# the pinned MSVC's system headers taken from vcvars64 (clang-cl driver mode, -imsvc). Rerun after adding source files.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not $Out){$Out=Join-Path $root 'compile_commands.json'}
$vcvars='C:/Program Files/Microsoft Visual Studio/18/Insiders/VC/Auxiliary/Build/vcvars64.bat'
$minHook='C:/Users/User/Tools/DS3-Decompilation/third-party/minhook-v1.3.4'
$streamline='C:/Users/User/Tools/DS3-Decompilation/third-party/streamline-2.14.16'   # I-0111 (R1): its headers (streamline_host.cpp)
if(-not (Test-Path -LiteralPath $vcvars)){throw 'Pinned MSVC unavailable'}
$include=(cmd /c "`"$vcvars`" >nul && set INCLUDE" | Select-String '^INCLUDE=').Line -replace '^INCLUDE=',''
if(-not $include){throw 'vcvars64 gave no INCLUDE'}
$generated=(Join-Path $root 'evidence/I-0066/artifacts/game-bootstrap-build').Replace('\','/')
$nrd='C:/Users/User/Tools/DS3-Decompilation/third-party/nrd-4.17.3-download/NRD'   # I-0126 R7c: NRD's headers (rt_denoise.cpp), its generated nrd_shaders.h
$common=@('clang-cl','/std:c++20','/EHsc','/W4',"/I$minHook/include","/I$minHook/src","/I$minHook/src/hde","/I$generated","/I$streamline/include","/I$nrd/Include","/I$generated/nrd",'/DNRD_STATIC_LIBRARY=1') +
    @($include -split ';' | Where-Object {$_} | ForEach-Object {"-imsvc$_"})
$entries=foreach($file in Get-ChildItem -LiteralPath (Join-Path $root 'native') -Recurse -Filter *.cpp | Where-Object {$_.FullName -notmatch '\\third_party\\'}){
    [ordered]@{directory=$file.DirectoryName -replace '\\','/';file=$file.FullName -replace '\\','/';arguments=@($common+@('/c',($file.FullName -replace '\\','/')))}}
$entries | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $Out -Encoding utf8
"compile_commands.json: $(@($entries).Count) files -> $Out"
