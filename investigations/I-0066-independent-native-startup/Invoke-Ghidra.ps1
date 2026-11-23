[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Script,       # Ghidra script file (.java)
    [Parameter(Mandatory = $true)][string]$Out,          # output file or directory (first script argument)
    [string]$ProjectDir = '',                           # 1118: a read-only mirror of the canonical project (parallel threads)
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Rest   # further script arguments (addresses)
)
# I-0066: I-0045's read-only, no-analysis headless session on the canonical database, resolving game/ through its
# junction so it also runs from a worktree under .claude (Ghidra refuses project paths with a '.' element).
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$env:JAVA_HOME = 'C:\Users\User\Tools\DS3-Decompilation\jdk-21.0.12.1+1'
$launcher = 'C:\Users\User\Tools\DS3-Decompilation\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat'
$game = Get-Item -LiteralPath (Join-Path $root 'game') -Force
$project = if ($ProjectDir) { $ProjectDir } else { Join-Path ($(if ($game.LinkType) { $game.Target } else { $game.FullName })) 'ghidra' }; $name = 'DS3_1_15_2_10167187_I0002_FINAL'
if (Test-Path -LiteralPath (Join-Path $project "$name.lock")) { throw 'Project in use; no concurrent analysis' }
$scriptFile = Get-Item -LiteralPath $Script
$arguments = @($project, $name, '-process', 'DarkSoulsIII.exe', '-readOnly', '-noanalysis', '-scriptPath', $scriptFile.DirectoryName, '-postScript', $scriptFile.Name, $Out) + @($Rest)
$log = "$Out.console.txt"
'' | & $launcher @arguments *> $log
if ($LASTEXITCODE) { throw "Ghidra failed ($LASTEXITCODE), see $log" }
# D-0269: Ghidra exits 0 when the program cannot be opened (a newer database format); the log says so
if (Select-String -LiteralPath $log -Pattern 'Error during analysis|LanguageNotFoundException|Failed to open' -Quiet) { throw "Ghidra could not open the program, see $log" }
Write-Output "ok $Out"
