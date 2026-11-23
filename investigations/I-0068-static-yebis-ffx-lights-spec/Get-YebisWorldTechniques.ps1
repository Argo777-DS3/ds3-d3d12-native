[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Techniques = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0067/queries/yebis-techniques.tsv'),
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/yebis-ready/world-techniques.tsv')
)
# I-0068 part BL: the YEBIS techniques the world frames use (YEBIS-TECHNIQUES section 4, YEBIS-HAL-READY section 8) with the
# SHA-256 of their vertex and pixel blobs (pinned in the I-0067 technique table). `observed` marks a name that a witness frame
# draws; the `MadT1_S<n>` blurs and the DoF composite variants are listed as candidates because their exact n / variant is chosen
# by the library at run time (YEBIS-TECHNIQUES section 5).
$ErrorActionPreference = 'Stop'
$observed = @(
    'tech_DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLutT1_S1_RVALUE', 'tech_MadT1_S1', 'tech_MadT1_S2', 'tech_MadT1_S4',
    'tech_DepthOfField_FrontBlurBlendFactor', 'tech_DepthOfField_CompositeBlurLevelsFrontBlur_TS0_3',
    'tech_BrightPassT1_THRESHOLD_LUMINANCE_REMAP_S1', 'tech_BrightPassT1_Vignette_THRESHOLD_LUMINANCE_REMAP_S1',
    'tech_BrightPassT1_VignetteSimulate_THRESHOLD_LUMINANCE_REMAP_S1', 'tech_MadLumGammaT1', 'tech_MadLumGammaT2', 'tech_MadLumGammaT6',
    'tech_MadLumGammaT7', 'tech_LightShaftMask_SM3', 'tech_TexCoordMadT1_S4', 'tech_TexCoordMadT1_S2', 'tech_Alpha_R',
    'tech_TonemapHDR_Exposure_Gamma', 'tech_TonemapHDR_Exposure_Matrix_Gamma', 'tech_TonemapHDR_Dither_Exposure_Gamma',
    'tech_TonemapHDR_Dither_Exposure_Matrix_Gamma', 'tech_TonemapHDR_Vignette_Exposure_Gamma', 'tech_TonemapHDR_Vignette_Exposure_Matrix_Gamma',
    'tech_TonemapHDR_Dither_Vignette_Exposure_Gamma', 'tech_TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma', 'tech_MadColorGradingLutT1_S1')
$candidate = '^tech_(MadT1_S(3|5|7|9|11|13)|DepthOfField_CompositeBlurLevelsFrontBlur_TS[0-4]_[0-7](_USNORM)?|MadOffsetT[0-9]+(_S[0-9]+)?|MadLumGammaT(3|4|5|8|9|10|11|12|16))$'
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("technique`tstatus`tvs`tvs_sha256`tps`tps_sha256")
foreach ($l in [IO.File]::ReadLines($Techniques)) {
    $c = $l -split "`t"; if ($c.Count -lt 8 -or $c[0] -eq 'technique') { continue }
    $status = if ($observed -contains $c[0]) { 'observed' } elseif ($c[0] -match $candidate) { 'candidate' } else { '' }
    if ($status) { $rows.Add("$($c[0])`t$status`t$($c[1])`t$($c[3])`t$($c[4])`t$($c[7])") }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
$missing = $observed | Where-Object { -not ($rows -match ("^" + [regex]::Escape($_) + "`t")) }
Write-Output "ok $($rows.Count - 1) rows -> $Out; missing observed: $($missing -join ', ')"
