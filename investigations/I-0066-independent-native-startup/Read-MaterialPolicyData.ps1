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
    function Read-Ascii([uint64]$va) {
        Seek-VA $va
        $text = [Text.StringBuilder]::new()
        for ($i = 0; $i -lt 256; $i++) {
            $unit = $reader.ReadByte()
            if (!$unit) { return $text.ToString() }
            [void]$text.Append([char]$unit)
        }
        throw 'Material string exceeds bound'
    }
    $types = @()
    for ($i = 0; $i -lt 12; $i++) {
        Seek-VA (0x1446bee90L + 8 * $i)
        $va = $reader.ReadUInt64()
        $types += [pscustomobject]@{ index = $i; address = ('{0:x}' -f $va); text = Read-Ascii $va }
    }
    $names = @()
    foreach ($va in @(0x143d1d710L,0x143d1d758L,0x143d1d760L,0x143d1d768L,0x143d1d780L,0x143d1d788L,0x143d1d790L,0x143d1d7a0L,0x143d1d7b8L,0x143d1d7d0L)) {
        $names += [pscustomobject]@{ address = ('{0:x}' -f $va); text = Read-Wide $va }
    }
    $parameters = @()
    for ($i = 0; $i -lt 0x41; $i++) {
        Seek-VA (0x143d20580L + 0x40 * $i)
        $va = $reader.ReadUInt64()
        $kind = $reader.ReadUInt32(); $selector = $reader.ReadUInt32()
        $offset = $reader.ReadUInt32(); $bytes = $reader.ReadUInt32(); $extra = $reader.ReadUInt32()
        Seek-VA (0x143d205a0L + 0x40 * $i)
        $defaults = @(for ($j = 0; $j -lt 8; $j++) { $reader.ReadUInt32() })
        $parameters += [pscustomobject]@{ index = $i; name = Read-Wide $va; kind = $kind; animation_selector = $selector; vector_index = $offset; scalar_offset = $bytes; extra = $extra; descriptor = $defaults }
    }
    Seek-VA 0x14472be30L
    $crcBytes = $reader.ReadBytes(1024)
    if ($crcBytes.Length -ne 1024) { throw 'CRC table extent' }
    $crcTable = @(for ($i = 0; $i -lt 256; $i++) {
        [uint32]$value = $i
        for ($bit = 0; $bit -lt 8; $bit++) {
            $value = if ($value -band 1) { ($value -shr 1) -bxor [uint32]3988292384 } else { $value -shr 1 }
        }
        $actual = [BitConverter]::ToUInt32($crcBytes,4*$i)
        if ($actual -ne $value) { throw "CRC polynomial mismatch at $i" }
        $actual
    })
    $crcKeys = @(foreach ($parameter in $parameters) {
        [uint32]$key = [uint32]::MaxValue
        foreach ($byte in [Text.Encoding]::Unicode.GetBytes($parameter.name)) {
            $key = ($key -shr 8) -bxor $crcTable[($key -bxor $byte) -band 255]
        }
        [pscustomobject]@{ name = $parameter.name; lookup_key = $key -bxor [uint32]::MaxValue }
    })
    $crc = [pscustomobject]@{ address = '14472be30'; sha256 = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($crcBytes)); polynomial = 'EDB88320'; verified_words = 256; keys = $crcKeys }
    Seek-VA 0x143d7ee90L
    $rgbPower = $reader.ReadSingle()
    $policySlots = @(foreach ($va in @(0x143d60198L,0x143d60338L,0x143d60598L,0x143d60648L,0x143d607d8L,0x143d608f8L,0x143d60a38L,0x143d60d08L,0x143d60f18L,0x143d603e8L,0x143d60498L,0x143d60fc8L)) {
        $slots = @(foreach ($offset in @(0x50,0x58,0x80,0x90)) {
            Seek-VA ($va+$offset)
            [pscustomobject]@{ offset = $offset; address = ('{0:x}' -f $reader.ReadUInt64()) }
        })
        [pscustomobject]@{ table = ('{0:x}' -f $va); slots = $slots }
    })
    [pscustomobject]@{ schema_version = 3; input_sha256 = $expected; source = '193AED0 types, 2DD60 family tests, loaded A7E40 parameter definitions and 1FF910 RGB exponent; loaded specialized constructors758 identify policy vtables'; types = $types; names = $names; parameters = $parameters; rgb_power = $rgbPower; crc = $crc; policy_slots = $policySlots; parameter_prefix = Read-Wide 0x143d48660L } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutputPath -Encoding utf8NoBOM
} finally { $reader.Dispose(); $stream.Dispose() }
