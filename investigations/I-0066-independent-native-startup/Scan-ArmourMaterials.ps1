[CmdletBinding()]
param(
    [string]$OutDir = 'C:\Users\User\Desktop\DS3_evidences\I-0066\artifacts\armour-families-1113',
    [string]$MtdTsv = 'C:\Users\User\Desktop\DS3_evidences\I-0063\artifacts\scan\mtd.tsv',
    [string]$OldFlverTsv = 'C:\Users\User\Desktop\DS3_evidences\I-0063\artifacts\scan\flver.tsv',
    [string[]]$Archives = @('Data0','Data1','Data2','Data3','Data4','Data5','DLC1','DLC2'),
    [int]$Limit = 0
)
# I-0066: one read-only pass over the parts BNDs of the game archives. It inventories the material families of the
# armour part models (HD_, BD_, AM_, LG_) and of the face/hair part models (FG_, HR_, FC_), where a family is
# (MTD leaf name, shader .spx, blend mode, alpha ref).
#   - Only entries whose dictionary name is under parts/ with an armour or face/hair leaf are parsed. Entries the
#     dictionary does not name (most of DLC1/DLC2, some of Data1-5) are read and kept only when their inner names
#     place them under \parts\ with an armour or face/hair leaf.
#   - Each part BND is unwrapped (DCX outer and DCX inner), its inner FLVERs are parsed by SoulsFormats, and the
#     inner-file signatures (extension, magic, raw or DCX) are counted so that a missed format is visible.
#   - MTD shader, blend and alpha values come from the I-0063 mtd.tsv evidence (the MTD files are not read here).
#   - Nothing read from the game is written to disk; outputs are TSV text and SUMMARY.txt only.
# Pinned tools and inputs are checked as in I-0063 Scan-Content.ps1.
$ErrorActionPreference = 'Stop'
if ($Archives.Count -eq 1 -and $Archives[0] -like "*,*") { $Archives = $Archives[0] -split "," }   # pwsh -File passes a list as one string
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
foreach ($p in @($MtdTsv, $OldFlverTsv)) { if (-not (Test-Path -LiteralPath $p)) { throw "Evidence input missing: $p" } }
$inputHashes = @{}
foreach ($p in @($MtdTsv, $OldFlverTsv)) { $inputHashes[$p] = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash }
Add-Type -Path (Join-Path $tools 'BouncyCastle.Crypto.dll'); Add-Type -Path (Join-Path $tools 'BinderTool.Core.dll'); Add-Type -Path $souls
$binder = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes((Join-Path $tools 'BinderTool.exe')))
$keyType = $binder.GetType('BinderTool.DecryptionKeys', $true)
# Byte-level FLVER2 material table, used only as a cross-check of the SoulsFormats material count.
Add-Type -TypeDefinition @'
using System; using System.Collections.Generic; using System.Text;
public static class I66Flver {
    // FLVER2 header (0x80), dummies (0x40 each), materials (0x20 each).
    public static int MaterialCount(byte[] b) {
        if (b.Length < 0x80 || Encoding.ASCII.GetString(b, 0, 5) != "FLVER" || b[6] != (byte)'L') return -1;
        return BitConverter.ToInt32(b, 0x18);
    }
}
'@
Add-Type -AssemblyName System.Text.Encoding.CodePages -ErrorAction SilentlyContinue
[Text.Encoding]::RegisterProvider([Text.CodePagesEncodingProvider]::Instance)
# Dictionary names by hash (the hash is the archive-independent path hash).
$dictionary = [BinderTool.FileNameDictionary]::OpenFromFile($dictionaryPath, [BinderTool.Core.GameVersion]::DarkSouls3)
$rootMap = $dictionary.GetType().GetField('_dictionary', [Reflection.BindingFlags]'Instance,NonPublic').GetValue($dictionary)
$names = @{}
foreach ($root in $rootMap.Keys) { foreach ($hash in $rootMap[$root].Keys) { foreach ($name in $rootMap[$root][$hash]) {
    $k = [UInt64]$hash; if (-not $names.ContainsKey($k) -or ($name -like '*.dcx' -and $names[$k] -notlike '*.dcx')) { $names[$k] = "$root/$name" } } } }

$armourRx = '^(hd|bd|am|lg)_'
$faceRx = '^(fg|hr|fc)_'
function Leaf([string]$p) { ($p -split '[\\/]')[-1] }
function Clean([string]$s) { if ($null -eq $s) { '' } else { $s -replace "[`t`r`n]", ' ' } }

$flverRecs = [Collections.Generic.List[object]]::new()
$failures = [Collections.Generic.List[string]]::new()
$notes = [Collections.Generic.List[string]]::new()
$zeroMaterialFlvers = [Collections.Generic.List[string]]::new()   # FLVERs that parse but carry no material table
$kindCount = @{}      # "archive|ext|magic|raw-or-dcx" -> count, inner files of part BNDs
$stat = [ordered]@{}  # "archive|counter" -> count
$containers = [Collections.Generic.List[object]]::new()
function Add-Stat([string]$archive, [string]$key, [int]$by = 1) { $k = "$archive|$key"; $stat[$k] = $by + [int]$stat[$k] }
function Add-Kind([string]$key) { $kindCount[$key] = 1 + [int]$kindCount[$key] }

function Add-Flver([string]$label, [string]$archive, [string]$hash, [string]$fname, [byte[]]$b) {
    $leaf = Leaf $fname
    try { $fl = [SoulsFormats.FLVER]::Read($b) } catch { $failures.Add("FLVER-READ`t$archive`t$hash`t$(Clean $label)`t$leaf`t$(Clean $_.Exception.Message)"); return 0 }
    $use = @{}
    foreach ($m in $fl.Meshes) { $mi = [int]$m.MaterialIndex; $use[$mi] = 1 + [int]$use[$mi] }
    $byteCount = [I66Flver]::MaterialCount($b)
    if ($byteCount -ne $fl.Materials.Count) { $failures.Add("MATERIAL-COUNT-MISMATCH`t$archive`t$hash`t$(Clean $label)`t$leaf`tsoulsformats=$($fl.Materials.Count) bytes=$byteCount") }
    if ($fl.Materials.Count -eq 0) { $zeroMaterialFlvers.Add("$archive`t$hash`t$(Clean $label)`t$leaf`tmeshes=$($fl.Meshes.Count)") }
    for ($i = 0; $i -lt $fl.Materials.Count; $i++) {
        $mat = $fl.Materials[$i]
        $flverRecs.Add([pscustomobject]@{ container = (Clean $label); flver = $leaf; archive = $archive; hash = $hash; idx = $i
            name = (Clean $mat.Name); mtd = (Clean (Leaf ([string]$mat.MTD))); meshes = [int]$use[$i] })
    }
    return 1
}

# Walks the inner files of an unwrapped BND4. Returns the number of FLVERs parsed; $seen collects inner signatures.
function Walk-Binder($bnd, [string]$label, [string]$archive, [string]$hash, [int]$depth, $seen) {
    $n = 0
    foreach ($f in $bnd.Files) {
        $fname = [string]$f.Name
        $ext = ([IO.Path]::GetExtension((Leaf $fname))).ToLowerInvariant()
        $b = $f.Bytes
        if (-not $b -or $b.Length -lt 4) { Add-Kind "$archive|$ext|empty|-"; [void]$seen.Add("${ext}:empty"); continue }
        $dcx = [SoulsFormats.DCX]::Is($b)
        if ($dcx) { try { $b = [SoulsFormats.DCX]::Decompress($b) } catch { $failures.Add("INNER-DCX`t$archive`t$hash`t$(Clean $label)`t$(Leaf $fname)`t$(Clean $_.Exception.Message)"); [void]$seen.Add("${ext}:dcx-failed"); continue } }
        $mode = if ($dcx) { 'dcx' } else { 'raw' }
        $magic = if ($b.Length -ge 4) { ([Text.Encoding]::ASCII.GetString($b, 0, 4)) -replace '[^ -~]', '.' } else { 'short' }
        Add-Kind "$archive|$ext|$magic|$mode"
        [void]$seen.Add("${ext}:${magic}:${mode}")
        if ($magic -eq 'FLVE') { $n += (Add-Flver $label $archive $hash $fname $b) }
        elseif ($magic -eq 'BND4' -and $depth -lt 2) {
            $sub = [SoulsFormats.BND4]::Read($b)
            $n += (Walk-Binder $sub "$label|$(Leaf $fname)" $archive $hash ($depth + 1) $seen)
        }
    }
    return $n
}

$total = 0
try {
foreach ($archive in $Archives) {
    $bhd = Join-Path $game "$archive.bhd"; if (-not (Test-Path -LiteralPath $bhd)) { Add-Stat $archive 'missing-archive'; continue }
    $keyField = $keyType.GetField(($archive -replace '^DLC','Dlc') + 'Key', [Reflection.BindingFlags]'NonPublic,Static')
    if (-not $keyField) { $notes.Add("skipped ${archive}: no index key (Data0 is the regulation file, I-0033)"); continue }
    $stream = [BinderTool.Core.CryptographyUtility]::DecryptRsa($bhd, [string]$keyField.GetRawConstantValue())
    try { $index = [BinderTool.Core.Bhd5.Bhd5File]::Read($stream, [BinderTool.Core.GameVersion]::DarkSouls3) } finally { $stream.Dispose() }
    $bdt = [BinderTool.Core.Bdt5.Bdt5FileStream]::OpenFile((Join-Path $game "$archive.bdt"), [IO.FileMode]::Open, [IO.FileAccess]::Read)
    try {
        foreach ($bucket in $index.GetBuckets()) { foreach ($entry in $bucket.GetEntries()) {
            $h = [UInt64]$entry.FileNameHash; $name = [string]$names[$h]
            $named = [bool]$name
            Add-Stat $archive 'entries'
            if ($named) {
                $ln = $name.ToLowerInvariant()
                if ($ln -notmatch '(^|/)parts/' -or (Leaf $ln) -notmatch '^(hd|bd|am|lg|fg|hr|fc)_') { Add-Stat $archive 'named-not-armour-or-face-part'; continue }
            } else { Add-Stat $archive 'unnamed-read' }
            if ($Limit -and $total -ge $Limit) { break }
            $range = $bdt.Read([Int64]$entry.FileOffset, [Int64]$entry.PaddedFileSize); try { $bytes = [byte[]]$range.ToArray() } finally { $range.Dispose() }
            if ($entry.AesKey) { $ms = [IO.MemoryStream]::new($bytes, $true); try { [BinderTool.Core.CryptographyUtility]::DecryptAesEcb($ms, [byte[]]$entry.AesKey.Key, $entry.AesKey.Ranges); $bytes = [byte[]]$ms.ToArray() } finally { $ms.Dispose() } }
            if ($entry.FileSize -gt 0 -and $entry.FileSize -lt $bytes.Length) { [Array]::Resize([ref]$bytes, [int]$entry.FileSize) }
            $hashText = '{0:X16}' -f $h
            try {
                $b = $bytes
                if ([SoulsFormats.DCX]::Is($b)) { $b = [SoulsFormats.DCX]::Decompress($b) }
                $magic = if ($b.Length -ge 4) { [Text.Encoding]::ASCII.GetString($b, 0, 4) } else { '' }
                if ($magic -ne 'BND4') {
                    if ($named) { $failures.Add("NOT-BND4`t$archive`t$hashText`t$name`t$(Clean $magic)"); Add-Stat $archive 'named-part-not-bnd4' }
                    continue
                }
                $bnd = [SoulsFormats.BND4]::Read($b)
                $label = $name
                if (-not $named) {
                    $inner = @($bnd.Files | Where-Object { $_.Name -match '(?i)\\parts\\' -and (Leaf $_.Name) -match '(?i)^(hd|bd|am|lg|fg|hr|fc)_' } | Select-Object -First 1)
                    if (-not $inner) { Add-Stat $archive 'unnamed-not-armour-or-face-part'; continue }
                    $label = [string]$inner[0].Name
                }
                $leafLabel = Leaf $label
                $category = if ($leafLabel -match '(?i)^(hd|bd|am|lg)_') { 'armour' } elseif ($leafLabel -match '(?i)^(fg|hr|fc)_') { 'face-hair' } else { 'other' }
                $seen = [Collections.Generic.List[string]]::new()
                $n = Walk-Binder $bnd $label $archive $hashText 0 $seen
                $containers.Add([pscustomobject]@{ archive = $archive; hash = $hashText; label = (Clean $label); named = $named; category = $category; flvers = $n; kinds = ($seen -join ' ') })
                Add-Stat $archive "parts-read-$category"
                if ($n -eq 0) { Add-Stat $archive "parts-read-$category-no-flver" }
                if (-not $named) { Add-Stat $archive 'unnamed-part-bnd' }
                Add-Stat $archive "flvers-parsed" $n
                ++$total
            } catch { $failures.Add("CONTAINER`t$archive`t$hashText`t$(Clean $name)`t$(Clean $_.Exception.Message)"); Add-Stat $archive 'container-failed' }
        } }
    } finally { $bdt.Dispose() }
    Write-Output "$archive done ($total part BNDs read so far)"
}
} finally { }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false)

# MTD join from the I-0063 evidence (leaf name, case-insensitive).
$mtdMap = [Collections.Generic.Dictionary[string, object]]::new([StringComparer]::OrdinalIgnoreCase)
$mtdDuplicates = 0
foreach ($r in (Import-Csv -LiteralPath $MtdTsv -Delimiter "`t")) { if ($mtdMap.ContainsKey($r.mtd)) { $mtdDuplicates++ } else { $mtdMap[$r.mtd] = $r } }
$mtdUnresolved = [Collections.Generic.SortedSet[string]]::new([StringComparer]::Ordinal)
function Resolve-Mtd([string]$mtd) {
    if (-not $mtd) { return @('NONE', 'NONE', 'NONE', 'NONE') }
    if ($mtdMap.ContainsKey($mtd)) { $r = $mtdMap[$mtd]; return @($r.shader, $r.blend, $r.alpha_ref) }
    [void]$mtdUnresolved.Add($mtd); return @('UNRESOLVED', 'UNRESOLVED', 'UNRESOLVED')
}
$rows = foreach ($r in $flverRecs) {
    $res = Resolve-Mtd $r.mtd
    [pscustomobject]@{ container = $r.container; flver = $r.flver; slot = ($r.flver -split '_')[0].ToUpperInvariant(); idx = $r.idx
        name = $r.name; mtd = $r.mtd; shader = $res[0]; blend = $res[1]; alpha_ref = $res[2]; meshes = $r.meshes; archive = $r.archive; hash = $r.hash }
}
$armourRows = @($rows | Where-Object { $_.slot -match '^(HD|BD|AM|LG)$' })
$faceRows = @($rows | Where-Object { $_.slot -match '^(FG|HR|FC)$' })

function Write-Materials([string]$path, $items) {
    $w = [IO.StreamWriter]::new($path, $false, $utf8)
    try {
        $w.WriteLine("container`tflver`tslot`tmaterial_index`tmaterial_name`tmtd`tshader`tblend`talpha_ref`tmeshes`tarchive`thash")
        foreach ($x in $items) { $w.WriteLine("$($x.container)`t$($x.flver)`t$($x.slot)`t$($x.idx)`t$($x.name)`t$($x.mtd)`t$($x.shader)`t$($x.blend)`t$($x.alpha_ref)`t$($x.meshes)`t$($x.archive)`t$($x.hash)") }
    } finally { $w.Dispose() }
}
function Get-Families($items) {
    $fam = @{}
    foreach ($x in $items) {
        $key = "$($x.mtd)|$($x.shader)|$($x.blend)|$($x.alpha_ref)"
        if (-not $fam.ContainsKey($key)) { $fam[$key] = [pscustomobject]@{ mtd = $x.mtd; shader = $x.shader; blend = $x.blend; alpha_ref = $x.alpha_ref
            materials = 0; flverKeys = [Collections.Generic.HashSet[string]]::new(); flverNames = [Collections.Generic.SortedSet[string]]::new([StringComparer]::Ordinal); slots = @{} } }
        $f = $fam[$key]
        $f.materials++
        if ($f.flverKeys.Add("$($x.container)|$($x.flver)")) { [void]$f.flverNames.Add($x.flver); $f.slots[$x.slot] = 1 + [int]$f.slots[$x.slot] }
    }
    $fam.Values | ForEach-Object { [pscustomobject]@{ mtd = $_.mtd; shader = $_.shader; blend = $_.blend; alpha_ref = $_.alpha_ref; materials = $_.materials
        flvers = $_.flverKeys.Count; slots = (($_.slots.GetEnumerator() | Sort-Object Name | ForEach-Object { "$($_.Name):$($_.Value)" }) -join ';')
        ex1 = @($_.flverNames)[0]; ex2 = @($_.flverNames)[1]; ex3 = @($_.flverNames)[2] } } |
        Sort-Object @{Expression = 'flvers'; Descending = $true}, @{Expression = 'materials'; Descending = $true}, mtd
}
function Write-Families([string]$path, $families) {
    $w = [IO.StreamWriter]::new($path, $false, $utf8)
    try {
        $w.WriteLine("mtd`tshader`tblend`talpha_ref`tmaterials`tflvers`tslots`texample_1`texample_2`texample_3")
        foreach ($f in $families) { $w.WriteLine("$($f.mtd)`t$($f.shader)`t$($f.blend)`t$($f.alpha_ref)`t$($f.materials)`t$($f.flvers)`t$($f.slots)`t$($f.ex1)`t$($f.ex2)`t$($f.ex3)") }
    } finally { $w.Dispose() }
}
Write-Materials (Join-Path $OutDir 'armour-materials.tsv') $armourRows
$armourFamilies = @(Get-Families $armourRows)
Write-Families (Join-Path $OutDir 'armour-families.tsv') $armourFamilies
Write-Materials (Join-Path $OutDir 'face-hair-materials.tsv') $faceRows
$faceFamilies = @(Get-Families $faceRows)
Write-Families (Join-Path $OutDir 'face-hair-families.tsv') $faceFamilies

# Coverage against the earlier scan (I-0063 scan/flver.tsv). Leaf names are compared, case-insensitively.
$oldNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$oldRawPartsAny = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)   # any FLVER leaf whose earlier container label is a raw N:\...\parts\ path
$oldRawPartsArmour = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$oldDictPartsArmour = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)  # armour/face leaf under a dictionary-named data*/parts/ label
$firstLine = $true
foreach ($line in [IO.File]::ReadLines($OldFlverTsv)) {
    if ($firstLine) { $firstLine = $false; continue }
    $cols = $line -split "`t"
    if ($cols.Count -lt 2) { continue }
    $isArmourFace = $cols[1] -match '^(hd|bd|am|lg|fg|hr|fc)_'
    if ($cols[0].Contains('\parts\')) { [void]$oldRawPartsAny.Add($cols[1]); if ($isArmourFace) { [void]$oldRawPartsArmour.Add($cols[1]) } }
    if ($isArmourFace) { [void]$oldNames.Add($cols[1]); if ($cols[0] -match '^data\d+/parts/') { [void]$oldDictPartsArmour.Add($cols[1]) } }
}
$newNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($r in ($armourRows + $faceRows)) { [void]$newNames.Add($r.flver) }
$newOnly = @($newNames | Where-Object { -not $oldNames.Contains($_) } | Sort-Object)
$oldOnly = @($oldNames | Where-Object { -not $newNames.Contains($_) } | Sort-Object)

# FLVERs without materials, and part BNDs without any FLVER.
$noFlver = @($containers | Where-Object { $_.flvers -eq 0 })

$summary = [Collections.Generic.List[string]]::new()
$summary.Add("I-0066 armour and face/hair material-family inventory (Scan-ArmourMaterials.ps1)")
$summary.Add("generated: $((Get-Date).ToString('yyyy-MM-dd HH:mm:ss zzz'))")
$summary.Add("")
$summary.Add("Pinned inputs verified (BinderTool, BinderTool.Core, BouncyCastle, DictionaryDS3, SoulsFormats, DarkSoulsIII.exe): OK")
$summary.Add("Evidence inputs (SHA-256):")
foreach ($p in $inputHashes.Keys) { $summary.Add("  $p  $($inputHashes[$p])") }
$summary.Add("MTD values joined from mtd.tsv (distinct leaves: $($mtdMap.Count); duplicate leaves ignored: $mtdDuplicates). MTD files are not read here.")
$summary.Add("")
$summary.Add("PARTS BNDS READ PER ARCHIVE (only parts BNDs; DS3 names under parts/ with HD_ BD_ AM_ LG_ FG_ HR_ FC_ leaves)")
$summary.Add("archive`tentries`tnamed_read_by_dictionary`tunnamed_read`tunnamed_part_bnd`tarmour_read`tface_hair_read`tflvers_parsed`tcontainers_no_flver`tfailed")
foreach ($a in $Archives) {
    $g = { param($k) [int]$stat["$a|$k"] }
    $namedRead = (& $g 'parts-read-armour') + (& $g 'parts-read-face-hair') + (& $g 'parts-read-other') - (& $g 'unnamed-part-bnd')
    $summary.Add("$a`t$(& $g 'entries')`t$namedRead`t$(& $g 'unnamed-read')`t$(& $g 'unnamed-part-bnd')`t$(& $g 'parts-read-armour')`t$(& $g 'parts-read-face-hair')`t$(& $g 'flvers-parsed')`t$((& $g 'parts-read-armour-no-flver') + (& $g 'parts-read-face-hair-no-flver'))`t$(& $g 'container-failed')")
}
$summary.Add("")
$summary.Add("FLVERS AND FAMILIES")
foreach ($slot in 'HD','BD','AM','LG','FG','HR','FC') {
    $rs = @($rows | Where-Object { $_.slot -eq $slot })
    $summary.Add("slot $slot`tflvers=$(@($rs | ForEach-Object { "$($_.container)|$($_.flver)" } | Sort-Object -Unique).Count)`tmaterials=$($rs.Count)")
}
$summary.Add("armour families (mtd, shader, blend, alpha_ref): $($armourFamilies.Count)")
$summary.Add("face-hair families (mtd, shader, blend, alpha_ref): $($faceFamilies.Count)")
$summary.Add("MTD leaves not found in mtd.tsv: $($mtdUnresolved.Count)$(if ($mtdUnresolved.Count) { ' -> ' + (@($mtdUnresolved) -join ', ') } else { '' })")
$summary.Add("")
$summary.Add("INNER FILE SIGNATURES IN PART BNDS (archive|extension|magic|raw-or-dcx : count)")
foreach ($k in ($kindCount.Keys | Sort-Object)) { $summary.Add("  $k : $($kindCount[$k])") }
$summary.Add("")
$summary.Add("FLVERS THAT PARSE WITH NO MATERIALS (no material rows possible): $($zeroMaterialFlvers.Count)")
foreach ($z in $zeroMaterialFlvers) { $summary.Add("  $z") }
$summary.Add("")
$summary.Add("PART BNDS WITH NO FLVER INSIDE (only tpf/clm2/other inner files; first 60 of $($noFlver.Count))")
foreach ($c in @($noFlver | Select-Object -First 60)) { $summary.Add("  $($c.archive) $($c.hash) $($c.label) [$($c.kinds)]") }
$summary.Add("")
$summary.Add("COVERAGE AGAINST THE EARLIER SCAN (I-0063 scan/flver.tsv)")
$summary.Add("earlier scan distinct armour and face/hair FLVER leaf names: $($oldNames.Count)")
$summary.Add("this scan distinct armour and face/hair FLVER leaf names with materials: $($newNames.Count)")
$summary.Add("in this scan but not in earlier scan: $($newOnly.Count)$(if ($newOnly.Count) { ' -> ' + ($newOnly -join ', ') } else { '' })")
$summary.Add("in earlier scan but not in this scan: $($oldOnly.Count)$(if ($oldOnly.Count) { ' -> ' + ($oldOnly -join ', ') } else { '' })")
$summary.Add("earlier-scan rows whose container label is a raw N:\...\parts\ path (no dictionary name), any leaf: $($oldRawPartsAny.Count) distinct FLVER leaves")
$summary.Add("  of which armour/face/hair leaves: $($oldRawPartsArmour.Count)")
$summary.Add("earlier-scan armour/face/hair leaves under a dictionary-named data*/parts/ label: $($oldDictPartsArmour.Count)")
$summary.Add("")
$summary.Add("NOTES")
foreach ($x in $notes) { $summary.Add("  $x") }
$summary.Add("")
$summary.Add("FAILURES ($($failures.Count))")
foreach ($x in @($failures | Select-Object -First 200)) { $summary.Add("  $x") }
$summary.Add("")
[IO.File]::WriteAllLines((Join-Path $OutDir 'SUMMARY.txt'), $summary, $utf8)
Write-Output "PASS part BNDs read: $total; armour material rows: $($armourRows.Count); armour families: $($armourFamilies.Count); face-hair families: $($faceFamilies.Count); failures: $($failures.Count) -> $OutDir"
