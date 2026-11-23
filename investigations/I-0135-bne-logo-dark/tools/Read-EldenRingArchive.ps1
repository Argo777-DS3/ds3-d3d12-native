[CmdletBinding()]
param(
    # Archive paths as Elden Ring hashes them (lower case, '/' first), e.g. /menu/05_901_logo_bne.gfx
    [Parameter(Mandatory)][string[]]$Names,
    [Parameter(Mandatory)][string]$OutDir,
    [string]$Game = 'C:\Program Files (x86)\Steam\steamapps\common\ELDEN RING\Game',
    [string[]]$Archives = @('Data0','Data1','Data2','Data3','DLC')
)
# I-0135 (Thomas, 2026-11-12 05:40, the coordinator's frame: Thomas's own Elden Ring, read-only, on his PC only). A local
# development reader, never shipped: for each name, finds its entry in the Elden Ring archives (the BHD5 index RSA-decrypted
# with the public keys the game's executable carries in clear), reads exactly its BDT range, AES-decrypts its keyed ranges,
# and writes the file to $OutDir (outside Git, never published). A DCX (Kraken) is also written decompressed (.raw) through the
# game's own oo2core_6_win64.dll. Nothing in the game folder is written; the game is not started.
$ErrorActionPreference = 'Stop'
$exe = Join-Path $Game 'eldenring.exe'
$oodle = Join-Path $Game 'oo2core_6_win64.dll'
if (-not (Test-Path -LiteralPath $exe) -or -not (Test-Path -LiteralPath $oodle)) { throw "Elden Ring not found in $Game" }
Add-Type -Path (Join-Path $PSScriptRoot 'ErArchive.cs')
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$keys = [ErArchive]::Keys($exe)
Write-Output "keys in the executable: $($keys.Count)"
$wanted = @{}; foreach ($n in $Names) { $wanted[[ErArchive]::Hash($n)] = $n }
foreach ($archive in $Archives) {
    $bhd = Join-Path $Game "$archive.bhd"; if (-not (Test-Path -LiteralPath $bhd)) { continue }
    $index = $null
    foreach ($k in $keys) { $try = [ErArchive]::DecryptIndex($bhd, $k); if ([Text.Encoding]::ASCII.GetString($try, 0, 4) -eq 'BHD5') { $index = $try; break } }
    if (-not $index) { Write-Output "$archive : no key opens its index"; continue }
    $entries = [ErArchive]::Entries($index)
    Write-Output "$archive : $($entries.Count) entries"
    foreach ($e in $entries) { if (-not $wanted.ContainsKey($e.Hash)) { continue }
        $name = $wanted[$e.Hash]; $bytes = [ErArchive]::Read((Join-Path $Game "$archive.bdt"), $e)
        $leaf = ($name.TrimStart('/') -replace '[/:]', '_'); $path = Join-Path $OutDir $leaf
        [IO.File]::WriteAllBytes($path, $bytes)
        $magic = [Text.Encoding]::ASCII.GetString($bytes, 0, [Math]::Min(4, $bytes.Length))
        $line = "  $name -> $leaf ($($bytes.Length) bytes, $magic, aes $([bool]$e.AesKey))"
        if ($magic -eq "DCX`0") { try { $raw = [ErArchive]::Undcx($bytes, $oodle); [IO.File]::WriteAllBytes("$path.raw", $raw); $line += ", raw $($raw.Length) bytes $([Text.Encoding]::ASCII.GetString($raw, 0, 4))" }
            catch { $line += ", DCX not read: $($_.Exception.InnerException.Message) header $(-join ($bytes[0..0x4F] | ForEach-Object { $_.ToString('x2') }))" } }
        Write-Output $line }
}
