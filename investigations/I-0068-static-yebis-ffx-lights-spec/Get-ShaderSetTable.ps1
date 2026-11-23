[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Decompiled,   # Ghidra output of FUN_140291520 (the FFX shader set builder)
    [string]$Exe = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'game\DarkSoulsIII.exe'),
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: rebuilds the FFX shader set table of FUN_140291520. The function fills stack arrays of UTF-16 programme names
# (`apuStack_XXXX[index] = &UNK_<va>;`), one array per stage; this script groups the assignments by array, resolves each VA
# to its string through the PE section table of the executable, and writes one row per (array, index, name).
$ErrorActionPreference = 'Stop'
$bytes = [IO.File]::ReadAllBytes((Resolve-Path $Exe).Path)
$pe = [BitConverter]::ToInt32($bytes, 0x3C); $nsec = [BitConverter]::ToUInt16($bytes, $pe + 6)
$optSize = [BitConverter]::ToUInt16($bytes, $pe + 20); $base = [BitConverter]::ToUInt64($bytes, $pe + 24 + 24)
$sections = for ($k = 0; $k -lt $nsec; $k++) { $o = $pe + 24 + $optSize + 40 * $k
    [pscustomobject]@{ va = [BitConverter]::ToUInt32($bytes, $o + 12); vsize = [BitConverter]::ToUInt32($bytes, $o + 8); raw = [BitConverter]::ToUInt32($bytes, $o + 20) } }
function Read-Name([uint64]$va) {
    $rva = $va - $base
    foreach ($s in $sections) { if ($rva -ge $s.va -and $rva -lt $s.va + $s.vsize) { $f = [int]($rva - $s.va + $s.raw)
        if ($bytes[$f + 1] -eq 0) { $e = $f; while ($e + 1 -lt $bytes.Length -and ($bytes[$e] -ne 0 -or $bytes[$e + 1] -ne 0)) { $e += 2 }; return [Text.Encoding]::Unicode.GetString($bytes, $f, $e - $f) }
        $e = $f; while ($bytes[$e] -ne 0) { $e++ }; return [Text.Encoding]::ASCII.GetString($bytes, $f, $e - $f) } }
    return '?' }
# The VS and PS arrays (apuStack_1d88, apuStack_1898) hold 158 entries (their bases are 0x4F0 bytes apart). Ghidra
# declares 128 and names entries 128..157 as separate stack variables (`puStack_1930 = &UNK_...` is apuStack_1d88[139]);
# those are mapped back through their stack offset (part L correction: the first version kept only indices 0..127).
# The other arrays share their stack slots with later tables, so only their declared form is read, as before.
$rows = foreach ($line in Get-Content -LiteralPath $Decompiled) {
    if ($line -match '(\w+Stack_\w+)\[(0x[0-9a-f]+|\d+)\] = &UNK_([0-9a-f]+);') {
        $idx = if ($Matches[2] -like '0x*') { [Convert]::ToInt32($Matches[2].Substring(2), 16) } else { [int]$Matches[2] }
        "$($Matches[1])`t$idx`t$(Read-Name ([Convert]::ToUInt64($Matches[3], 16)))"
    }
    elseif ($line -match '\bpuStack_([0-9a-f]+) = &UNK_([0-9a-f]+);') {
        $off = [Convert]::ToInt32($Matches[1], 16)
        foreach ($b in 0x1d88, 0x1898) {
            $idx = ($b - $off) / 8
            if ($idx -ge 128 -and $idx -le 157 -and $idx -eq [math]::Floor($idx)) {
                $name = Read-Name ([Convert]::ToUInt64($Matches[2], 16))
                if ($name -like 'GXFfx*') { "apuStack_$('{0:x}' -f $b)`t$idx`t$name" }
            }
        }
    } }
@("array`tindex`tname") + $rows | Set-Content -LiteralPath $Out -Encoding utf8
$rows | ForEach-Object { $_.Split("`t")[0] } | Group-Object | ForEach-Object { "$($_.Name) $($_.Count)" }
