[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Out,          # output directory
    [string]$ScriptName = 'I0067Map.java',               # the I-0067 query script (reused unchanged)
    [string]$ProjectDir = 'C:\Users\User\Tools\DS3-Decompilation\ghidra-mirror-i0068',  # private read-only copy of the canonical FINAL database
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Query
)
# I-0068: same read-only, no-analysis headless session as I-0067's Invoke-Map.ps1, on a private copy of the canonical
# database so that the parallel I-0067 workstream never finds its project locked. Nothing is saved.
$ErrorActionPreference = 'Stop'
$env:JAVA_HOME = 'C:\Users\User\Tools\DS3-Decompilation\jdk-21.0.12.1+1'
$launcher = 'C:\Users\User\Tools\DS3-Decompilation\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat'
$name = 'DS3_1_15_2_10167187_I0002_FINAL'
$scripts = Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0067-static-menus-subsystems-map'
if (Test-Path -LiteralPath (Join-Path $ProjectDir "$name.lock")) { throw 'Project in use; no concurrent analysis' }
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$arguments = @($ProjectDir, $name, '-process', 'DarkSoulsIII.exe', '-readOnly', '-noanalysis', '-scriptPath', "$scripts;$PSScriptRoot",'-postScript', $ScriptName, $Out) + @($Query)
$log = Join-Path $Out 'console.txt'
'' | & $launcher @arguments *> $log
if ($LASTEXITCODE) { throw "Ghidra failed ($LASTEXITCODE), see $log" }
# D-0269: Ghidra exits 0 when the program cannot be opened (a newer database format); the log says so
if (Select-String -LiteralPath $log -Pattern 'Error during analysis|LanguageNotFoundException|Failed to open' -Quiet) { throw "Ghidra could not open the program, see $log" }
Write-Output "ok $Out"
