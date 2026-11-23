[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Script,       # Ghidra script file (.java)
    [Parameter(Mandatory = $true)][string]$Out,          # output file or directory (first script argument)
    [ValidateSet('i0070-a', 'i0072-a', 'i0072-b')][string]$Mirror = 'i0070-a',
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Rest   # further script arguments (addresses)
)
# I-0104: the read-only, no-analysis headless session on one of the Elden Ring 2.7.1 mirrors (copies of the analysed project,
# never the canonical one; D-0195, read-only). Two runs never open one mirror at once (its lock); three mirrors, three runs.
$ErrorActionPreference = 'Stop'
$env:JAVA_HOME = 'C:\Users\User\Tools\DS3-Decompilation\jdk-21.0.12.1+1'
$launcher = 'C:\Users\User\Tools\DS3-Decompilation\ghidra_12.1.3_PUBLIC\support\analyzeHeadless.bat'
$project = "C:\Users\User\Tools\DS3-Decompilation\er-mirror-$Mirror"; $name = 'ER_2_7_1_25080141_I0001'
if (Test-Path -LiteralPath (Join-Path $project "$name.lock")) { throw "Mirror $Mirror in use; no concurrent analysis" }
$scriptFile = Get-Item -LiteralPath $Script
$arguments = @($project, $name, '-process', 'eldenring_2.7.1.0_25080141_1A354710.exe', '-readOnly', '-noanalysis',
    '-scriptPath', $scriptFile.DirectoryName, '-postScript', $scriptFile.Name, $Out) + @($Rest)
$log = "$Out.console.txt"
'' | & $launcher @arguments *> $log
if ($LASTEXITCODE) { throw "Ghidra failed ($LASTEXITCODE), see $log" }
Write-Output "ok $Out"
