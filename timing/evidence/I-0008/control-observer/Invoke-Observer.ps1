[CmdletBinding()]
param([Parameter(Mandatory)][int]$GameProcessId)
$ErrorActionPreference='Stop'
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Build=Join-Path $Here '../artifacts/control-observer-build'
$Game=Get-Process -Id $GameProcessId
if($Game.ProcessName -ne 'DarkSoulsIII'){throw 'Selected process is not DarkSoulsIII'}
$Executable=$Game.MainModule.FileName
$ExecutableHash=(Get-FileHash -LiteralPath $Executable -Algorithm SHA256).Hash
if($ExecutableHash -ne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'){throw 'Wrong executable build'}
$Dll=(Resolve-Path (Join-Path $Build 'observer.dll')).Path
if(@($Game.Modules | Where-Object { $_.FileName -eq $Dll -or $_.ModuleName -match '^I000[5678].*(Probe|Observer|Patch)' }).Count){throw 'Existing instrument detected; require a clean process'}
$ValidationPath=Join-Path $Build 'validation.json';$Validation=Get-Content $ValidationPath -Raw|ConvertFrom-Json
foreach($item in $Validation.binaries){if((Get-FileHash (Join-Path $Build $item.name) -Algorithm SHA256).Hash -ne $item.sha256){throw 'Observer binary identity mismatch'}}
$Started=Get-Date
& (Join-Path $Build 'controller.exe') observe $GameProcessId $Dll
if($LASTEXITCODE -ne 0){throw 'Observer did not complete; do not reinject'}
$Files=@(Get-ChildItem -LiteralPath $env:TEMP -Filter "I0008-control-$GameProcessId-*.bin" | Where-Object { $_.LastWriteTime -ge $Started.AddSeconds(-1) })
if($Files.Count -ne 1){throw 'Capture file identity is ambiguous'}
$Capture=Join-Path $Here "../artifacts/control-live-$($Started.ToUniversalTime().ToString('yyyyMMddTHHmmssZ'))-pid$GameProcessId"
New-Item -ItemType Directory -Path $Capture|Out-Null
$Raw=Join-Path $Capture 'capture.bin';$Analysis=Join-Path $Capture 'analysis.json'
Copy-Item -LiteralPath $Files[0].FullName -Destination $Raw
Copy-Item -LiteralPath $ValidationPath -Destination (Join-Path $Capture 'validation.json')
node (Join-Path $Here 'analyze_capture.mjs') $Raw $Analysis
if($LASTEXITCODE -ne 0){throw 'Capture gates failed; preserve artifacts and do not reinject'}
$Session=[ordered]@{schema_version=1;started_at=$Started.ToUniversalTime().ToString('o');pid=$GameProcessId;
    executable=$Executable;executable_sha256=$ExecutableHash;observer_sha256=(Get-FileHash $Dll -Algorithm SHA256).Hash;
    capture_sha256=(Get-FileHash $Raw -Algorithm SHA256).Hash;analysis_sha256=(Get-FileHash $Analysis -Algorithm SHA256).Hash;
    intervention='temporary in-memory central timing 165/1 plus four execution breakpoints; header must prove timing and debug restoration'}
$Session|ConvertTo-Json -Depth 6|Set-Content (Join-Path $Capture 'session.json')
Write-Output "Retained: $Capture. Timing and debug restoration gates passed; interpret delta classification from analysis.json."
