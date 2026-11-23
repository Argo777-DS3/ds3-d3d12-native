[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/upscaler-ready/upscaler-goldens.tsv')
)
# I-0068 part BQ: reference tables of the upscaler sheet, computed (nothing is launched):
#  - the Halton (2, 3) jitter sequence for the phase counts of the render scales, in pixels (centred on 0) and in NDC for a render extent,
#    as the projection terms m[2][0], m[2][1] of the engine's row-vector projection (ndc.x = x*sx/z + m[2][0]);
#  - the render extents of the STUB for the usual modes at several output extents (floor, kept even; DLAA = output; with NGX the sizes come from NGX_DLSS_GET_OPTIMAL_SETTINGS), the texture LOD bias log2(render/output) - 1,
#    and the per-axis scale of the half- and quarter-size targets;
#  - the projection words of the engine at zero jitter: m[2][0] and m[2][1] are -0.0 (80000000), not +0.0 (the producer comment, producers.cpp).
$ErrorActionPreference = 'Stop'
$inv = [Globalization.CultureInfo]::InvariantCulture
function Halton([int]$index, [int]$base) { [double]$f = 1; [double]$r = 0; $i = $index; while ($i -gt 0) { $f = $f / $base; $r += $f * ($i % $base); $i = [math]::Floor($i / $base) }; $r }
function F32Hex([double]$v) { '{0:X8}' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$v), 0) }
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("table`tkey`tfield`tvalue`tnote")
$rows.Add("zero-jitter`tm[2][0]`thex`t80000000`tthe engine stores -0.0 for the zero centre offset")
$rows.Add("zero-jitter`tm[2][1]`thex`t80000000`tthe same")
foreach ($scale in @(@('DLAA', 1.0), @('Quality', (2.0 / 3.0)), @('Balanced', 0.58), @('Performance', 0.5), @('UltraPerformance', (1.0 / 3.0)))) {
    $name = $scale[0]; $s = [double]$scale[1]
    $phases = [int][math]::Ceiling(8.0 * [math]::Pow(1.0 / $s, 2))
    $rows.Add("phases`t$name`tcount`t$phases`t8 * (output/render)^2, rounded up")
    foreach ($ext in @(@(1280, 720), @(1920, 1080), @(2560, 1440), @(3840, 2160), @(1681, 945))) {
        $ow = $ext[0]; $oh = $ext[1]
        $rw = [int]([math]::Floor($ow * $s)); $rh = [int]([math]::Floor($oh * $s)); if ($name -eq 'DLAA') { $rw = $ow; $rh = $oh } else { if ($rw % 2) { $rw-- }; if ($rh % 2) { $rh-- } }
        $bias = [math]::Log([double]$rw / $ow, 2) - 1.0
        $rows.Add("extent`t$name $ow x $oh`trender`t$rw x $rh`tbias $($bias.ToString('F4', $inv)); half $([int][math]::Ceiling($rw / 2)) x $([int][math]::Ceiling($rh / 2)); quarter $([int][math]::Ceiling($rw / 4)) x $([int][math]::Ceiling($rh / 4))")
    }
}
foreach ($n in @(8, 16, 18, 32)) {
    for ($i = 1; $i -le $n; $i++) {
        $jx = (Halton $i 2) - 0.5; $jy = (Halton $i 3) - 0.5
        $ndcx = 2.0 * $jx / 1280.0; $ndcy = -2.0 * $jy / 720.0     # +x right, pixel y down -> NDC y up
        $rows.Add("halton`tN=$n i=$i`tpixels`t$($jx.ToString('F6', $inv)),$($jy.ToString('F6', $inv))`tNDC at 1280x720: $(F32Hex $ndcx),$(F32Hex $ndcy)")
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"
