[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Repository = (Resolve-Path (Join-Path $Here '..\..\..')).Path
$Base = Join-Path $Repository 'evidence\I-0007\runtime\spatial-exchange-observer'
$MinHook = 'C:\Users\User\Tools\DS3-Decompilation\third-party\minhook-v1.3.4'
$VcVars = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat'
$ExpectedMinHookCommit = 'c3fcafdc10146beb5919319d0683e44e3c30d537'
$ExpectedProbeHash = 'E27813588D304EC6798128A316AAF75B794F9C54FC72AAF12864B38E2BCF2CC2'
$ExpectedInjectorHash = 'F96D65B143D7040FABC6E585856F174AF234598911D4A12D5AAE8B8FF8A0E14D'

foreach ($Path in @($Base, $MinHook)) {
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "Required source directory not found: $Path"
    }
}
if (-not (Test-Path -LiteralPath $VcVars -PathType Leaf)) {
    throw "MSVC environment script not found: $VcVars"
}
if ((& git -C $MinHook rev-parse HEAD).Trim() -ne $ExpectedMinHookCommit) {
    throw 'Unexpected MinHook revision.'
}

$ProbeBase = Join-Path $Base 'pass_c_probe.cpp'
$InjectorBase = Join-Path $Base 'injector.cpp'
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $ProbeBase).Hash -ne $ExpectedProbeHash) {
    throw 'I-0007 probe source hash mismatch.'
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $InjectorBase).Hash -ne $ExpectedInjectorHash) {
    throw 'I-0007 injector source hash mismatch.'
}

function Replace-ExactlyOnce {
    param([string]$Text, [string]$Old, [string]$New, [string]$Label)
    $First = $Text.IndexOf($Old, [StringComparison]::Ordinal)
    if ($First -lt 0 -or $Text.IndexOf($Old, $First + $Old.Length, [StringComparison]::Ordinal) -ge 0) {
        throw "Replacement is not unique: $Label"
    }
    return $Text.Substring(0, $First) + $New + $Text.Substring($First + $Old.Length)
}

$Probe = [IO.File]::ReadAllText($ProbeBase)
$Probe = Replace-ExactlyOnce $Probe '    else return EXCEPTION_CONTINUE_SEARCH;' @'
    else return EXCEPTION_CONTINUE_SEARCH;

    // I-0013: retain only the current PlayerIns-backed proxy driver. This keeps
    // the four-site observation boundary while excluding every enemy owner.
    const uintptr_t candidate_owner = (site == 1 || site == 3)
        ? static_cast<uintptr_t>(context->Rcx)
        : static_cast<uintptr_t>(context->Rdi);
    uintptr_t candidate_player = 0, world = 0, current_player = 0;
    const bool current_player_owner = candidate_owner &&
        SafeRead(candidate_owner + 0x90, &candidate_player) && candidate_player &&
        SafeRead(g_image + kRvaWorldChrManGlobal, &world) && world &&
        SafeRead(world + 0x80, &current_player) && current_player &&
        candidate_player == current_player;
    if (!current_player_owner) {
        context->Dr6 = 0;
        context->EFlags |= 1u << 16;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
'@ 'player-only filter'
$Probe = Replace-ExactlyOnce $Probe 'std::fabs(g_config.target_fps - 165.0f) < 0.001f' 'std::fabs(g_config.target_fps - 240.0f) < 0.001f' '240 configuration gate'
$Probe = Replace-ExactlyOnce $Probe 'g_config.target_fps = target60 ? 60.0f : 165.0f;' 'g_config.target_fps = target60 ? 60.0f : 240.0f;' '240 live target'
$Probe = Replace-ExactlyOnce $Probe '"target=165 success=1"' '"target=240 success=1"' '240 control marker'
$Probe = Replace-ExactlyOnce $Probe 'L"Local\\DS3_I0007_SPATIAL_OBSERVER_TARGET165_%lu"' 'L"Local\\DS3_I0007_SPATIAL_OBSERVER_TARGET240_%lu"' '240 event name'
$Probe = Replace-ExactlyOnce $Probe '    DWORD payload48[12]{};' @'
    DWORD payload48[12]{};
    unsigned int source_transform_valid = 0;
    DWORD source_position[4]{};
    unsigned int source_extra_valid = 0;
    DWORD source_extra = 0;
    unsigned int source_selector_equal = 0;
    unsigned int root_position_valid = 0;
    DWORD root_position[3]{};
'@ 'source/root fields'
$Probe = Replace-ExactlyOnce $Probe '        SafeRead(modules+0x28,&hit.module28_object)&&hit.module28_object;' @'
        SafeRead(modules+0x28,&hit.module28_object)&&hit.module28_object;

    uintptr_t movement_state = 0;
    if (modules && SafeRead(modules + 0x68, &movement_state) && movement_state) {
        hit.source_transform_valid = SafeCopy(
            movement_state + 0x80, hit.source_position,
            sizeof(hit.source_position)) ? 1u : 0u;
        hit.source_extra_valid = SafeRead(
            movement_state + 0x3CC, &hit.source_extra) ? 1u : 0u;
        int selector_a = 0, selector_b = 1;
        const bool selectors_valid =
            SafeRead(movement_state + 0x1F0, &selector_a) &&
            SafeRead(movement_state + 0x3C8, &selector_b);
        hit.source_selector_equal =
            selectors_valid && selector_a == selector_b ? 1u : 0u;
    }
    const PlayerSnapshot root = ReadPlayerSnapshot();
    hit.root_position_valid = root.position_valid ? 1u : 0u;
    if (root.position_valid) {
        std::memcpy(&hit.root_position[0], &root.x, sizeof(float));
        std::memcpy(&hit.root_position[1], &root.z, sizeof(float));
        std::memcpy(&hit.root_position[2], &root.y, sizeof(float));
    }
'@ 'source/root capture'
$Probe = Replace-ExactlyOnce $Probe '"elapsed_b0,elapsed_b1,elapsed_b2,elapsed_b3,selected48_bits,payload48_bits");' '"elapsed_b0,elapsed_b1,elapsed_b2,elapsed_b3,selected48_bits,payload48_bits,source_transform_valid,source_b0,source_b1,source_b2,source_b3,source_extra_valid,source_extra_b,source_selector_equal,root_position_valid,root_x_b,root_z_b,root_y_b");' 'extended CSV header'
$Probe = Replace-ExactlyOnce $Probe '        for(DWORD value:hit.payload48) std::fprintf(g_locomotion_writer_mirror,"%08lX",static_cast<unsigned long>(value));' @'
        for(DWORD value:hit.payload48) std::fprintf(g_locomotion_writer_mirror,"%08lX",static_cast<unsigned long>(value));
        std::fprintf(g_locomotion_writer_mirror,",%u",hit.source_transform_valid);
        WriteBitVector(g_locomotion_writer_mirror,hit.source_position,false);
        std::fprintf(g_locomotion_writer_mirror,",%u,0x%08lX,%u,%u,0x%08lX,0x%08lX,0x%08lX",
            hit.source_extra_valid,static_cast<unsigned long>(hit.source_extra),
            hit.source_selector_equal,hit.root_position_valid,
            static_cast<unsigned long>(hit.root_position[0]),
            static_cast<unsigned long>(hit.root_position[1]),
            static_cast<unsigned long>(hit.root_position[2]));
'@ 'extended CSV rows'

$Injector = [IO.File]::ReadAllText($InjectorBase)
$Injector = Replace-ExactlyOnce $Injector '<60|165>' '<60|240>' 'injector usage'
$Injector = Replace-ExactlyOnce $Injector 'L"165") == 0) return Control(pid, L"TARGET165")' 'L"240") == 0) return Control(pid, L"TARGET240")' 'injector 240 control'

$GeneratedProbe = Join-Path $Base 'i0013_player_proxy_probe.generated.cpp'
$GeneratedInjector = Join-Path $Base 'i0013_player_proxy_injector.generated.cpp'
$Obj = Join-Path $Here 'obj'
$Bin = Join-Path $Here 'bin'
$DllPath = Join-Path $Bin 'I0013PlayerProxyBoundaryObserver.dll'
$InjectorPath = Join-Path $Bin 'I0013PlayerProxyBoundaryObserverInjector.exe'

New-Item -ItemType Directory -Force -Path $Obj, $Bin | Out-Null
try {
    [IO.File]::WriteAllText($GeneratedProbe, $Probe, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText($GeneratedInjector, $Injector, [Text.UTF8Encoding]::new($false))

    $Include = Join-Path $MinHook 'include'
    $Source = Join-Path $MinHook 'src'
    $Hde = Join-Path $Source 'hde'
    $Commands = @(
        "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hook.obj`" `"$Source\hook.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
        "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\buffer.obj`" `"$Source\buffer.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
        "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\trampoline.obj`" `"$Source\trampoline.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
        "cl /nologo /c /O2 /MT /W3 /TC /Fo`"$Obj\hde64.obj`" `"$Hde\hde64.c`" /I`"$Include`" /I`"$Source`" /I`"$Hde`"",
        "cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\probe.obj`" `"$GeneratedProbe`" /I`"$Include`"",
        "link /nologo /DLL /OUT:`"$DllPath`" /PDB:`"$Bin\I0013PlayerProxyBoundaryObserver.pdb`" `"$Obj\probe.obj`" `"$Obj\hook.obj`" `"$Obj\buffer.obj`" `"$Obj\trampoline.obj`" `"$Obj\hde64.obj`" kernel32.lib user32.lib",
        "cl /nologo /O2 /MT /W4 /WX /EHsc /std:c++20 /Fo`"$Obj\injector.obj`" `"$GeneratedInjector`" /Fe:`"$InjectorPath`" /link /PDB:`"$Bin\I0013PlayerProxyBoundaryObserverInjector.pdb`" kernel32.lib"
    )
    foreach ($Command in $Commands) {
        & $env:ComSpec /d /s /c "call `"$VcVars`" >nul && $Command"
        if ($LASTEXITCODE -ne 0) { throw "Build command failed: $LASTEXITCODE" }
    }
}
finally {
    Remove-Item -LiteralPath $GeneratedProbe, $GeneratedInjector -Force -ErrorAction SilentlyContinue
}

$Outputs = Get-FileHash -Algorithm SHA256 -LiteralPath $DllPath, $InjectorPath
$Validation = [ordered]@{
    schema_version = 1
    investigation_id = 'I-0013'
    status = 'pass'
    observation_only = $true
    player_proxy_only = $true
    supported_targets = @(60, 240)
    measure_seconds = 3
    inherited_probe_sha256 = $ExpectedProbeHash
    inherited_injector_sha256 = $ExpectedInjectorHash
    minhook_commit = $ExpectedMinHookCommit
    outputs = @($Outputs | ForEach-Object { [ordered]@{
        file = [IO.Path]::GetFileName($_.Path)
        sha256 = $_.Hash
        size = (Get-Item -LiteralPath $_.Path).Length
    }})
    limit = 'Offline build validation only. No runtime behavior or subsystem ownership is established.'
}
$Validation | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Here 'BUILD-VALIDATION.json') -Encoding utf8
$Validation | ConvertTo-Json -Depth 5
