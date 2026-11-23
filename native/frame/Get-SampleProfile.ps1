[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[switch]$RemoveRaw,[switch]$SlowFrames,[double]$SlowMs=50,
    [string]$BuildFolder='')
# m45 frame cost: names the samples of a run made with sample-profile.flag (Run-LiveWitness.ps1 -EffectFlags sample-profile)
# from the module's PDB in the build folder (it must be the build the run loaded: the symbolizer refuses a mismatched PDB and
# the report then says "symbols: none"), and writes sample-profile-report.txt next to the trace. -RemoveRaw deletes the raw
# samples afterwards (D-0142: the report is what the analysis reads).
# I-0074: -SlowFrames also writes sample-profile-slow-report.txt, the samples of the frames whose interval exceeded -SlowMs
# (the journal's native_slow_frame rows, over 50 ms): a sample carries the count of frames finished when it was taken, so the
# interval that ends at the finish of frame number P holds the samples marked P-1. -BuildFolder names the build the run loaded.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$run="$root/evidence/I-0066/artifacts/$RunName"
$samples="$run/sample-profile.txt"
if(-not (Test-Path -LiteralPath $samples)){ throw "No samples: $samples" }
$build=if($BuildFolder){$BuildFolder}else{"$root/evidence/I-0066/artifacts/game-bootstrap-build"}
$tool="$build/sample_symbolize.exe"
& $tool $samples "$run/sample-profile-report.txt"
if($LASTEXITCODE){ throw "sample_symbolize failed: $LASTEXITCODE" }
$report=Get-Content -LiteralPath "$run/sample-profile-report.txt" -Raw
if($report -notmatch 'symbols: pdb, lines'){ Write-Warning 'The module PDB did not match: own frames are unnamed.' }
if($SlowFrames){
    $marks=Get-Content -LiteralPath "$run/startup.jsonl" | Where-Object {$_ -like '*"native_slow_frame"*'} | ForEach-Object {$_|ConvertFrom-Json} |
        Where-Object {$_.presented -and $_.interval_ms -gt $SlowMs -and $_.content_draws -gt 0} | ForEach-Object {[int64]$_.presented-1}
    $list="$run/sample-profile-slow-frames.txt";@($marks) | Set-Content -LiteralPath $list
    & $tool $samples "$run/sample-profile-slow-report.txt" game_bootstrap.dll "frames:$list"
    if($LASTEXITCODE){ throw "sample_symbolize (slow frames) failed: $LASTEXITCODE" }
    "$run/sample-profile-slow-report.txt ($(@($marks).Count) frames)"
}
if($RemoveRaw){ Remove-Item -LiteralPath $samples }
"$run/sample-profile-report.txt"
