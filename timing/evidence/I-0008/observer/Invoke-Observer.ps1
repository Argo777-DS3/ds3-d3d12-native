[CmdletBinding()]
param([Parameter(Mandatory)][int]$GameProcessId)
$ErrorActionPreference='Stop'
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Build=Join-Path $Here '../artifacts/observer-build'
$Game=Get-Process -Id $GameProcessId
if($Game.ProcessName -ne 'DarkSoulsIII'){throw 'Selected process is not DarkSoulsIII'}
$hash=(Get-FileHash -LiteralPath $Game.MainModule.FileName -Algorithm SHA256).Hash
if($hash -ne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'){throw 'Wrong executable build'}
$Dll=(Resolve-Path (Join-Path $Build 'observer.dll')).Path
if(@($Game.Modules | Where-Object { $_.FileName -eq $Dll -or $_.ModuleName -match '^I000[567].*(Probe|Observer|Patch)' }).Count){throw 'Existing instrument detected; require clean process'}
$validation=Get-Content (Join-Path $Build 'validation.json') -Raw|ConvertFrom-Json
foreach($item in $validation.binaries){if((Get-FileHash (Join-Path $Build $item.name) -Algorithm SHA256).Hash -ne $item.sha256){throw 'Observer binary identity mismatch'}}
$Started=Get-Date
& (Join-Path $Build 'controller.exe') observe $GameProcessId $Dll
if($LASTEXITCODE -ne 0){throw 'Observer did not complete; do not reinject'}
$files=@(Get-ChildItem -LiteralPath $env:TEMP -Filter "I0008-ring-$GameProcessId-*.bin" | Where-Object { $_.LastWriteTime -ge $Started.AddSeconds(-1) })
if($files.Count -ne 1){throw 'Capture file identity is ambiguous'}
$Capture=Join-Path $Here "../artifacts/live-$($Started.ToUniversalTime().ToString('yyyyMMddTHHmmssZ'))-pid$GameProcessId"
New-Item -ItemType Directory -Path $Capture|Out-Null
Copy-Item -LiteralPath $files[0].FullName -Destination (Join-Path $Capture 'capture.bin')
node (Join-Path $Here 'analyze_capture.mjs') (Join-Path $Capture 'capture.bin') (Join-Path $Capture 'analysis.json')
if($LASTEXITCODE -ne 0){throw 'Capture parsing failed'}
Write-Output "Retained: $Capture. Verify all gates; never infer timing causality from a last-getter match."
