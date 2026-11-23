[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Msb,     # a recovered *.msb.dcx (Recover-NamedGameData.ps1 of I-0068)
    [Parameter(Mandatory = $true)][string]$Out      # TSV, one row per placed part (map pieces, objects, enemies)
)
# I-0110: the parts a TLAS would place, read with the pinned SoulsFormats 1.0.1: kind, name, model, scale, the four draw-group
# and display-group words, and the part flags under SoulsFormats 1.0.1's names (IsShadowOnly, DrawByReflectCam,
# DrawOnlyReflectCam, DisablePointLightEffect, LodParamID, UseDepthBiasFloat), which are not confirmed in the engine and do not
# fit every part kind. Numbers in the invariant culture. Read-only; the game is not started; nothing is written but $Out.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$inv = [Globalization.CultureInfo]::InvariantCulture
$m = [SoulsFormats.MSB3]::Read($Msb)
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("kind`tname`tmodel`tscale`tdraw_groups`tdisp_groups`tis_shadow_only`tdraw_by_reflect_cam`tdraw_only_reflect_cam`tdisable_point_light`tlod_param`tdepth_bias_float")
foreach ($kind in 'MapPieces', 'Objects', 'Enemies') {
    foreach ($p in @($m.Parts.$kind)) {
        $scale = [string]::Format($inv, '{0:G6};{1:G6};{2:G6}', $p.Scale.X, $p.Scale.Y, $p.Scale.Z)
        $draw = ($p.DrawGroup1, $p.DrawGroup2, $p.DrawGroup3, $p.DrawGroup4 | ForEach-Object { ([uint32]$_).ToString('X8') }) -join ';'
        $disp = ($p.DispGroup1, $p.DispGroup2, $p.DispGroup3, $p.DispGroup4 | ForEach-Object { ([uint32]$_).ToString('X8') }) -join ';'
        [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7}`t{8}`t{9}`t{10}`t{11}" -f $kind, ($p.Name -replace "[`t`r`n]", ' '), $p.ModelName,
            $scale, $draw, $disp, [int]$p.IsShadowOnly, [int]$p.DrawByReflectCam, [int]$p.DrawOnlyReflectCam,
            [int]$p.DisablePointLightEffect, $p.LodParamID, [int]$p.UseDepthBiasFloat))
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"parts $($rows.Count - 1) -> $Out"
