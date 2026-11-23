[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Table,      # quality-table.tsv (Get-QualityTable.ps1 output)
    [Parameter(Mandatory = $true)][string[]]$Levels   # (comma list ok) the twelve slot levels, slot order of MENUS-MAP section 77 (slot 8 = 3)
)
# I-0067: reference oracle for the native quality object. Applies the rewrite of FUN_140023790 (lines 107..157 of its export) to the
# 0x24C-byte table and prints the resulting 0x78-byte object [GXRS+0x5360] as hex, 16 bytes per row. Used as the expected value of
# the native unit test (QUALITY-OBJECT-NATIVE.md, Q6). Levels outside 0..4 are refused (the original reads the table unchecked).
$ErrorActionPreference = 'Stop'
$lv = @($Levels -join ',' -split ',' | ForEach-Object { [int]$_ })   # pwsh -File passes a list as one string
if ($lv.Count -ne 12) { throw 'twelve levels expected' }
if (($lv | Where-Object { $_ -lt 0 -or $_ -gt 4 }).Count) { throw 'level out of 0..4' }
$t = New-Object byte[] 0x24c
foreach ($l in Get-Content -LiteralPath $Table) { $p = $l -split "`t"; $o = [Convert]::ToInt32($p[0], 16); $h = $p[1].Trim().Split(' '); for ($i = 0; $i -lt $h.Length; $i++) { $t[$o + $i] = [Convert]::ToByte($h[$i], 16) } }
$obj = New-Object byte[] 0x78
function Copy-T([int]$from, [int]$to, [int]$n) { if ($from + $n -gt $script:t.Length) { throw "table range $from+$n" }; [Array]::Copy($script:t, $from, $script:obj, $to, $n) }
$s = $lv
Copy-T ($s[0] * 8) 0x00 8                  # slot 0 Texture: two ints
Copy-T (0x28 + $s[1]) 0x08 1               # slot 1 Antialiasing: one byte
Copy-T ($s[2] * 7 + 0x2D) 0x09 4           # slot 2 SSAO: dword at +9, word at +0xD, byte at +0xF
Copy-T ($s[2] * 7 + 0x31) 0x0D 2
Copy-T ($s[2] * 7 + 0x33) 0x0F 1
Copy-T (($s[3] + 5) * 16) 0x10 16          # slot 3 DepthOfField: four dwords
Copy-T (0xA0 + $s[4] * 8) 0x20 8           # slot 4 MotionBlur
Copy-T (0xC8 + $s[5] * 0x18) 0x28 0x18     # slot 5 ShadowQuality: six dwords
Copy-T (0x140 + $s[6] * 8) 0x40 8          # slot 6 LightingQuality
Copy-T (0x168 + $s[7] * 0x20) 0x48 0x20    # slot 7 EffectsQuality: flags dword and seven floats
Copy-T (0x208 + $s[8]) 0x68 1              # slot 8 (no XML key): decal byte
Copy-T (0x210 + $s[9] * 8) 0x6C 8          # slot 9 ReflectionQuality
Copy-T (0x238 + $s[10]) 0x74 1             # slot 10 WaterSurfaceQuality
Copy-T (($s[11] + 0xBF) * 3) 0x75 2        # slot 11 ShadeQuality: word at +0x75, byte at +0x77
Copy-T (($s[11] + 0xBF) * 3 + 2) 0x77 1
for ($r = 0; $r -lt 0x78; $r += 16) {
    $n = [Math]::Min(16, 0x78 - $r)
    '{0:x2}  {1}' -f $r, (($obj[$r..($r + $n - 1)] | ForEach-Object { '{0:x2}' -f $_ }) -join ' ')
}
