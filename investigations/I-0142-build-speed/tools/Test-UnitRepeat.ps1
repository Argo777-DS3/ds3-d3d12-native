# I-0142: is one unit's object the same on every compilation? Touches the unit's source date, rebuilds the cache (-NoControls), and
# hashes the object and the published module, -Times times. Writes only under -Evidence (each published folder removed after).
param([string]$Repo=(Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))),
      [string]$Evidence='C:/Users/User/Desktop/DS3_evidences/I-0142',[string]$Cache='C:/Users/User/Desktop/DS3_evidences/I-0142/cache-final',
      [string]$Source='native/frame/game_bootstrap.cpp',[string]$Object='game_bootstrap.obj',[int]$Times=3,[string]$Tag='repeat')
$ErrorActionPreference='Stop'
$report="$Evidence/unit-$Tag.txt"
if(Test-Path -LiteralPath $report){ throw "$report exists: choose another -Tag" }
Set-Content -LiteralPath $report -Value "I-0142 unit repeat $Tag, $(Get-Date -Format o), $(git -C $Repo rev-parse --short HEAD), $Source"
Add-Content -LiteralPath $report -Value ("before    object {0}" -f (Get-FileHash -LiteralPath "$Cache/$Object").Hash)
for($i=1;$i -le $Times;$i++){
    (Get-Item -LiteralPath "$Repo/$Source").LastWriteTime=Get-Date
    $published="$Evidence/unit-$Tag-out-$i"; $t=Get-Date
    & "$Repo/native/frame/Build-GameBootstrap.ps1" -Out $published -Cache $Cache -NoControls *> "$Evidence/unit-$Tag-$i.log"
    $line=(Select-String -LiteralPath "$Evidence/unit-$Tag-$i.log" -Pattern 'Ninja build:' | Select-Object -Last 1).Line
    Add-Content -LiteralPath $report -Value ("build {0}   object {1}  module {2}  {3:N0} s  {4}" -f $i,(Get-FileHash -LiteralPath "$Cache/$Object").Hash,(Get-FileHash -LiteralPath "$published/game_bootstrap.dll").Hash,((Get-Date)-$t).TotalSeconds,$line)
    Remove-Item -LiteralPath $published -Recurse -Force
}
Get-Content -LiteralPath $report
