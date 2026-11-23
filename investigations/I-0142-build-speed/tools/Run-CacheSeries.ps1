# I-0142 step 2's checks: -Cache builds (-NoControls) of one tree, with one source, one shader, one header and one .inc of
# game_bootstrap touched (their dates only), then the cache's log removed and everything rebuilt. The sources never change, so every
# published module must have one hash, and the objects must be the same after the complete rebuild. Writes only under -Evidence.
param([string]$Repo=(Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))),
      [string]$Evidence='C:/Users/User/Desktop/DS3_evidences/I-0142',[string]$Tag='final')
$ErrorActionPreference='Stop'
$cache="$Evidence/cache-$Tag"
$report="$Evidence/cache-series-$Tag.txt"
$modules=[ordered]@{}
# Never over an earlier series (the fix review's L6): a new -Tag each time.
if((Test-Path -LiteralPath $report) -or (Test-Path -LiteralPath $cache) -or @(Get-ChildItem -LiteralPath $Evidence -Directory -Filter "cache-$Tag-out-*" -ErrorAction SilentlyContinue).Count){ throw "A series tagged $Tag exists in ${Evidence}: choose another -Tag" }
$least=@{first=289;nothing=0;cpp=3;shader=6;header=10;inc=3;complete=289}   # the fix review's L7: each touch reruns at least its unit, the module and dumpbin
function Hash-Objects { $h=@{}; Get-ChildItem -LiteralPath $cache -Filter *.obj -File | ForEach-Object { $h[$_.Name]=(Get-FileHash -LiteralPath $_.FullName).Hash }; $h }
function Build([string]$name){
    $t=Get-Date
    $log="$Evidence/cache-$Tag-$name.log"
    $published="$Evidence/cache-$Tag-out-$name"
    & "$Repo/native/frame/Build-GameBootstrap.ps1" -Out $published -Cache $cache -NoControls *> $log
    $line=(Select-String -LiteralPath $log -Pattern 'Ninja build:' | Select-Object -Last 1).Line
    $modules[$name]=(Get-FileHash -LiteralPath "$published/game_bootstrap.dll").Hash
    $ran=[int][regex]::Match($line,'Ninja build: (\d+) of').Groups[1].Value
    if($ran -lt $least[$name]){ Add-Content -LiteralPath $report -Value "FAIL $name ran $ran commands, at least $($least[$name]) expected" }
    $msg="{0,-9} {1,5:N0} s  module {2}  {3}" -f $name,((Get-Date)-$t).TotalSeconds,$modules[$name].Substring(0,8),$line
    Add-Content -LiteralPath $report -Value $msg; $msg
    Remove-Item -LiteralPath $published -Recurse -Force   # a disposable output (D-0142): its module's hash is kept above
}
function Touch([string]$path){ (Get-Item -LiteralPath "$Repo/$path").LastWriteTime=Get-Date; Add-Content -LiteralPath $report -Value "touched $path" }
Set-Content -LiteralPath $report -Value "I-0142 cache series $Tag, $(Get-Date -Format o), $(git -C $Repo rev-parse --short HEAD)"
Build 'first'
Build 'nothing'
Touch 'native/renderer/backend.cpp'; Build 'cpp'
Touch 'native/renderer/shaders/rt_shadow.hlsl'; Build 'shader'
Touch 'native/renderer/scene_pass.h'; Build 'header'
Touch 'native/frame/game_bootstrap/17_render_step.inc'; Build 'inc'
$before=Hash-Objects
Remove-Item -LiteralPath "$cache/.ninja_log","$cache/.ninja_deps" -Force
Build 'complete'
$after=Hash-Objects
$diff=@($before.Keys | Where-Object { $after[$_] -ne $before[$_] })
Add-Content -LiteralPath $report -Value ("objects: {0} before, {1} after the complete rebuild, {2} differing" -f $before.Count,$after.Count,$diff.Count)
Add-Content -LiteralPath $report -Value ("modules: {0} builds, {1} distinct hash(es)" -f $modules.Count,@($modules.Values | Select-Object -Unique).Count)
Get-Content -LiteralPath $report
