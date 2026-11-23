[CmdletBinding()]
param([Parameter(Mandatory)][string]$Tpf,          # a recovered map:/mAA/mAA_cgrading.tpf.dcx (Recover-GameData output, outside the repository)
      [Parameter(Mandatory)][string]$Texture,      # the strip's entry name, e.g. m31_00_CGrading0001
      [Parameter(Mandatory)][string]$Volume,       # a captured grading LUT (reference-draw-NN-t16.raw: 16x16x16 R32G32B32A32_FLOAT, x fastest, then y, then z)
      [string]$Out='')                             # optional text report
# I-0066 W10 GR (W10-NATIVE-PASSES 12, review): the original's grading volume against the TPF strip the native selects, texel for texel:
# 3D[x][y][z] = 2D[x][16z + y] (LutCopy2DTo3DCs), each channel the UNORM byte / 255 (the DDS masks give R in byte 0). Read-only.
$ErrorActionPreference = 'Stop'
$souls = 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
if ((Get-FileHash -LiteralPath $souls -Algorithm SHA256).Hash -cne '19706BA6528982B83C6B6DD0DBE0ABDDF0E73CD51404F44A9D92CD27DEAF6735') { throw 'SoulsFormats mismatch' }
Add-Type -Path $souls
$entry = [SoulsFormats.TPF]::Read($Tpf).Textures | Where-Object Name -eq $Texture | Select-Object -First 1
if (-not $entry) { throw "no texture $Texture in $Tpf" }
$dds = $entry.Bytes; $lut = [IO.File]::ReadAllBytes($Volume)
$height = [BitConverter]::ToUInt32($dds, 12); $width = [BitConverter]::ToUInt32($dds, 16)
$masks = 92, 96, 100, 104 | ForEach-Object { [BitConverter]::ToUInt32($dds, $_) }
if ($width -ne 16 -or $height -ne 256 -or [BitConverter]::ToUInt32($dds, 88) -ne 32 -or ($masks -join ',') -ne '255,65280,16711680,4278190080' -or $lut.Length -ne 65536) {
    throw "unexpected layout: ${width}x$height, masks $($masks -join ','), volume $($lut.Length) bytes" }
$differing = 0
for ($z = 0; $z -lt 16; $z++) { for ($y = 0; $y -lt 16; $y++) { for ($x = 0; $x -lt 16; $x++) {
    $texel = 128 + 4 * ((16 * $z + $y) * 16 + $x); $voxel = 16 * (($z * 16 + $y) * 16 + $x)
    for ($c = 0; $c -lt 4; $c++) { if ([BitConverter]::ToSingle($lut, $voxel + 4 * $c) -ne [float]($dds[$texel + $c] / 255.0)) { $differing++; break } } } } }
$line = "grading LUT: $Texture of $(Split-Path $Tpf -Leaf) (SHA-256 $((Get-FileHash -LiteralPath $Tpf).Hash)) against $Volume (SHA-256 $((Get-FileHash -LiteralPath $Volume).Hash)): $differing of 4096 voxels differ"
if ($Out) { $line | Set-Content -LiteralPath $Out -Encoding utf8 }
$line
if ($differing) { exit 1 }
