[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Dir,     # folder of recovered *.mapbnd.dcx (Recover-GameData.ps1 of I-0045, pattern ^data\d/map[/\\]m30_00_00_00[/\\].*\.mapbnd\.dcx$)
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068 part AP: one TSV row per FLVER material of every map piece of a zone, from the retained game data (read-only):
# piece (the model number, as in the MSB model name m<piece>), material name, MTD file, mesh count, vertex count, the LOD0
# face set's index count, CullBackfaces of any mesh (0 = two-sided), the face-set count of the first mesh, the index size
# in bits, the vertex layout with value types and offsets, and the GX item names. The game is not started; nothing is written
# but $Out. SoulsFormats 1.0.1 (the pinned library of the earlier workstreams).
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("piece`tmaterial`tmtd`tmeshes`tverts`tfaceset0_indices`tcull_backfaces_any`tfacesets`tindex_bits`tlayout`tgx")
foreach ($f in Get-ChildItem -LiteralPath $Dir -Filter *.mapbnd.dcx) {
    $piece = $f.Name -replace '.*_(\d{6})\.mapbnd\.dcx', '$1'
    $bnd = [SoulsFormats.BND4]::Read($f.FullName)
    foreach ($e in ($bnd.Files | Where-Object { $_.Name -like '*.flver' })) {
        $fl = [SoulsFormats.FLVER]::Read($e.Bytes)
        for ($k = 0; $k -lt $fl.Materials.Count; $k++) {
            $mat = $fl.Materials[$k]
            $meshes = @($fl.Meshes | Where-Object { $_.MaterialIndex -eq $k })
            $verts = 0; $idx = 0; $cull = 0; $nfs = 0; $bits = 0; $layout = ''
            foreach ($m in $meshes) {
                foreach ($g in $m.VertexGroups) { $verts += $g.Vertices.Count }
                if ($m.FaceSets.Count) { $idx += $m.FaceSets[0].Vertices.Length; if ($m.FaceSets[0].CullBackfaces) { $cull = 1 }; $nfs = $m.FaceSets.Count; $bits = $m.FaceSets[0].IndexSize }
            }
            if ($meshes.Count -and $meshes[0].VertexGroups.Count) {
                $layout = ($fl.VertexStructLayouts[$meshes[0].VertexGroups[0].VertexStructLayoutIndex] | ForEach-Object { "$($_.Semantic)$($_.Index):$($_.ValueType)@$($_.StructOffset)" }) -join ' '
            }
            $gx = @(); $b = $mat.GXBytes
            if ($b -ne $null) { $at = 0; while ($at + 12 -le $b.Length) { $id = [BitConverter]::ToUInt32($b, $at); if ($id -eq 0x7FFFFFFF) { break }; $sz = [BitConverter]::ToUInt32($b, $at + 8); if ($sz -lt 12 -or $at + $sz -gt $b.Length) { break }; $gx += [Text.Encoding]::ASCII.GetString($b, $at, 4); $at += $sz } }
            [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7}`t{8}`t{9}`t{10}" -f $piece, $mat.Name, [IO.Path]::GetFileName($mat.MTD), $meshes.Count, $verts, $idx, $cull, $nfs, $bits, $layout, ($gx -join ' ')))
        }
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
