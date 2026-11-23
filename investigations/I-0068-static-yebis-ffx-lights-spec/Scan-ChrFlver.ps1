[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Root,          # folder holding chr\cNNNN.chrbnd (the Data3 character archive, recovered read-only)
    [Parameter(Mandatory = $true)][string[]]$Chr,         # for example c2150,c1090,c5270,c5110
    [Parameter(Mandatory = $true)][string]$Out            # output TSV, one row per FLVER mesh
)
# I-0068 part AR: one TSV row per mesh of the main FLVER of each character: material and MTD, vertex count, the vertex layout
# (semantic, value type, offset), the face sets (index size in bits, flags, indices), whether the layout carries bone indices
# and weights, and the model totals (meshes, materials, bones, dummies). Read-only; SoulsFormats 1.0.1.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("chr`tmesh`tmaterial`tmtd`tdynamic`tdefault_bone`tbone_indices`tvertices`tstride_groups`tlayout`tfacesets`tfaceset_index_bits`tfaceset_flags`tindices_lod0`tmodel_meshes`tmodel_materials`tmodel_bones`tmodel_dummies`tversion")
foreach ($c in $Chr) {
    $bnd = [SoulsFormats.BND4]::Read((Join-Path $Root "chr\$c.chrbnd"))
    $f = $bnd.Files | Where-Object { $_.Name -like '*.flver' } | Select-Object -First 1
    $fl = [SoulsFormats.FLVER]::Read($f.Bytes)
    for ($i = 0; $i -lt $fl.Meshes.Count; $i++) {
        $m = $fl.Meshes[$i]; $mat = $fl.Materials[$m.MaterialIndex]
        $verts = 0; foreach ($g in $m.VertexGroups) { $verts += $g.Vertices.Count }
        $layout = ''
        if ($m.VertexGroups.Count) { $layout = ($fl.VertexStructLayouts[$m.VertexGroups[0].VertexStructLayoutIndex] | ForEach-Object { "$($_.Semantic)$($_.Index):$($_.ValueType)@$($_.StructOffset)" }) -join ' ' }
        $fs = @($m.FaceSets | ForEach-Object { "$($_.Flags)/$($_.IndexSize)/$($_.Vertices.Length)/cull=$($_.CullBackfaces)" })
        $bits = ($m.FaceSets | ForEach-Object { $_.IndexSize } | Sort-Object -Unique) -join ','
        $lod0 = if ($m.FaceSets.Count) { $m.FaceSets[0].Vertices.Length } else { 0 }
        [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7}`t{8}`t{9}`t{10}`t{11}`t{12}`t{13}`t{14}`t{15}`t{16}`t{17}`t{18}" -f $c, $i, $mat.Name, [IO.Path]::GetFileName($mat.MTD), [int]$m.Dynamic, $m.DefaultBoneIndex, $m.BoneIndices.Count, $verts, $m.VertexGroups.Count, $layout, $m.FaceSets.Count, $bits, ($fs -join ';'), $lod0, $fl.Meshes.Count, $fl.Materials.Count, $fl.Bones.Count, $fl.Dummies.Count, $fl.Header.Version))
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
