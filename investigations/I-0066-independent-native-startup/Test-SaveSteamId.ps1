[CmdletBinding()]
param([Parameter(Mandatory)][string]$Save, [string]$SteamFolder = '')
# I-0066: read-only check that a DS3 save belongs to this PC's Steam account before it is installed by the runner.
# DS30000.sl2 is a BND4 of USER_DATA entries; each entry is MD5(16) + IV(16) + AES-128-CBC body with the public DS3
# save key. The account id is the folder name under %APPDATA%\DarkSoulsIII (SteamID64 in hex). The check counts the
# SteamID64 (little-endian) inside every decrypted entry of the candidate save; it writes nothing.
$ErrorActionPreference = 'Stop'
if (-not $SteamFolder) {
    $dirs = @(Get-ChildItem (Join-Path $env:APPDATA 'DarkSoulsIII') -Directory | Where-Object Name -match '^[0-9a-f]{16}$')
    if ($dirs.Count -ne 1) { throw "Expected one Steam account folder, found $($dirs.Count)" }
    $SteamFolder = $dirs[0].Name
}
$id = [Convert]::ToUInt64($SteamFolder, 16)
$needle = [BitConverter]::GetBytes($id)
$key = [byte[]](0xFD,0x46,0x4D,0x69,0x5E,0x69,0xA3,0x9A,0x10,0xE3,0x19,0xA7,0xAC,0xE8,0xB7,0xFA)
$raw = [IO.File]::ReadAllBytes($Save)
if ([Text.Encoding]::ASCII.GetString($raw, 0, 4) -ne 'BND4' -or [BitConverter]::ToInt64($raw, 0x20) -ne 0x20) { throw 'Unexpected save container' }
# Entry headers (0x20 each from 0x40): flags, -1, size (i64), data offset (u32), name offset (u32, UTF-16).
$files = for ($i = 0; $i -lt [BitConverter]::ToInt32($raw, 0xC); $i++) { $h = 0x40 + 0x20 * $i
    $size = [int][BitConverter]::ToInt64($raw, $h + 8); $off = [BitConverter]::ToInt32($raw, $h + 0x10); $no = [BitConverter]::ToInt32($raw, $h + 0x14)
    $e = $no; while ($raw[$e] -or $raw[$e + 1]) { $e += 2 }
    [pscustomobject]@{ Name = [Text.Encoding]::Unicode.GetString($raw, $no, $e - $no); Bytes = [byte[]]$raw[$off..($off + $size - 1)] } }
$aes = [Security.Cryptography.Aes]::Create(); $aes.Mode = 'CBC'; $aes.Padding = 'None'; $aes.Key = $key
$entries = foreach ($f in $files) {
    $b = [byte[]]$f.Bytes
    $aes.IV = $b[16..31]
    $d = $aes.CreateDecryptor().TransformFinalBlock($b, 32, $b.Length - 32)
    $md5ok = -not (Compare-Object ([Security.Cryptography.MD5]::HashData($b[16..($b.Length-1)])) $b[0..15])
    $n = 0; for ($i = 0; $i -le $d.Length - 8; $i++) { if ($d[$i] -eq $needle[0] -and [BitConverter]::ToUInt64($d, $i) -eq $id) { $n++ } }
    [pscustomobject]@{ entry = $f.Name; bytes = $b.Length; md5_ok = $md5ok; steamid_hits = $n }
}
[pscustomobject]@{ save = $Save; sha256 = (Get-FileHash -LiteralPath $Save).Hash; steam_folder = $SteamFolder; steamid64 = $id
    entries = $entries.Count; entries_with_id = @($entries | Where-Object steamid_hits).Count; md5_all_ok = -not @($entries | Where-Object { -not $_.md5_ok }).Count
    per_entry = ($entries | ForEach-Object { "$($_.entry):$($_.steamid_hits)" }) -join ' ' } | ConvertTo-Json
