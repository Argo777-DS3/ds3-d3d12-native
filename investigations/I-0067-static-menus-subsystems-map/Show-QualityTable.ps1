[CmdletBinding(PositionalBinding = $false)]
param([Parameter(Mandatory = $true)][string]$Table)   # Get-QualityTable.ps1 output (offset<TAB>16 hex bytes)
# I-0067: decodes the 0x24C-byte GX quality table per setting slot and level, with the layout of FUN_140023790
# (slot -> table offset -> field of the quality object [GXRS+0x5360]). Slot order: Texture, Antialiasing, SSAO, DepthOfField,
# MotionBlur, ShadowQuality, LightingQuality, EffectsQuality, (decal byte, no XML key), ReflectionQuality, WaterSurfaceQuality, ShadeQuality.
$ErrorActionPreference = 'Stop'
$b = New-Object byte[] 0x24c
foreach ($l in Get-Content -LiteralPath $Table) { $p = $l -split "`t"; $o = [Convert]::ToInt32($p[0], 16); $h = $p[1].Trim().Split(' '); for ($i = 0; $i -lt $h.Length; $i++) { $b[$o + $i] = [Convert]::ToByte($h[$i], 16) } }
function D([int]$o) { [BitConverter]::ToInt32($b, $o) }
function F([int]$o) { [Math]::Round([BitConverter]::ToSingle($b, $o), 4) }
function B([int]$o) { $b[$o] }
$lv = 'DISABLE', 'LOW', 'MEDIUM', 'HIGH', 'MAX'
"slot 0 Texture      obj+0x00/+0x04 = table[0x00 + s*8] (dword a, dword b) -> FUN_1400D3650(texture manager, a, b)"
0..4 | ForEach-Object { '  {0,-8} a={1} b={2}' -f $lv[$_], (D (0x00 + $_ * 8)), (D (0x04 + $_ * 8)) }
"slot 1 Antialiasing obj+0x08 = table[0x28 + s] (byte)"
0..4 | ForEach-Object { '  {0,-8} {1}' -f $lv[$_], (B (0x28 + $_)) }
"slot 2 SSAO         obj+0x09 dword, +0x0D word, +0x0F byte = table[0x2D + s*7] (bytes shown)"
0..4 | ForEach-Object { $o = 0x2D + $_ * 7; '  {0,-8} {1}' -f $lv[$_], ((0..6 | ForEach-Object { '{0:x2}' -f $b[$o + $_] }) -join ' ') }
"slot 3 DepthOfField obj+0x10..+0x1C = table[0x50 + s*0x10] (4 dwords)"
0..4 | ForEach-Object { $o = 0x50 + $_ * 0x10; '  {0,-8} {1} {2} {3} {4}' -f $lv[$_], (D $o), (D ($o + 4)), (D ($o + 8)), (D ($o + 12)) }
"slot 4 MotionBlur   obj+0x20 = table[0xA0 + s*8] (+0x20 enable, +0x21 object velocity, +0x24 sample cap)"
0..4 | ForEach-Object { $o = 0xA0 + $_ * 8; '  {0,-8} +20={1} +21={2} +22={3} +23={4} cap={5}' -f $lv[$_], (B $o), (B ($o + 1)), (B ($o + 2)), (B ($o + 3)), (D ($o + 4)) }
"slot 5 Shadow       obj+0x28..+0x3F = table[0xC8 + s*0x18] (6 dwords: +28 on, +2C, +30, +34, +38 map size, +3C)"
0..4 | ForEach-Object { $o = 0xC8 + $_ * 0x18; '  {0,-8} {1} {2} {3} {4} {5} {6}' -f $lv[$_], (D $o), (D ($o + 4)), (D ($o + 8)), (D ($o + 12)), (D ($o + 16)), (D ($o + 20)) }
"slot 6 Lighting     obj+0x40 = table[0x140 + s*8] (float +40, bytes +44 +45 +46 +47)"
0..4 | ForEach-Object { $o = 0x140 + $_ * 8; '  {0,-8} {1} {2:x2} {3:x2} {4:x2} {5:x2}' -f $lv[$_], (F $o), (B ($o + 4)), (B ($o + 5)), (B ($o + 6)), (B ($o + 7)) }
"slot 7 Effects      obj+0x48 = four flag bytes, +0x4C..+0x64 = seven floats, table[0x168 + s*0x20]"
0..4 | ForEach-Object { $o = 0x168 + $_ * 0x20; '  {0,-8} flags {1:x2} {2:x2} {3:x2} {4:x2}; floats {5}' -f $lv[$_], (B $o), (B ($o + 1)), (B ($o + 2)), (B ($o + 3)), ((1..7 | ForEach-Object { F ($o + 4 * $_) }) -join ' ') }
"slot 8 (decal byte, no XML key) obj+0x68 = table[0x208 + s] (byte)"
0..4 | ForEach-Object { '  {0,-8} {1}' -f $lv[$_], (B (0x208 + $_)) }
"slot 9 Reflection   obj+0x6C..+0x73 = table[0x210 + s*8] (+6C enable, +6D lights, +6E mask, +70 dword)"
0..4 | ForEach-Object { $o = 0x210 + $_ * 8; '  {0,-8} +6C={1} +6D={2} +6E={3} +6F={4} +70={5}' -f $lv[$_], (B $o), (B ($o + 1)), (B ($o + 2)), (B ($o + 3)), (D ($o + 4)) }
"slot 10 Water       obj+0x74 = table[0x238 + s] (byte)"
0..4 | ForEach-Object { '  {0,-8} {1}' -f $lv[$_], (B (0x238 + $_)) }
"slot 11 Shade       obj+0x75 word, +0x77 byte = table[(s+0xBF)*3 = 0x23D + s*3] (3 bytes)"
0..4 | ForEach-Object { $o = 0x23D + $_ * 3; '  {0,-8} {1:x2} {2:x2} {3:x2}' -f $lv[$_], (B $o), (B ($o + 1)), (B ($o + 2)) }
