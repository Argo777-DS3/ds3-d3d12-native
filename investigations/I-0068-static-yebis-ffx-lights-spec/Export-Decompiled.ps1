[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string[]]$From,      # I0067Map output directories (<entry>.c / <entry>.asm.txt)
    [Parameter(Mandatory = $true)][string[]]$Entry      # function entries to export (hex, with or without 14 prefix)
)
# I-0068: copies cited functions from query outputs into decompiled/ under the repository naming
# DS3-1152-F-<RVA>.{c,asm.txt}. Existing exports are kept (never overwritten).
$ErrorActionPreference = 'Stop'
$target = Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'decompiled'
foreach ($e in $Entry) {
    $va = [Convert]::ToUInt64(($e -replace '^0x', ''), 16)
    if ($va -lt 0x140000000) { $va += 0x140000000 }
    $hex = '{0:x}' -f $va; $rva = '{0:X8}' -f ($va - 0x140000000)
    $found = $false
    foreach ($dir in $From) {
        $c = Join-Path $dir "$hex.c"
        if (-not (Test-Path -LiteralPath $c)) { continue }
        foreach ($ext in @('c', 'asm.txt')) {
            $src = Join-Path $dir "$hex.$ext"; $dst = Join-Path $target "DS3-1152-F-$rva.$ext"
            if ((Test-Path -LiteralPath $src) -and -not (Test-Path -LiteralPath $dst)) { Copy-Item -LiteralPath $src -Destination $dst }
        }
        $found = $true; break
    }
    if (-not $found) { Write-Warning "no export for $hex" }
}
