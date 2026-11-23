[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Dir,        # folder of recovered map_<zone>_..._<piece>.mapbnd.dcx (Recover-GameData.ps1 of I-0045)
    [Parameter(Mandatory = $true)][string]$Out,        # output TSV (one row per FLVER material)
    [string]$MtdTable = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0063/artifacts/scan/mtd.tsv')
)
# I-0068 part BO: one row per FLVER material of every map piece of the recovered zones, with the MTD's shader family
# (mtd.tsv of I-0063), the GX04/GX90 tessellation record (MENUS-MAP section 67: d[3]&1 displacement flag, d[4] mode, d[5..6]
# edge/inside factor, d[7], d[8..9] fade start/end, d[10], d[11]; the item is {u32 id, u32 unk, u32 size, data...} so d[k] is
# the k-th dword of the item) and the water-class flag. The game is not started; nothing is written but $Out.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$shader = @{}; $tessm = @{}; $disp = @{}
foreach ($l in [IO.File]::ReadLines($MtdTable)) { $c = $l -split "`t"; if ($c[0] -eq 'mtd') { continue }; $shader[$c[0].ToLowerInvariant()] = $c[1]; $tessm[$c[0].ToLowerInvariant()] = $c[4]; $disp[$c[0].ToLowerInvariant()] = $c[5] }
$inv = [Globalization.CultureInfo]::InvariantCulture
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("zone`tpiece`tmaterial`tmtd`tshader`tmtd_tess`tmtd_disp`tmeshes`tverts`tfaceset0_indices`tgx04_disp`tgx04_mode`tgx04_edge`tgx04_inside`tgx04_fade_start`tgx04_fade_end`tgx04_d7`tgx04_d10`tgx04_d11`tgx90`titems")
foreach ($f in (Get-ChildItem -LiteralPath $Dir -Filter '*.mapbnd.dcx' | Sort-Object Name)) {
    $m = [regex]::Match($f.Name, '^(?:dlc\d_)?map_(m\d+)_.*_(\d{6})\.mapbnd\.dcx$'); if (-not $m.Success) { continue }
    $zone = $m.Groups[1].Value; $piece = $m.Groups[2].Value
    $bnd = [SoulsFormats.BND4]::Read($f.FullName)
    foreach ($e in ($bnd.Files | Where-Object { $_.Name -like '*.flver' })) {
        $fl = [SoulsFormats.FLVER]::Read($e.Bytes)
        for ($k = 0; $k -lt $fl.Materials.Count; $k++) {
            $mat = $fl.Materials[$k]
            $meshes = @($fl.Meshes | Where-Object { $_.MaterialIndex -eq $k })
            $verts = 0; $idx = 0
            foreach ($ms in $meshes) { foreach ($g in $ms.VertexGroups) { $verts += $g.Vertices.Count }; if ($ms.FaceSets.Count) { $idx += $ms.FaceSets[0].Vertices.Length } }
            $mtd = [IO.Path]::GetFileName($mat.MTD); $key = $mtd.ToLowerInvariant()
            $items = @(); $g4 = $null; $g90 = 0; $b = $mat.GXBytes
            if ($b -ne $null) {
                $at = 0
                while ($at + 12 -le $b.Length) {
                    $id = [BitConverter]::ToUInt32($b, $at); if ($id -eq 0x7FFFFFFF) { break }
                    $sz = [BitConverter]::ToUInt32($b, $at + 8); if ($sz -lt 12 -or $at + $sz -gt $b.Length) { break }
                    $name = [Text.Encoding]::ASCII.GetString($b, $at, 4); $items += $name
                    if ($name -eq 'GX04' -and $sz -ge 48) { $g4 = for ($d = 0; $d -lt 12; $d++) { [BitConverter]::ToUInt32($b, $at + 4 * $d) } }
                    if ($name -eq 'GX90') { $g90 = 1 }
                    $at += $sz
                }
            }
            $fo = { param($u) ([BitConverter]::ToSingle([BitConverter]::GetBytes([uint32]$u), 0)).ToString('R', $inv) }
            if ($g4) { $cols = @(($g4[3] -band 1), $g4[4], (& $fo $g4[5]), (& $fo $g4[6]), (& $fo $g4[8]), (& $fo $g4[9]), (& $fo $g4[7]), (& $fo $g4[10]), (& $fo $g4[11])) } else { $cols = @('', '', '', '', '', '', '', '', '') }
            [void]$rows.Add((@($zone, $piece, $mat.Name, $mtd, $shader[$key], $tessm[$key], $disp[$key], $meshes.Count, $verts, $idx) + $cols + @($g90, ($items -join ' '))) -join "`t")
        }
    }
}
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"
