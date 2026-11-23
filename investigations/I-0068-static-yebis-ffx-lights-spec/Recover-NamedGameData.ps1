# I-0068 part BO: variant of I-0045/Recover-GameData.ps1 for archive entries the pinned dictionary does not name (the DLC map pieces): the names come from a file, the hash is the dictionary's own (prime 37 on '/' + lowercase name, verified on a Data5 entry).
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$NamesFile,   # one archive-relative name per line, for example map/m45_00_00_00/m45_00_00_00_000020.mapbnd.dcx
    [string]$OutDir = 'C:\Users\User\.codex\I0045-backups\gamedata',
    [string]$ReportPath = '',
    [string[]]$Archives = @('DLC1','DLC2','Data5')
)
# I-0045: recover map lighting data files (GPARAM, DrawParam, envmap TPF...) whose pinned DS3 dictionary names match
# $Pattern, with the method and pinned tools of I-0042's Recover-MaterialBank.ps1: RSA-decrypt the Data0..Data5 BHD5
# indexes, match the dictionary hashes, read exactly each BDT range, AES-decrypt it when keyed. Read-only on the game.
# Licensed bytes are written outside the repository; the report keeps names, ranges and hashes only.
$ErrorActionPreference = 'Stop'
if (-not $ReportPath) { throw 'ReportPath is required' }
$game = 'C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game'
$tools = 'C:\Users\User\.codex\I0033-tools\BinderTool-v0.5.2'
$dictionaryPath = Join-Path $tools 'DictionaryDS3.csv'
$expected = @{
    BinderTool = '392FA5F736551F9346482EA2C9DFD62A711DA76C1F6EF4ABBF7ED7CE167BBDFA'
    Core = '727365E6E66CB093A8D05A8476E10FBB8AA7F3F798DC937088E709CE373E69C0'
    Bouncy = '1985B85BB44BE6C6EAF35E02EF11E23A890E809B8EC2E53210A4AD5A85B26C70'
    Dictionary = '8AF17A83D9711B760A73D5EF182992B7BCDF445883C85B92F61E7C3151E89884'
}
function Get-Sha256([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant() }
function Get-Sha256Bytes([byte[]]$Bytes) { ([Security.Cryptography.SHA256]::Create().ComputeHash($Bytes) | ForEach-Object { $_.ToString('X2') }) -join '' }
foreach ($check in @(@{P=(Join-Path $tools 'BinderTool.exe');H=$expected.BinderTool},@{P=(Join-Path $tools 'BinderTool.Core.dll');H=$expected.Core},
                     @{P=(Join-Path $tools 'BouncyCastle.Crypto.dll');H=$expected.Bouncy},@{P=$dictionaryPath;H=$expected.Dictionary})) {
    if ((Get-Sha256 $check.P) -cne $check.H) { throw "Pinned input mismatch: $($check.P)" } }
if ((Get-Sha256 (Join-Path $game 'DarkSoulsIII.exe')) -cne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C') { throw 'Game identity mismatch' }
Add-Type -Path (Join-Path $tools 'BouncyCastle.Crypto.dll'); Add-Type -Path (Join-Path $tools 'BinderTool.Core.dll')
$binder = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes((Join-Path $tools 'BinderTool.exe')))
$keyType = $binder.GetType('BinderTool.DecryptionKeys', $true)


$wanted = @{}
$hashFn = $binder.GetType('BinderTool.FileNameDictionary').GetMethod('GetHashCode', [Reflection.BindingFlags]'Static,Public,NonPublic')
foreach ($n in [IO.File]::ReadAllLines($NamesFile)) { $n = $n.Trim().ToLowerInvariant(); if (-not $n) { continue }
    $h = [UInt64]($hashFn.Invoke($null, @(('/' + $n), [uint32]37))); if (-not $wanted.ContainsKey($h)) { $wanted[$h] = @() }; $wanted[$h] += ('data5/' + $n) }if (-not $wanted.Count) { throw "No dictionary name matches $Pattern" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$files = @()
foreach ($archive in $Archives) {
    $bhd = Join-Path $game "$archive.bhd"; if (-not (Test-Path -LiteralPath $bhd)) { continue }
    $keyField = $keyType.GetField(($archive -replace '^DLC','Dlc') + 'Key', [Reflection.BindingFlags]'NonPublic,Static'); if (-not $keyField) { continue }; $key = [string]$keyField.GetRawConstantValue()
    $stream = [BinderTool.Core.CryptographyUtility]::DecryptRsa($bhd, $key)
    try { $index = [BinderTool.Core.Bhd5.Bhd5File]::Read($stream, [BinderTool.Core.GameVersion]::DarkSouls3) } finally { $stream.Dispose() }
    $bdt = $null
    try {
        foreach ($bucket in $index.GetBuckets()) { foreach ($entry in $bucket.GetEntries()) {
            if (-not $wanted.ContainsKey([UInt64]$entry.FileNameHash)) { continue }
            if (-not $bdt) { $bdt = [BinderTool.Core.Bdt5.Bdt5FileStream]::OpenFile((Join-Path $game "$archive.bdt"), [IO.FileMode]::Open, [IO.FileAccess]::Read) }
            $range = $bdt.Read([Int64]$entry.FileOffset, [Int64]$entry.PaddedFileSize); try { $bytes = [byte[]]$range.ToArray() } finally { $range.Dispose() }
            $rangeHash = Get-Sha256Bytes $bytes
            if ($entry.AesKey) { $ms = [IO.MemoryStream]::new($bytes, $true); try { [BinderTool.Core.CryptographyUtility]::DecryptAesEcb($ms, [byte[]]$entry.AesKey.Key, $entry.AesKey.Ranges); $bytes = [byte[]]$ms.ToArray() } finally { $ms.Dispose() } }
            $names = $wanted[[UInt64]$entry.FileNameHash]
            $leaf = ($names | Where-Object { $_ -like '*.dcx' } | Select-Object -First 1); if (-not $leaf) { $leaf = $names[0] }
            $out = Join-Path $OutDir ($(if ($archive -like 'DLC*') { $archive.ToLowerInvariant() + '_' } else { '' }) + ($leaf -replace '^data\d/', '') -replace '[/:]', '_')
            if ($entry.FileSize -gt 0 -and $entry.FileSize -lt $bytes.Length) { $bytes = $bytes[0..([int]$entry.FileSize - 1)] }
            [IO.File]::WriteAllBytes($out, $bytes)
            $files += [ordered]@{ names=$names; archive=$archive; file_offset=[UInt64]$entry.FileOffset; padded_size=[UInt64]$entry.PaddedFileSize
                aes=[bool]$entry.AesKey; magic=[Text.Encoding]::ASCII.GetString($bytes, 0, 4); range_sha256=$rangeHash; file_sha256=(Get-Sha256 $out); bytes=$bytes.Length; output=$out }
        } }
    } finally { if ($bdt) { $bdt.Dispose() } }
}
$report = [ordered]@{ schema=1; result='PASS_GAME_DATA_RANGE_RECOVERY'; game_started=$false; names_file=$NamesFile; dictionary_hashes=$wanted.Count; recovered=$files.Count
    files=$files; tools=$expected; script_sha256=(Get-Sha256 $MyInvocation.MyCommand.Path) }
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ReportPath) | Out-Null
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ReportPath -Encoding utf8
Write-Output "PASS game data: $($files.Count) of $($wanted.Count) dictionary hashes recovered -> $OutDir"
$files | ForEach-Object { "{0,-6} {1,8} {2} {3}" -f $_.archive, $_.bytes, $_.magic, ($_.names -join ' | ') }

