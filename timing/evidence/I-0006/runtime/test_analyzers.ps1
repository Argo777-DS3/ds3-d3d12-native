$ErrorActionPreference = 'Stop'

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $Here 'build_analyzers.ps1') | Out-Host

$Executable = Join-Path $Here 'bin/reconstruct_writer_mirror.exe'
$Input = Join-Path $Here 'testdata/writer-mirror-valid.csv'
$Output = Join-Path $Here 'obj/writer-mirror-valid-enriched.csv'
$Summary = Join-Path $Here 'obj/writer-mirror-valid-summary.json'
& $Executable $Input $Output $Summary
if ($LASTEXITCODE -ne 0) {
    throw "Reconstruction self-test failed with exit code $LASTEXITCODE"
}

$Result = Get-Content -LiteralPath $Summary -Raw | ConvertFrom-Json
if (-not $Result.all_qualified_rows_precommit_valid -or
    $Result.rows -ne 1 -or
    $Result.qualified_rows -ne 1 -or
    $Result.qualified_precommit_valid_rows -ne 1) {
    throw 'Reconstruction self-test summary did not pass.'
}

$Enriched = Import-Csv -LiteralPath $Output
if ($Enriched.x_reconstructed_b0 -ne '0x40400000' -or
    $Enriched.x_reconstructed_b1 -ne '0x40800000' -or
    $Enriched.x_reconstructed_b2 -ne '0x40A00000' -or
    $Enriched.x_reconstructed_b3 -ne '0x3F800000') {
    throw 'Reconstruction self-test produced unexpected output bits.'
}

Write-Output 'PASS: sequential float32/MXCSR reconstruction is bit-exact for the synthetic mirror row.'

& node (Join-Path $Here 'test_compare_writer_mirror.mjs')
if ($LASTEXITCODE -ne 0) {
    throw "Ordered comparison self-test failed with exit code $LASTEXITCODE"
}
