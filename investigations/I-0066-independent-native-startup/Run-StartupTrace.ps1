[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,
    # I-0085: up to two hours for the night endurance test (the native controller's bound; 900 s before).
    [ValidateRange(1,7200)][int]$Seconds=120,[switch]$SteamLaunch,
    [Parameter(Mandatory)][ValidateSet('Reference','Native','Original')][string]$BootstrapMode,
    # 1118: install a test save after the verified backup (restored by hash at the end like every protected file);
    # its SHA-256 must be given and its embedded Steam account must match this PC's save folder (Test-SaveSteamId.ps1).
    [string]$TestSave='',[string]$TestSaveSha256='',
    # Optimisation review (2026-10-18): the folder holding game_bootstrap.dll and game_bootstrap_ctl.exe when it is not the shared
    # game-bootstrap-build (a session building with Build-GameBootstrap -Out keeps its module and flags apart from another session's).
    [string]$BuildFolder='',
    # 1118: native measurement run without the D3D12 validation layer (flag file next to the module, removed after).
    [switch]$NoDebugLayer,
    # D3D12 review R15 (phase A): measurement run, measure.flag next to the module (removed after): no periodic readback,
    # one frame row per pacing window. Witness pilots that read per-frame rows must not use it.
    [switch]$Measure,
    # D3D12 phase D cD6/cD8: two frame slots and the waitable three-buffer chain are the default (2026-10-18); -OneFrameSlot
    # (one-frame-slot.flag) keeps one slot, -NoWaitablePresent (no-waitable-present.flag) the former two-buffer chain.
    [switch]$OneFrameSlot,[switch]$NoWaitablePresent,
    # R13 cR6 (D-0168): the render worker and the worker prepare are the default; -InlineRender (inline-render.flag) runs
    # every frame job inline in Clear(), -InlinePrepare (inline-prepare.flag) prepares on the engine thread.
    [switch]$InlineRender,[switch]$InlinePrepare,
    # Workstream K C-0c: no-kprofile.flag, a measurement run without the collect split (instrumentation overhead).
    [switch]$NoKProfile,
    # C7-2 A/B: no-model-records.flag (every model admitted afresh, as before the per-model collect record).
    [switch]$NoModelRecords,
    # C4-2 A/B: no-persistent-draws.flag (every main-view row prepared afresh, as before the kept draws).
    [switch]$NoPersistentDraws,
    # Phase E E2: graph-shadow.flag (the main view's frames recorded with the frame graph's shadow, graph_barrier_mismatch rows).
    [switch]$GraphShadow,
    # Phase E: gpu-validation.flag (GPU-based validation with the debug layer; timings are not judged with it).
    [switch]$GpuValidation,
    # D-0171: hand-barriers.flag (the hand-written barriers instead of the frame graph, the default since D-0171).
    [switch]$HandBarriers,
    # 1118: snapshot the game process's loaded modules at these seconds after attach (modules.json)
    # (a string such as "60;180": pwsh -File passes a list as one string, and "60,180" would bind as 60180)
    [string]$ModuleSnapshotSeconds='',
    # 1186: GraphicsConfig MotionBlur HIGH for this run (Thomas, 2026-10-11; tokens DISABLE/LOW/MEDIUM/HIGH/MAX, MENUS-MAP 77);
    # the configuration is backed up and restored by hash with every protected file, like the window settings below.
    [switch]$MotionBlur,
    # 1263 (QUALITY-OBJECT-NATIVE QL3): GraphicsConfig QualitySetting for this run (restored by hash afterwards like the rest).
    [ValidateSet("","LOW","MEDIUM","HIGH","MAX")][string]$QualityPreset="",
    # D-0166 (options lane O3): GraphicsConfig keys for this run, "Key=TOKEN;Key=TOKEN" (quality keys DISABLE..MAX, Antialiasing
    # ON/OFF, QualitySetting LOW..MAX or CUSTOM), applied after the verified backup and restored by hash like the rest.
    [string]$GraphicsLevels="",
    # Fable review (the loading screen's edges): the observation window, 1280x720 unless a witness needs the play size (1920x1080,
    # whose window the desktop clamps to a 1061-high client).
    [ValidateRange(640,3840)][int]$WindowWidth=1280,[ValidateRange(360,2160)][int]$WindowHeight=720,
    # I-0084 (lead D1): video-memory-budget-mb.txt next to the module: a ballast leaves that many MB of headroom above the local
    # usage measured when the device opens (about 0.5 GB then).
    [ValidateRange(0,65536)][int]$VideoMemoryBudgetMb=0,
    # Menu tests FL5-FL7 (Thomas, 2026-10-13): a Steam language name placed in test-language.flag (removed after); the module then
    # gives the engine that language's Steam id (NativeTestSteamLanguage); Steam itself is not touched.
    [ValidatePattern('^(|brazilian|danish|english|french|german|italian|japanese|koreana|polish|russian|schinese|spanish|tchinese)$')][string]$TestLanguage="",
    # W3 (GAMMA-COLOUR-PATH-READY 3.6): "no-gamma" places no-gamma.flag (the frame straight into the back buffer, A/B);
    # "brightness=N" (0..10) places brightness-override.flag (development: the gamma curve of brightness N).
    [ValidatePattern('^(|no-gamma|legacy-present|ui-cache|brightness=(10|[0-9]))$')][string]$DisplayFlags="",
    # W4 (FFX-WAVE1-READY section 6): "ffx-observe" (the effect observers) and "no-native-effects" (W4 off, A/B; the effect
    # system is the default since FW-9), one flag file each next to
    # the module, removed after; any left by a killed run removed first).
    [ValidatePattern('^((ffx-observe|no-native-effects|no-yebis|no-yebis-tonemap|no-yebis-glare|no-yebis-shafts|no-yebis-dof|no-yebis-motionblur|no-effect-fog-banks|effect-fog-observe|effect-light-observe|fog-build-observe|shaft-observe|no-effect-light-banks|no-override-fill-lights|no-cutscene-fill-lights|no-yebis-grading|shadow-observe|no-menu-shadow|menu-shadow-blur-toggle|shadow-local|no-local-shadows|no-spot-lights|native-tess|dlss|dlss-dlaa|no-taa|no-taa-reactive|taa-no-jitter|taa-reset-always|taa-reactive-view|no-cloth-velocity|velo-depth-legacy|cloth-velocity-observe|face-unbound-observe|sharpen-50|sharpen-100|rekindled-overlay-test|dlss-quality|dlss-balanced|dlss-performance|dlss-ultra-performance|test-dlss-refuse|world-pictures|pictures-from-cutscene|character-fade-observe|gbuffer-pictures|no-sun|light-observe|pixel-keys|record-revalidation|cascade-observe|caster-place|no-caster-volume|sample-profile|sample-profile-load|no-pipeline-library|no-glow|ffx-draw-observe|ffx-record|ffx-entity-life|no-ffx-draw|reference-no-ffx-draw|reference-no-occlusion|reference-share-variant|hold-world-freeze|aa-observe|no-menu-taa|no-ffx-gpu|no-ffx-effect-copy|no-effect-layer|no-ffx-models|no-ffx-trails|ffx-skip-e03|caption-observe|fade-observe|decal-observe|chr-positions|decal-debug|no-aa|no-water-reflection|all-groups|pose-observe|no-record-chunks|no-prepare-chunks|no-output-follow|display-cycle|borderless|admission-marks|original-probe|original-unlock|no-ui-worker|no-collect-side|no-state-reuse|inline-retire|no-frame-store|serial-effects|update-observe|sound-observe|no-rekindled-graphics|audio-census|no-dred|test-crash|test-device-removed|test-gpu-hang|no-exit-order-fix|frame-dump|sample-ds3-threads|pin-census|crossing-observe|popin-observe|no-drawn-bits|pix-capture|renderdoc-capture|vsync-off|warp|test-no-ai|test-freeze-ai|test-death|test-region-close|no-cull-margin|no-view-weight-fix|lod-launch-scale|no-lod-launch-scale|cadence-trace|display-clock|unlock-20|unlock-30|unlock-120|unlock-240|unlock-unlimited|unlock-observe|locomotion-observe|mechanics-observe|jump-observe|reflex-on|reflex-boost|no-unlock-fix|unlock-fix-below-60|input-observe|no-input-focus-fix|no-pad-focus-fix|rt-no-cloth|no-wall-recovery|camera-fixed-step|no-camera-law-focus|no-camera-law-pitch|no-camera-law-view|no-camera-law-distance|no-camera-law-row|no-camera-law-lock|no-camera-law-speed|no-camera-law-recentre|no-camera-law-wall|no-terrain-fix|no-takeoff-fix|no-target-timer-fix|rt-scene|rt-view|rt-shadow-witness|rt-lights-check|rt-pt|no-rt-reflect|no-rt-cache|rt-gi-no-ambient|rt-gi-no-local|rt-gi-no-sky|rt-gi-mean|rt-gi-no-temporal|rt-gi-no-spatial|rt-restir|no-rt-restir|rt-gi-jacobian-wide|rt-gi-no-glossy|rt-cache-witness|rt-sky-witness|rt-view-last|no-rr|no-sound-stop|no-fdp-stop|no-effect-fix|no-dlss|recipe-verify|zero-census|ffx-series|effect-measure|effect-scan|rt-gpu-measure|rt-pt-no-cache|rt-bounces-1|rt-bounces-2|rt-bounces-3|rt-bounces-4|rt-gi-paths-1|rt-gi-paths-2|rt-gi-paths-3|rt-gi-paths-4|no-vignette|no-native-warm|precompile-screen-hold|precompile-bonfire-textures-test|no-precompile-music|no-black-window|no-precompile-input-block|no-rekindled-overlay|no-rt-probes|rt-probe-observe|no-rt-light-grid|no-rt-roulette|no-rt-ser|rt-local-x1|rt-local-x2|rt-local-x4|rt-emissive-x4|rt-emissive-x16|rt-fail-magenta|effect-scan-draws|effect-census|ffx-series-auto|rt-restir-no-permute|rt-restir-history-1|rt-restir-history-2|rt-restir-history-4|rt-restir-history-16|rt-restir-no-decorrelate|world-burst|world-burst-after-(300|600|900|1200|1800)|fg-on|fg-x3|fg-x4|no-agility|no-gpu-upload|no-rays-aside|no-auto-exposure|rt-auto-exposure-full|rt-local-physical-10|rt-local-physical-50|rt-local-physical-200|hdr-output|hdr-output-force|hdr-paper-80|hdr-cycle|tone-look-agx|tone-look-aces|tone-look-er|tone-look-neutral|tone-look-cycle|tone-look-keys)([;,](ffx-observe|no-native-effects|no-yebis|no-yebis-tonemap|no-yebis-glare|no-yebis-shafts|no-yebis-dof|no-yebis-motionblur|no-effect-fog-banks|effect-fog-observe|effect-light-observe|fog-build-observe|shaft-observe|no-effect-light-banks|no-override-fill-lights|no-cutscene-fill-lights|no-yebis-grading|shadow-observe|no-menu-shadow|menu-shadow-blur-toggle|shadow-local|no-local-shadows|no-spot-lights|native-tess|dlss|dlss-dlaa|no-taa|no-taa-reactive|taa-no-jitter|taa-reset-always|taa-reactive-view|no-cloth-velocity|velo-depth-legacy|cloth-velocity-observe|face-unbound-observe|sharpen-50|sharpen-100|rekindled-overlay-test|dlss-quality|dlss-balanced|dlss-performance|dlss-ultra-performance|test-dlss-refuse|world-pictures|pictures-from-cutscene|character-fade-observe|gbuffer-pictures|no-sun|light-observe|pixel-keys|record-revalidation|cascade-observe|caster-place|no-caster-volume|sample-profile|sample-profile-load|no-pipeline-library|no-glow|ffx-draw-observe|ffx-record|ffx-entity-life|no-ffx-draw|reference-no-ffx-draw|reference-no-occlusion|reference-share-variant|hold-world-freeze|aa-observe|no-menu-taa|no-ffx-gpu|no-ffx-effect-copy|no-effect-layer|no-ffx-models|no-ffx-trails|ffx-skip-e03|caption-observe|fade-observe|decal-observe|chr-positions|decal-debug|no-aa|no-water-reflection|all-groups|pose-observe|no-record-chunks|no-prepare-chunks|no-output-follow|display-cycle|borderless|admission-marks|original-probe|original-unlock|no-ui-worker|no-collect-side|no-state-reuse|inline-retire|no-frame-store|serial-effects|update-observe|sound-observe|no-rekindled-graphics|audio-census|no-dred|test-crash|test-device-removed|test-gpu-hang|no-exit-order-fix|frame-dump|sample-ds3-threads|pin-census|crossing-observe|popin-observe|no-drawn-bits|pix-capture|renderdoc-capture|vsync-off|warp|test-no-ai|test-freeze-ai|test-death|test-region-close|no-cull-margin|no-view-weight-fix|lod-launch-scale|no-lod-launch-scale|cadence-trace|display-clock|unlock-20|unlock-30|unlock-120|unlock-240|unlock-unlimited|unlock-observe|locomotion-observe|mechanics-observe|jump-observe|reflex-on|reflex-boost|no-unlock-fix|unlock-fix-below-60|input-observe|no-input-focus-fix|no-pad-focus-fix|rt-no-cloth|no-wall-recovery|camera-fixed-step|no-camera-law-focus|no-camera-law-pitch|no-camera-law-view|no-camera-law-distance|no-camera-law-row|no-camera-law-lock|no-camera-law-speed|no-camera-law-recentre|no-camera-law-wall|no-terrain-fix|no-takeoff-fix|no-target-timer-fix|rt-scene|rt-view|rt-shadow-witness|rt-lights-check|rt-pt|no-rt-reflect|no-rt-cache|rt-gi-no-ambient|rt-gi-no-local|rt-gi-no-sky|rt-gi-mean|rt-gi-no-temporal|rt-gi-no-spatial|rt-restir|no-rt-restir|rt-gi-jacobian-wide|rt-gi-no-glossy|rt-cache-witness|rt-sky-witness|rt-view-last|no-rr|no-sound-stop|no-fdp-stop|no-effect-fix|no-dlss|recipe-verify|zero-census|ffx-series|effect-measure|effect-scan|rt-gpu-measure|rt-pt-no-cache|rt-bounces-1|rt-bounces-2|rt-bounces-3|rt-bounces-4|rt-gi-paths-1|rt-gi-paths-2|rt-gi-paths-3|rt-gi-paths-4|no-vignette|no-native-warm|precompile-screen-hold|precompile-bonfire-textures-test|no-precompile-music|no-black-window|no-precompile-input-block|no-rekindled-overlay|no-rt-probes|rt-probe-observe|no-rt-light-grid|no-rt-roulette|no-rt-ser|rt-local-x1|rt-local-x2|rt-local-x4|rt-emissive-x4|rt-emissive-x16|rt-fail-magenta|effect-scan-draws|effect-census|ffx-series-auto|rt-restir-no-permute|rt-restir-history-1|rt-restir-history-2|rt-restir-history-4|rt-restir-history-16|rt-restir-no-decorrelate|world-burst|world-burst-after-(300|600|900|1200|1800)|fg-on|fg-x3|fg-x4|no-agility|no-gpu-upload|no-rays-aside|no-auto-exposure|rt-auto-exposure-full|rt-local-physical-10|rt-local-physical-50|rt-local-physical-200|hdr-output|hdr-output-force|hdr-paper-80|hdr-cycle|tone-look-agx|tone-look-aces|tone-look-er|tone-look-neutral|tone-look-cycle|tone-look-keys))*)?$')][string]$EffectFlags="")
$ModuleSnapshots=@($ModuleSnapshotSeconds -split '[;\s]+' | Where-Object { $_ } | ForEach-Object { [int]$_ })
$ErrorActionPreference='Stop'
# D-0166: every -GraphicsLevels entry is checked before anything is backed up or changed.
$levelEdits=@(foreach($entry in @($GraphicsLevels -split '[;\s]+' | Where-Object { $_ })){
    # I-0076: the screen mode and the resolutions too (ScreenMode=FULLSCREEN, Resolution-FullScreenWidth=1920).
    if($entry -cnotmatch '^(QualitySetting|TextureQuality|Antialiasing|SSAO|DepthOfField|MotionBlur|ShadowQuality|LightingQuality|EffectsQuality|ReflectionQuality|WaterSurfaceQuality|ShadeQuality|ScreenMode|Resolution-WindowScreenWidth|Resolution-WindowScreenHeight|Resolution-FullScreenWidth|Resolution-FullScreenHeight)=([A-Z]+|[0-9]{3,5})$'){ throw "GraphicsLevels entry $entry" }
    $key=$Matches[1];$token=$Matches[2]   # case-sensitive: the edit below matches the file's exact spelling
    $allowed=switch -Wildcard ($key){ 'Antialiasing' {@('ON','OFF')} 'QualitySetting' {@('LOW','MEDIUM','HIGH','MAX','CUSTOM')} 'ScreenMode' {@('WINDOW','FULLSCREEN')} 'Resolution-*' {$null} default {@('DISABLE','LOW','MEDIUM','HIGH','MAX')} }
    if($null -eq $allowed){ if($token -notmatch '^[0-9]{3,5}$' -or [int]$token -lt 320 -or [int]$token -gt 16384){ throw "GraphicsLevels $key does not take $token" } }
    elseif($token -cnotin $allowed){ throw "GraphicsLevels $key does not take $token" }
    [pscustomobject]@{key=$key;token=$token} })
if($levelEdits.Count -and ($MotionBlur -or $QualityPreset)){ throw 'GraphicsLevels replaces -MotionBlur and -QualityPreset' }
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$out=Join-Path $root "evidence/I-0066/artifacts/$RunName"
$game='C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game'
$exe=Join-Path $game 'DarkSoulsIII.exe'
# I-0102: every run launches through Steam (the former startup-trace launch and its observer modes are gone).
if(-not $SteamLaunch){ throw 'Run-StartupTrace launches the game through Steam (-SteamLaunch)' }
$trace=$null;$module=$null
if($BootstrapMode -ne 'Original'){
    $build=if($BuildFolder){ (Resolve-Path -LiteralPath $BuildFolder).Path }else{ Join-Path $root 'evidence/I-0066/artifacts/game-bootstrap-build' }   # a full path: the injector gets it from the game's own folder
    $trace=Join-Path $build 'game_bootstrap_ctl.exe'
    $module=Join-Path $build 'game_bootstrap.dll'
    if(-not (Test-Path -LiteralPath $module)){ throw 'Bootstrap requires the built module' }
    # D-0260: the build folder's last use, read by scripts/Clear-DiskCache.ps1 (a folder used within 48 h is never rotated).
    try{ Set-Content -LiteralPath (Join-Path $build 'last-used.txt') -Value (Get-Date -Format o) }catch{ Write-Warning "last-used marker not written: $_" }
}
# I-0092: a run without a module (Original) still clears the default build folder's stale flags (the game loads none of them).
if(-not $module){ $module=Join-Path $root 'evidence/I-0066/artifacts/game-bootstrap-build/game_bootstrap.dll' }
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ throw 'Game already running' }
if(Test-Path -LiteralPath $out){ throw 'Run exists; never overwrite evidence' }
if($NoDebugLayer -and $BootstrapMode -ne 'Native'){ throw 'NoDebugLayer applies to Native runs only' }
if($NoDebugLayer -and $GpuValidation){ throw 'GpuValidation needs the debug layer (I-0082: gpu-validation.flag turns it on)' }
# I-0117 T7: the module never turns the debug layer on with pix-capture.flag; the baseline says so (no run counted as validated).
if(@($EffectFlags -split '[;,]') -contains 'pix-capture'){
    if($GpuValidation){ throw 'GpuValidation and pix-capture exclude each other (I-0117 T7: the module drops the debug layer under PIX)' }
    if($Measure){ throw 'Measure and pix-capture exclude each other (the game freezes at each capture)' }
    if($BootstrapMode -eq 'Native'){ $NoDebugLayer=[switch]$true } }
$saveCheck=$null
if($TestSave){
    if(-not (Test-Path -LiteralPath $TestSave) -or (Get-FileHash -LiteralPath $TestSave).Hash -ne $TestSaveSha256){ throw 'Test save missing or hash mismatch' }
    $saveCheck=pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Test-SaveSteamId.ps1') -Save $TestSave | ConvertFrom-Json
    if(-not $saveCheck.md5_all_ok -or $saveCheck.entries_with_id -lt 1){ throw 'Test save is not bound to this Steam account' }
}
if((Get-PSDrive C).Free -lt 2GB){ throw 'Insufficient disk for preservation' }
if((Get-FileHash -LiteralPath $exe).Hash -ne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'){ throw 'Game identity mismatch' }
if($BootstrapMode -ne 'Original' -and -not (Test-Path -LiteralPath $trace)){ throw 'Build trace first' }
# D-0260: the run save backups beyond the newest 100 move to D: (copied, hash-checked, never deleted), at most 20 a run; a failure
# never stops the run.
try{ pwsh -NoProfile -File (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'scripts/Move-SaveBackups.ps1') -Apply -Quiet -MaxMove 20
     if($LASTEXITCODE){ Write-Warning "Save backup rotation failed (exit $LASTEXITCODE); the run goes on" } }
catch{ Write-Warning "Save backup rotation skipped: $_" }
$backup=Join-Path $env:USERPROFILE ".ds3-i0039-backups/$RunName-$(Get-Date -Format yyyyMMdd-HHmmss)"
New-Item -ItemType Directory -Path $out,$backup | Out-Null
$targets=@(Get-ChildItem -LiteralPath "$env:APPDATA/DarkSoulsIII" -File -Recurse)
$targets+=@(Get-ChildItem -LiteralPath $game -File | Where-Object { $_.Name -in @('ReShade.ini','ReShade.log','ReShadePreset.ini') })
$rows=@(for($i=0;$i -lt $targets.Count;$i++) {
    $file=$targets[$i]; $copy=Join-Path $backup "$i-$($file.Name)"
    Copy-Item -LiteralPath $file.FullName -Destination $copy
    $hash=(Get-FileHash -LiteralPath $file.FullName).Hash
    if((Get-FileHash -LiteralPath $copy).Hash -ne $hash){ throw 'Backup mismatch' }
    [ordered]@{source=$file.FullName;backup=$copy;sha256=$hash}
})
$installed=@(Get-ChildItem -LiteralPath $game -File | Where-Object { $_.Extension -in @('.exe','.dll','.addon64') } | ForEach-Object {
    [ordered]@{path=$_.FullName;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
})
$baseline=[ordered]@{schema=1;run=$RunName;created_utc=[DateTime]::UtcNow.ToString('o');files=$rows;installed=$installed;
    trace_sha256=$(if($BootstrapMode -ne 'Original'){(Get-FileHash -LiteralPath $trace).Hash}else{$null});seconds=$Seconds;reference_d3d11=($BootstrapMode -ne 'Native');
    native_requested=($BootstrapMode -eq 'Native');bootstrap_mode=$BootstrapMode;
    test_save=$(if($TestSave){[ordered]@{path=$TestSave;sha256=$TestSaveSha256;steam_check=$saveCheck}}else{$null});
    debug_layer_disabled=[bool]$NoDebugLayer;measure=[bool]$Measure;frames_in_flight=(-not $OneFrameSlot);waitable_present=(-not $NoWaitablePresent);no_kprofile=[bool]$NoKProfile;no_model_records=[bool]$NoModelRecords;no_persistent_draws=[bool]$NoPersistentDraws;graph_shadow=[bool]$GraphShadow;gpu_validation=[bool]$GpuValidation;hand_barriers=[bool]$HandBarriers;display_flags=$DisplayFlags;effect_flags=$EffectFlags;render_thread=($BootstrapMode -eq 'Native' -and -not $InlineRender);prepare_thread=($BootstrapMode -eq 'Native' -and -not ($InlineRender -or $InlinePrepare));graphics_levels=$GraphicsLevels;test_language=$TestLanguage;module_snapshots=$ModuleSnapshots;motion_blur=[bool]$MotionBlur;quality_preset=$QualityPreset}
if($BootstrapMode -ne 'Original'){ $baseline.module_sha256=(Get-FileHash -LiteralPath $module).Hash }
$baseline | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$out/baseline.json" -Encoding utf8
$oldApp=$env:SteamAppId; $oldGame=$env:SteamGameId
try {
    $env:SteamAppId='374320'; $env:SteamGameId='374320'
    if($TestSave){
        $active=@($rows | Where-Object { $_.source -like '*DS30000.sl2' })
        if($active.Count -ne 1){ throw 'Expected exactly one backed-up active save' }
        Copy-Item -LiteralPath $TestSave -Destination $active[0].source -Force
        if((Get-FileHash -LiteralPath $active[0].source).Hash -ne $TestSaveSha256){ throw 'Test save installation mismatch' }
    }
    # I-0082: the debug layer is opt-in in the module (debug-layer.flag); a native run keeps it unless -NoDebugLayer.
    $flag=if($BootstrapMode -eq 'Native' -and -not $NoDebugLayer){ Join-Path (Split-Path -Parent $module) 'debug-layer.flag' }else{ $null }
    if($flag){ Set-Content -LiteralPath $flag -Value $RunName }
    # A flag left by a killed run would change this run silently: remove any one this run did not ask for.
    foreach($stale in @(@('measure.flag',[bool]$Measure),@('debug-layer.flag',($BootstrapMode -eq 'Native' -and -not $NoDebugLayer)),@('one-frame-slot.flag',[bool]$OneFrameSlot),@('no-waitable-present.flag',[bool]$NoWaitablePresent),@('no-kprofile.flag',[bool]$NoKProfile),@('no-model-records.flag',[bool]$NoModelRecords),@('inline-render.flag',[bool]$InlineRender),@('inline-prepare.flag',[bool]$InlinePrepare),@('no-persistent-draws.flag',[bool]$NoPersistentDraws),@('graph-shadow.flag',[bool]$GraphShadow),@('gpu-validation.flag',[bool]$GpuValidation),@('hand-barriers.flag',[bool]$HandBarriers))){
        $path=Join-Path (Split-Path -Parent $module) $stale[0]
        if(-not $stale[1] -and (Test-Path -LiteralPath $path)){ Remove-Item -LiteralPath $path; Write-Output "removed stale $($stale[0])" } }
    $measureFlag=if($Measure){ Join-Path (Split-Path -Parent $module) 'measure.flag' }else{ $null }
    if($measureFlag){ Set-Content -LiteralPath $measureFlag -Value $RunName }
    $slotsFlag=if($OneFrameSlot){ Join-Path (Split-Path -Parent $module) 'one-frame-slot.flag' }else{ $null }
    if($slotsFlag){ Set-Content -LiteralPath $slotsFlag -Value $RunName }
    $presentFlag=if($NoWaitablePresent){ Join-Path (Split-Path -Parent $module) 'no-waitable-present.flag' }else{ $null }
    if($presentFlag){ Set-Content -LiteralPath $presentFlag -Value $RunName }
    $collectFlags=@(foreach($pair in @(@('no-kprofile.flag',[bool]$NoKProfile),@('no-model-records.flag',[bool]$NoModelRecords),@('inline-render.flag',[bool]$InlineRender),@('inline-prepare.flag',[bool]$InlinePrepare),@('no-persistent-draws.flag',[bool]$NoPersistentDraws),@('graph-shadow.flag',[bool]$GraphShadow),@('gpu-validation.flag',[bool]$GpuValidation),@('hand-barriers.flag',[bool]$HandBarriers))){
        if($pair[1]){ $path=Join-Path (Split-Path -Parent $module) $pair[0]; Set-Content -LiteralPath $path -Value $RunName; $path } })
    # W3: the display flags of this run only (any left by a killed run removed first).
    foreach($name in @('no-gamma.flag','brightness-override.flag','legacy-present.flag','ui-cache.flag')){ $path=Join-Path (Split-Path -Parent $module) $name
        if(Test-Path -LiteralPath $path){ Remove-Item -LiteralPath $path; Write-Output "removed stale $name" } }
    $displayFlagPaths=@()
    if($true){ $path=Join-Path (Split-Path -Parent $module) 'test-language.flag'; if(Test-Path -LiteralPath $path){ Remove-Item -LiteralPath $path; Write-Output 'removed stale test-language.flag' }
      if($TestLanguage){ Set-Content -LiteralPath $path -Value $TestLanguage -NoNewline; $displayFlagPaths+=$path } }
    # Review (menu tests): a leftover witness file would act in the next session of any kind (witness-flag.txt sets event flag 20
    # and runs the ending): removed before the launch, and again after the run (finally).
    foreach($name in 'witness-flag.txt','witness-teleport.txt','witness-phantom.txt'){ $path=Join-Path (Split-Path -Parent $module) $name
        if(Test-Path -LiteralPath $path){ Remove-Item -LiteralPath $path; Write-Output "removed stale $name" } }
    if($DisplayFlags -eq 'no-gamma'){ $path=Join-Path (Split-Path -Parent $module) 'no-gamma.flag'; Set-Content -LiteralPath $path -Value $RunName; $displayFlagPaths+=$path }
    elseif($DisplayFlags -eq 'ui-cache'){ $path=Join-Path (Split-Path -Parent $module) 'ui-cache.flag'; Set-Content -LiteralPath $path -Value $RunName; $displayFlagPaths+=$path }
    elseif($DisplayFlags -eq 'legacy-present'){ $path=Join-Path (Split-Path -Parent $module) 'legacy-present.flag'; Set-Content -LiteralPath $path -Value $RunName; $displayFlagPaths+=$path }
    elseif($DisplayFlags -match '^brightness=(\d+)$'){ $path=Join-Path (Split-Path -Parent $module) 'brightness-override.flag'; Set-Content -LiteralPath $path -Value $Matches[1] -NoNewline; $displayFlagPaths+=$path }
    # I-0088 final review M2: the unlock (a 360 rate request and a tearing chain) and the A/B switches never outlive a killed run.
    foreach($name in @('no-menu-shadow.flag','menu-shadow-blur-toggle.flag','menu-shadow-hard.now','rekindled-door-test.flag','no-rekindled-graphics.flag','no-effect-fog-banks.flag','effect-fog-observe.flag','effect-light-observe.flag','fog-build-observe.flag','shaft-observe.flag','no-effect-light-banks.flag','no-override-fill-lights.flag','no-cutscene-fill-lights.flag','sound-observe.flag','agility.flag','no-agility.flag','no-gpu-upload.flag','no-rays-aside.flag','rt-auto-exposure.flag','rt-auto-exposure-half.flag','no-auto-exposure.flag','post-tone-old.flag','post-tone-verify.flag','post-glare-old.flag','post-glare-verify.flag','hdr-output.flag','hdr-output-force.flag','hdr-paper-80.flag','hdr-cycle.flag','rt-auto-exposure-full.flag','rt-local-physical-10.flag','rt-local-physical-50.flag','rt-local-physical-200.flag','no-rt-local-physical.flag','fg-on.flag','fg-x3.flag','fg-x4.flag','gbuffer-pictures.flag','pix-capture.flag','renderdoc-capture.flag','dlss.flag','dlss-dlaa.flag','no-taa.flag','no-taa-reactive.flag','taa-no-jitter.flag','taa-reset-always.flag','taa-reactive-view.flag','no-cloth-velocity.flag','velo-depth-legacy.flag','cloth-velocity-observe.flag','face-unbound-observe.flag','sharpen-50.flag','sharpen-100.flag','dlss-quality.flag','dlss-balanced.flag','dlss-performance.flag','dlss-ultra-performance.flag','test-dlss-refuse.flag','original-unlock.flag','original-probe.flag','frame-dump.flag','sample-ds3-threads.flag','pin-census.flag','crossing-observe.flag','popin-observe.flag','drawn-bits.flag','no-drawn-bits.flag','no-caster-volume.flag','caster-place.flag','vsync-off.flag','test-no-ai.flag','test-freeze-ai.flag','test-death.flag','test-region-close.flag','no-cull-margin.flag','no-view-weight-fix.flag','lod-launch-scale.flag','no-lod-launch-scale.flag','cadence-trace.flag','display-clock.flag','unlock-20.flag','unlock-30.flag','unlock-120.flag','unlock-240.flag','unlock-unlimited.flag','unlock-observe.flag','locomotion-observe.flag','mechanics-observe.flag','jump-observe.flag','no-takeoff-fix.flag','no-target-timer-fix.flag','rt-scene.flag','rt-view.flag','rt-shadow.flag','rt-shadow-witness.flag','rt-lights-check.flag','rt-pt.flag','no-rt-reflect.flag','no-rt-scene.flag','no-rt-water.flag','no-rt-gi.flag','no-rt-cache.flag','rt-gi-no-ambient.flag','rt-gi-no-local.flag','rt-gi-no-sky.flag','rt-gi-mean.flag','rt-gi-no-temporal.flag','rt-gi-no-spatial.flag','rt-restir.flag','no-rt-restir.flag','rt-gi-jacobian-wide.flag','rt-gi-no-glossy.flag','rt-gpu-measure.flag','rt-pt-no-cache.flag','rt-bounces-1.flag','rt-bounces-2.flag','rt-bounces-3.flag','rt-bounces-4.flag','rt-gi-fill.flag','no-rt-gi-fill.flag','rt-fill-shadow.flag','rt-fill-shadow-all.flag','no-rt-fill-shadow.flag','no-rt-probes.flag','rt-probe-observe.flag','no-rt-light-grid.flag','no-rt-roulette.flag','no-rt-ser.flag','rt-local-x1.flag','rt-local-x2.flag','rt-local-x4.flag','rt-emissive-x4.flag','rt-emissive-x16.flag','rt-fail-magenta.flag','rt-roulette.flag','rt-restir-no-permute.flag','rt-restir-history-1.flag','rt-restir-history-2.flag','rt-restir-history-4.flag','rt-restir-history-16.flag','rt-restir-no-decorrelate.flag','world-burst.flag','world-burst-after-300.flag','world-burst-after-600.flag','world-burst-after-900.flag','world-burst-after-1200.flag','world-burst-after-1800.flag','rt-cache-witness.flag','no-effect-fix.flag','reflex-on.flag','reflex-boost.flag','no-unlock-fix.flag','unlock-fix-below-60.flag','input-observe.flag','no-input-focus-fix.flag','no-pad-focus-fix.flag','rt-no-cloth.flag','no-wall-recovery.flag','camera-fixed-step.flag','no-camera-law-focus.flag','no-camera-law-pitch.flag','no-camera-law-view.flag','no-camera-law-distance.flag','no-camera-law-row.flag','no-camera-law-lock.flag','no-camera-law-speed.flag','no-camera-law-recentre.flag','no-camera-law-wall.flag','no-dlss.flag','recipe-verify.flag','fade-shadow-alt.flag','fade-dither-all.flag','ffx-series.flag','ffx-series-go.txt','effect-scan.flag','effect-scan-draws.flag','effect-census.flag','ffx-series-auto.flag','effect-measure.flag','zero-census.flag','no-ui-worker.flag','no-collect-side.flag','no-state-reuse.flag','inline-retire.flag','no-frame-store.flag','serial-effects.flag','update-observe.flag','audio-census.flag','no-record-chunks.flag','no-probe-layout.flag','all-groups.flag','chr-positions.flag','no-water-reflection.flag','pose-observe.flag','sample-profile-load.flag','fade-geometry-pass.flag','no-native-sh.flag','engine-instance-rows.flag','no-pipeline-library.flag','post-layout-drains.flag','ffx-skip-e03.flag','caption-observe.flag','fade-observe.flag','decal-observe.flag','decal-debug.flag','no-aa.flag','no-ffx-models.flag','no-ffx-trails.flag','no-ffx-effect-copy.flag','ffx-copy-late.flag','ffx-observe.flag','no-native-effects.flag','shadow-observe.flag','shadow-fit.flag','shadow-atlas.flag','ffx-system.flag','ffx-publish.flag','ffx-door.flag','ffx-wind.flag','shadow-local.flag','no-local-shadows.flag','no-spot-lights.flag','native-tess.flag','yebis.flag','yebis-tonemap.flag','yebis-glare.flag','world-pictures.flag','pictures-from-cutscene.flag','character-fade-observe.flag','no-sun.flag','sun-shadow.flag','light-observe.flag','pixel-keys.flag','record-revalidation.flag','no-sun-shadow.flag','cascade-observe.flag','sample-profile.flag','no-glow.flag','ffx-draw-observe.flag','ffx-record.flag','ffx-entity-life.flag','ffx-execute.flag','no-ffx-draw.flag','reference-no-ffx-draw.flag','reference-no-occlusion.flag','reference-share-variant.flag','hold-world-freeze.flag','aa-observe.flag','no-menu-taa.flag','ffx-gpu.flag','ffx-default-null.flag','no-ffx-gpu.flag','ffx-viewport-zero.flag','no-yebis.flag','no-yebis-tonemap.flag','no-yebis-glare.flag','no-yebis-shafts.flag','no-yebis-dof.flag','no-yebis-motionblur.flag','no-yebis-grading.flag','yebis-shafts.flag','yebis-dof.flag','yebis-motionblur.flag','yebis-grading.flag','no-dred.flag','test-crash.flag','test-device-removed.flag','test-gpu-hang.flag','no-exit-order-fix.flag','no-sound-stop.flag','no-fdp-stop.flag','warp.flag','video-memory-budget-mb.txt','no-native-warm.flag','precompile-screen-hold.flag','precompile-bonfire-textures-test.flag','no-precompile-music.flag','no-black-window.flag','no-precompile-input-block.flag','no-rekindled-overlay.flag','rekindled-overlay-test.flag','rt-gi-paths-1.flag','rt-gi-paths-2.flag','rt-gi-paths-3.flag','rt-gi-paths-4.flag','no-vignette.flag','tone-look-er-vivid.flag')){ $path=Join-Path (Split-Path -Parent $module) $name
        if(Test-Path -LiteralPath $path){ Remove-Item -LiteralPath $path; Write-Output "removed stale $name" } }
    if($VideoMemoryBudgetMb){ $path=Join-Path (Split-Path -Parent $module) 'video-memory-budget-mb.txt'; Set-Content -LiteralPath $path -Value $VideoMemoryBudgetMb -NoNewline; $displayFlagPaths+=$path }
    foreach($name in @($EffectFlags -split '[;,]' | Where-Object { $_ })){ $path=Join-Path (Split-Path -Parent $module) "$name.flag"; Set-Content -LiteralPath $path -Value $RunName; $displayFlagPaths+=$path }
    # I-0132 R12c (the step review's M4): the Rekindled page writes rekindled-settings.ini beside the module; a run's changes never reach
    # the next run: the file kept aside here and put back (or removed when there was none) at the end.
    $settingsFile=Join-Path (Split-Path -Parent $module) 'rekindled-settings.ini';$settingsKept=$settingsFile+'.run-kept'
    if(Test-Path -LiteralPath $settingsKept){ Move-Item -LiteralPath $settingsKept -Destination $settingsFile -Force; Write-Output 'restored rekindled-settings.ini kept by an interrupted run' }
    $settingsHad=Test-Path -LiteralPath $settingsFile;if($settingsHad){ Copy-Item -LiteralPath $settingsFile -Destination $settingsKept -Force }
    # Bounded window for observation; byte-identical configuration restored below.
    $config="$env:APPDATA/DarkSoulsIII/GraphicsConfig.xml"
    $xml=[IO.File]::ReadAllText($config,[Text.Encoding]::Unicode)
    $xml=[regex]::Replace($xml,'<ScreenMode>[A-Z]+</ScreenMode>','<ScreenMode>WINDOW</ScreenMode>')
    $xml=[regex]::Replace($xml,'<Resolution-WindowScreenWidth>\d+</Resolution-WindowScreenWidth>',"<Resolution-WindowScreenWidth>$WindowWidth</Resolution-WindowScreenWidth>")
    $xml=[regex]::Replace($xml,'<Resolution-WindowScreenHeight>\d+</Resolution-WindowScreenHeight>',"<Resolution-WindowScreenHeight>$WindowHeight</Resolution-WindowScreenHeight>")
    if($MotionBlur){ $xml=[regex]::Replace($xml,'<MotionBlur>[A-Z]+</MotionBlur>','<MotionBlur>HIGH</MotionBlur>') }
    if($QualityPreset){ $preset=$QualityPreset.ToUpperInvariant(); $xml=[regex]::Replace($xml,"<QualitySetting>[A-Z]+</QualitySetting>","<QualitySetting>$preset</QualitySetting>")
        if($xml -notmatch "<QualitySetting>$preset</QualitySetting>"){ throw "QualitySetting not set to $preset" } }
    foreach($edit in $levelEdits){ $k=$edit.key;$v=$edit.token
        if($xml -notmatch "<$k>[A-Z0-9]+</$k>"){ throw "GraphicsConfig has no $k" }   # I-0076: numbers for the resolutions
        $xml=[regex]::Replace($xml,"<$k>[A-Z0-9]+</$k>","<$k>$v</$k>")
        if($xml -cnotmatch "<$k>$v</$k>"){ throw "GraphicsConfig $k not set to $v" } }
    [IO.File]::WriteAllText($config,$xml,[Text.UnicodeEncoding]::new($false,$false))
    Start-Process 'steam://rungameid/374320' -WindowStyle Hidden
    $began=[DateTime]::UtcNow; $target=$null
    while(-not $target -and ([DateTime]::UtcNow-$began).TotalSeconds -lt 60){
        $target=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1
        if(-not $target){ Start-Sleep -Milliseconds 10 }
    }
    if(-not $target){ throw 'Steam did not create game process' }
    if($BootstrapMode -eq 'Original') {
        # Separate uninstrumented reference; no observer is attached.
        $deadline=[DateTime]::UtcNow.AddSeconds($Seconds)
        while((Get-Process -Id $target.Id -ErrorAction SilentlyContinue) -and [DateTime]::UtcNow -lt $deadline){ Start-Sleep -Milliseconds 100 }
        $boundedStop=[bool](Get-Process -Id $target.Id -ErrorAction SilentlyContinue)
        if($boundedStop){ Stop-Process -Id $target.Id; Wait-Process -Id $target.Id -Timeout 10 -ErrorAction SilentlyContinue }
        [ordered]@{pid=$target.Id;uninstrumented=$true;bounded_stop=$boundedStop} | ConvertTo-Json -Compress | Set-Content -LiteralPath "$out/controller.jsonl" -Encoding utf8
        $global:LASTEXITCODE=0
    }
    else {
        $controllerMode=$BootstrapMode.ToLowerInvariant()
        $snapshotJob=if($ModuleSnapshots.Count){ Start-ThreadJob -ArgumentList $target.Id,$ModuleSnapshots,"$out/modules.json" -ScriptBlock {
            param($id,$at,$path) $began=[DateTime]::UtcNow; $all=@()
            foreach($s in $at){ while(([DateTime]::UtcNow-$began).TotalSeconds -lt $s){ Start-Sleep -Milliseconds 200; if(-not (Get-Process -Id $id -ErrorAction SilentlyContinue)){ break } }
                $p=Get-Process -Id $id -ErrorAction SilentlyContinue; if(-not $p){ break }
                $names=@(try { $p.Modules | ForEach-Object { $_.ModuleName } } catch { 'error: ' + $_.Exception.Message })
                $all+=[ordered]@{seconds=$s;count=$names.Count;graphics=@($names | Where-Object { $_ -match '(?i)^(d3d1[12]|dxgi|d3d12core|d3d12sdklayers|d3d11on12|d3dcompiler|dxgidebug)' });modules=$names} }
            ConvertTo-Json -InputObject @($all) -Depth 4 | Set-Content -LiteralPath $path -Encoding utf8 } }else{ $null }
        & $trace $target.Id $module "$out/startup.jsonl" $controllerMode $Seconds | Tee-Object -FilePath "$out/controller.jsonl"
    }
    if($snapshotJob){ $null=Wait-Job $snapshotJob -Timeout 30; Remove-Job $snapshotJob -Force }
    $code=$LASTEXITCODE
    Set-Content -LiteralPath "$out/controller-exit.txt" -Value $code
} finally {
    $env:SteamAppId=$oldApp; $env:SteamGameId=$oldGame
    if($module){ foreach($name in 'witness-flag.txt','witness-teleport.txt','witness-phantom.txt'){ $path=Join-Path (Split-Path -Parent $module) $name
        if(Test-Path -LiteralPath $path){ Remove-Item -LiteralPath $path } } }
    if($flag -and (Test-Path -LiteralPath $flag)){ Remove-Item -LiteralPath $flag }
    if($measureFlag -and (Test-Path -LiteralPath $measureFlag)){ Remove-Item -LiteralPath $measureFlag }
    if($slotsFlag -and (Test-Path -LiteralPath $slotsFlag)){ Remove-Item -LiteralPath $slotsFlag }
    if($presentFlag -and (Test-Path -LiteralPath $presentFlag)){ Remove-Item -LiteralPath $presentFlag }
    foreach($path in @($collectFlags)+@($displayFlagPaths)){ if($path -and (Test-Path -LiteralPath $path)){ Remove-Item -LiteralPath $path } }
    if($target){
        $until=[DateTime]::UtcNow.AddSeconds(10)
        while((Get-Process -Id $target.Id -ErrorAction SilentlyContinue) -and [DateTime]::UtcNow -lt $until){ Start-Sleep -Milliseconds 50 }
        if(Get-Process -Id $target.Id -ErrorAction SilentlyContinue){
            # The sole test PID created by this announced protocol, never a
            # different relaunch or a user's independently running game.
            Stop-Process -Id $target.Id
            Wait-Process -Id $target.Id -Timeout 10 -ErrorAction SilentlyContinue
        }
    }
    # Restoration never races a surviving game (including a Steam relaunch).
    # Process enumeration can briefly retain an exiting Steam/game process.
    # Wait without touching any other PID; a real survivor still forbids restore.
    # 1160: a native close from the title or Load Game can take several seconds after the controller exits.
    $settle=[DateTime]::UtcNow.AddSeconds(20)
    while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and [DateTime]::UtcNow -lt $settle){ Start-Sleep -Milliseconds 50 }
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){
        throw "Game still running; preserve baseline and restore using $out/baseline.json after shutdown"
    }
    # The settings file kept aside at the start put back once the game is gone and settled (I-0132 R12c; the final review's L6).
    if($settingsKept -and (Test-Path -LiteralPath $settingsKept)){ Move-Item -LiteralPath $settingsKept -Destination $settingsFile -Force }elseif($settingsFile -and -not $settingsHad -and (Test-Path -LiteralPath $settingsFile)){ Remove-Item -LiteralPath $settingsFile }
    $post=@(for($i=0;$i -lt $rows.Count;$i++) {
        $r=$rows[$i]
        if(Test-Path -LiteralPath $r.source){
            $hash=(Get-FileHash -LiteralPath $r.source).Hash
            if($hash -ne $r.sha256){
                $copy=Join-Path $backup "post-$i-$([IO.Path]::GetFileName($r.source))"
                Copy-Item -LiteralPath $r.source -Destination $copy
                [ordered]@{source=$r.source;preserved=$copy;sha256=$hash}
            }
        }
    })
    foreach($r in $rows){
        if((Get-FileHash -LiteralPath $r.backup).Hash -ne $r.sha256){ throw 'Protected backup changed' }
        Copy-Item -LiteralPath $r.backup -Destination $r.source -Force
        if((Get-FileHash -LiteralPath $r.source).Hash -ne $r.sha256){ throw 'Restoration mismatch' }
    }
    $changed=@($installed | Where-Object { -not (Test-Path -LiteralPath $_.path) -or (Get-FileHash -LiteralPath $_.path).Hash -ne $_.sha256 })
    $extra=@(Get-ChildItem -LiteralPath $game -File | Where-Object { $_.Extension -in @('.exe','.dll','.addon64') -and $_.FullName -notin $installed.path })
    [ordered]@{restored_utc=[DateTime]::UtcNow.ToString('o');files=$rows;post_play=$post;installed_changed=$changed;
        installed_extra=@($extra | ForEach-Object FullName);ok=($changed.Count -eq 0 -and $extra.Count -eq 0)} |
        ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$out/restoration.json" -Encoding utf8
    if($changed.Count -or $extra.Count){ throw 'Installed components changed' }
}
if($code){ throw "Startup trace controller failed: $code (retained observation)" }
