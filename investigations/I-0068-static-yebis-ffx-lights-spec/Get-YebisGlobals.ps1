[CmdletBinding()]
param(
    [string]$Exe = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'game\DarkSoulsIII.exe'),
    [string]$Asm = (Join-Path (Split-Path -Parent $PSScriptRoot) '..\decompiled\DS3-1152-F-000E8210.asm.txt'),  # export of FUN_1400E8210
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068 part AO: the YEBIS name registration FUN_1400E8210 binds each gparam-visible parameter name (a UTF-16 string in the
# executable) to a global of the YEBIS block 144961F70..1449622DF. Every registration is a call preceded, since the previous call,
# by `LEA RAX,[name]` and `LEA R8,[global]`. This reads both from the assembly and resolves the name through the PE sections.
# 108 of the 131 entries match this pattern; the others (luminances, shape) use another helper. Output: global<TAB>name.
$ErrorActionPreference = 'Stop'
$bytes = [IO.File]::ReadAllBytes((Resolve-Path $Exe).Path)
$pe = [BitConverter]::ToInt32($bytes, 0x3C); $nsec = [BitConverter]::ToUInt16($bytes, $pe + 6)
$optSize = [BitConverter]::ToUInt16($bytes, $pe + 20); $base = [BitConverter]::ToUInt64($bytes, $pe + 24 + 24)
$sections = for ($k = 0; $k -lt $nsec; $k++) { $o = $pe + 24 + $optSize + 40 * $k
    [pscustomobject]@{ va = [BitConverter]::ToUInt32($bytes, $o + 12); vsize = [BitConverter]::ToUInt32($bytes, $o + 8); raw = [BitConverter]::ToUInt32($bytes, $o + 20) } }
function Off([uint64]$va) { $rva = $va - $base; foreach ($s in $sections) { if ($rva -ge $s.va -and $rva -lt $s.va + $s.vsize) { return [int]($rva - $s.va + $s.raw) } }; -1 }
function WStr([uint64]$va) { $f = Off $va; if ($f -lt 0) { return '?' }; $e = $f; while (($bytes[$e] -ne 0 -or $bytes[$e + 1] -ne 0) -and ($e - $f) -lt 200) { $e += 2 }; [Text.Encoding]::Unicode.GetString($bytes, $f, $e - $f) }
$rows = @(); $win = @()
foreach ($l in Get-Content -LiteralPath $Asm) {
    if ($l -match 'CALL 0x') {
        $name = $null; $glob = $null
        foreach ($w in $win) {
            if ($w -match 'LEA R8,\[0x(14496[0-9a-f]+)\]') { $glob = $Matches[1] }
            if ($w -match 'LEA RAX,\[0x(143d[0-9a-f]+)\]') { $name = $Matches[1] }
        }
        if ($name -and $glob) { $rows += "{0}`t{1}" -f $glob, (WStr ([Convert]::ToUInt64($name, 16))).Trim() }
        $win = @()
    } else { $win += $l }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
$rows | Sort-Object -Unique | Set-Content -LiteralPath $Out
"rows $($rows.Count) -> $Out"
