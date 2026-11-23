param(
    [string]$InputPath = (Join-Path $PSScriptRoot '../../game/DarkSoulsIII.exe'),
    [Parameter(Mandatory)][string]$OutputPath
)
$ErrorActionPreference = 'Stop'
$expected = 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'
if ((Get-FileHash -LiteralPath $InputPath -Algorithm SHA256).Hash -ne $expected) { throw 'DS3 input identity mismatch' }
$stream = [IO.File]::OpenRead((Resolve-Path -LiteralPath $InputPath).Path)
$reader = [IO.BinaryReader]::new($stream)
try {
    $stream.Position = 0x3c
    $pe = $reader.ReadUInt32()
    $stream.Position = $pe
    if ($reader.ReadUInt32() -ne 0x4550) { throw 'Missing PE header' }
    $stream.Position = $pe + 6
    $sectionCount = $reader.ReadUInt16()
    $stream.Position = $pe + 20
    $optionalBytes = $reader.ReadUInt16()
    $stream.Position = $pe + 24
    if ($reader.ReadUInt16() -ne 0x20b) { throw 'Expected PE32+' }
    $stream.Position = $pe + 48
    $imageBase = $reader.ReadUInt64()
    $sections = @()
    for ($i = 0; $i -lt $sectionCount; $i++) {
        $stream.Position = $pe + 24 + $optionalBytes + 40 * $i + 12
        $sections += [pscustomobject]@{ rva = $reader.ReadUInt32(); bytes = $reader.ReadUInt32(); file = $reader.ReadUInt32() }
    }
    function Seek-VA([uint64]$va) {
        $rva = $va - $imageBase
        foreach ($section in $sections) {
            if ($rva -ge $section.rva -and $rva -lt ([uint64]$section.rva + $section.bytes)) {
                $stream.Position = $section.file + $rva - $section.rva
                return
            }
        }
        throw ('VA outside file-backed sections: {0:x}' -f $va)
    }
    function Read-Wide([uint64]$va) {
        Seek-VA $va
        $text = [Text.StringBuilder]::new()
        for ($i = 0; $i -lt 256; $i++) {
            $unit = $reader.ReadUInt16()
            if (!$unit) { return $text.ToString() }
            [void]$text.Append([char]$unit)
        }
        throw 'Recipe string exceeds bound'
    }
    $rows = @()
    foreach ($va in @(0x143d48a40L,0x143d48a50L,0x143d48a88L,0x143d48b90L,0x143d48be0L,0x143d1f228L,0x143d48bd0L,0x143d48bf8L,0x143d43f70L)) {
        $rows += [pscustomobject]@{ kind = 'format'; table = ('{0:x}' -f $va); index = 0; address = ('{0:x}' -f $va); text = Read-Wide $va }
    }
    foreach ($table in @(@(0x14473ce60L,20,'pass'),@(0x14473ce38L,4,'layout'),@(0x14473cf00L,2,'instance'),@(0x14473cf10L,2,'channel'),@(0x14473ca90L,4,'morph'))) {
        for ($i = 0; $i -lt $table[1]; $i++) {
            Seek-VA ($table[0] + 8 * $i)
            $va = $reader.ReadUInt64()
            $rows += [pscustomobject]@{ kind = $table[2]; table = ('{0:x}' -f $table[0]); index = $i; address = ('{0:x}' -f $va); text = Read-Wide $va }
        }
    }
    [pscustomobject]@{ schema_version = 1; input_sha256 = $expected; source = 'AA630/081FF0 formatting operands, file-backed PE data'; rows = $rows } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutputPath -Encoding utf8NoBOM
} finally { $reader.Dispose(); $stream.Dispose() }
