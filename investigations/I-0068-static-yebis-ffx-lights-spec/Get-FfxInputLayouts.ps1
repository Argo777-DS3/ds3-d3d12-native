[CmdletBinding()]
param(
    [string]$Exe = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'game\DarkSoulsIII.exe'),
    [string]$ShaderSet,        # ffx-shader-set.tsv (Get-ShaderSetTable.ps1), VS names per index (array apuStack_1d88)
    [string]$Signatures,       # ffx-vs-signatures.tsv (Get-Isgn.ps1), optional: validates the layouts against the VS ISGN
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068 part AM: decodes the input layouts the FFX shader-set builder FUN_140291520 creates for its vertex programmes.
# STATIC. The builder (two loops) calls FUN_14006e450(out, device, elements, count, vsObject, ...) with
#   loop 1 (158 VS, index 0..157): elements = qword table 14473EBC0[i], count = dword table 143D22320[i]
#   loop 2 (127 entries)          : elements = qword table 14473E7C0[i], count = dword table 143D22120[i]
# The element record is GX's own 28-byte record, converted by FUN_14192F090 into D3D11_INPUT_ELEMENT_DESC:
#   +0 semantic enum (FUN_141929A90: 0..9 -> name strings at 143A3A208..), +4 semantic index, +8 format (FUN_1419294A0:
#   identity on the DXGI_FORMAT value), +0xC slot, +0x10 aligned byte offset, +0x14 class (FUN_141929B70: 1 = per instance),
#   +0x18 step rate.
$ErrorActionPreference = 'Stop'
$bytes = [IO.File]::ReadAllBytes((Resolve-Path $Exe).Path)
$pe = [BitConverter]::ToInt32($bytes, 0x3C); $nsec = [BitConverter]::ToUInt16($bytes, $pe + 6)
$optSize = [BitConverter]::ToUInt16($bytes, $pe + 20); $base = [BitConverter]::ToUInt64($bytes, $pe + 24 + 24)
$sections = for ($k = 0; $k -lt $nsec; $k++) { $o = $pe + 24 + $optSize + 40 * $k
    [pscustomobject]@{ va = [BitConverter]::ToUInt32($bytes, $o + 12); vsize = [BitConverter]::ToUInt32($bytes, $o + 8); raw = [BitConverter]::ToUInt32($bytes, $o + 20) } }
function Off([uint64]$va) { $rva = $va - $base; foreach ($s in $sections) { if ($rva -ge $s.va -and $rva -lt $s.va + $s.vsize) { return [int]($rva - $s.va + $s.raw) } }; throw "VA not mapped $va" }
function U64([uint64]$va) { [BitConverter]::ToUInt64($bytes, (Off $va)) }
function U32([uint64]$va) { [BitConverter]::ToUInt32($bytes, (Off $va)) }
function Str([uint64]$va) { $f = Off $va; $e = $f; while ($bytes[$e] -ne 0) { $e++ }; [Text.Encoding]::ASCII.GetString($bytes, $f, $e - $f) }

# FUN_141929A90: semantic enum -> string address (switch of the decompilation)
$semAddr = @(0x143a3a208, 0x143a3a218, 0x143a3a228, 0x143a3a238, 0x143a3a240, 0x143a3a248, 0x143a3a260, 0x143a3a268, 0x143a3a280, 0x143a3a254)
$sem = $semAddr | ForEach-Object { Str $_ }

# DXGI format: name and size in bytes (the formats found in the tables; others print as numbers)
$fmt = @{
    2 = @('R32G32B32A32_FLOAT', 16); 3 = @('R32G32B32A32_UINT', 16); 4 = @('R32G32B32A32_SINT', 16)
    6 = @('R32G32B32_FLOAT', 12); 7 = @('R32G32B32_UINT', 12); 8 = @('R32G32B32_SINT', 12)
    10 = @('R16G16B16A16_FLOAT', 8); 11 = @('R16G16B16A16_UNORM', 8); 12 = @('R16G16B16A16_UINT', 8); 13 = @('R16G16B16A16_SNORM', 8); 14 = @('R16G16B16A16_SINT', 8)
    16 = @('R32G32_FLOAT', 8); 17 = @('R32G32_UINT', 8); 18 = @('R32G32_SINT', 8)
    24 = @('R10G10B10A2_UNORM', 4); 25 = @('R10G10B10A2_UINT', 4); 26 = @('R11G11B10_FLOAT', 4)
    28 = @('R8G8B8A8_UNORM', 4); 30 = @('R8G8B8A8_UINT', 4); 31 = @('R8G8B8A8_SNORM', 4); 32 = @('R8G8B8A8_SINT', 4)
    34 = @('R16G16_FLOAT', 4); 35 = @('R16G16_UNORM', 4); 36 = @('R16G16_UINT', 4); 37 = @('R16G16_SNORM', 4); 38 = @('R16G16_SINT', 4)
    41 = @('R32_FLOAT', 4); 42 = @('R32_UINT', 4); 43 = @('R32_SINT', 4)
    49 = @('R8G8_UNORM', 2); 50 = @('R8G8_UINT', 2); 51 = @('R8G8_SNORM', 2); 52 = @('R8G8_SINT', 2)
    54 = @('R16_FLOAT', 2); 56 = @('R16_UNORM', 2); 57 = @('R16_UINT', 2); 58 = @('R16_SNORM', 2); 59 = @('R16_SINT', 2)
    61 = @('R8_UNORM', 1); 62 = @('R8_UINT', 1); 63 = @('R8_SNORM', 1); 64 = @('R8_SINT', 1)
}
function FmtName([uint32]$f) { if ($fmt.ContainsKey([int]$f)) { $fmt[[int]$f][0] } else { "DXGI_$f" } }
function FmtSize([uint32]$f) { if ($fmt.ContainsKey([int]$f)) { $fmt[[int]$f][1] } else { 0 } }

function Decode-Table([uint64]$addr, [int]$count) {
    for ($e = 0; $e -lt $count; $e++) {
        $a = $addr + 28 * $e
        $s = U32 $a
        [pscustomobject]@{
            semantic = if ($s -lt $sem.Count) { $sem[$s] } else { "SEM_$s" }; sindex = U32 ($a + 4); format = FmtName (U32 ($a + 8))
            fsize = FmtSize (U32 ($a + 8)); slot = U32 ($a + 12); offset = U32 ($a + 16); cls = U32 ($a + 20); step = U32 ($a + 24)
        }
    }
}
function Layout-Text($els) { ($els | ForEach-Object { "$($_.semantic)$($_.sindex):$($_.format)@$($_.offset)" + $(if ($_.slot) { "/s$($_.slot)" }) + $(if ($_.cls) { '/inst' }) }) -join ' ' }
function Stride($els) { $m = 0; foreach ($el in $els | Where-Object { $_.slot -eq 0 }) { $m = [Math]::Max($m, $el.offset + $el.fsize) }; $m }

# VS names (loop 1 table), when the shader-set table is given
$vsName = @{}
if ($ShaderSet) { foreach ($r in Import-Csv -Delimiter "`t" -LiteralPath $ShaderSet) { if ($r.array -eq 'apuStack_1d88') { $vsName[[int]$r.index] = $r.name } } }
# VS input signatures, when given: program name -> semantic+index set
$sig = @{}
if ($Signatures) { foreach ($r in Import-Csv -Delimiter "`t" -LiteralPath $Signatures | Where-Object { $_.chunk -eq 'ISGN' }) {
    $p = $r.file -replace '^gxffxshader__', '' -replace '\.vpo$', ''
    if (-not $sig.ContainsKey($p)) { $sig[$p] = New-Object System.Collections.ArrayList }
    [void]$sig[$p].Add(('{0}{1}:{2}' -f $r.name, $r.index, $r.mask)) } }

$rows = New-Object System.Collections.ArrayList
$null = $rows.Add("loop`tindex`tvs_name`ttable`tcount`tstride_slot0`tlayout`tsignature`tmissing_from_layout")
foreach ($loop in @(@{ n = 1; tab = 0x14473ebc0; cnt = 0x143d22320; num = 158 }, @{ n = 2; tab = 0x14473e7c0; cnt = 0x143d22120; num = 127 })) {
    for ($i = 0; $i -lt $loop.num; $i++) {
        $t = U64 ($loop.tab + 8 * $i); $c = U32 ($loop.cnt + 4 * $i)
        $els = @(Decode-Table $t $c)
        $name = if ($loop.n -eq 1 -and $vsName.ContainsKey($i)) { $vsName[$i] } else { '' }
        $sg = ''; $miss = ''
        if ($name -and $sig.ContainsKey($name)) {
            $sg = ($sig[$name] -join ' ')
            $have = $els | ForEach-Object { "$($_.semantic)$($_.sindex)" }
            $bad = foreach ($s in $sig[$name]) { $k, $m = $s -split ':'; $el = $els | Where-Object { "$($_.semantic)$($_.sindex)" -eq $k } | Select-Object -First 1
                if (-not $el) { "$k(absent)" } elseif ((([regex]'[RGBA]\d+').Matches($el.format)).Count -lt $m.Length) { "$k(narrow)" } }
            $miss = $bad -join ' '
        }
        $null = $rows.Add("$($loop.n)`t$i`t$name`t{0:X}`t$c`t$(Stride $els)`t$(Layout-Text $els)`t$sg`t$miss" -f $t)
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
