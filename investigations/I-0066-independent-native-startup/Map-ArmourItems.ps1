[CmdletBinding()]
param(
    [string]$OutDir = 'C:\Users\User\Desktop\DS3_evidences\I-0066\artifacts\armour-items-1118',
    [string]$MaterialsTsv = 'C:\Users\User\Desktop\DS3_evidences\I-0066\artifacts\armour-families-1113\armour-materials.tsv',
    [string]$Language = 'engUS',
    [string]$Save = 'C:\Users\User\.ds3-i0039-backups\shebang-alpha-prepared-20261007-134754\DS30000.sl2',
    [string]$Character = 'Alpha'
)
# I-0066: joins the armour part models of armour-families-1113 to the wearable armour items, then chooses a small set
# of items whose models cover every armour material family (shader, blend, alpha ref), per body type.
#   - Items: EquipParamProtector rows of the regulation (Data0.bdt, decrypted in memory with the public DS3 regulation
#     key). The model id field offset is not taken from a paramdef: every u16 offset of the row is scored against the
#     armour FLVER numbers of its slot, and the best offset is used and reported (with its hit count).
#   - Slot from the row id: (id / 1000) % 10 = 0 head, 1 body, 2 arms, 3 legs (reported counts let this be checked).
#   - Names: the protector-name FMGs of msg/<Language>/item_dlc2.msgbnd.dcx (Data archives, BinderTool index).
#   - Body type: M wears <slot>_M_<id> (or _A_). F wears <slot>_F_<id> when it exists, else the M/A model (interpretation:
#     only ~100 models have an F variant; to be checked on the first female witness).
#   - Owned items: the cover only uses items present in the character slot of $Save (the u32 0x10000000|id, the DS3
#     protector inventory form, found aligned in the decrypted slot). The slot is the USER_DATA entry holding $Character.
#   - Only the full-detail FLVER (no _L suffix) is used for the families; _L is the low LOD of the same part.
# Nothing read from the game is written to disk beyond ids, names and family keys (TSV text and SUMMARY.txt).
$ErrorActionPreference = 'Stop'
$game = 'C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game'
$tools = 'C:\Users\User\.codex\I0033-tools\BinderTool-v0.5.2'
$souls = 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$dictionaryPath = Join-Path $tools 'DictionaryDS3.csv'
$pins = @{ (Join-Path $tools 'BinderTool.exe')='392FA5F736551F9346482EA2C9DFD62A711DA76C1F6EF4ABBF7ED7CE167BBDFA'
           (Join-Path $tools 'BinderTool.Core.dll')='727365E6E66CB093A8D05A8476E10FBB8AA7F3F798DC937088E709CE373E69C0'
           (Join-Path $tools 'BouncyCastle.Crypto.dll')='1985B85BB44BE6C6EAF35E02EF11E23A890E809B8EC2E53210A4AD5A85B26C70'
           $dictionaryPath='8AF17A83D9711B760A73D5EF182992B7BCDF445883C85B92F61E7C3151E89884'
           $souls='19706BA6528982B83C6B6DD0DBE0ABDDF0E73CD51404F44A9D92CD27DEAF6735'
           (Join-Path $game 'DarkSoulsIII.exe')='EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C' }
foreach ($p in $pins.Keys) { if ((Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash -cne $pins[$p]) { throw "Pinned input mismatch: $p" } }
$regPath = Join-Path $game 'Data0.bdt'
$regHash = (Get-FileHash -LiteralPath $regPath -Algorithm SHA256).Hash
$matHash = (Get-FileHash -LiteralPath $MaterialsTsv -Algorithm SHA256).Hash
Add-Type -Path (Join-Path $tools 'BouncyCastle.Crypto.dll'); Add-Type -Path (Join-Path $tools 'BinderTool.Core.dll'); Add-Type -Path $souls
$binder = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes((Join-Path $tools 'BinderTool.exe')))
$keyType = $binder.GetType('BinderTool.DecryptionKeys', $true)
Add-Type -AssemblyName System.Text.Encoding.CodePages -ErrorAction SilentlyContinue
[Text.Encoding]::RegisterProvider([Text.CodePagesEncodingProvider]::Instance)

# --- armour families by part model -------------------------------------------------------------------------------
$families = @{}   # "HD_M_2200" -> set of family keys
foreach ($r in (Import-Csv -LiteralPath $MaterialsTsv -Delimiter "`t")) {
    if ($r.flver -notmatch '^(HD|BD|AM|LG)_([MFA])_(\d{4})\.flver$') { continue }
    $k = "$($Matches[1])_$($Matches[2])_$($Matches[3])"
    if (-not $families.ContainsKey($k)) { $families[$k] = [Collections.Generic.HashSet[string]]::new() }
    [void]$families[$k].Add("$($r.shader)|$($r.blend)|$($r.alpha_ref)")
}

# --- regulation: EquipParamProtector ------------------------------------------------------------------------------
$raw = [IO.File]::ReadAllBytes($regPath)
$aes = [Security.Cryptography.Aes]::Create(); $aes.Mode = 'CBC'; $aes.Padding = 'None'
$aes.Key = [Text.Encoding]::ASCII.GetBytes('ds3#jn/8_7(rsY9pg55GFN7VFL#+3n/)'); $aes.IV = $raw[0..15]
$reg = [SoulsFormats.BND4]::Read($aes.CreateDecryptor().TransformFinalBlock($raw, 16, $raw.Length - 16))
$pf = @($reg.Files | Where-Object { $_.Name -like '*\EquipParamProtector.param' })
if ($pf.Count -ne 1) { throw "EquipParamProtector.param count $($pf.Count)" }
$b = [byte[]]$pf[0].Bytes
if (($b[0x2D] -band 4) -eq 0) { throw 'EquipParamProtector: expected 64-bit row offsets' }
$rowCount = [BitConverter]::ToUInt16($b, 0xA)
$rows = for ($i = 0; $i -lt $rowCount; $i++) { $o = 0x40 + 24 * $i; [pscustomobject]@{ id = [BitConverter]::ToInt32($b, $o); data = [BitConverter]::ToInt64($b, $o + 8) } }
$rowSize = $rows[1].data - $rows[0].data
$slotOf = @{ 0 = 'HD'; 1 = 'BD'; 2 = 'AM'; 3 = 'LG' }
$best = -1; $bestHits = -1; $scores = @()
for ($off = 0; $off -lt $rowSize - 1; $off += 2) {
    $hits = 0
    foreach ($r in $rows) {
        $s = $slotOf[[int]([Math]::Floor($r.id / 1000) % 10)]; if (-not $s) { continue }
        $v = '{0:D4}' -f [BitConverter]::ToUInt16($b, $r.data + $off)
        if ($families.ContainsKey("${s}_M_$v") -or $families.ContainsKey("${s}_F_$v") -or $families.ContainsKey("${s}_A_$v")) { $hits++ }
    }
    $scores += [pscustomobject]@{ off = $off; hits = $hits }
    if ($hits -gt $bestHits) { $best = $off; $bestHits = $hits }
}
$runnerUp = ($scores | Where-Object off -ne $best | Sort-Object hits -Descending | Select-Object -First 1)

# --- item names -----------------------------------------------------------------------------------------------------
$dictionary = [BinderTool.FileNameDictionary]::OpenFromFile($dictionaryPath, [BinderTool.Core.GameVersion]::DarkSouls3)
$rootMap = $dictionary.GetType().GetField('_dictionary', [Reflection.BindingFlags]'Instance,NonPublic').GetValue($dictionary)
$wanted = @{}
foreach ($root in $rootMap.Keys) { foreach ($hash in $rootMap[$root].Keys) { foreach ($name in $rootMap[$root][$hash]) {
    if ($name -ieq "msg/$Language/item_dlc2.msgbnd.dcx") { $wanted[[UInt64]$hash] = "$root/$name" } } } }
if (-not $wanted.Count) { throw "msg/$Language/item_dlc2.msgbnd.dcx not in dictionary" }
$names = @{}; $fmgFiles = @()
foreach ($archive in @('Data1','Data2','Data3','Data4','Data5','DLC1','DLC2')) {
    if ($names.Count) { break }
    $keyField = $keyType.GetField(($archive -replace '^DLC','Dlc') + 'Key', [Reflection.BindingFlags]'NonPublic,Static')
    $stream = [BinderTool.Core.CryptographyUtility]::DecryptRsa((Join-Path $game "$archive.bhd"), [string]$keyField.GetRawConstantValue())
    try { $index = [BinderTool.Core.Bhd5.Bhd5File]::Read($stream, [BinderTool.Core.GameVersion]::DarkSouls3) } finally { $stream.Dispose() }
    $bdt = [BinderTool.Core.Bdt5.Bdt5FileStream]::OpenFile((Join-Path $game "$archive.bdt"), [IO.FileMode]::Open, [IO.FileAccess]::Read)
    try {
        foreach ($bucket in $index.GetBuckets()) { foreach ($entry in $bucket.GetEntries()) {
            if (-not $wanted.ContainsKey([UInt64]$entry.FileNameHash)) { continue }
            $range = $bdt.Read([Int64]$entry.FileOffset, [Int64]$entry.PaddedFileSize); try { $bytes = [byte[]]$range.ToArray() } finally { $range.Dispose() }
            if ($entry.AesKey) { $ms = [IO.MemoryStream]::new($bytes, $true); try { [BinderTool.Core.CryptographyUtility]::DecryptAesEcb($ms, [byte[]]$entry.AesKey.Key, $entry.AesKey.Ranges); $bytes = [byte[]]$ms.ToArray() } finally { $ms.Dispose() } }
            if ($entry.FileSize -gt 0 -and $entry.FileSize -lt $bytes.Length) { [Array]::Resize([ref]$bytes, [int]$entry.FileSize) }
            if ([SoulsFormats.DCX]::Is($bytes)) { $bytes = [SoulsFormats.DCX]::Decompress($bytes) }
            $msg = [SoulsFormats.BND4]::Read($bytes)
            # Protector names: 防具名 (base), 防具名_dlc1, 防具名_dlc2; later files override earlier ones.
            foreach ($f in ($msg.Files | Where-Object { ($_.Name -split '\\')[-1] -match '^防具名(_dlc\d)?\.fmg$' } | Sort-Object { if ($_.Name -match '_dlc(\d)') { [int]$Matches[1] } else { 0 } })) {
                $fmgFiles += ($f.Name -split '\\')[-1]
                foreach ($e in ([SoulsFormats.FMG]::Read($f.Bytes)).Entries) { if ($e.Text) { $names[[int]$e.ID] = ($e.Text -replace "[`t`r`n]", ' ') } }
            }
        } }
    } finally { $bdt.Dispose() }
}
if (-not $names.Count) { throw 'No protector names read' }

# --- join -----------------------------------------------------------------------------------------------------------
$items = [Collections.Generic.List[object]]::new()
foreach ($r in $rows) {
    $s = $slotOf[[int]([Math]::Floor($r.id / 1000) % 10)]
    $model = '{0:D4}' -f [BitConverter]::ToUInt16($b, $r.data + $best)
    $name = [string]$names[$r.id]
    $m = @("${s}_M_$model", "${s}_A_$model" | Where-Object { $families.ContainsKey($_) })
    $f = @("${s}_F_$model", "${s}_A_$model" | Where-Object { $families.ContainsKey($_) }); if (-not $f) { $f = $m }
    $fm = if ($m) { @($m | ForEach-Object { $families[$_] } | ForEach-Object { $_ } | Sort-Object -Unique) } else { @() }
    $ff = if ($f) { @($f | ForEach-Object { $families[$_] } | ForEach-Object { $_ } | Sort-Object -Unique) } else { @() }
    $items.Add([pscustomobject]@{ id = $r.id; slot = $s; model = $model; name = $name; flver_m = ($m -join ';'); flver_f = ($f -join ';')
        families_m = $fm; families_f = $ff })
}
# --- owned items in the save --------------------------------------------------------------------------------------
$saveHash = (Get-FileHash -LiteralPath $Save -Algorithm SHA256).Hash
$sraw = [IO.File]::ReadAllBytes($Save)
if ([Text.Encoding]::ASCII.GetString($sraw, 0, 4) -ne 'BND4' -or [BitConverter]::ToInt64($sraw, 0x20) -ne 0x20) { throw 'Unexpected save container' }
$sk = [Security.Cryptography.Aes]::Create(); $sk.Mode = 'CBC'; $sk.Padding = 'None'
$sk.Key = [byte[]](0xFD,0x46,0x4D,0x69,0x5E,0x69,0xA3,0x9A,0x10,0xE3,0x19,0xA7,0xAC,0xE8,0xB7,0xFA)
$needle = [Text.Encoding]::Unicode.GetBytes("$Character`0"); $owned = $null; $slotIndex = -1
for ($i = 0; $i -lt 10 -and -not $owned; $i++) {
    $h = 0x40 + 0x20 * $i; $size = [int][BitConverter]::ToInt64($sraw, $h + 8); $off = [BitConverter]::ToInt32($sraw, $h + 0x10)
    $eb = [byte[]]$sraw[$off..($off + $size - 1)]; $sk.IV = $eb[16..31]; $d = $sk.CreateDecryptor().TransformFinalBlock($eb, 32, $eb.Length - 32)
    if ([Text.Encoding]::Unicode.GetString($d).IndexOf("$Character`0") -lt 0) { continue }
    $owned = [Collections.Generic.HashSet[int]]::new(); $slotIndex = $i
    for ($o = 0; $o -le $d.Length - 4; $o += 4) { $v = [BitConverter]::ToUInt32($d, $o); if (($v -band 0xF0000000) -eq 0x10000000) { [void]$owned.Add([int]($v -band 0x0FFFFFFF)) } }
}
if (-not $owned) { throw "Character $Character not found in $Save" }
$named = @($items | Where-Object { $_.name -and $_.slot -and $owned.Contains([int]$_.id) })

# Greedy cover of the families reachable through named items, then packing into outfits (at most one piece per slot).
function Cover([string]$body) {
    $prop = "families_$body"
    $universe = [Collections.Generic.HashSet[string]]::new(); foreach ($i in $named) { foreach ($k in $i.$prop) { [void]$universe.Add($k) } }
    $left = [Collections.Generic.HashSet[string]]::new($universe); $chosen = @()
    while ($left.Count) {
        $pick = $named | Sort-Object @{ e = { @($_.$prop | Where-Object { $left.Contains($_) }).Count }; Descending = $true }, id | Select-Object -First 1
        $gain = @($pick.$prop | Where-Object { $left.Contains($_) }); if (-not $gain.Count) { break }
        foreach ($k in $gain) { [void]$left.Remove($k) }
        $chosen += [pscustomobject]@{ item = $pick; gain = $gain }
    }
    $outfits = @()
    foreach ($c in $chosen) {
        $o = $outfits | Where-Object { -not $_.Contains($c.item.slot) } | Select-Object -First 1
        if (-not $o) { $o = [ordered]@{}; $outfits += $o }
        $o[$c.item.slot] = $c
    }
    [pscustomobject]@{ universe = $universe; chosen = $chosen; outfits = $outfits }
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false)
$lines = @("id`tslot`tmodel`tname`tflver_m`tflver_f`tfamilies_m`tfamilies_f")
foreach ($i in $items) { $lines += "$($i.id)`t$($i.slot)`t$($i.model)`t$($i.name)`t$($i.flver_m)`t$($i.flver_f)`t$($i.families_m -join ' ; ')`t$($i.families_f -join ' ; ')" }
[IO.File]::WriteAllLines((Join-Path $OutDir 'armour-items.tsv'), $lines, $utf8)

$allFamilies = [Collections.Generic.HashSet[string]]::new(); foreach ($v in $families.Values) { foreach ($k in $v) { [void]$allFamilies.Add($k) } }
$sum = @("I-0066 armour items and minimal family cover (Map-ArmourItems.ps1)", "generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')", '',
    'Pinned tools verified: OK',
    "Data0.bdt (regulation) SHA-256 $regHash", "armour-materials.tsv SHA-256 $matHash", '',
    "EquipParamProtector rows: $rowCount, row size $rowSize",
    ("model id field: offset 0x{0:X} ({1} rows whose u16 names an armour FLVER of their slot); runner-up 0x{2:X} ({3})" -f $best, $bestHits, $runnerUp.off, $runnerUp.hits),
    "slots from id: " + (($items | Group-Object slot | ForEach-Object { "$(if($_.Name){$_.Name}else{'none'})=$($_.Count)" }) -join ' '),
    "protector names read: $($names.Count) from $($fmgFiles -join ', ') (msg/$Language/item_dlc2.msgbnd.dcx)",
    "save $Save SHA-256 $saveHash; character $Character in USER_DATA00$slotIndex; owned named protector items: $($named.Count) of $(@($items | Where-Object { $_.name -and $_.slot }).Count) named",
    "owned items whose model has no FLVER for M: $(@($named | Where-Object { -not $_.flver_m }).Count), for F: $(@($named | Where-Object { -not $_.flver_f }).Count)",
    "armour families in all part models: $($allFamilies.Count)")
foreach ($body in 'm', 'f') {
    $c = Cover $body
    $sum += ''; $sum += "BODY $($body.ToUpper()): families reachable through named items: $($c.universe.Count); chosen pieces: $($c.chosen.Count); outfits: $($c.outfits.Count)"
    $sum += "  families not reachable through any named item: " + ((@($allFamilies | Where-Object { -not $c.universe.Contains($_) }) | Sort-Object) -join ' ; ')
    $n = 0
    foreach ($o in $c.outfits) {
        $n++; $sum += "  outfit $n"
        foreach ($s in 'HD', 'BD', 'AM', 'LG') { if ($o.Contains($s)) { $x = $o[$s]; $sum += "    $s  $($x.item.id)  $($x.item.name)  (model $($x.item.model); new: $($x.gain -join ' ; '))" } }
    }
}
[IO.File]::WriteAllLines((Join-Path $OutDir 'SUMMARY.txt'), $sum, $utf8)
$sum
