[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Dir,        # folder of recovered *_MapStudio_<zone>.msb.dcx (Recover-GameData.ps1 of I-0045)
    [Parameter(Mandatory = $true)][string]$Out         # output TSV, one row per map piece / collision part
)
# I-0068 part BO: the MSB (DS3 MSB3) map pieces of the zones with their reflection-camera flags and the collision parts with
# their ReflectPlaneHeight, to answer which parts the water reflection view gathers (WATER-REFLECTION-INTERACTION-E08 section 1.2:
# the view gathers with the mask low byte 0x10; the candidate in the data is Part.DrawByReflectCam / DrawOnlyReflectCam). Read-only.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$inv = [Globalization.CultureInfo]::InvariantCulture
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("zone`ttype`tname`tmodel`tposx`tposy`tposz`tdraw_by_reflect`tdraw_only_reflect`tshadow_src`tshadow_dest`tis_shadow_only`treflect_plane_height`tdraw_groups")
foreach ($f in (Get-ChildItem -LiteralPath $Dir -Filter '*.msb.dcx' | Sort-Object Name)) {
    $zone = [regex]::Match($f.Name, '(m\d+)_00_00_00').Groups[1].Value
    $msb = [SoulsFormats.MSB3]::Read($f.FullName)
    foreach ($p in $msb.Parts.MapPieces) {
        [void]$rows.Add((@($zone, 'MapPiece', $p.Name, $p.ModelName, $p.Position.X.ToString('R', $inv), $p.Position.Y.ToString('R', $inv), $p.Position.Z.ToString('R', $inv), [int]$p.DrawByReflectCam, [int]$p.DrawOnlyReflectCam, [int]$p.ShadowSource, [int]$p.ShadowDest, [int]$p.IsShadowOnly, '', ($p.DrawGroups -join ',')) -join "`t"))
    }
    foreach ($p in $msb.Parts.Collisions) {
        [void]$rows.Add((@($zone, 'Collision', $p.Name, $p.ModelName, $p.Position.X.ToString('R', $inv), $p.Position.Y.ToString('R', $inv), $p.Position.Z.ToString('R', $inv), [int]$p.DrawByReflectCam, [int]$p.DrawOnlyReflectCam, [int]$p.ShadowSource, [int]$p.ShadowDest, [int]$p.IsShadowOnly, $p.ReflectPlaneHeight.ToString('R', $inv), ($p.DrawGroups -join ',')) -join "`t"))
    }
}
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"
