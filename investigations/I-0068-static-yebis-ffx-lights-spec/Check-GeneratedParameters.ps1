[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Root,          # folder holding chr\cNNNN.chrbnd
    [Parameter(Mandatory = $true)][string[]]$Chr,
    [Parameter(Mandatory = $true)][string]$MtdTable,      # all-mtd.tsv of Dump-AllMtd.ps1 (mtd, shader, param, type, value): the shader family of each MTD
    [string]$ShaderBdle = 'C:\Users\User\.codex\I0063-cache\shaderbnd\shader_shaderbdle.shaderbdlebnd.dcx',
    [Parameter(Mandatory = $true)][string]$Out            # output TSV, one row per (character, material)
)
# I-0068 part AR: replays native/content/model_parameters.cpp ReadModelGeneratedParameters (the SAT "generated" scalar words of
# cbMatDynParam: Veill, FlowmapVertexOffset, cinder, crystal families) on the game data, read-only. For every material of each
# character whose shader family has a .metaparam (the SPX the engine keeps at material+0x?; nested in shader_shaderbdle), the
# descriptor table and the material's FLVER GXMD record are read and the native status is computed with the same rules:
#   Invalid: bad envelope; Missing: no descriptors, no GX bytes, no GXMD record, or record type 100;
#   Unsupported: reg < 0x27, two GXMD records, a row type outside {0,1,2,3,4,5,6,7,8,11}, a descriptor type 0 or > 11,
#   a descriptor component > 4, or (unordered branch) a component beyond the row's components; Ready otherwise.
# The outcome column also prints the first reason found. Nothing is written but $Out.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$meta = @{}
$family = @{}
Import-Csv $MtdTable -Delimiter "`t" | ForEach-Object { if (-not $family.ContainsKey($_.mtd.ToLower())) { $family[$_.mtd.ToLower()] = $_.shader } }
$outer = [SoulsFormats.BND4]::Read($ShaderBdle)
foreach ($f in $outer.Files) {
    $n = [SoulsFormats.BND4]::Read($f.Bytes)
    $m = $n.Files | Where-Object { $_.Name -like '*.metaparam' } | Select-Object -First 1
    if ($m) { $meta[(Split-Path -Leaf (Split-Path -Parent $f.Name))] = $m.Bytes }
}
function U32([byte[]]$b, [int]$at) { [BitConverter]::ToUInt32($b, $at) }
function U64([byte[]]$b, [int]$at) { [BitConverter]::ToUInt64($b, $at) }
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("chr`tmaterial`tmtd`tfamily`tcount`treg`tscalar_bytes`tspx_key`tspx_rows`tgxmd_key`tgxmd_rows`tordered`tdescriptor_types`tstatus`treason")
foreach ($c in $Chr) {
    $bnd = [SoulsFormats.BND4]::Read((Join-Path $Root "chr\$c.chrbnd"))
    $fl = [SoulsFormats.FLVER]::Read(($bnd.Files | Where-Object { $_.Name -like '*.flver' } | Select-Object -First 1).Bytes)
    $seen = @{}
    for ($k = 0; $k -lt $fl.Materials.Count; $k++) {
        $mat = $fl.Materials[$k]; $mtd = [IO.Path]::GetFileName($mat.MTD)
        $fam = $family[$mtd.ToLower()]   # the shader family of the MTD (all-mtd.tsv)
        $key = "$mtd|$k"; if ($seen.ContainsKey($key)) { continue }; $seen[$key] = 1
        if (-not $fam -or -not $meta.ContainsKey($fam)) { continue }
        $spx = $meta[$fam]; $status = 'Ready'; $reason = ''
        $count = [int](U32 $spx 0x28); $table = [int](U64 $spx 0x18); $scalarBytes = [int](U32 $spx 0x4C); $reg = [int](U32 $spx 0x50)
        $spxKey = U32 $spx 0x48; $spxRows = U32 $spx 0x54
        $types = @(); for ($i = 0; $i -lt $count; $i++) { $types += $spx[$table + $i * 0x60 + 0x38] }
        $gx = $mat.GXBytes; $gxKey = ''; $gxRows = ''; $ordered = ''
        function Fail([string]$s, [string]$r) { if ($script:status -eq 'Ready') { $script:status = $s; $script:reason = $r } }
        if ($count -le 0 -or ($scalarBytes -band 3) -or [int]($scalarBytes / 4) -ne $count -or $reg -gt 4096) { Fail 'Invalid' 'spx envelope' }
        elseif ($gx -eq $null -or $gx.Length -eq 0) { Fail 'Missing' 'no GX bytes' }
        elseif ($reg -lt 0x27) { Fail 'Unsupported' "reg 0x$('{0:X}' -f $reg) < 0x27" }
        else {
            $at = 0; $record = $null; $termin = $false
            while ($at + 12 -le $gx.Length) {
                $gk = U32 $gx $at; $len = U32 $gx ($at + 8)
                if ($gk -eq 0x7FFFFFFF) { $termin = $true; break }
                if ($len -lt 12 -or $len -gt $gx.Length - $at) { Fail 'Invalid' 'gx record length'; break }
                if ($gk -eq 0x444D5847) { if ($record -ne $null) { Fail 'Unsupported' 'two GXMD records' }; $record = $gx[$at..($at + $len - 1)] }
                $at += $len
            }
            if ($status -eq 'Ready' -and -not $termin) { Fail 'Invalid' 'gx not terminated' }
            if ($status -eq 'Ready' -and ($record -eq $null -or (U32 $record 4) -eq 100)) { Fail 'Missing' 'no GXMD record or type 100' }
            if ($status -eq 'Ready') {
                $gxKey = U32 $record 4; $gxRows = U32 $record 12
                $p = 16; $comps = @()
                for ($i = 0; $i -lt $gxRows; $i++) {
                    $rk = U32 $record $p; $type = U32 $record ($p + 4); $p += 8
                    $cc = if ($type -in 0, 1, 5) { 1 } elseif ($type -ge 2 -and $type -le 4) { $type } elseif ($type -ge 6 -and $type -le 8) { $type - 4 } elseif ($type -eq 11) { 5 } else { -1 }
                    if ($cc -lt 0) { Fail 'Unsupported' "row type $type"; break }
                    $comps += , @($rk, $cc); $p += 4 * $cc
                }
                if ($status -eq 'Ready') { foreach ($t in $types) { if ($t -eq 0 -or $t -gt 11) { Fail 'Unsupported' "descriptor type $t"; break } } }
                if ($status -eq 'Ready') {
                    $ordered = ($gxKey -eq $spxKey -and $gxRows -eq $spxRows)
                    if (-not $ordered) {
                        for ($i = 0; $i -lt $count -and $status -eq 'Ready'; $i++) {
                            $d = $table + $i * 0x60; $dk = U32 $spx ($d + 0x2C); $dc = U32 $spx ($d + 0x30)
                            if ($dc -gt 4) { Fail 'Unsupported' "descriptor component $dc"; break }
                            $row = $comps | Where-Object { $_[0] -eq $dk } | Select-Object -First 1
                            if ($row -and $dc -ge $row[1]) { Fail 'Unsupported' "component $dc beyond row $dk components $($row[1])" }
                        }
                    }
                }
            }
        }
        [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t0x{5:X}`t{6}`t{7}`t{8}`t{9}`t{10}`t{11}`t{12}`t{13}`t{14}" -f $c, $mat.Name, $mtd, $fam, $count, $reg, $scalarBytes, $spxKey, $spxRows, $gxKey, $gxRows, $ordered, ($types -join ','), $status, $reason))
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
