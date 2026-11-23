[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Dir,     # recovered *.mapbnd.dcx / *.chrbnd.dcx / *.partsbnd.dcx (Recover-NamedGameData.ps1 of I-0068)
    [Parameter(Mandatory = $true)][string]$Filter,  # e.g. *m40_00_00_00_*.mapbnd.dcx
    [Parameter(Mandatory = $true)][string]$Out      # TSV, one row per FLVER mesh
)
# I-0110: what a ray-tracing BLAS builder meets in DS3's FLVER models, one row per mesh: the binder and FLVER, the mesh's
# Dynamic flag (skinned), its bone list size and default bone, the FLVER's bone count, the material and its MTD, the vertex
# count, the first vertex group's layout (semantic:value type@offset), every face set (flags in hex, strip, back-face culling,
# index bits, index count), and up to 16 distinct NORMAL.w values of the mesh (the rigid bone index of DS3's single-bone meshes).
# Read-only, the pinned SoulsFormats 1.0.1; the game is not started; nothing is written but $Out.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("file`tflver`tmesh`tdynamic`tmesh_bones`tdefault_bone`tflver_bones`tmaterial`tmtd`tverts`tlayout`tfacesets`tnormal_w")
foreach ($f in Get-ChildItem -LiteralPath $Dir -Filter $Filter | Sort-Object Name) {
    $bnd = [SoulsFormats.BND4]::Read($f.FullName)
    foreach ($e in ($bnd.Files | Where-Object { $_.Name -like '*.flver' })) {
        $fl = [SoulsFormats.FLVER]::Read($e.Bytes)
        for ($m = 0; $m -lt $fl.Meshes.Count; $m++) {
            $mesh = $fl.Meshes[$m]; $mat = $fl.Materials[$mesh.MaterialIndex]
            $verts = 0; foreach ($g in $mesh.VertexGroups) { $verts += $g.Vertices.Count }
            $layout = ''
            if ($mesh.VertexGroups.Count) {
                $layout = ($fl.VertexStructLayouts[$mesh.VertexGroups[0].VertexStructLayoutIndex] | ForEach-Object { "$($_.Semantic)$($_.Index):$($_.ValueType)@$($_.StructOffset)" }) -join ' '
            }
            $fs = ($mesh.FaceSets | ForEach-Object { '{0:X}/{1}/{2}/{3}/{4}' -f [int64]$_.Flags, [int]$_.TriangleStrip, [int]$_.CullBackfaces, $_.IndexSize, $_.Vertices.Length }) -join ' '
            $w = @{}; foreach ($g in $mesh.VertexGroups) { foreach ($v in $g.Vertices) { if ($w.Count -lt 16) { $w[[string]$v.Normal.W] = 1 } } }
            [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7}`t{8}`t{9}`t{10}`t{11}`t{12}" -f ($f.Name -replace '\.(mapbnd|chrbnd|partsbnd)\.dcx$', ''),
                [IO.Path]::GetFileName($e.Name), $m, [int]$mesh.Dynamic, $mesh.BoneIndices.Count, $mesh.DefaultBoneIndex, $fl.Bones.Count,
                $mat.Name, [IO.Path]::GetFileName($mat.MTD), $verts, $layout, $fs, (($w.Keys | Sort-Object) -join ',')))
        }
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"meshes $($rows.Count - 1) -> $Out"
