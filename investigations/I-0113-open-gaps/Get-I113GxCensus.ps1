[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Out,        # TSV, one row per FLVER material
    [string]$Pattern = '/(map|chr|obj|parts|sfx)/.*\.(mapbnd|chrbnd|objbnd|partsbnd|ffxbnd)\.dcx$',
    [string[]]$Archives = @('Data0','Data1','Data2','Data3','Data4','Data5','DLC1','DLC2'),
    [int]$UnnamedMaxBytes = 67108864                   # unnamed entries (the DLC map pieces) read when no larger than this
)
# I-0113 tranche 7 (the zero-constant census): which GX items DS3's FLVER materials carry, by MTD. Every archive entry whose
# dictionary name matches $Pattern, and every unnamed entry (the DLC map pieces the pinned dictionary does not name) that turns
# out to be a DCX binder holding FLVERs, is read into memory with Recover-GameDataByName's pinned tools and method (RSA-decrypted
# BHD5, the BDT range, AES when keyed); the binder is opened with the pinned SoulsFormats 1.0.1 and each FLVER material's GX list
# is walked as 141070 walks it ({tag, version, length, data}). Read-only on the game; no game byte is written: only $Out, one row
# per material (archive, binder, flver, material, MTD, items as TAG.version[first data words in hex]). Windows PowerShell 5.1.
$ErrorActionPreference = 'Stop'
$game = 'C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game'
$tools = 'C:\Users\User\.codex\I0033-tools\BinderTool-v0.5.2'
$dictionaryPath = Join-Path $tools 'DictionaryDS3.csv'
$expected = @{
    BinderTool = '392FA5F736551F9346482EA2C9DFD62A711DA76C1F6EF4ABBF7ED7CE167BBDFA'
    Core = '727365E6E66CB093A8D05A8476E10FBB8AA7F3F798DC937088E709CE373E69C0'
    Bouncy = '1985B85BB44BE6C6EAF35E02EF11E23A890E809B8EC2E53210A4AD5A85B26C70'
    Dictionary = '8AF17A83D9711B760A73D5EF182992B7BCDF445883C85B92F61E7C3151E89884'
}
function Get-Sha256([string]$Path) {   # .NET, not Get-FileHash: a 5.1 host started from PowerShell 7 does not autoload its module
    $s = [IO.File]::OpenRead($Path); try { ([Security.Cryptography.SHA256]::Create().ComputeHash($s) | ForEach-Object { $_.ToString('X2') }) -join '' } finally { $s.Dispose() } }
foreach ($check in @(@{P=(Join-Path $tools 'BinderTool.exe');H=$expected.BinderTool},@{P=(Join-Path $tools 'BinderTool.Core.dll');H=$expected.Core},
                     @{P=(Join-Path $tools 'BouncyCastle.Crypto.dll');H=$expected.Bouncy},@{P=$dictionaryPath;H=$expected.Dictionary})) {
    if ((Get-Sha256 $check.P) -cne $check.H) { throw "Pinned input mismatch: $($check.P)" } }
if ((Get-Sha256 (Join-Path $game 'DarkSoulsIII.exe')) -cne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C') { throw 'Game identity mismatch' }
Add-Type -Path (Join-Path $tools 'BouncyCastle.Crypto.dll'); Add-Type -Path (Join-Path $tools 'BinderTool.Core.dll')
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$binder = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes((Join-Path $tools 'BinderTool.exe')))
$keyType = $binder.GetType('BinderTool.DecryptionKeys', $true)
$dictionary = [BinderTool.FileNameDictionary]::OpenFromFile($dictionaryPath, [BinderTool.Core.GameVersion]::DarkSouls3)
$rootMap = $dictionary.GetType().GetField('_dictionary', [Reflection.BindingFlags]'Instance,NonPublic').GetValue($dictionary)
$named = @{}; $wanted = @{}
foreach ($root in $rootMap.Keys) { $hashMap = $rootMap[$root]
    foreach ($hash in $hashMap.Keys) { foreach ($name in $hashMap[$hash]) { $full = "$root/$name"; $named[[UInt64]$hash] = $full
        if ($full -match $Pattern) { $wanted[[UInt64]$hash] = $full } } } }
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("archive`tbinder`tflver`tmaterial`tmtd`titems")
$stats = [ordered]@{ named_read=0; unnamed_read=0; unnamed_flver_binders=0; flvers=0; materials=0; flver_errors=0; named_binder_errors=0; unnamed_binder_errors=0 }
function Add-Binder([string]$archive, [string]$label, [byte[]]$bytes, [bool]$isNamed) {
    try { $bnd = [SoulsFormats.BND4]::Read($bytes) } catch { if ($isNamed) { $script:stats.named_binder_errors++ } else { $script:stats.unnamed_binder_errors++ }; return $false }
    $any = $false
    foreach ($e in $bnd.Files) {
        if ($e.Name -notlike '*.flver') { continue }
        $any = $true
        try { $fl = [SoulsFormats.FLVER]::Read($e.Bytes) } catch { $script:stats.flver_errors++; continue }
        $script:stats.flvers++
        foreach ($mat in $fl.Materials) {
            $script:stats.materials++; $items = New-Object System.Collections.Generic.List[string]; $b = $mat.GXBytes
            if ($b -ne $null) { $at = 0
                while ($at + 12 -le $b.Length) { $id = [BitConverter]::ToUInt32($b, $at); if ($id -eq 0x7FFFFFFF -or $id -eq 0xFFFFFFFF) { break }
                    $ver = [BitConverter]::ToInt32($b, $at + 4); $sz = [BitConverter]::ToInt32($b, $at + 8); if ($sz -lt 12 -or $at + $sz -gt $b.Length) { break }
                    $w = New-Object System.Collections.Generic.List[string]
                    for ($o = $at + 12; $o + 4 -le $at + $sz -and $w.Count -lt 8; $o += 4) { $w.Add(('{0:X}' -f [BitConverter]::ToUInt32($b, $o))) }
                    $items.Add(('{0}.{1:X}[{2}]' -f [Text.Encoding]::ASCII.GetString($b, $at, 4), $ver, ($w -join ','))); $at += $sz } }
            $script:rows.Add(((@('{0}','{1}','{2}','{3}','{4}','{5}') -join [char]9) -f $archive, $label, [IO.Path]::GetFileName($e.Name), $mat.Name, [IO.Path]::GetFileName($mat.MTD), ($items -join ' ')))
        }
    }
    return $any
}
foreach ($archive in $Archives) {
    $bhd = Join-Path $game "$archive.bhd"; if (-not (Test-Path -LiteralPath $bhd)) { continue }
    $keyField = $keyType.GetField(($archive -replace '^DLC','Dlc') + 'Key', [Reflection.BindingFlags]'NonPublic,Static'); if (-not $keyField) { continue }; $key = [string]$keyField.GetRawConstantValue()
    $stream = [BinderTool.Core.CryptographyUtility]::DecryptRsa($bhd, $key)
    try { $index = [BinderTool.Core.Bhd5.Bhd5File]::Read($stream, [BinderTool.Core.GameVersion]::DarkSouls3) } finally { $stream.Dispose() }
    $bdt = [BinderTool.Core.Bdt5.Bdt5FileStream]::OpenFile((Join-Path $game "$archive.bdt"), [IO.FileMode]::Open, [IO.FileAccess]::Read)
    try {
        foreach ($bucket in $index.GetBuckets()) { foreach ($entry in $bucket.GetEntries()) {
            $h = [UInt64]$entry.FileNameHash; $isWanted = $wanted.ContainsKey($h); $isUnnamed = -not $named.ContainsKey($h)
            if (-not $isWanted -and -not ($isUnnamed -and [Int64]$entry.PaddedFileSize -le $UnnamedMaxBytes)) { continue }
            $range = $bdt.Read([Int64]$entry.FileOffset, [Int64]$entry.PaddedFileSize); try { $bytes = [byte[]]$range.ToArray() } finally { $range.Dispose() }
            if ($entry.AesKey) { $ms = [IO.MemoryStream]::new($bytes, $true); try { [BinderTool.Core.CryptographyUtility]::DecryptAesEcb($ms, [byte[]]$entry.AesKey.Key, $entry.AesKey.Ranges); $bytes = [byte[]]$ms.ToArray() } finally { $ms.Dispose() } }
            if ($entry.FileSize -gt 0 -and $entry.FileSize -lt $bytes.Length) { [Array]::Resize([ref]$bytes, [int]$entry.FileSize) }
            if ($bytes.Length -lt 4 -or [Text.Encoding]::ASCII.GetString($bytes, 0, 3) -ne 'DCX') { continue }
            if ($isWanted) { $stats.named_read++; [void](Add-Binder $archive (($wanted[$h] -split '/')[-1]) $bytes $true) }
            else { $stats.unnamed_read++; if (Add-Binder $archive ('unnamed_{0:X16}' -f $h) $bytes $false) { $stats.unnamed_flver_binders++ } }
        } }
    } finally { $bdt.Dispose() }
    Write-Output ("{0} done: {1}" -f $archive, (($stats.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" }) -join ' '))
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output ("PASS gx census -> {0}: {1}" -f $Out, (($stats.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" }) -join ' '))
