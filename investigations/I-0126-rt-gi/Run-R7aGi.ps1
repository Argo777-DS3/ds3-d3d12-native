# I-0126 R7a (a copy of I-0125's Run-R6aCheck.ps1, its checkout this script's): the traced GI against the baked ambient at the witness
# zones; each zone's Shebang save, natively, Run-ZoneWalk.ps1 (offline, restored by hash), -Seconds, three looks, the rays and
# world-pictures (the presented frame every 300th frame); for each zone the GI run (`-gi`) then, R7c, the one NRD denoises (`-nrd`,
# no-rr.flag; R7a's `-baked` went with no-rt-gi.flag, D-0251). -Screen a base key (the ASUS by default; the OLED's 2560x1440@280 with -GraphicsLevels, WORKFLOW). One process;
# -NotBefore HH:mm waits for the announced start (same day).
# R7b: two more modes, `seed` (no-rt-cache.flag: the hits' baked ambient, R7a's GI; since I-0132 R11d the hits without any ambient) and `nosky` (rt-gi-no-sky.flag: the GI's misses
# bring nothing, the sky's share of the GI by difference); two interior views, by a teleport from a zone's save: `m37hall` (Anor
# Londo's great staircase hall, I-0113 gap 1's: LIVE-RUN-NOTES) and `m40chair` (inside Firelink Shrine, the Shrine Handmaid's chair).
# I-0128 R8 (R8-PLAN §2): the cost modes (rt-gpu-measure.flag: the native_rt_gpu rows) at the RT level (`rtgpu`) and at the PT level
# with 1 to 4 path vertices (`pt1gpu` to `pt4gpu`), ReSTIR GI's (`ptrgpu`, 2 vertices); the bias check's modes at the PT level with
# 1 to 3 vertices (`pt1mean` to `pt3mean`) and its reference (`ptref`: 8 vertices, no cache); -Measure (Run-ZoneWalk -Measure
# -NoDebugLayer: the launcher's configuration, for the GPU times). I-0128: the view `m40low` (Firelink's unlit lower corridor past the Shrine Handmaid). The original's runs of that comparison are Run-ZoneWalk
# -BootstrapMode Reference on the same save, teleport and looks (I-0128 RESULT « The fill lights »).
# I-0128 R8d: ReSTIR GI's decorrelation modes (`pt7`, `ptp`, `ptp4`, `ptp2`, `ptpd`, `ptp4d`) and `nrdmean` (below).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string[]]$Zones=@('m40','m30','m31','m37'),[string]$NotBefore='',[int]$Seconds=150,[string]$Log='',[string]$Flags='test-no-ai,world-pictures,rt-scene,rt-fail-magenta',
    [string]$Screen='1920x1080@60',[string]$GraphicsLevels='',[string[]]$Modes=@('gi','nrd'),[string[]]$Sizes=@('1280x720'),
    [switch]$Still,   # R7d: no look (the camera still for the bias check's accumulation)
    [switch]$Measure,   # I-0128 R8a: the walk without the debug layer (its GPU times)
    [ValidateRange(0,600)][int]$Stay=0,   # I-0128 R8d: Run-ZoneWalk's -StaySeconds (a still run's frames: -Seconds does not lengthen it)
    [switch]$Video,   # I-0132 R11j: Run-ZoneWalk's -Video (the window filmed into walk.mp4, for scripts/video/videosheet.py)
    [string[]]$ExtraSaves=@())   # I-0132 R11c: more zone saves, each « zone=folder=sha256 » (a folder under .ds3-i0039-backups), e.g. the tour's kept saves
$ErrorActionPreference='Stop'
$Zones=@($Zones | ForEach-Object { $_ -split ',' } | Where-Object { $_ });$Sizes=@($Sizes | ForEach-Object { $_ -split ',' } | Where-Object { $_ })   # R7c: WIDTHxHEIGHT windows
$Modes=@($Modes | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$walk="$repo/investigations/I-0066-independent-native-startup/Run-ZoneWalk.ps1"
$root='C:\Users\User\.ds3-i0039-backups'
$saves=@{m40=@('shebang-zones-eta-m40-20261014-095203','1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4')
    m31=@('shebang-zones-beta-m31-20261013-044305','93D3A76E8C8CC4EA5D6CB4FE76EBFBF23AA30F2A114422EF355968F651CEB0C0')
    m37=@('shebang-zones-gamma-m37-anorlondo-20261026-123818','E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411')
    m45=@('shebang-zones-epsilon-m45-20261013-045053','8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C')
    m50=@('shebang-zones-delta-m50-20261013-044839','CA2DC958DA179DAA5B9A7BFF45C63736B7D1E0CBB6D95035C900B03EA5FFC7DF')
    m30=@('shebang-zones-theta-m30-20261014-095541','397FDB20DC014B708586ECD7C427BB90C15E9F37CA8B09964A4DF0471F667578')}
foreach($x in @($ExtraSaves | ForEach-Object { $_ -split ',' } | Where-Object { $_ })){ $v=$x -split '=';if($v.Count -ne 3){throw "-ExtraSaves $x : zone=folder=sha256"};$saves[$v[0]]=@($v[1],$v[2]) }   # I-0132 R11c
# I-0132 R11c: the runs last 150 s by default (60 s closed a zone's views before their looks: a zone's load is longer than Firelink's).
# R7b's interior views: a zone's save and the teleport (x y z hold, the teleport frame) from its bonfire.
$views=@{m37hall=@('m37','611.5 -129.6 -1042.4 30');m40chair=@('m40','254.8 -54.5 604.7 90')
    m40low=@('m40','243.4 -59.8 589.4 90')   # I-0128: Firelink's unlit lower corridor by the roots, past the Handmaid (found by a piloted walk, i128-fill1-explore)
    m30water=@('m30','-89.76 -48.58 425.36 60')}   # I-0132 R11c: the High Wall's water (m004003 31 m away), from the m30 MSB's player start c0000_0003 (the MSB frame is the teleport frame)
# R7c: `nrd` (no-rr.flag: the GI denoised by NRD); `baked` gone with no-rt-gi.flag (D-0251).
# `seed` (no-rt-cache.flag): since I-0132 R11d the hits without any ambient (no baked seed any more).
$modeFlags=@{gi='';nrd='no-rr';seed='no-rt-cache';nosky='rt-gi-no-sky'
    # R7d (R7-PLAN §8), the bias check (rt-gi-mean.flag, -Still): the PT level's one ray (`ptone`; ReSTIR opt-in from R7d's end to I-0132 R11i, its default since), ReSTIR GI without reuse
    # (`pt0`, the tool's control), with the temporal reuse alone (`ptt`), the spatial alone (`pts`), both (`pt`); the RT level's (`rtmean`).
    # Since I-0128 R8d these ReSTIR modes and `ptrgpu` take rt-restir-no-decorrelate (R7d's and R8a's form: no share); the permutation, R8d's, stays on.
    rtmean='rt-gi-mean';ptone='rt-pt,rt-gi-mean,no-rt-restir';pt0='rt-pt,rt-restir,rt-gi-mean,rt-gi-no-temporal,rt-gi-no-spatial,rt-restir-no-decorrelate';ptt='rt-pt,rt-restir,rt-gi-mean,rt-gi-no-spatial,rt-restir-no-decorrelate'
    pts='rt-pt,rt-restir,rt-gi-mean,rt-gi-no-temporal,rt-restir-no-decorrelate';pt='rt-pt,rt-restir,rt-gi-mean,rt-restir-no-decorrelate';ptwide='rt-pt,rt-restir,rt-gi-mean,rt-gi-jacobian-wide,rt-restir-no-decorrelate'
    # ptwide: the Jacobian's window [0.1, 10]; ptoneng and ptng: the GI hits without their specular
    ptoneng='rt-pt,rt-gi-mean,rt-gi-no-glossy,no-rt-restir';ptng='rt-pt,rt-restir,rt-gi-mean,rt-gi-no-glossy,rt-restir-no-decorrelate'
    # I-0128 R8: the cost and the bias of the PT level's paths.
    rtgpu='rt-gpu-measure';pt1gpu='rt-pt,rt-bounces-1,rt-gpu-measure,no-rt-restir';pt2gpu='rt-pt,rt-bounces-2,rt-gpu-measure,no-rt-restir';pt3gpu='rt-pt,rt-bounces-3,rt-gpu-measure,no-rt-restir'
    pt4gpu='rt-pt,rt-bounces-4,rt-gpu-measure,no-rt-restir';ptrgpu='rt-pt,rt-restir,rt-bounces-2,rt-gpu-measure,rt-restir-no-decorrelate'
    pt1mean='rt-pt,rt-bounces-1,rt-gi-mean,no-rt-restir';pt2mean='rt-pt,rt-bounces-2,rt-gi-mean,no-rt-restir';pt3mean='rt-pt,rt-bounces-3,rt-gi-mean,no-rt-restir';ptref='rt-pt,rt-pt-no-cache,rt-gi-mean,no-rt-restir'
    # I-0129 R9 (R9-PLAN §2): the forward rows and the effects with DS3's baked probes (`noprobe`, no-rt-probes.flag), the traced probes' cost (`probegpu`).
    noprobe='no-rt-probes';probegpu='rt-gpu-measure';probeobs='rt-probe-observe'
    # I-0129 (Thomas, 2026-11-08 08:46 and 08:50): the PT level at its most (`ptmax`, 4 path vertices). I-0132 R11e (D-0262, no
    # fallback): the `raster` mode (no-rt-scene.flag) is removed with the module's raster world; the original game is the comparison.
    ptmax='rt-pt,rt-bounces-4'
    # I-0132 R12g (its final review's M1): the GI quality's check at the PT level with ReSTIR GI (the default, its share on): the GI's mean with
    # one and four GI paths a pixel with their GPU times (`ptg1`, `ptg4`; -Still -Measure), the one ray averaging four (`ptone4`, ReSTIR off:
    # against `ptone`, R8's reference).
    ptg1='rt-pt,rt-gi-mean,rt-gi-paths-1,rt-gpu-measure';ptg4='rt-pt,rt-gi-mean,rt-gi-paths-4,rt-gpu-measure';ptone4='rt-pt,rt-gi-mean,no-rt-restir,rt-gi-paths-4,rt-gpu-measure'
    # I-0132 R11d (D-0262): the fill lights and R9d's fill shadow removed; their modes gone
    # I-0128 R8c (R8-PLAN §6): the same cost and bias modes without the hits' light grid (no-rt-light-grid.flag), the grid's baseline in
    # the same build.
    rtgpunog='rt-gpu-measure,no-rt-light-grid';pt1gpunog='rt-pt,rt-bounces-1,rt-gpu-measure,no-rt-light-grid,no-rt-restir';pt2gpunog='rt-pt,rt-bounces-2,rt-gpu-measure,no-rt-light-grid,no-rt-restir'
    rtmeannog='rt-gi-mean,no-rt-light-grid';pt2meannog='rt-pt,rt-bounces-2,rt-gi-mean,no-rt-light-grid,no-rt-restir'
    # I-0128 R8d (R8-PLAN §2): ReSTIR GI's decorrelation for RR against the one ray (`ptone`), with the bias check and the stability's
    # pictures (-Flags with world-burst): R7d's form (`pt7`: the temporal neighbour not permuted, no share), permuted (`ptp`), the
    # history capped at 4 or 2 (`ptp4`, `ptp2`), a share of the pixels shaded from their initial sample (`ptpd`, the default form since
    # R8d's slot; `ptp4d`); R7d's L12, the seed fix on NRD's frames: the RT level's GI mean on NRD's frames (`nrdmean`) against RR's
    # (`rtmean`). The slot's build-r8d2 took the share with rt-restir-decorrelate.flag; the modes keep their meaning with the later flag.
    pt7='rt-pt,rt-restir,rt-gi-mean,rt-restir-no-permute,rt-restir-no-decorrelate';ptp='rt-pt,rt-restir,rt-gi-mean,rt-restir-no-decorrelate'
    ptp4='rt-pt,rt-restir,rt-gi-mean,rt-restir-history-4,rt-restir-no-decorrelate';ptp2='rt-pt,rt-restir,rt-gi-mean,rt-restir-history-2,rt-restir-no-decorrelate'
    ptpd='rt-pt,rt-restir,rt-gi-mean';ptp4d='rt-pt,rt-restir,rt-gi-mean,rt-restir-history-4';nrdmean='no-rr,rt-gi-mean'
    # I-0131 R10 (R10-PLAN §3, §4): DLSS frame generation (`fg`, fg-on.flag) at the RT level and the PT level at its most (`ptmaxfg`).
    fg='fg-on';ptmaxfg='rt-pt,rt-bounces-4,fg-on';fgx4='fg-x4'
    # I-0132 R11f: the Agility SDK's runtime (agility.flag, then opt-in, its mode `agility`; the default since I-0132 R11i, the mode
    # removed, `noagility` below the inbox runtime), and the cost modes with and without GPU upload heaps (`rtgpunoup`).
    rtgpunoup='rt-gpu-measure,no-gpu-upload'
    # I-0132 R11a (R11-PLAN §2): Russian roulette past the first vertex, the default since R11a's slot (build-r11a1 took it with
    # rt-roulette.flag: its runs `*roul*` had it, the others not); from then on the PT modes above have it and the `*noroul` modes are
    # the paths without it (no-rt-roulette.flag); the ReSTIR modes above (`pt`, `ptt`, `ptpd`, ...) take it too since then, so a rerun is
    # not like R7d's and R8d's runs unless no-rt-roulette is added (the final review's L7). The DLSS Quality modes (dlss-quality.flag; DLAA the default until I-0132 R11j, the TAA since): `rtgpuq`, `pt2gpuq`.
    pt2gpunoroul='rt-pt,rt-bounces-2,rt-gpu-measure,no-rt-roulette,no-rt-restir';pt2meannoroul='rt-pt,rt-bounces-2,rt-gi-mean,no-rt-roulette,no-rt-restir'
    pt4gpunoroul='rt-pt,rt-bounces-4,rt-gpu-measure,no-rt-roulette,no-rt-restir';ptmaxnoroul='rt-pt,rt-bounces-4,no-rt-roulette'
    rtgpuq='rt-gpu-measure,dlss-quality';pt2gpuq='rt-pt,rt-bounces-2,rt-gpu-measure,dlss-quality,no-rt-restir'
    # I-0132 R11d (R11-PLAN §5, D-0262): the real sources' sheets: the PT level as it is (`ptsrc`, rt-pt; the local lights x2 since
    # Thomas's choice, 2026-11-09 05:11: R11d's first sheets' `ptsrc` was x1 and their `ptl2`, rt-local-x2.flag, this default), DS3's
    # own x1 (`ptl1`), x4 (`ptl4`), with the hits' emission x4 (`pte4`) or x4 and x16 (`ptl4e16`); the RT level is `gi` (the `raster` mode removed by R11e).
    ptsrc='rt-pt';ptl1='rt-pt,rt-local-x1';ptl4='rt-pt,rt-local-x4';pte4='rt-pt,rt-emissive-x4';ptl4e16='rt-pt,rt-local-x4,rt-emissive-x16'
    # I-0132 R11g: the rays' walk on the engine thread after the admission as before R11g (`inline`, no-rays-aside.flag), against `gi` (the
    # walk on its own thread beside the main walk, the default); their collect's time in native_collect_split with -Measure; `guard`
    # (record-revalidation.flag): both walks made again serially every 64th walk and compared (native_main_walk_check), the walks overlapping.
    inline='no-rays-aside';guard='record-revalidation'
    # I-0132 R11h (R11-PLAN section 9): the automatic exposure (rt-auto-exposure.flag) at the RT level (`auto`) and the PT level
    # (`ptauto`); `orig`, the original game (Run-ZoneWalk -BootstrapMode Reference, no module flag) on the same save, teleport and looks.
    auto='rt-auto-exposure-full';ptauto='rt-pt,rt-auto-exposure-full';orig='';ptfixed='rt-pt,no-auto-exposure'
    # I-0132 R11h step 2: the PT level with the automatic exposure and the local lights in physical terms at 0.1, 0.02, 0.005 of DS3's against the sun.
    ptphys10='rt-pt,rt-auto-exposure-full,rt-local-physical-10';ptphys50='rt-pt,rt-auto-exposure-full,rt-local-physical-50';ptphys200='rt-pt,rt-auto-exposure-full,rt-local-physical-200'
    # R11h: half the adaptation (rt-auto-exposure-half.flag), with DS3's x2 (`pthalf`) and with the local lights at 0.02 (`pthalf50`).
    pthalf='rt-pt';pthalf50='rt-pt,rt-local-physical-50'
    # Since R11h's default (Thomas, 2026-11-09 23:55): the automatic exposure at half strength in every mode; `ptfixed` the area's fixed
    # exposure (no-auto-exposure.flag); `auto`, `ptauto` and `ptphys*` full strength (rt-auto-exposure-full.flag; slot 1's runs had
    # rt-auto-exposure.flag, its slot 2's `pthalf*` rt-auto-exposure-half.flag: the same strengths); `ptsrc` and `gi` now carry the half.
    # The local lights stay at DS3's x2 by default (D-0264); the physical ratios are test tools (`ptphys*`, `pthalf50`).
    # I-0132 R11i (D-0265): the Agility SDK's runtime and ReSTIR GI at the PT level by default. Every `rt-pt` mode without no-rt-restir
    # now carries ReSTIR GI (R8's one-ray modes, `ptone`, `ptoneng`, `pt*gpu`, `pt*mean`, `ptref`, the `*nog`, the `*noroul` and `pt2gpuq`, took no-rt-restir to keep
    # their meaning; the `ptsrc`, `pthalf*`, `ptauto`, `ptphys*`, `ptfixed`, `ptmax*` views carry the default). `noagility` the inbox runtime (no-agility.flag;
    # R11f's `agility` changes nothing now); the stability pair at the PT level, ReSTIR (`ptr`) against the one ray (`ptnr`), and the PT
    # level asked without RR (`ptnrd`: its frames are the RT level's on NRD, the PT level running only where RR denoises); the cost pair
    # with two vertices (`ptgpu` ReSTIR, `ptgpunr` the one ray), -Measure.
    noagility='no-agility';ptr='rt-pt,rt-gi-mean';ptnr='rt-pt,rt-gi-mean,no-rt-restir';ptnrd='rt-pt,no-rr,rt-gi-mean'
    ptgpu='rt-pt,rt-bounces-2,rt-gpu-measure';ptgpunr='rt-pt,rt-bounces-2,rt-gpu-measure,no-rt-restir'
    # I-0132 R11j (D-0265): no DLSS mode by default, the TAA instead (`gi` and every mode without a DLSS flag: the RT level on NRD); the
    # modes that need DLSS (the PT level, which runs only where RR denoises, and frame generation) take dlss-dlaa below unless they name
    # a DLSS mode. The anti-aliasing views: `taa` (the default), `dlaa` (NVIDIA's DLAA with RR, dlss-dlaa), `dsaa` (DS3's own spatial
    # filter, no-taa), `noaa` (none: no-taa, no-aa). Since I-0138 tranche 5c (2026-11-14) DS3's own filter is removed from the
    # module: `dsaa` gives no anti-aliasing, as `noaa`; the modes stay as the record of this runner's runs.
    taa='';dlaa='dlss-dlaa';dsaa='no-taa';noaa='no-taa,no-aa';dlssperf='dlss-performance'   # `dlssperf`: DLSS Performance with RR (Thomas's own mode)
    taas50='sharpen-50';taas100='sharpen-100'   # the TAA with Thomas's sharpening at 0.5 and 1 (0 the default)
    # I-0132 R11b (R11-PLAN §13): shader execution reordering on the PT level's paths, the default; its pixel-shader pair (no-rt-ser.flag)
    # for the cost (`ptgpu` against `ptgpunoser`, -Measure) and the GI's mean (`ptr` against `ptrnoser`, -Still).
    ptgpunoser='rt-pt,rt-bounces-2,rt-gpu-measure,no-rt-ser';ptrnoser='rt-pt,rt-gi-mean,no-rt-ser';ptsrcnoser='rt-pt,no-rt-ser'
}   # fgx4: x4 asked (capped by the adapter: x2 on an RTX 40)
$say={param($t) if($Log){$t | Add-Content $Log}else{$t}}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    $levels=@{};if($GraphicsLevels){$levels.GraphicsLevels=$GraphicsLevels}   # a named splat (R7c: an array's '-GraphicsLevels' was bound by position to -KeyPath)
    if($Measure){$levels.Measure=$true;$levels.NoDebugLayer=$true}   # I-0128 R8a
    if($Stay){$levels.StaySeconds=$Stay}   # I-0128 R8d
    if($Video){$levels.Video=$true}   # I-0132 R11j
    foreach($zone in $Zones){
        $save=if($views.ContainsKey($zone)){$views[$zone][0]}else{$zone}
        if(-not $saves.ContainsKey($save)){& $say "$zone skipped: no save";continue}
        $teleport=@{};if($views.ContainsKey($zone)){$teleport.Teleport=$views[$zone][1]}   # a named splat (an array's '-Teleport' was bound by position)
        foreach($size in $Sizes){$w,$h=($size -split 'x' | ForEach-Object {[int]$_})
        foreach($mode in $Modes){
            if(-not $modeFlags.ContainsKey($mode)){& $say "$zone $mode skipped: no such mode";continue}
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$zone $mode skipped: DarkSoulsIII already runs";continue}
            $run=if($size -eq '1280x720'){"$Prefix-$zone-$mode"}else{"$Prefix-$zone-$mode-$size"};& $say "$(Get-Date -Format HH:mm:ss) $run"
            $runFlags=if($modeFlags[$mode]){"$Flags,$($modeFlags[$mode])"}else{$Flags}   # not $flags: PowerShell names are case-blind (LIVE-RUN-NOTES)
            $words=@($runFlags -split '[;,]' | Where-Object { $_ })   # I-0132 R11j: DLSS for the PT level and frame generation (above)
            if(($words | Where-Object { $_ -in 'rt-pt','fg-on','fg-x3','fg-x4' }) -and -not ($words | Where-Object { $_ -like 'dlss-*' -or $_ -eq 'no-dlss' })){$runFlags=(@($words)+@('dlss-dlaa')) -join ','}
            $bootstrap=if($mode -eq 'orig'){'Reference'}else{'Native'};$walkFlags=if($mode -eq 'orig'){''}else{$runFlags}   # I-0132 R11h
            $walkLevels=if($mode -eq 'orig'){@{}}else{$levels}
            & $walk -RunName $run -BootstrapMode $bootstrap -TestSave (Join-Path $root "$($saves[$save][0])\DS30000.sl2") -TestSaveSha256 $saves[$save][1] `
                -Seconds $Seconds -SettleSeconds 20 -Look $(if($Still){''}else{'-300,0;-300,0;-300,0'}) -Shots 3 -BuildFolder $BuildFolder -WindowWidth $w -WindowHeight $h `
                -EffectFlags $walkFlags @teleport @walkLevels *>&1 | ForEach-Object { & $say "$_" }
            & $say "$(Get-Date -Format HH:mm:ss) $run done"}}}
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
