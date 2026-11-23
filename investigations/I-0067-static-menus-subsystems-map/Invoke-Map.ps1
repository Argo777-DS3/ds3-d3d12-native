[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Out,          # output directory
    [string]$ScriptName = 'I0067Map.java',             # I0067Map.java (queries) or I0067Rtti.java (RTTI inventory)
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Query   # I0067Map queries
)
# I-0067: read-only, no-analysis headless session on the shared canonical database (I-0066 Invoke-Ghidra pattern).
# The database lives in the main checkout's ignored game/ folder; worktrees have no game/ of their own.
# Never runs while another session holds the project lock (the other workstream may be using it).
$ErrorActionPreference = 'Stop'
$env:JAVA_HOME = 'C:\Users\User\Tools\DS3-Decompilation\jdk-21.0.12.1+1'
$launcher = 'C:\Users\User\Tools\DS3-Decompilation\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat'
$project = 'C:\Users\User\Desktop\DS3 Decompilation\game\ghidra'; $name = 'DS3_1_15_2_10167187_I0002_FINAL'
if (Test-Path -LiteralPath (Join-Path $project "$name.lock")) { throw 'Project in use; no concurrent analysis' }
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$arguments = @($project, $name, '-process', 'DarkSoulsIII.exe', '-readOnly', '-noanalysis', '-scriptPath', $PSScriptRoot, '-postScript', $ScriptName, $Out) + @($Query)
$log = Join-Path $Out 'console.txt'
'' | & $launcher @arguments *> $log
if ($LASTEXITCODE) { throw "Ghidra failed ($LASTEXITCODE), see $log" }
# D-0269: Ghidra exits 0 when the program cannot be opened (a newer database format); the log says so
if (Select-String -LiteralPath $log -Pattern 'Error during analysis|LanguageNotFoundException|Failed to open' -Quiet) { throw "Ghidra could not open the program, see $log" }
Write-Output "ok $Out"
