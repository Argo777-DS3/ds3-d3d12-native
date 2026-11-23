[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Zone,             # for example m30_00
    [Parameter(Mandatory = $true)][string]$MapDir,           # folder of the zone's recovered *.mapbnd.dcx (see Scan-MapFlver.ps1)
    [Parameter(Mandatory = $true)][double[]]$Eye,            # camera position, MSB coordinates (native_pass_rows "eye")
    [Parameter(Mandatory = $true)][double[]]$Forward,        # camera axis (native_pass_rows "axis_z")
    [double]$FovVerticalDegrees = 60,
    [double]$Aspect = 16.0 / 9.0,
    [double]$Radius = 160,
    [string]$MsbDir = 'C:\Users\User\.codex\I0063-cache\gamedata\msb',
    [string]$MtdPattern = 'grass|_l_e\.mtd|_e\.mtd|_al|_tr',
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068 part AP: which foliage-class FLVER meshes of a zone fall on screen for one camera pose. For every MSB map-piece part
# within $Radius of the eye, the vertices of each mesh whose MTD matches $MtdPattern are placed (Euler X, Z, Y order, scale)
# and projected with a pinhole camera; one TSV row per (part, mesh) with the on-screen vertex count and the pixel extent.
# APPROXIMATE by design: no display groups, no occlusion, no frustum planes of the engine (the game's own culler decides at
# run time); use it to rank what a witness frame should contain, not to count what the original draws.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$msb = [SoulsFormats.MSB3]::Read((Join-Path $MsbDir "map_MapStudio_${Zone}_00_00.msb.dcx"))
$camPos = [numerics.Vector3]::new($Eye[0], $Eye[1], $Eye[2])
$fwd = [numerics.Vector3]::Normalize([numerics.Vector3]::new($Forward[0], $Forward[1], $Forward[2]))
$right = [numerics.Vector3]::Normalize([numerics.Vector3]::Cross([numerics.Vector3]::UnitY, $fwd))
$up = [numerics.Vector3]::Normalize([numerics.Vector3]::Cross($fwd, $right))
$tanH = [math]::Tan($FovVerticalDegrees * [math]::PI / 360); $tanW = $tanH * $Aspect
$files = @{}; foreach ($f in Get-ChildItem -LiteralPath $MapDir -Filter *.mapbnd.dcx) { $files[($f.Name -replace '.*_(\d{6})\.mapbnd\.dcx', '$1')] = $f.FullName }
$cache = @{}
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("part`tmaterial`tmtd`tdistance`tvertices`tvertices_on_screen`tx_min`tx_max`ty_min`ty_max")
foreach ($p in $msb.Parts.MapPieces) {
    $num = $p.ModelName -replace '^m', ''
    if (-not $files.ContainsKey($num)) { continue }
    $d0 = ($p.Position - $camPos).Length(); if ($d0 -gt $Radius) { continue }
    if (-not $cache.ContainsKey($num)) { $cache[$num] = [SoulsFormats.FLVER]::Read(([SoulsFormats.BND4]::Read($files[$num])).Files[0].Bytes) }
    $fl = $cache[$num]
    $R = [numerics.Matrix4x4]::CreateRotationX($p.Rotation.X * [math]::PI / 180) * [numerics.Matrix4x4]::CreateRotationZ($p.Rotation.Z * [math]::PI / 180) * [numerics.Matrix4x4]::CreateRotationY($p.Rotation.Y * [math]::PI / 180)
    foreach ($mesh in $fl.Meshes) {
        $mat = $fl.Materials[$mesh.MaterialIndex]; $mtd = [IO.Path]::GetFileName($mat.MTD)
        if ($mtd -notmatch $MtdPattern) { continue }
        $n = 0; $tot = 0; $x0 = 1e9; $x1 = -1e9; $y0 = 1e9; $y1 = -1e9
        foreach ($vg in $mesh.VertexGroups) { foreach ($v in $vg.Vertices) {
            $tot++
            $w = [numerics.Vector3]::Transform($v.Position * $p.Scale, $R) + $p.Position; $d = $w - $camPos
            $z = [numerics.Vector3]::Dot($d, $fwd); if ($z -lt 0.3) { continue }
            $sx = 640 + 640 * ([numerics.Vector3]::Dot($d, $right) / $z) / $tanW
            $sy = 360 - 360 * ([numerics.Vector3]::Dot($d, $up) / $z) / $tanH
            if ($sx -ge 0 -and $sx -le 1280 -and $sy -ge 0 -and $sy -le 720) { $n++; if ($sx -lt $x0) { $x0 = $sx }; if ($sx -gt $x1) { $x1 = $sx }; if ($sy -lt $y0) { $y0 = $sy }; if ($sy -gt $y1) { $y1 = $sy } }
        } }
        if ($n -gt 0) { [void]$rows.Add(("{0}`t{1}`t{2}`t{3:F1}`t{4}`t{5}`t{6:F0}`t{7:F0}`t{8:F0}`t{9:F0}" -f $p.Name, $mat.Name, ($mtd -replace '\.mtd', ''), $d0, $tot, $n, $x0, $x1, $y0, $y1)) }
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
