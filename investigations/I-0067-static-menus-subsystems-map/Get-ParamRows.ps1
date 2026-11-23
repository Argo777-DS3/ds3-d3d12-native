# Read-only: dumps every row of one param of the DS3 regulation (Data0.bdt, decrypted in memory with the public
# regulation key, as in I-0066 Map-ArmourItems.ps1) as hex dwords with float and int views. No paramdef is used.
# Usage: pwsh -NoProfile -File Get-ParamRows.ps1 -Param MenuOffscrRendParam -Out <tsv>
param([Parameter(Mandatory)][string]$Param, [Parameter(Mandatory)][string]$Out)
$ErrorActionPreference = 'Stop'
$game = 'C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game'
$souls = 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
if ((Get-FileHash -LiteralPath $souls -Algorithm SHA256).Hash -cne '19706BA6528982B83C6B6DD0DBE0ABDDF0E73CD51404F44A9D92CD27DEAF6735') { throw 'SoulsFormats mismatch' }
if ((Get-FileHash -LiteralPath (Join-Path $game 'DarkSoulsIII.exe') -Algorithm SHA256).Hash -cne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C') { throw 'game mismatch' }
Add-Type -Path $souls
Add-Type -AssemblyName System.Text.Encoding.CodePages -ErrorAction SilentlyContinue
[Text.Encoding]::RegisterProvider([Text.CodePagesEncodingProvider]::Instance)
$regPath = Join-Path $game 'Data0.bdt'
$raw = [IO.File]::ReadAllBytes($regPath)
$aes = [Security.Cryptography.Aes]::Create(); $aes.Mode = 'CBC'; $aes.Padding = 'None'
$aes.Key = [Text.Encoding]::ASCII.GetBytes('ds3#jn/8_7(rsY9pg55GFN7VFL#+3n/)'); $aes.IV = $raw[0..15]
$reg = [SoulsFormats.BND4]::Read($aes.CreateDecryptor().TransformFinalBlock($raw, 16, $raw.Length - 16))
$pf = @($reg.Files | Where-Object { $_.Name -like "*\$Param.param" })
if ($pf.Count -ne 1) { throw "$Param.param count $($pf.Count)" }
$b = [byte[]]$pf[0].Bytes
$rowCount = [BitConverter]::ToUInt16($b, 0xA)
$rows = for ($i = 0; $i -lt $rowCount; $i++) { $o = 0x40 + 24 * $i; [pscustomobject]@{ id = [BitConverter]::ToInt32($b, $o); data = [BitConverter]::ToInt64($b, $o + 8) } }
$rowSize = if ($rowCount -gt 1) { $rows[1].data - $rows[0].data } else { 0x40 }
$lines = @("# $Param from Data0.bdt SHA-256 $((Get-FileHash -LiteralPath $regPath -Algorithm SHA256).Hash); rows $rowCount; row size $rowSize", "id`toffset`thex`tfloat`tint")
foreach ($r in $rows) { for ($k = 0; $k -lt $rowSize; $k += 4) {
    $u = [BitConverter]::ToUInt32($b, $r.data + $k); $f = [BitConverter]::ToSingle($b, $r.data + $k)
    $lines += "{0}`t0x{1:X2}`t{2:X8}`t{3:G6}`t{4}" -f $r.id, $k, $u, $f, [BitConverter]::ToInt32($b, $r.data + $k) } }
$lines | Set-Content -LiteralPath $Out -Encoding utf8
"$rowCount rows, $rowSize bytes each"
