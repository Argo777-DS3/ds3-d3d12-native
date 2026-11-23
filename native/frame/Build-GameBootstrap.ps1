[CmdletBinding()]
param([switch]$NoControls,[string[]]$ChangedUnits=@(),
    # Optimisation review (2026-10-18): a session's own output folder (the module, controller, tests and flags), so that two sessions
    # building at once do not overwrite each other's module in the shared game-bootstrap-build; the run scripts take it as -BuildFolder.
    [string]$Out='',
    # Disk guard (Thomas, 2026-11-08, D-0260): a successful build into a session's own -Out folder deletes its compiler objects
    # (.obj, ~470 MB a build, D-0142); -KeepObjects keeps them for a later -ChangedUnits build of the same folder. The launcher's
    # default folder and an incremental build always keep theirs.
    [switch]$KeepObjects,
    # The build study of 2026-11-16: the commands run in parallel by Ninja, -Jobs at once (lower it while another thread's run
    # captures); -Serial runs them one after another as before (build.cmd), the reference the parallel build must equal.
    [ValidateRange(1,64)][int]$Jobs=12,[switch]$Serial,
    # I-0142 step 2 (Thomas, 2026-11-16, D-0295): a thread's object cache. The build runs incrementally in this folder (Ninja reruns
    # only the commands whose sources, included headers or command line changed; its objects are kept, D-0260 relaxed for it), then
    # publishes the module, its tests' results and every other output of this build into -Out, which stays one folder per version.
    [string]$Cache='')
$ErrorActionPreference='Stop'
$runStart=[DateTime]::UtcNow
$cacheLock=$null   # the third review's L3: set here, so the trap below never reads a caller's variable of that name
$generated=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)   # the files Set-GeneratedContent wrote this run
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$launcherFolder=Join-Path $root 'evidence/I-0066/artifacts/game-bootstrap-build'
# Before the next line (PowerShell names are case-insensitive, $out is $Out); an explicit -Out naming the launcher's folder is the launcher's.
# Any spelling of the launcher's folder (through a checkout's junction, the store's own path, relative) is recognised by its tail.
$ownFolder=[bool]$Out -and -not ([IO.Path]::GetFullPath($PSCmdlet.GetUnresolvedProviderPathFromPSPath($Out)).TrimEnd([char[]]'\/') -match '[\\/]I-0066[\\/]artifacts[\\/]game-bootstrap-build$')
$out=if($Out){ $Out }else{ $launcherFolder }
# D-0260: a session's -Out is a new folder or an earlier build's (ds3-build.marker or build.cmd), never a folder of reports or runs,
# which the disk rotation would then take for a build.
if($ownFolder -and (Test-Path -LiteralPath $out) -and @(Get-ChildItem -LiteralPath $out -Force).Count -and
   -not (Test-Path -LiteralPath (Join-Path $out 'ds3-build.marker')) -and
   -not ((Test-Path -LiteralPath (Join-Path $out 'build.cmd')) -and (Select-String -LiteralPath (Join-Path $out 'build.cmd') -Pattern 'game_bootstrap_ctl' -SimpleMatch -Quiet))){
    throw "-Out must be a new folder or an earlier build's: $out holds other files" }
# I-0142 (step review L3): a session's own folder as one absolute spelling (Ninja's header lists name their objects by it).
if($ownFolder){ $out=[IO.Path]::GetFullPath($PSCmdlet.GetUnresolvedProviderPathFromPSPath($out)).TrimEnd([char[]]'\/') }
elseif(-not [IO.Path]::IsPathFullyQualified($out)){ $out=[IO.Path]::GetFullPath($PSCmdlet.GetUnresolvedProviderPathFromPSPath($out)).TrimEnd([char[]]'\/') }   # the final review's L6: the build runs in its folder
function Resolve-RealPath([string]$Path) {   # the final review's L1: a folder's path through every junction or link of its ancestors
    $full=[IO.Path]::GetFullPath($Path).TrimEnd([char[]]'\/'); $parts=$full.Split([char[]]'\/'); $current=$parts[0]+'\'
    for($i=1;$i -lt $parts.Count;$i++){
        $current=Join-Path $current $parts[$i]
        if(Test-Path -LiteralPath $current){ $target=(Get-Item -LiteralPath $current -Force).LinkTarget
            if($target){ $target=$target -replace '^\\\?\?\\',''; if(-not [IO.Path]::IsPathRooted($target)){ $target=Join-Path (Split-Path $current) $target }; $current=Resolve-RealPath $target } } }
    [IO.Path]::GetFullPath($current).TrimEnd([char[]]'\/')
}
# D-0295: a cache (ds3-cache.marker) is never a build's -Out (the re-review's L2, L6).
if((Test-Path -LiteralPath (Join-Path $out 'ds3-cache.marker'))){ throw "-Out is a thread's object cache, never an output folder: $out" }
function Test-SameFolder([string]$A,[string]$B) {   # through any junction, link or drive letter: a probe written in A appears in B
    if(-not (Test-Path -LiteralPath $A) -or -not (Test-Path -LiteralPath $B)){ return $false }
    $probe=".folder-probe-$([guid]::NewGuid().ToString('N'))"; Set-Content -LiteralPath (Join-Path $A $probe) -Value '' -Encoding ascii
    try{ return (Test-Path -LiteralPath (Join-Path $B $probe)) }finally{ Remove-Item -LiteralPath (Join-Path $A $probe) -Force }
}
$publishTo=''
if($Cache){   # D-0295: the build runs in the cache, then publishes into -Out (Publish-CacheBuild)
    if(-not $ownFolder){ throw '-Cache needs an -Out folder of its own (never the launcher''s)' }
    if($Serial -or $ChangedUnits.Count){ throw '-Cache is Ninja''s incremental build: no -Serial, no -ChangedUnits' }
    $Cache=[IO.Path]::GetFullPath($PSCmdlet.GetUnresolvedProviderPathFromPSPath($Cache)).TrimEnd([char[]]'\/')
    if($Cache -match '[\\/]I-0066[\\/]artifacts[\\/]game-bootstrap-build$'){ throw '-Cache is never the launcher''s folder' }   # step review H1
    if((Test-Path -LiteralPath $Cache) -and @(Get-ChildItem -LiteralPath $Cache -Force).Count -and -not (Test-Path -LiteralPath (Join-Path $Cache 'ds3-cache.marker'))){
        throw "-Cache must be a new folder or an earlier cache (ds3-cache.marker): $Cache holds other files" }
    # One folder per version (step review L5): with a cache, -Out is new or empty.
    if((Test-Path -LiteralPath $out) -and @(Get-ChildItem -LiteralPath $out -Force).Count){ throw "-Out must be a new or empty folder with -Cache: $out" }
    # Neither inside the other nor inside the launcher's folder (the third review's L2), by their full paths.
    $launcherReal=Resolve-RealPath $launcherFolder; $cacheReal=Resolve-RealPath $Cache; $outReal=Resolve-RealPath $out
    foreach($pair in @(@($cacheReal,$launcherReal),@($cacheReal,$outReal),@($outReal,$cacheReal))){
        if($pair[0].StartsWith($pair[1]+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){ throw "-Cache must be a folder of its own, outside -Out and the launcher's folder: $($pair[0]) is inside $($pair[1])" } }
    New-Item -ItemType Directory -Force -Path $out,$Cache | Out-Null
    # -Cache neither -Out nor the launcher's folder through any junction (step review L4, the re-review's L8).
    if((Test-SameFolder $out $Cache) -or (Test-SameFolder $Cache $launcherFolder)){ throw '-Cache must be a folder of its own, neither -Out nor the launcher''s' }
    Set-Content -LiteralPath (Join-Path $Cache 'ds3-cache.marker') -Encoding ascii -Value "Build-GameBootstrap.ps1 object cache (D-0295), $(Get-Date -Format o)"
    # One build at a time in a cache (step review L5): its lock, released when the script ends or fails (the re-review's L2).
    try{ $cacheLock=[IO.File]::Open((Join-Path $Cache '.build-lock'),'OpenOrCreate','ReadWrite','None') }catch{ throw "Another build is using the cache $Cache" }
    # -Out claimed at once and atomically (the re-review's L7, the third review's L1): a second build naming it is refused.
    try{ $claim=[IO.File]::Open((Join-Path $out 'ds3-build.marker'),'CreateNew','Write','None'); $claim.Dispose() }catch{ $cacheLock.Dispose(); throw "-Out is claimed by another build: $out" }
    Set-Content -LiteralPath (Join-Path $out 'ds3-build.marker') -Encoding ascii -Value "Build-GameBootstrap.ps1 output, being built in the cache $Cache, $(Get-Date -Format o)"
    $publishTo=$out; $out=$Cache
}
trap { if($cacheLock){ $cacheLock.Dispose() }; break }
# D-0260: no build under 10 GB free on the output's drive (a build needs ~0.7 GB, a live run's preservation 2 GB).
$outDrive=[IO.DriveInfo]::new([IO.Path]::GetPathRoot([IO.Path]::GetFullPath($out)))
if($outDrive.AvailableFreeSpace -lt 10GB){ throw ("Disk nearly full: {0:N1} GB free on {1} (10 GB needed); clean with scripts/Clear-DiskCache.ps1" -f ($outDrive.AvailableFreeSpace/1GB),$outDrive.Name) }
function Write-BuildMarker {   # D-0260: what scripts/Clear-DiskCache.ps1 rotates (with an older build.cmd naming game_bootstrap_ctl, what counts as a build)
    Set-Content -LiteralPath (Join-Path $out 'ds3-build.marker') -Encoding ascii -Value "Build-GameBootstrap.ps1 output, $(Get-Date -Format o)"
}
function Clear-BuildObjects {   # D-0260: the compiler objects of a session's own folder, once its module is linked
    if($KeepObjects -or -not $ownFolder -or $ChangedUnits.Count -or $Cache){ return }   # a cache keeps its objects (D-0295)
    $objs=@(Get-ChildItem -LiteralPath $out -Filter *.obj -File)
    $objs | Remove-Item -Force -ErrorAction SilentlyContinue   # a locked object (antivirus) never fails a passed build
    $left=@($objs | Where-Object { Test-Path -LiteralPath $_.FullName }).Count
    Write-Output ("Compiler objects deleted: {0} files, {1:N0} MB (-KeepObjects keeps them){2}" -f ($objs.Count-$left),(($objs | Measure-Object Length -Sum).Sum/1MB),$(if($left){"; $left locked, left for Clear-DiskCache"}else{''}))
}
function Publish-CacheBuild {   # D-0295: the cache's build into -Out: the graph's outputs (module, PDB, controller, tests, the shaders' headers, imports.txt) and this run's results; no objects
    if(-not $publishTo){ return }
    # Step review L1: a file an earlier build left in the cache (a test binary or result of a build with controls) is never published.
    $built=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach($path in $edgeOutputs){ [void]$built.Add([IO.Path]::GetFullPath($path)) }
    # The re-review's L3: besides the graph's outputs, only this run's results (.txt, .raw: the tests' and NVIDIA's notice); never
    # the build files naming the cache, its stamps, response files or the headers Set-GeneratedContent writes. The module's PDB path
    # and imports.txt still name the cache (the link and dumpbin ran there).
    foreach($file in @(Get-ChildItem -LiteralPath $out -File)){
        if($file.Extension -eq '.obj' -or $file.Name -in $streamlineDlls){ continue }
        if(-not $built.Contains($file.FullName) -and -not ($file.Extension -in '.txt','.raw' -and $file.LastWriteTimeUtc -ge $runStart)){ continue }
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $publishTo $file.Name) -Force }
    # NVIDIA's and Microsoft's DLLs: hard links to the SDKs' files, as in a build of its own (D-0260).
    New-Item -ItemType Directory -Force -Path "$publishTo/D3D12" | Out-Null
    foreach($pair in @(@($streamlineDlls | ForEach-Object { ,@("$streamline/bin/x64/$_","$publishTo/$_") })+@('D3D12Core.dll','d3d12SDKLayers.dll' | ForEach-Object { ,@("$agility/$_","$publishTo/D3D12/$_") }))){
        if((Test-Path -LiteralPath $pair[1]) -and (Get-FileHash -LiteralPath $pair[1]).Hash -eq (Get-FileHash -LiteralPath $pair[0]).Hash){ continue }
        if(Test-Path -LiteralPath $pair[1]){ Remove-Item -LiteralPath $pair[1] -Force }
        try{ New-Item -ItemType HardLink -Path $pair[1] -Target $pair[0] | Out-Null }catch{ Copy-Item -LiteralPath $pair[0] -Destination $pair[1] -Force } }
    Set-Content -LiteralPath (Join-Path $publishTo 'ds3-build.marker') -Encoding ascii -Value "Build-GameBootstrap.ps1 output (published from the cache $out), $(Get-Date -Format o)"
    Write-Output "Published from the cache $out into $publishTo"
    $script:out=$publishTo
    if($cacheLock){ $cacheLock.Dispose() }
}
function Set-GeneratedContent([string]$Path,[object[]]$Value) {   # I-0142: a generated header rewritten only when its bytes change (else every includer recompiles)
    [void]$generated.Add([IO.Path]::GetFullPath($Path)); $staged="$Path.new"
    Set-Content -LiteralPath $staged -Encoding ascii -Value $Value
    if((Test-Path -LiteralPath $Path) -and (Get-FileHash -LiteralPath $Path).Hash -eq (Get-FileHash -LiteralPath $staged).Hash){ Remove-Item -LiteralPath $staged -Force }
    else{ Move-Item -LiteralPath $staged -Destination $Path -Force }
}
function Expand-PinnedArchives([string[]]$Zips,[string]$Destination) {   # I-0142: extracted again only when a pinned archive changed (stable headers' times)
    $stamp=Join-Path $Destination '.archives-sha256'
    $expected=@($Zips | ForEach-Object { "$([IO.Path]::GetFileName($_)) $((Get-FileHash -LiteralPath $_).Hash)" }) -join ';'
    if((Test-Path -LiteralPath $stamp) -and (Get-Content -LiteralPath $stamp -Raw).Trim() -eq $expected){ return }
    if(Test-Path -LiteralPath $Destination){ Remove-Item -LiteralPath $Destination -Recurse -Force }
    foreach($zip in $Zips){ Expand-Archive -LiteralPath $zip -DestinationPath $Destination }
    # Step review M3: extraction keeps the archive's dates, older than the objects; dated now, so their includers rebuild.
    $now=[DateTime]::UtcNow; Get-ChildItem -LiteralPath $Destination -Recurse -File | ForEach-Object { $_.LastWriteTimeUtc=$now }
    Set-Content -LiteralPath $stamp -Encoding ascii -Value $expected   # written last: an interrupted extraction is redone
}
$vcvars='C:/Program Files/Microsoft Visual Studio/18/Insiders/VC/Auxiliary/Build/vcvars64.bat'
$minHook='C:/Users/User/Tools/DS3-Decompilation/third-party/minhook-v1.3.4'
# I-0111 (R1, D-0165): Streamline 2.14.16, its headers for streamline_host.cpp and its signed x64 DLLs copied beside the module (the
# interposer and the DLSS plugin with NVIDIA's model DLL); downloaded with Thomas's yes (2026-10-30), never in the game folder.
$streamline='C:/Users/User/Tools/DS3-Decompilation/third-party/streamline-2.14.16'
$streamlineDlls=@('sl.interposer.dll','sl.common.dll','sl.dlss.dll','sl.dlss_d.dll','sl.reflex.dll','sl.pcl.dll','nvngx_dlss.dll','nvngx_dlssd.dll','NvLowLatencyVk.dll','sl.dlss_g.dll','nvngx_dlssg.dll','sl.nis.dll')   # sl.common imports NvLowLatencyVk; Reflex and PCL: I-0111 tranche 2; Ray Reconstruction: I-0123; frame generation: I-0131 R10b; NIS's sharpening: I-0132 R12f
# I-0126 R7b (D-0252): NVIDIA's SHaRC 1.6.5, its shader headers (include/) read by rt_reflect.hlsl's compile, outside the repository;
# downloaded with Thomas's yes (2026-11-06 17:18). Its headers pinned by hash (the programmes are built from exactly these).
$sharc='C:/Users/User/Tools/DS3-Decompilation/third-party/sharc-1.6.5.0/include'
foreach($pin in @(@('SharcCommon.h','E242E7B2734363658BFFE4035572B11C67E93B6C9360B0993280F03F7E61D71E'),@('HashGridCommon.h','ADCE84CDD622D8E8657FEDF804DF47A2FEC5C9A10EC013C7F29500A80E85C5F6'),@('SharcTypes.h','55EBF4D6046586830C30A1BCDFC267220586A4489C4FA353A52CD23EAA6FE5A8'))){
    if((Get-FileHash -LiteralPath (Join-Path $sharc $pin[0])).Hash -ne $pin[1]){ throw "SHaRC pin mismatch: $($pin[0])" }}
# I-0126 R7c (D-0252): NVIDIA's NRD 4.17.3 (the denoiser of the GI frames without Ray Reconstruction), cloned at its tag, and the
# two source archives its CMake pins (MathLib v11, ShaderMake 18f5a344, MIT), outside the repository; compiled by this build from its
# sources (NRD's custom integration: no CMake, no ShaderMake executable), its archives extracted into the output each build.
$nrdDownload='C:/Users/User/Tools/DS3-Decompilation/third-party/nrd-4.17.3-download'
$nrd="$nrdDownload/NRD"
if((git -C $nrd rev-parse HEAD).Trim() -ne '792eff196afdd350fd9c3f862119017ccb438a0e' -or @(git -C $nrd status --porcelain --ignored).Count){ throw 'NRD pin mismatch' }   # --ignored: a CMake-written NRDConfig.hlsli would win over the build's (the final review's L1)
foreach($pin in @(@('mathlib-v11.zip','4A0772103FD7EF832F8C2FB3EC7FB6BF5B3BEBBD41ECAFC476671D11ECA28A06'),@('shadermake-18f5a344.zip','35DE2547E28CF10F18A0E2782CF154A3D1610212D00D829FA8E73A18210623B6'))){
    if((Get-FileHash -LiteralPath "$nrdDownload/$($pin[0])").Hash -ne $pin[1]){ throw "NRD dependency pin mismatch: $($pin[0])" }}
# I-0126 R7d (D-0252): NVIDIA's RTXDI library (its GI part: ReSTIR GI's reservoirs and resampling), the commit RTXDI v3.1.0 pins
# (NVIDIA-RTX/RTXDI-Library f12037fa), its GitHub archive outside the repository, downloaded with Thomas's yes (2026-11-07 09:59),
# pinned by hash and extracted into the output each build (the programmes are built from exactly its headers).
$rtxdiZip='C:/Users/User/Tools/DS3-Decompilation/third-party/rtxdi-library-f12037fa-download/RTXDI-Library-f12037fa8e97ebc08e9e3edfd2de528ed1772a4b.zip'
if((Get-FileHash -LiteralPath $rtxdiZip).Hash -ne 'F91DE92D7C27F9824915EE5FCF62B0DA664A6EB294EDB659056FF3C462823F75'){ throw 'RTXDI pin mismatch' }
$exe='C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/DarkSoulsIII.exe'
if((Get-FileHash -LiteralPath $exe).Hash -ne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'){ throw 'Original identity mismatch' }
if((git -C $minHook rev-parse HEAD).Trim() -ne 'c3fcafdc10146beb5919319d0683e44e3c30d537' -or @(git -C $minHook status --porcelain).Count){ throw 'MinHook pin mismatch' }
New-Item -ItemType Directory -Force -Path $out | Out-Null
Write-BuildMarker
$pe=[IO.File]::ReadAllBytes($exe)
$nt=[BitConverter]::ToInt32($pe,0x3c); $count=[BitConverter]::ToUInt16($pe,$nt+6)
$sectionTable=$nt+24+[BitConverter]::ToUInt16($pe,$nt+20)
# I-0137 (I-0120 A1): the controller knows the original by its file size and three words of its PE header (original_image.h):
# they are those of the executable pinned by SHA-256 above.
$imagePin=[regex]::Match((Get-Content -LiteralPath "$PSScriptRoot/original_image.h" -Raw),'original_image_pin\{(\d+),0x([0-9A-F]{8}),0x([0-9A-F]{8}),0x([0-9A-F]{8})\}')
if(-not $imagePin.Success -or [uint64]$imagePin.Groups[1].Value -ne $pe.Length -or [Convert]::ToUInt32($imagePin.Groups[2].Value,16) -ne [BitConverter]::ToUInt32($pe,$nt+8) -or
   [Convert]::ToUInt32($imagePin.Groups[3].Value,16) -ne [BitConverter]::ToUInt32($pe,$nt+24+56) -or [Convert]::ToUInt32($imagePin.Groups[4].Value,16) -ne [BitConverter]::ToUInt32($pe,$nt+24+64)){ throw 'Original image pin mismatch (original_image.h)' }
# W3 G0-2 T3 (GAMMA-COLOUR-PATH-READY 5.1): the gamma-pass blobs at their file offsets equal the SHA-256 fixed in
# display_blobs.h (not derived from this executable), with the DXBC header size and the image's size dword agreeing.
$blobHeader=Get-Content -LiteralPath "$PSScriptRoot/display_blobs.h" -Raw
foreach($blob in [regex]::Matches($blobHeader,'DisplayBlobPin display_\w+_blob\{0x([0-9A-F]+),0x([0-9A-F]+),0x([0-9A-F]+),(\d+),"([0-9A-F]{64})"\}')){
    $sizeRva=[Convert]::ToUInt32($blob.Groups[2].Value,16);$fileOffset=[Convert]::ToUInt32($blob.Groups[3].Value,16);$length=[int]$blob.Groups[4].Value
    $bytes=$pe[$fileOffset..($fileOffset+$length-1)];$sizeOffset=$fileOffset+($sizeRva-[Convert]::ToUInt32($blob.Groups[1].Value,16))
    if([Text.Encoding]::ASCII.GetString($bytes,0,4) -ne 'DXBC' -or [BitConverter]::ToUInt32($bytes,24) -ne $length -or [BitConverter]::ToUInt32($pe,$sizeOffset) -ne $length -or
       [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([byte[]]$bytes)) -ne $blob.Groups[5].Value){ throw "Display blob identity mismatch at RVA 0x$($blob.Groups[1].Value)" }
    $blobsChecked++
}
if($blobsChecked -ne 2){ throw 'Display blob pins missing' }
# W4 FW-2b (D-0173): every effect-module read of the GX display object in ffx_display_sites.h equals the file's bytes.
$sitesChecked=0
foreach($site in [regex]::Matches((Get-Content -LiteralPath "$PSScriptRoot/ffx_display_sites.h" -Raw),'\{0x([0-9A-F]+),\{((?:0x[0-9A-F]{2},?){7})\}\}')){
    $rva=[Convert]::ToUInt32($site.Groups[1].Value,16);$expected=@($site.Groups[2].Value -split ',' | ForEach-Object {[Convert]::ToByte($_.Substring(2),16)})
    $siteOffset=$null
    for($s=0;$s -lt $count;$s++){ $o=$sectionTable+40*$s; $va=[BitConverter]::ToUInt32($pe,$o+12); $n=[BitConverter]::ToUInt32($pe,$o+16)
        if($rva -ge $va -and $rva-$va+7 -le $n){ $siteOffset=[BitConverter]::ToUInt32($pe,$o+20)+$rva-$va; break } }
    if($null -eq $siteOffset -or (Compare-Object $expected @($pe[$siteOffset..($siteOffset+6)]) -SyncWindow 0)){ throw ('Effect display site mismatch at RVA 0x{0:X}' -f $rva) }
    $sitesChecked++
}
if($sitesChecked -ne 252){ throw "Effect display sites: $sitesChecked of 252" }
$pins=@('struct Pin { DWORD rva; BYTE bytes[16]; };','constexpr Pin pins[]{')
foreach($rva in @(0x1BD9D90,0x1BDAF50,0x1BDDEE0,0x1F855C0,0x1F85650,0x1CF8560,0x1BDA700,0x1BDA7A0,0x1C66C10,0x1BD8050,0x1BDE4F0,0x1C672D0,0x1BDE4E0,0x1C66250,0x1C66BB0,0xE52EE0,0x1C65B50,0x1C65A80,0x1C68A20,0x1C703E0,0x1BD5FE0,0x1F75730,0x1F7E060,0x1F7D360,0x1F7D350,0xAE5B70,0xAED060,0xE53880,0xE4FB30,0x1A97EE0,0x1A9B750,0xE0DD10,0xE69660,0x2155F90,0x2B7B80,0xE49320,0xE478F0,0x1AFA20,0x600A0,0x1FD3014,0x1FD3280,0xD1760,0xD16E0,0x81B40,0xD24490,0xD24430,0x82CC0,0x140560,0x140420,0x1B0AB0,0xA0430,0xA05C0,0x1934260,0x17A35D0,0x17A3610,0xEDF140,0x450370,0xDEEDD0,0xDE9860,0xEDE120,0x627760,0x814B50,0x8153B0,0x82AF80,0xEE1460,0xEBF740,0x12C73DF,0xEDE0D0,0xD4A120,0xD4C200,0xD4ACA0,0xD4B000,0xD50AB0,0xD505F0,0xD50590,0xEDF460,0xEDF9B0,0xDAE640,0xE506C0,0xE500C0,0xE50820,0xD59D40,0xD59E90,0xD59E20,0x192D650,0xD573B0,0x192C220,0x192CF00,0x1777A00,0xD53EE0,0xF0FA50,0xD5B250,0xD56780,0xD56930,0xEEE130,0xEEE190,0xEEE100,
    0xA60700,0xA5B930,0xA5C010,0xE53580,0xE586E0,0xA608E0,0xEB6CC0,0xEB6C10,0xEBB5B0,0xEB5B40,0x23891E0,0x183C540,0x183BE70,0x23B7960,0x23D5BC0,0x23B7280,0x34EBA0,0x34F7E0,0x23C5070,0x23B6C50,0x192B300,0x1932410,0x34F1B0,0x34F090,0x1AC840,0x2D860,0xB1DC0,0xE6EA10,0xE6EC60,0xE6ED20,0x810E90,0x2594E0,0x2596D0,0x248CC0,0x259C60,0x25B280,0x25B370,0x17A4DE0,0x17AFFC0,0x17B0040,0x213A0,0xD03050,0x64F000,0x619800,0xDFD8D0,0xD043C0,0xD04350,0xC89A0,0xC24E0,0xD514C0,0x2139C00,0x2139C90,0x32A80,0x134A20,0x32770,0x38E70,0x1496D0,0x14A2D0,0x14E900,0x14E5D0,0x15E920,0x23410,0x126EA0,0x127010,0x93CE0,0x17EF0,0xB24E0,0x2F9B0,0x366D0,0x3AE30,0x3F720,0x153FF0,0x12DEE0,0x44930,0x1595C0,0x15CC40,0x161150,0x1658D0,0x169A20,0x16CD30,0x170E70,0x1740B0,0x177950,0x17AAE0,0x17DBE0,0x180BA0,0x183C90,0x1877E0,0x18ACA0,0x18E480,0x133490,0x191470,0x194330,0x936E0,0x1CF8D60,0x1CF8D30,0xE0DAA0,0xE0DB20,0x233BBD0,
    0x7CB820,0x7CB630,0x1A879A0,0x1A879F0,0x249950,
    0xE6DCD0,0xE6DA30,0xE6DBC0,0x7CB8F0,0x7CB750,0xD5B140,0xE6E750,0xD3E950,0x245F90,0x25B870,0x259F60,0x2552C0,0x19254F0,0x1925BF0,0x1925B10,0xD8780,0x19AD90,0x24D000,0xE6E570,0xE6E600,0x1925210,0x19250A0,0x291520,0x239610,0x23C1E0,0x13AA60,0x1D6D20,0x192F970,0x212C40,0x214350,0x212810,0x2136B0,0x242340,0xB3B20,0x10A220,0xEFF10,0x6E780,0x222FF0,0x1D7850,0x1BB810,0x1B26E0,
    0x8C68F0,0x88CFF0,0xE00670,0xE6E620,0x88E300,0x82C160,0xEAA9D0,0x49D00,0x65370,0x65D60,0x652D0,0x65320,0x652A0,0x66640,0x664D0,0x650F0,0x66330,0x699D0,0x66FA0,0x17CC360,0x17CC330,
    0x51320,0x502E0,0x50720,0x505E0,0x51BF0,0x52B60,0x50120,0xC1FB0,0x19B200,0x19BDB0,0x19B690,0x198910,0x51510,0x51860,0x51780,0x1988E0,0x17788B0,
    0x1BDBFC0,0x1BDD010,0x1BDBEC0,0x1BDA8E0,0x1C63970,0x1C64F00,0x1BDB600,0x1BDD980,0x1BDD270,
    0xA0170,0xD57900,0xE006A0,0xC1B20,0xE6ECB0,0x06E9E0,0x06E750,0x06E770,0xCFA750,0xD52AF0,0xD52B70,0x0884F0,0x04B4C0,
    0x234620,0x234720,0x234840,0x2348F0,0x06F310,0xD03430,0xDFD240,0xC8BC0,0xC86B0,0xC2630,0x02D7E0,0xD07E10,
    0x15420,0x15490,0x15B10,0x15FC0,0x16050,0x16000,0xD01610,0xD07710,0xC34420,0xC47CD0,0xC48180,0x8C1BC0,0x888C00,0x65CAC0,0x8CA1E0,0x88E1F0,0x8DD2E0,0x8FA3E0,0xD2D040,
    0xD3F7C0,0xD3F710,0x51B940,0x833DF0,0x833DE0,0x773590,0x7735F0,0x7735C0,
    0x835820,0x8362C0,0x835BD0,0x835D60,0x836720,0x831110,0x836E40,0x140680,
    0xE319B0,0xD1E8A0,0xD4ED20,0xC30D0,0x017E30,0xCED670,0xC3F20,0x2043A0,0x204480,0x2045B0,0x204620,0x204630,0x204650,0x2046A0,0xC2C10,0x04B5C0,0x20A000,0x20A200,0x20A360,0x207250,0x207470,0x207550,0xE2A0,0xE350,0xE400,0xD8200,0x17F2750,0x1799370,
    0x6D700,0x6D8F0,0x6DAE0,0x6DCF0,0x6DFD0,0x6E230,0x6D680,0x6D870,0x6DA60,0x6DC50,0x6DF50,0x6E180,0xD6EB00,
    0x2AD40,0x2AF40,0x2AA80,0x2AAD0,0x2AB40,0x2ABA0,0x2ABE0,0x2B260,0xAC7F0,0x147090,0x147110,0xAC110,0xAC020,0xAC830,0xABD90,0xABDD0,0xABE10,0xABE60,0xABED0,
    0x506320,0x5052F0,0x503E60,0xD27D30,0x6120F0,0x611170,0x612510,0x60F3D0,0x1B520,0x4FBFD0,0x4F7560,0x4F1730,0x234A480,0x2388090,
    0xA9370,0xA9230,0xADFE0,0xACC20,0x1ACED0,0x1ACF10,0x1ACFC0,0xA97F0,0xA98C0,0x1AD070,0x179F4C0,0x179F6C0,0x17A2CD0,0x17A2F10,0x17A2BF0,
    0xD43BB0,0xD439A0,0xD42D50,0xD40790,0xD40540,0x19343C0,0x1934C80,0x19343D0,0xD4F250,0xD4E100,0xD4F690,0xD4EE10,0xD4F4B0,0xD4F7A0,
    0x19349A0,0x19348E0,0x1935550,0x193DA10,0x193DB40,0x1935010,0x1934E50,0x1934D60,
    0xD272C0,0xD26800,0xD27970,0xD27790,0xD27BA0,0xEAA9D0,0x1F741F0,0x1F7E360,0x1CF8750,0xD275F0,0xD27670,0xD26A60,0xD26A70,0xCE6350,0xCDEE50,0xCE1930,0xCE1DB0,0x4AD2D0,0xE72D50,0xE4070,0xE42A0,0xE4460,0xE4700,0xE4890,0xE4B50,
    0x1CF2170,0x1CF1FC0,0x1CF1DB0,0x1CF1EA0,0x1BCE9C0,0x1BD1310,0x1F85430,0x1BD0140,0x1BB6C10,0x1BB6D00,0x1F84DA0,0x1F83A50,0x1C76A10,
    0x1BCDED0,0x1F82940,0x1BD20F0,0x1F851D0,0x1BD0990,0x1BD0A30,0x1F83B10,0x1BB4880,0x1BB4FA0,0x1BCB340,0x1BCB370,
    0x1BD08C0,0x1BCE850,0x1F83990,0x1F83980,0x1F839A0,0x1F83A30,0x1F83AC0,0x1BCF5B0,0x1BCE930,0x1BD1C50,0x1BD1C00,0x1C6EFA0,0x1CEEC30,0x1CEF1A0,
    0x1BD0D10,0x1BCE680,0x1BD0EF0,0x1CEF1D0,0xE48260,0x1F81A90,0x1F78390,0x1F78AD0,0x1C23FF0,0x1F784C0,0x1BDB130,0x1BDB080,0x1BDDCC0,0x1BDAF90,0x1F769E0,0x1C642A0,0x1C605A0,0x1C23EB0,0x1F76FF0,
    0x1F7C8E0,0x1CF7440,0x1F7CB20,0x1F7CA70,0x1CF7510,0x1CF9020,0x1CF76B0,0x1CF7760,0x1CF71F0,0x1CF72E0,0x1CF9290,0x1BB6CA0,
    0x1C3AB10,0x1CFB9C0,0x1CFB810,0xE35EF0,0x1C3BA40,0x1BDAE90,0x1C414C0,0x1C3D720,0x1BDB220,0x1BDB340,0x1C40CD0,
    0x1BD8E70,0x1C79350,0x1C69510,0x1C69470,0x1C90860,0x1C8AC50,0x1BD5F10,
    0x1C622B0,0x1C62330,0x1C628C0,0x1CF8650,0x1CF6A50,0x1C689F0,0x1C68B50,0x1C68470,0x1C67C30,0x1C3A200,0x1C3C6D0,
    0x1C6DE10,0x1C702E0,0x1C6E410,0x1BD97A0,0x1C63010,0x1C6F0A0,0x1C6FC20,0x1C71EE0,
    0x102380,0x13BFC0,0x13D810,0xD079C0,0x57A20,0x5A120,0x219B0,
    0xEAF7E0,0xEAFAC0,0xEADE50,0x1ADF230,0x1ADE850,0x1ADEAF0,0x1AE0830,0x1AE0BA0,0x1ADFF10,0x1ADE670,0x1ADE5B0,0x1777A00,0x1ADE6E0,0x62000,0xEAFD80,
    0x17E8070,0xEB0890,0x1777DE0,0xEEE240,0x17930E0,0x1793820,0x177A330,0x23C0,0x1C040,0x63170,0x17931A0,0x1AE0570,0x1AE0660,0x1AE0800,0xB4E0,
    0xC83D0,0x1778090,0x1FD3430,0x1C66270,0x1F75440,0x1F75180,0x1F770A0,0x1F81EB0,0x1D12410,0x1C65EF0,0x1BB6AF0,0x1CC2AB0,0x1CC2AA0,0x1C66B90,0x1C663C0,0x1C66BE0,0x19853A0,0x1A2DE20)){
    $offset=$null
    for($s=0;$s -lt $count;$s++){
        $o=$sectionTable+40*$s; $va=[BitConverter]::ToUInt32($pe,$o+12); $n=[BitConverter]::ToUInt32($pe,$o+16)
        if($rva -ge $va -and $rva-$va+16 -le $n){ $offset=[BitConverter]::ToUInt32($pe,$o+20)+$rva-$va; break }
    }
    if($null -eq $offset){ throw 'Pin is not file-backed' }
    $prefix=($pe[$offset..($offset+15)] | ForEach-Object { '0x{0:X2}' -f $_ }) -join ','
    $pins+=('    {{0x{0:X}, {{{1}}}}},' -f $rva,$prefix)
}
$pins+='};'
$sfxSource=Get-Content -LiteralPath "$root/evidence/I-0066/artifacts/i66-original-sfx-104/loaded-sfx-after.json" -Raw | ConvertFrom-Json
$sfxRecords=@($sfxSource.records | Where-Object rva -eq 0xE6E620)
if($sfxSource.game_sha256 -ne (Get-FileHash -LiteralPath $exe).Hash -or $sfxRecords.Count -ne 1){throw 'Loaded SFX CPU source unavailable'}
$sfxRecord=$sfxRecords[0];$sfxCode=[Convert]::FromHexString($sfxRecord.hex)
if($sfxRecord.bytes -ne 64 -or $sfxCode.Length -ne 64 -or
    [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($sfxCode)) -ne $sfxRecord.sha256 -or $sfxCode[52] -ne 0xC3){throw 'Loaded SFX CPU source identity mismatch'}
# Entire leaf body: only two qword reads of46402E0 and four dword writes at C0..CC.
$pins+=('constexpr BYTE loaded_sfx_scene_reset_code[]{'+(($sfxCode[0..52] | ForEach-Object {'0x{0:X2}' -f $_}) -join ',')+'};')
$sceneSourceFolder="$root/evidence/I-0066/artifacts/i66-original-85-scene-update-source"
$sceneUpdateRecords=@(
    (Get-Content -LiteralPath "$sceneSourceFolder/loaded-scene-update.json" -Raw | ConvertFrom-Json).records
    (Get-Content -LiteralPath "$sceneSourceFolder/loaded-scene-layer.json" -Raw | ConvertFrom-Json).records
    (Get-Content -LiteralPath "$sceneSourceFolder/loaded-scene-layer-tail.json" -Raw | ConvertFrom-Json).records
)
$pins+='constexpr Pin loaded_scene_update_pins[]{'
foreach($sceneRva in @(0x7D9C50,0xD5E550,0xD5D7E0)){
    $sceneRecords=@($sceneUpdateRecords | Where-Object rva -eq $sceneRva)
    if($sceneRecords.Count -ne 1){throw 'Loaded scene CPU source unavailable'}
    $sceneRecord=$sceneRecords[0];$sceneCode=[Convert]::FromHexString($sceneRecord.hex)
    if($sceneRecord.bytes -ne 512 -or $sceneCode.Length -ne 512 -or
        [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($sceneCode)) -ne $sceneRecord.sha256){throw 'Loaded scene CPU source identity mismatch'}
    $scenePrefix=($sceneCode[0..15] | ForEach-Object {'0x{0:X2}' -f $_}) -join ','
    $pins+=('    {{0x{0:X}, {{{1}}}}},' -f $sceneRva,$scenePrefix)
}
$pins+='};'
$protectedRequest=Get-Content -LiteralPath "$root/evidence/I-0066/artifacts/i66-reference-startup-04/startup.jsonl" | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object event -eq 'reference_cpu_request_code'
$protectedConstructor=(Get-Content -LiteralPath "$root/evidence/I-0066/artifacts/i66-reference-startup-06/loaded-request-constructor.json" -Raw | ConvertFrom-Json).records
if(@($protectedRequest).Count -ne 1 -or @($protectedConstructor).Count -ne 1 -or $protectedRequest.rva -ne 0x506450 -or $protectedConstructor.rva -ne 0x4FB2B0){throw 'Protected CPU source observation unavailable'}
$pins+='constexpr Pin loaded_request_pins[]{'
foreach($record in @($protectedRequest,$protectedConstructor)){
    if($record.bytes -ne 512 -or $record.hex.Length -ne 1024){throw 'Protected CPU source record incomplete'}
    $code=[Convert]::FromHexString($record.hex)
    if($record.sha256 -and [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($code)) -ne $record.sha256){throw 'Protected CPU source hash mismatch'}
    $prefix=($code[0..15] | ForEach-Object { '0x{0:X2}' -f $_ }) -join ','
    $pins+=('    {{0x{0:X}, {{{1}}}}},' -f $record.rva,$prefix)
}
$pins+='};'
$materialRecords=@(
    (Get-Content -LiteralPath "$root/evidence/I-0066/artifacts/original-07/material-loaded.json" -Raw | ConvertFrom-Json).records
    (Get-Content -LiteralPath "$root/evidence/I-0066/artifacts/original-08/material-subscription-loaded.json" -Raw | ConvertFrom-Json).records
)
$pins+='constexpr Pin loaded_material_pins[]{'
foreach($rva in @(0xD42F60,0xD42FD0,0xD42EB0,0xD401C0)){
    $records=@($materialRecords | Where-Object rva -eq $rva)
    if($records.Count -ne 1){throw 'Protected material source record unavailable'}
    $record=$records[0];$code=[Convert]::FromHexString($record.hex)
    if($record.bytes -ne 512 -or $code.Length -ne 512 -or
        [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($code)) -ne $record.sha256){throw 'Protected material source hash mismatch'}
    $prefix=($code[0..15] | ForEach-Object { '0x{0:X2}' -f $_ }) -join ','
    $pins+=('    {{0x{0:X}, {{{1}}}}},' -f $rva,$prefix)
}
$pins+='};'
Set-GeneratedContent "$out/game_bootstrap_pins.inc" $pins
$commands=@('@echo off',"call `"$vcvars`" >nul || exit /b 1")
foreach($unit in @('hook','buffer','trampoline')){
    $commands+="cl /nologo /c /O2 /MT /W3 /TC /Brepro /Fo`"$out/mh_$unit.obj`" `"$minHook/src/$unit.c`" /I`"$minHook/include`" /I`"$minHook/src`" /I`"$minHook/src/hde`" || exit /b 1"
}
$commands+="cl /nologo /c /O2 /MT /W3 /TC /Brepro /Fo`"$out/mh_hde64.obj`" `"$minHook/src/hde/hde64.c`" || exit /b 1"
$cxx='cl /nologo /c /O2 /MT /W4 /WX /EHsc /std:c++20 /Brepro /Gy /Z7 /bigobj'   # /Z7 (with /bigobj for its sections): debug information for the module's PDB (sample-profile.flag symbolization)
# I-0109 (R0): the ray-tracing path's DXIL programmes, compiled by the Windows SDK's dxc (on PATH after vcvars) into headers.
$commands+="dxc --version || exit /b 1"   # final review L10: the compiler's version in the build log
# I-0126 R7c: NRD. Its configuration header (what its CMake writes into its source folder, here into the output), its library's C++
# (static, no embedded shaders: each pipeline names its programme by `shaderIdentifier`) and the REBLUR_DIFFUSE_SPECULAR programmes (I-0132 R11) compiled
# by dxc as its Shaders.cfg asks (cs 6.0, NRD_INTERNAL, all resources bound, MathLib's ml.hlsli), mapped by identifier in
# nrd_shaders.h (rt_denoise.cpp). Its MathLib and ShaderMake headers from the pinned archives.
$nrdOut="$out/nrd";$nrdDeps="$out/nrd-deps"
New-Item -ItemType Directory -Force -Path $nrdOut | Out-Null
Expand-PinnedArchives @("$nrdDownload/mathlib-v11.zip","$nrdDownload/shadermake-18f5a344.zip") $nrdDeps
# I-0126 R7d: RTXDI's headers from its pinned archive (rt_reflect.hlsl's ReSTIR GI).
$rtxdiOut="$out/rtxdi";Expand-PinnedArchives @($rtxdiZip) $rtxdiOut
$rtxdi="$rtxdiOut/RTXDI-Library-f12037fa8e97ebc08e9e3edfd2de528ed1772a4b/Include"
$mathLib="$nrdDeps/MathLib-11";$shaderMake="$nrdDeps/ShaderMake-18f5a344e7ca8fa65daaf079d07bc8ce38453e05"
$nrdOptions=[ordered]@{NRD_NORMAL_ENCODING=2;NRD_ROUGHNESS_ENCODING=1;NRD_SUPPORTS_VIEWPORT_OFFSET=0;NRD_SUPPORTS_CHECKERBOARD=1;NRD_SUPPORTS_HISTORY_CONFIDENCE=1;
    NRD_SUPPORTS_DISOCCLUSION_THRESHOLD_MIX=1;NRD_SUPPORTS_ANTIFIREFLY=1;REBLUR_PERFORMANCE_MODE=0}
Set-GeneratedContent "$nrdOut/NRDConfig.hlsli" (@('// Written by Build-GameBootstrap.ps1 (I-0126 R7c): NRD''s CMake options.')+@($nrdOptions.Keys | ForEach-Object {"#define $_ $($nrdOptions[$_])"}))
$nrdDefines=(@('WIN32_LEAN_AND_MEAN','NOMINMAX','_CRT_SECURE_NO_WARNINGS','NRD_EMBEDS_SPIRV_SHADERS=0','NRD_EMBEDS_DXIL_SHADERS=0','NRD_EMBEDS_DXBC_SHADERS=0','SPIRV_SREG_OFFSET=0',
    'SPIRV_BREG_OFFSET=2','SPIRV_UREG_OFFSET=3','SPIRV_TREG_OFFSET=20','NRD_STATIC_LIBRARY=1')+@($nrdOptions.Keys | Where-Object {$_ -like '*SUPPORTS*' -or $_ -like 'REBLUR*'} | ForEach-Object {"$_=$($nrdOptions[$_])"}) |
    ForEach-Object {"/D$_"}) -join ' '
$nrdUnits=@('InstanceImpl','Reblur','Reference','Relax','Sigma','Timer','Wrapper')
foreach($unit in $nrdUnits){
    $commands+="cl /nologo /c /O2 /MT /W3 /EHsc /std:c++17 /Brepro /Gy /wd4324 $nrdDefines /I`"$nrd/Include`" /I`"$nrdOut`" /I`"$mathLib`" /I`"$shaderMake`" /Fo`"$out/nrd_$($unit.ToLowerInvariant()).obj`" `"$nrd/Source/$unit.cpp`" || exit /b 1"}
$nrdPermutations=@(@('REBLUR_ClassifyTiles',@()),@('REBLUR_HitDistReconstruction',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE','MODE_5X5=0')),@('REBLUR_HitDistReconstruction',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE','MODE_5X5=1')),
    @('REBLUR_PrePass',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE')),@('REBLUR_TemporalAccumulation',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE')),@('REBLUR_HistoryFix',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE')),
    @('REBLUR_Blur',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE')),@('REBLUR_PostBlur',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE','TEMPORAL_STABILIZATION=0')),
    @('REBLUR_PostBlur',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE','TEMPORAL_STABILIZATION=1')),@('REBLUR_TemporalStabilization',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE')),
    @('REBLUR_SplitScreen',@('NRD_SIGNAL=BOTH','NRD_MODE=RADIANCE')),@('REBLUR_Validation',@()),@('Clear',@('FLOAT=0')),@('Clear',@('FLOAT=1')))
$nrdTable=@('// Written by Build-GameBootstrap.ps1 (I-0126 R7c): NRD''s programmes by their shaderIdentifier (NRD 4.17.3, compiled by dxc).','#pragma once')
$nrdRows=@();$index=0
foreach($permutation in $nrdPermutations){
    $name="nrd_$index";$index++
    $defines=($permutation[1] | ForEach-Object {"-D $_"}) -join ' '
    $commands+="dxc -nologo -T cs_6_0 -E main -O3 -D NRD_INTERNAL $defines -all_resources_bound -I `"$nrd/Shaders`" -I `"$nrdOut`" -I `"$mathLib`" -Qstrip_debug -Qstrip_reflect -Fh `"$nrdOut/$name.dxil.h`" -Vn $($name)_dxil `"$nrd/Shaders/$($permutation[0]).cs.hlsl`" >nul || exit /b 1"
    $nrdTable+="#include `"$name.dxil.h`""
    $nrdRows+="    {`"$((@("$($permutation[0]).cs.hlsl")+$permutation[1]) -join '|')`",$($name)_dxil,sizeof($($name)_dxil)},"}
$nrdTable+='struct NrdProgramme {const char* identifier;const unsigned char* bytes;size_t size;};'
$nrdTable+='inline constexpr NrdProgramme nrd_programmes[]{'
$nrdTable+=$nrdRows
$nrdTable+='};'
Set-GeneratedContent "$nrdOut/nrd_shaders.h" $nrdTable
# D-0252 (the final review's L7): NVIDIA's notice beside the module (SHaRC's headers, NRD and, I-0126 R7d, RTXDI's GI reservoirs in it).
Set-Content -LiteralPath "$out/NVIDIA-NOTICE.txt" -Encoding ascii -Value 'This software contains source code provided by NVIDIA Corporation (NRD 4.17.3, SHaRC 1.6.5, RTXDI f12037fa; NVIDIA RTX SDKs LICENSE).'
$commands+="$cxx /Fo`"$out/rt_denoise.obj`" `"$root/native/renderer/rt_denoise.cpp`" /DNRD_STATIC_LIBRARY=1 /I`"$nrd/Include`" /I`"$nrdOut`" || exit /b 1"   # I-0126 R7c
$commands+="dxc -nologo -T cs_6_6 -E main -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/rt_selftest.dxil.h`" -Vn rt_selftest_dxil `"$root/native/renderer/shaders/rt_selftest.hlsl`" >nul || exit /b 1"
$commands+="$cxx /Fo`"$out/raytracing_base.obj`" `"$root/native/renderer/raytracing_base.cpp`" /I`"$out`" || exit /b 1"
$commands+="dxc -nologo -T cs_6_6 -E main -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/rt_probe.dxil.h`" -Vn rt_probe_dxil `"$root/native/renderer/shaders/rt_probe.hlsl`" >nul || exit /b 1"   # I-0117 (R2 T1): the unit test's rays
$commands+="dxc -nologo -T cs_6_6 -E main -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/rt_skin.dxil.h`" -Vn rt_skin_dxil `"$root/native/renderer/shaders/rt_skin.hlsl`" >nul || exit /b 1"   # I-0117 (R2 T2): the skinned positions
# I-0120 S12: the control view's family count is written twice (the programme's RT_FAMILIES, the renderer's kRtViewFamilies): equal or no build.
$rtFamiliesHlsl=[regex]::Match((Get-Content -Raw -LiteralPath "$root/native/renderer/shaders/rt_view.hlsl"),'(?m)^#define RT_FAMILIES (\d+)').Groups[1].Value
$rtFamiliesCpp=[regex]::Match((Get-Content -Raw -LiteralPath "$root/native/renderer/rt_scene.h"),'constexpr uint32_t kRtViewFamilies=(\d+);').Groups[1].Value
if(-not $rtFamiliesHlsl -or $rtFamiliesHlsl -ne $rtFamiliesCpp){ throw "rt_view.hlsl's RT_FAMILIES ($rtFamiliesHlsl) differs from rt_scene.h's kRtViewFamilies ($rtFamiliesCpp)" }
$commands+="dxc -nologo -T cs_6_6 -E main -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/rt_view.dxil.h`" -Vn rt_view_dxil `"$root/native/renderer/shaders/rt_view.hlsl`" >nul || exit /b 1"   # I-0117 (R2 T4): the control view
# I-0117 R5 (on with the rays, the rays' scene): the traced sun shadow's full-screen programmes (one file, three entries).
foreach($entry in @(@('vs_6_6','vs','rt_shadow_vs'),@('ps_6_6','fill','rt_shadow_fill'),@('ps_6_6','trace','rt_shadow_trace'),@('ps_6_6','blur','rt_shadow_blur'))){
    $commands+="dxc -nologo -T $($entry[0]) -E $($entry[1]) -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/$($entry[2]).dxil.h`" -Vn $($entry[2])_dxil `"$root/native/renderer/shaders/rt_shadow.hlsl`" >nul || exit /b 1"}
# I-0122 R4a (on with the rays): the local lights' full-screen programmes (one file, three entries: R4c's pt).
foreach($entry in @(@('vs_6_6','vs','rt_lights_vs'),@('ps_6_6','shade','rt_lights_shade'),@('ps_6_6','pt','rt_lights_pt'))){
    $commands+="dxc -nologo -T $($entry[0]) -E $($entry[1]) -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/$($entry[2]).dxil.h`" -Vn $($entry[2])_dxil `"$root/native/renderer/shaders/rt_lights.hlsl`" >nul || exit /b 1"}
# I-0126 R7b: every rt_reflect programme with 16-bit types and SHaRC's headers (the queries at the hits); the cache's update with
# SHARC_UPDATE 1, its resolve a compute programme. I-0128 R8d (R8's final review L8): PathFrom only in the RR frames' reflections and GI
# (`*_path`, RT_PATHS 1), which a frame takes when its paths go past their first hit (0.7 ms off the RT level), and in gi_temporal always.
foreach($entry in @(@('vs_6_6','vs','rt_reflect_vs',''),@('ps_6_6','trace','rt_reflect_trace',''),@('ps_6_6','trace_hit','rt_reflect_trace_hit',''),@('ps_6_6','water','rt_reflect_water',''),@('ps_6_6','gi','rt_reflect_gi',''),@('ps_6_6','nrd_signal','rt_reflect_nrd_signal',''),@('ps_6_6','gi_compose','rt_reflect_gi_compose',''),@('ps_6_6','spec_compose','rt_reflect_spec_compose',''),@('ps_6_6','gi_temporal','rt_reflect_gi_temporal','-D RT_PATHS=1'),@('ps_6_6','trace_hit','rt_reflect_trace_hit_path','-D RT_PATHS=1'),@('ps_6_6','gi','rt_reflect_gi_path','-D RT_PATHS=1'),@('ps_6_6','gi_spatial','rt_reflect_gi_spatial',''),@('ps_6_6','gi_shade','rt_reflect_gi_shade',''),
    @('ps_6_6','cache_update','rt_reflect_cache_update','-D SHARC_UPDATE=1'),@('cs_6_6','cache_resolve','rt_reflect_cache_resolve',''),@('cs_6_6','probe','rt_reflect_probe',''),@('ps_6_6','ser_apply','rt_reflect_ser_apply',''))){   # I-0125 R6b: the traced reflections; R6c: RR's hit distance, the water; I-0126 R7a: the GI; R7b: the cache; R7c: NRD's signal and the compose; I-0132 R11: the specular's compose
    $commands+="dxc -nologo -T $($entry[0]) -E $($entry[1]) -O3 -enable-16bit-types $($entry[3]) -I `"$sharc`" -I `"$nrd/Shaders`" -I `"$nrdOut`" -I `"$rtxdi`" -Qstrip_debug -Qstrip_reflect -Fh `"$out/$($entry[2]).dxil.h`" -Vn $($entry[2])_dxil `"$root/native/renderer/shaders/rt_reflect.hlsl`" >nul || exit /b 1"}
# I-0132 R11b (R11-PLAN §13): the PT level's path programmes as ray generation shaders for shader execution reordering (shader model 6.9,
# the Agility SDK's runtime): trace_hit_ser and gi_temporal_ser, one library.
$commands+="dxc -nologo -T lib_6_9 -O3 -enable-16bit-types -Wno-misplaced-attributes -D RT_PATHS=1 -D RT_RAYGEN=1 -exports `"trace_hit_ser;gi_temporal_ser`" -I `"$sharc`" -I `"$nrd/Shaders`" -I `"$nrdOut`" -I `"$rtxdi`" -Qstrip_debug -Qstrip_reflect -Fh `"$out/rt_reflect_ser.dxil.h`" -Vn rt_reflect_ser_dxil `"$root/native/renderer/shaders/rt_reflect.hlsl`" >nul || exit /b 1"
foreach($entry in @(@('vs_6_6','vs','rr_guides_vs'),@('ps_6_6','guide','rr_guides_guide'))){   # I-0123 RR-a: Ray Reconstruction's guides
    $commands+="dxc -nologo -T $($entry[0]) -E $($entry[1]) -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/$($entry[2]).dxil.h`" -Vn $($entry[2])_dxil `"$root/native/renderer/shaders/rr_guides.hlsl`" >nul || exit /b 1"}
$commands+="$cxx /Fo`"$out/rt_scene.obj`" `"$root/native/renderer/rt_scene.cpp`" /I`"$out`" || exit /b 1"   # I-0117 (R2)
# I-0138 tranche 1 (ROADMAP workstream 11): our tone-map pass's programmes, compiled by fxc (on PATH after vcvars, beside dxc) as shader
# model 5 with IEEE strictness: the original's compiler family, each operation kept as post_tone.hlsl writes it.
foreach($entry in @(@('vs_5_0','vs','post_tone_vs'),@('ps_5_0','ps','post_tone_ps'),@('ps_5_0','ps_reinhard','post_tone_reinhard_ps'),@('ps_5_0','ps_lut','post_tone_lut_ps'),@('ps_5_0','ps_hdr','post_tone_hdr_ps'),@('ps_5_0','ps_hdr_reinhard','post_tone_hdr_reinhard_ps'),
    @('ps_5_0','ps_agx','post_tone_agx_ps'),@('ps_5_0','ps_aces','post_tone_aces_ps'),@('ps_5_0','ps_hdr_agx','post_tone_hdr_agx_ps'),@('ps_5_0','ps_hdr_aces','post_tone_hdr_aces_ps'),
    @('ps_5_0','ps_er','post_tone_er_ps'),@('ps_5_0','ps_hdr_er','post_tone_hdr_er_ps'),@('cs_5_0','cs_lut_copy','post_tone_lut_copy_cs'))){   # the two ps_hdr: I-0138 tranche 5   # ps_reinhard: I-0138 part 4c   # agx, aces, er: tranche 6's candidates   # cs_lut_copy: tranche 8
    $commands+="fxc /nologo /T $($entry[0]) /E $($entry[1]) /O3 /Gis /Fh `"$out/$($entry[2]).dxbc.h`" /Vn $($entry[2])_dxbc `"$root/native/renderer/shaders/post_tone.hlsl`" >nul || exit /b 1"}
$commands+="$cxx /Fo`"$out/post_tone_stage.obj`" `"$root/native/renderer/post_tone_stage.cpp`" /I`"$out`" || exit /b 1"
# I-0138 tranche 5 (HDR10): the output transform's programmes (shaders/post_output.hlsl), by fxc as shader model 5, and their stage.
foreach($entry in @(@('vs_5_0','vs','post_output_vs'),@('ps_5_0','ps_copy','post_output_copy_ps'),@('ps_5_0','ps_scene','post_output_scene_ps'))){
    $commands+="fxc /nologo /T $($entry[0]) /E $($entry[1]) /O3 /Fh `"$out/$($entry[2]).dxbc.h`" /Vn $($entry[2])_dxbc `"$root/native/renderer/shaders/post_output.hlsl`" >nul || exit /b 1"}
$commands+="$cxx /Fo`"$out/output_stage.obj`" `"$root/native/renderer/output_stage.cpp`" /I`"$out`" || exit /b 1"
# I-0132 R12h step 2: the native passes' programmes the module compiled at run time (D3DCompile, 31 entry points, about 470 ms at
# their first use by the R10 thread's timing),
# compiled here by fxc with the run-time flags (/Ges the strictness, the TAA's /Gis /O3 too): the same bytes as the run-time
# compilation (RESULT « R12h step 2 »), so the pipeline store's names are unchanged. Each <file>_<entry>.dxbc.h holds <file>_<entry>_dxbc.
$movedPrograms=@(@('taa','/Ges /Gis /O3',@(@('Resolve','cs_5_0'),@('Sharpen','cs_5_0'),@('Mirror','cs_5_0'))),
    @('auto_exposure','/Ges',@(@('Histogram','cs_5_0'),@('Adapt','cs_5_0'))),
    @('offscreen_composition','/Ges',@(@('VS','vs_5_0'),@('PS','ps_5_0'),@('BackgroundPS','ps_5_0'))),
    @('material_composition','/Ges',@(@('VS','vs_5_0'),@('PS','ps_5_0'))),
    @('effect_layer','/Ges',@(@('VS','vs_5_0'),@('Downsample','ps_5_0'),@('Composite','ps_5_0'),@('Bilateral','ps_5_0'))),
    @('velocity_resolve','/Ges',@(@('VS','vs_5_0'),@('PS','ps_5_0'))),@('velocity_half','/Ges',@(@('VS','vs_5_0'),@('PS','ps_5_0'))),
    @('velocity_occlusion','/Ges',@(@('VS','vs_5_0'),@('PS','ps_5_0'))),@('depth_out','/Ges',@(@('VS','vs_5_0'),@('PS','ps_5_0'))),
    @('cloth_velocity','/Ges',@(,@('VS','vs_5_0'))),@('effect_draw_scan','/Ges',@(,@('CS','cs_5_0'))),
    @('overlay','/Ges',@(@('VS','vs_5_1'),@('PS','ps_5_1'),@('PSScreen','ps_5_1'))),
    @('facegen_composition','/Ges',@(@('VS','vs_5_0'),@('PS','ps_5_0'))),@('facegen_mark','/Ges',@(@('VS','vs_5_0'),@('PS','ps_5_0'))))
# R11h's two step reviews' H1s (I-0132 RESULT): a comment after code on a line of the automatic exposure's kernels swallowed their
# stores, twice, compiling cleanly. auto_exposure_test checked their C++ string; the file is checked here: a comment only at a
# line's start (its header's lines are comments).
if(@(Get-Content -LiteralPath "$root/native/renderer/shaders/auto_exposure.hlsl" -Encoding utf8 | Where-Object { -not $_.TrimStart().StartsWith('//') -and $_.Contains('//') }).Count){
    throw 'shaders/auto_exposure.hlsl: a comment after code on a line (it can swallow the code after it: R11h''s step reviews)' }
foreach($program in $movedPrograms){foreach($entry in $program[2]){$name="$($program[0])_$($entry[0].ToLowerInvariant())"
    $commands+="fxc /nologo /T $($entry[1]) /E $($entry[0]) $($program[1]) /Fh `"$out/$name.dxbc.h`" /Vn $($name)_dxbc `"$root/native/renderer/shaders/$($program[0]).hlsl`" >nul || exit /b 1"}}
$movedProgramUnits=@('taa_stage','auto_exposure','scene_pass','scene_pass_frame','scene_pass_velocity','scene_pass_views','overlay_pass')   # their includers (and game_texture_ingress, whose loop has /I"$out")
# I-0138 tranche 2: the glare family's programmes of our own (post_glare.hlsl: the bloom's taps, bright pass and composite, the
# light shafts' mask, radial blurs and add, Alpha_R), a shape and a count each, compiled as post_tone's; their table by the
# technique a plan's draw names (post_glare_table.h, read by post_glare_programs.cpp).
$glareHlsl="$root/native/renderer/shaders/post_glare.hlsl";$glareUnits=@();$glareRows=@()
foreach($n in 1..8){$glareUnits+=,@("pg_vs_offset$n",'vs_5_0',"/D VS_OFFSET=$n")}
foreach($n in 1..4){$glareUnits+=,@("pg_vs_given$n",'vs_5_0',"/D VS_GIVEN=$n")}
foreach($n in 1..15){$glareUnits+=,@("pg_ps_taps$n",'ps_5_0',"/D PS_TAPS=$n");$glareRows+=,@("tech_MadT1_S$n","pg_vs_offset$([Math]::Min($n,8))","pg_ps_taps$n")}
foreach($n in 2..4){$glareUnits+=,@("pg_ps_given$n",'ps_5_0',"/D PS_GIVEN=$n");$glareRows+=,@("tech_TexCoordMadT1_S$n","pg_vs_given$n","pg_ps_given$n")}
foreach($n in 2..8){$glareUnits+=,@("pg_ps_add$n",'ps_5_0',"/D PS_ADD=$n");$glareUnits+=,@("pg_ps_addgamma$n",'ps_5_0',"/D PS_ADD=$n /D GAMMA")
    $glareRows+=,@("tech_MadT$n",'pg_vs_given1',"pg_ps_add$n");$glareRows+=,@("tech_MadLumGammaT$n",'pg_vs_given1',"pg_ps_addgamma$n")}
$glareUnits+=,@('pg_ps_bright','ps_5_0','/D PS_BRIGHT /D REMAP');$glareRows+=,@('tech_BrightPassT1_THRESHOLD_LUMINANCE_REMAP_S1','pg_vs_given1','pg_ps_bright')
$glareUnits+=,@('pg_ps_bright_vignette','ps_5_0','/D PS_BRIGHT /D VIGNETTE /D REMAP');$glareRows+=,@('tech_BrightPassT1_Vignette_THRESHOLD_LUMINANCE_REMAP_S1','pg_vs_given2','pg_ps_bright_vignette')
# I-0138 tranche 8f: GlareShape 3 (« Standard »): the bright pass without REMAP (RemapFactor 1), the arms' MaxColorT{n}, the afterimage,
# the ghosts' point symmetry and their three pixel programmes.
$glareUnits+=,@('pg_ps_bright_plain','ps_5_0','/D PS_BRIGHT');$glareRows+=,@('tech_BrightPassT1_THRESHOLD_LUMINANCE_S1','pg_vs_given1','pg_ps_bright_plain')
$glareUnits+=,@('pg_ps_bright_plain_vignette','ps_5_0','/D PS_BRIGHT /D VIGNETTE');$glareRows+=,@('tech_BrightPassT1_Vignette_THRESHOLD_LUMINANCE_S1','pg_vs_given2','pg_ps_bright_plain_vignette')
foreach($n in 2..8){$glareUnits+=,@("pg_ps_max$n",'ps_5_0',"/D PS_MAX=$n");$glareRows+=,@("tech_MaxColorT$n",'pg_vs_given1',"pg_ps_max$n")}
$glareUnits+=,@('pg_ps_mad_offset','ps_5_0','/D PS_MAD_OFFSET');$glareRows+=,@('tech_MadOffsetT2','pg_vs_given1','pg_ps_mad_offset')
$glareUnits+=,@('pg_vs_point_symmetry','vs_5_0','/D VS_POINT_SYMMETRY')
# I-0138 tranche 8f: the ghosts' pixel programmes in shader model 4, as the library's (ps_4_0): the same listing then, and no
# « precise » marks (SM5 /Gis writes them, SM4 has none), so the driver compiles both alike; as ps_5_0 their surfaces differed from
# the library's on the NVIDIA driver at extents other than the captures' (their log, exp, div and sqrt), never on WARP.
$glareUnits+=,@('pg_ps_ghost','ps_4_0','/D PS_GHOST');$glareRows+=,@('tech_GlareGhost_PointSymmetryT4_S8_TrimT1_S8','pg_vs_point_symmetry','pg_ps_ghost')
$glareUnits+=,@('pg_ps_ghost_dist','ps_4_0','/D PS_GHOST /D DIST');$glareRows+=,@('tech_GlareGhost_PointSymmetryDistScaleT4_S8_TrimT1_S8','pg_vs_point_symmetry','pg_ps_ghost_dist')
$glareUnits+=,@('pg_ps_ghost_t3','ps_5_0','/D PS_GHOST_T3');$glareRows+=,@('tech_GlareGhost_PointSymmetryT3_S8','pg_vs_point_symmetry','pg_ps_ghost_t3')
$glareUnits+=,@('pg_ps_shaft_mask','ps_5_0','/D PS_SHAFT_MASK');$glareRows+=,@('tech_LightShaftMask_SM3','pg_vs_given3','pg_ps_shaft_mask')
$glareUnits+=,@('pg_ps_corona','ps_5_0','/D PS_CORONA');$glareRows+=,@('tech_Corona','pg_vs_given2','pg_ps_corona')   # I-0138 tranche 8b: the shafts' corona
$glareUnits+=,@('pg_ps_alpha','ps_5_0','/D PS_ALPHA');$glareRows+=,@('tech_Alpha_R','pg_vs_offset1','pg_ps_alpha')
# I-0138 tranche 4 (part 4a): the depth of field's programmes of our own: the taps at 16, 20 and 25, the depth factor,
# the front blur's blend factor, the composite at its 40 counts.
foreach($n in 16,20,25){$glareUnits+=,@("pg_ps_taps$n",'ps_5_0',"/D PS_TAPS=$n");$glareRows+=,@("tech_MadT1_S$n",'pg_vs_offset8',"pg_ps_taps$n")}
$glareUnits+=,@('pg_ps_dof_factor','ps_5_0','/D PS_DOF_FACTOR');$glareRows+=,@('tech_DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut_RVALUE','pg_vs_given1','pg_ps_dof_factor')
$glareUnits+=,@('pg_ps_dof_front_factor','ps_5_0','/D PS_DOF_FRONT_FACTOR');$glareRows+=,@('tech_DepthOfField_FrontBlurBlendFactor','pg_vs_given1','pg_ps_dof_front_factor')
foreach($b in 0..4){foreach($f in 0..7){$glareUnits+=,@("pg_ps_dof_composite_${b}_$f",'ps_5_0',"/D PS_DOF_COMPOSITE /D DOF_BACK=$b /D DOF_FRONT=$f")
    $glareRows+=,@("tech_DepthOfField_CompositeBlurLevelsFrontBlur_TS${b}_$f",'pg_vs_given2',"pg_ps_dof_composite_${b}_$f")}}
# I-0138 tranche 4 (part 4b): the composite under holder flags 0, CompositeBlurredLevels_T1 to T8 (the scene's factor absolute).
# T1 is TS0_0's programme (one pin hash: no level, the absolute value unused): its unit.
$glareRows+=,@('tech_DepthOfField_CompositeBlurredLevels_T1','pg_vs_given2','pg_ps_dof_composite_0_0')
foreach($n in 2..8){$glareUnits+=,@("pg_ps_dof_levels_$n",'ps_5_0',"/D PS_DOF_COMPOSITE /D DOF_BACK=$($n-1) /D DOF_FRONT=0 /D DOF_ABS")
    $glareRows+=,@("tech_DepthOfField_CompositeBlurredLevels_T$n",'pg_vs_given2',"pg_ps_dof_levels_$n")}
# The motion blur's: the camera velocity and the blur at its seven tap counts.
$glareUnits+=,@('pg_ps_mb_velocity','ps_5_0','/D PS_MB_VELOCITY');$glareRows+=,@('tech_MotionBlurMakeCameraVelocityOverwrite_R','pg_vs_given3','pg_ps_mb_velocity')
foreach($n in 2..8){$glareUnits+=,@("pg_ps_mb_blur$n",'ps_5_0',"/D PS_MB_BLUR=$n");$glareRows+=,@("tech_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S$n",'pg_vs_given1',"pg_ps_mb_blur$n")}
foreach($unit in $glareUnits){
    $commands+="fxc /nologo /T $($unit[1]) /E main /O3 /Gis $($unit[2]) /Fh `"$out/$($unit[0]).dxbc.h`" /Vn $($unit[0])_dxbc `"$glareHlsl`" >nul || exit /b 1"}
$glareTable=@('// Written by Build-GameBootstrap.ps1 (I-0138 tranche 2): our glare programmes (post_glare.hlsl, fxc) by the technique a plan''s draw names.','#pragma once')
$glareTable+=@($glareUnits | ForEach-Object {"#include `"$($_[0]).dxbc.h`""})
$glareTable+='struct PostGlareRow {const char* technique;const unsigned char* vertex;size_t vertex_bytes;const unsigned char* pixel;size_t pixel_bytes;};'
$glareTable+='inline constexpr PostGlareRow post_glare_rows[]{'
$glareTable+=@($glareRows | ForEach-Object {"    {`"$($_[0])`",$($_[1])_dxbc,sizeof($($_[1])_dxbc),$($_[2])_dxbc,sizeof($($_[2])_dxbc)},"})
$glareTable+='};'
Set-GeneratedContent "$out/post_glare_table.h" $glareTable
$commands+="$cxx /Fo`"$out/post_glare_programs.obj`" `"$root/native/renderer/post_glare_programs.cpp`" /I`"$out`" || exit /b 1"
$commands+="$cxx /Fo`"$out/producers.obj`" `"$root/native/world/producers.cpp`" || exit /b 1"
$commands+="$cxx /Fo`"$out/cb_reads.obj`" `"$root/native/world/cb_reads.cpp`" || exit /b 1"
foreach($unit in @('light_producers','light_tiles','yebis_blocks','yebis_tonemap','yebis_glare','yebis_shafts','yebis_dof','yebis_motionblur','menu_tonemap')) {
    $commands+="$cxx /Fo`"$out/$unit.obj`" `"$root/native/world/$unit.cpp`" || exit /b 1"
}
foreach($unit in @('backend','device_removal','content','scene_view','frame_plan','frame_graph','frame_graph_shadow','scene_pass','scene_pass_lighting','scene_pass_velocity','scene_pass_decals','scene_pass_views','scene_pass_frame','effect_stage','render_worker','primitive_pass','model_draw','display_curve','display_stage','overlay_pass','glow_stage','tonemap_stage','auto_exposure','taa_stage','grading_stage','post_draws','glare_stage','mask_stage','frame_pacer','glow_math','tess_constants')){
    $built=if($unit -in $movedProgramUnits){' /I"'+$out+'"'}else{''}   # R12h step 2: the built programmes' headers
    $commands+="$cxx /Fo`"$out/$unit.obj`" `"$root/native/renderer/$unit.cpp`"$built || exit /b 1"
}
$commands+="$cxx /Fo`"$out/streamline_host.obj`" `"$root/native/renderer/streamline_host.cpp`" /I`"$streamline/include`" || exit /b 1"   # I-0111
foreach($unit in @('acquire','dds_resource','texture_container','gpu_texture_upload','shader_interface','gparam')){
    $commands+="$cxx /Fo`"$out/$unit.obj`" `"$root/native/content/$unit.cpp`" || exit /b 1"
}
foreach($unit in @('adler32','crc32','inflate','inffast','inftrees','uncompr','zutil')){
    $commands+="cl /nologo /c /O2 /MT /W3 /TC /Brepro /Gy /Fo`"$out/zlib_$unit.obj`" `"$root/native/third_party/zlib/$unit.c`" || exit /b 1"
}
foreach($unit in @('ffx_observe','effect_system','effect_draw','shadow_state','game_scene_geometry','scene_ingress','primitive_work','primitive_programs','primitive_constants','primitive_textures','engine_window','game_bootstrap','game_bootstrap_ctl','game_texture_ingress','game_ui_images','game_ui_bindings','game_ui_shaders','game_ui_primitive','game_ui_image_owner','game_ui_mesh_owner','game_ui_mesh_output','game_ui_cpu_shape_context','game_ui_session','game_ui_mesh_output_test','game_texture_ingress_test','game_primitive_geometry','game_primitive_geometry_test','game_primitive_mesh_source','game_primitive_mesh_source_test','game_primitive_image_source','game_primitive_image_source_test','startup_primitive_test','game_primitive_programs_test','game_primitive_uniforms_test','game_quality_object','game_quality_object_test','sample_profile','crash_dump')){
    $commands+="$cxx /Fo`"$out/$unit.obj`" `"$PSScriptRoot/$unit.cpp`" /I`"$minHook/include`" /I`"$out`" || exit /b 1"
}
$objects=@('ffx_observe','sample_profile','crash_dump','device_removal','effect_system','effect_draw','shadow_state','game_quality_object','game_scene_geometry','scene_ingress','acquire','game_bootstrap','game_ui_image_owner','game_ui_mesh_owner','game_ui_mesh_output','game_ui_cpu_shape_context','game_ui_session','game_ui_images','game_ui_bindings','game_ui_shaders','game_ui_primitive','primitive_textures','primitive_constants','game_texture_ingress','game_primitive_geometry','game_primitive_mesh_source','game_primitive_image_source','primitive_programs','engine_window','backend','streamline_host','content','mh_hook','mh_buffer','mh_trampoline','mh_hde64','dds_resource','texture_container','gpu_texture_upload','zlib_adler32','zlib_crc32','zlib_inflate','zlib_inffast','zlib_inftrees','zlib_uncompr','zlib_zutil','scene_view','frame_plan','frame_graph','frame_graph_shadow','scene_pass','scene_pass_lighting','scene_pass_velocity','scene_pass_decals','scene_pass_views','scene_pass_frame','effect_stage','render_worker','primitive_pass','model_draw','display_curve','display_stage','output_stage','overlay_pass','glow_stage','post_tone_stage','post_glare_programs','auto_exposure','taa_stage','post_draws','glare_stage','mask_stage','frame_pacer','glow_math','tess_constants','primitive_work','shader_interface','gparam')
$commands+="$cxx /Fo`"$out/game_model_selection.obj`" `"$PSScriptRoot/game_model_selection.cpp`" || exit /b 1"
$objects=@('cb_reads','producers','light_producers','light_tiles','yebis_blocks','yebis_tonemap','yebis_glare','yebis_shafts','yebis_dof','yebis_motionblur','menu_tonemap','game_model_selection')+$objects
$commands+="$cxx /Fo`"$out/model_programs.obj`" `"$root/native/content/model_programs.cpp`" || exit /b 1"
$objects=@('model_programs')+$objects
$objects=@('raytracing_base','rt_scene')+$objects   # I-0109 (R0), I-0117 (R2)
$objects=@('rt_denoise')+@($nrdUnits | ForEach-Object {"nrd_$($_.ToLowerInvariant())"})+$objects   # I-0126 R7c: NRD
$commands+="$cxx /Fo`"$out/model_pose.obj`" `"$root/native/content/model_pose.cpp`" || exit /b 1"
$objects=@('model_pose')+$objects
$commands+="$cxx /Fo`"$out/model_palette_indices.obj`" `"$root/native/content/model_palette_indices.cpp`" || exit /b 1"
$objects=@('model_palette_indices')+$objects
$commands+="$cxx /Fo`"$out/model_pass.obj`" `"$root/native/content/model_pass.cpp`" || exit /b 1"
$objects=@('model_pass')+$objects
$commands+="$cxx /Fo`"$out/model_parameters.obj`" `"$root/native/content/model_parameters.cpp`" || exit /b 1"
$objects=@('model_parameters')+$objects
$commands+="$cxx /fp:strict /Fo`"$out/decal_constants.obj`" `"$root/native/content/decal_constants.cpp`" || exit /b 1"
$objects=@('decal_constants')+$objects
$commands+="$cxx /fp:strict /Fo`"$out/paint_constants.obj`" `"$root/native/content/paint_constants.cpp`" || exit /b 1"
$objects=@('paint_constants')+$objects   # DECALS-PAINT-READY D2
$commands+="$cxx /fp:strict /Fo`"$out/model_common_textures.obj`" `"$root/native/content/model_common_textures.cpp`" || exit /b 1"
$objects=@('model_common_textures')+$objects
$commands+="$cxx /Fo`"$out/model_textures.obj`" `"$root/native/content/model_textures.cpp`" || exit /b 1"
$commands+="$cxx /Fo`"$out/texture_tiling.obj`" `"$root/native/content/texture_tiling.cpp`" || exit /b 1"
$objects=@('texture_tiling','model_textures')+$objects
$links=($objects | ForEach-Object { "`"$out/$_.obj`"" }) -join ' '
# Security audit L1 (2026-11-23): /DEPENDENTLOADFLAG:0x800, the module's imports from System32 only (never a proxy of the same name in the
# game's folder; a namesake the game already loaded is still the one used).
$commands+="link /nologo /DLL /DEBUG /INCREMENTAL:NO /OPT:REF,ICF /PDB:`"$out/game_bootstrap.pdb`" /Brepro /DEPENDENTLOADFLAG:0x800 /OUT:`"$out/game_bootstrap.dll`" $links d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib gdi32.lib dwmapi.lib windowscodecs.lib ole32.lib winmm.lib mfplat.lib mfreadwrite.lib mfuuid.lib delayimp.lib /DELAYLOAD:mfplat.dll /DELAYLOAD:mfreadwrite.dll bcrypt.lib dbghelp.lib || exit /b 1"
$commands+="link /nologo /Brepro /OUT:`"$out/game_bootstrap_ctl.exe`" `"$out/game_bootstrap_ctl.obj`" `"$out/crash_dump.obj`" psapi.lib dbghelp.lib user32.lib || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_primitive_geometry_test.exe`" `"$out/game_primitive_geometry_test.obj`" `"$out/game_primitive_geometry.obj`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_primitive_mesh_source_test.exe`" `"$out/game_primitive_mesh_source_test.obj`" `"$out/game_primitive_mesh_source.obj`" `"$out/game_primitive_geometry.obj`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_ui_mesh_output_test.exe`" `"$out/game_ui_mesh_output_test.obj`" `"$out/game_ui_mesh_output.obj`" `"$out/acquire.obj`" bcrypt.lib || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_primitive_image_source_test.exe`" `"$out/game_primitive_image_source_test.obj`" `"$out/game_primitive_image_source.obj`" `"$out/primitive_textures.obj`" `"$out/primitive_constants.obj`" `"$out/primitive_programs.obj`" `"$out/primitive_work.obj`" `"$out/shader_interface.obj`" d3dcompiler.lib bcrypt.lib || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_quality_object_test.exe`" `"$out/game_quality_object_test.obj`" `"$out/game_quality_object.obj`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_primitive_programs_test.exe`" `"$out/game_primitive_programs_test.obj`" `"$out/primitive_programs.obj`" `"$out/primitive_work.obj`" `"$out/shader_interface.obj`" d3dcompiler.lib bcrypt.lib || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_primitive_uniforms_test.exe`" `"$out/game_primitive_uniforms_test.obj`" `"$out/game_ui_shaders.obj`" `"$out/game_ui_primitive.obj`" `"$out/game_ui_bindings.obj`" `"$out/primitive_constants.obj`" `"$out/primitive_programs.obj`" `"$out/primitive_work.obj`" `"$out/shader_interface.obj`" `"$out/acquire.obj`" d3dcompiler.lib bcrypt.lib || exit /b 1"
$primitiveObjects=@('startup_primitive_test','game_primitive_geometry','backend','streamline_host','content','scene_view','frame_plan','frame_graph','frame_graph_shadow','scene_pass','scene_pass_lighting','scene_pass_velocity','scene_pass_decals','scene_pass_views','scene_pass_frame','rt_scene','rt_denoise','nrd_instanceimpl','nrd_reblur','nrd_reference','nrd_relax','nrd_sigma','nrd_timer','nrd_wrapper','effect_stage','render_worker','primitive_pass','model_draw','display_curve','display_stage','output_stage','overlay_pass','glow_stage','post_tone_stage','post_glare_programs','auto_exposure','taa_stage','post_draws','glare_stage','mask_stage','frame_pacer','glow_math','tess_constants','primitive_work','shader_interface','dds_resource','texture_container','gpu_texture_upload','gparam','zlib_adler32','zlib_crc32','zlib_inffast','zlib_inflate','zlib_inftrees','zlib_uncompr','zlib_zutil')
$primitiveLinks=($primitiveObjects | ForEach-Object { "`"$out/$_.obj`"" }) -join ' '
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/startup_primitive_test.exe`" $primitiveLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/game_texture_ingress_test.exe`" `"$out/game_texture_ingress_test.obj`" `"$out/game_texture_ingress.obj`" `"$out/game_ui_images.obj`" `"$out/game_ui_bindings.obj`" `"$out/game_primitive_image_source.obj`" `"$out/primitive_textures.obj`" `"$out/primitive_constants.obj`" `"$out/primitive_programs.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/gpu_texture_upload.obj`" `"$out/dds_resource.obj`" `"$out/texture_container.obj`" `"$out/zlib_adler32.obj`" `"$out/zlib_crc32.obj`" `"$out/zlib_inflate.obj`" `"$out/zlib_inffast.obj`" `"$out/zlib_inftrees.obj`" `"$out/zlib_uncompr.obj`" `"$out/zlib_zutil.obj`" `"$out/scene_view.obj`" `"$out/frame_plan.obj`" `"$out/frame_graph.obj`" `"$out/frame_graph_shadow.obj`" `"$out/scene_pass.obj`" `"$out/scene_pass_lighting.obj`" `"$out/scene_pass_velocity.obj`" `"$out/scene_pass_decals.obj`" `"$out/scene_pass_views.obj`" `"$out/scene_pass_frame.obj`" `"$out/rt_scene.obj`" `"$out/rt_denoise.obj`" `"$out/nrd_instanceimpl.obj`" `"$out/nrd_reblur.obj`" `"$out/nrd_reference.obj`" `"$out/nrd_relax.obj`" `"$out/nrd_sigma.obj`" `"$out/nrd_timer.obj`" `"$out/nrd_wrapper.obj`" `"$out/effect_stage.obj`" `"$out/display_stage.obj`" `"$out/output_stage.obj`" `"$out/overlay_pass.obj`" `"$out/glow_stage.obj`" `"$out/post_tone_stage.obj`" `"$out/auto_exposure.obj`" `"$out/taa_stage.obj`" `"$out/post_draws.obj`" `"$out/post_glare_programs.obj`" `"$out/glare_stage.obj`" `"$out/mask_stage.obj`" `"$out/glow_math.obj`" `"$out/display_curve.obj`" `"$out/frame_pacer.obj`" `"$out/primitive_pass.obj`" `"$out/model_draw.obj`" `"$out/primitive_work.obj`" `"$out/shader_interface.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
# Workstream K controls: register plans equal the pre-K preparation (P1); collect profile self-time attribution (K1).
$commands+="$cxx /Fo`"$out/register_plan_test.obj`" `"$PSScriptRoot/register_plan_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/register_plan_test.exe`" `"$out/register_plan_test.obj`" `"$out/cb_reads.obj`" d3dcompiler.lib || exit /b 1"
# PREPARE-REDUCTION T-P3/T-P5/T-P6: prepared model draws on a real device, drawn and read back.
$commands+="$cxx /Fo`"$out/model_draw_test.obj`" `"$PSScriptRoot/model_draw_test.cpp`" || exit /b 1"
$modelDrawLinks=((@('model_draw_test')+($primitiveObjects | Where-Object {$_ -notin @('startup_primitive_test','game_primitive_geometry')})) | ForEach-Object { "`"$out/$_.obj`"" }) -join ' '
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/model_draw_test.exe`" $modelDrawLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
# D3D12 phase D cD1 T-D1/T-D4: the frame ring under a forced overlap.
$commands+="$cxx /Fo`"$out/frame_ring_test.obj`" `"$PSScriptRoot/frame_ring_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/frame_ring_test.exe`" `"$out/frame_ring_test.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/frame_presenter_test.obj`" `"$PSScriptRoot/frame_presenter_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/frame_presenter_test.exe`" `"$out/frame_presenter_test.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
# I-0072: the device's pipeline cache (roots, memory hits, one build per name, the precompile list, a damaged list, ageing).
# I-0132 R11i (the step review's M2): the test's DXIL programmes (dxc as the module's own: OSG1 output signatures).
foreach($entry in @(@('depth','pipeline_cache_depth'),@('colour','pipeline_cache_colour'))){
    $commands+="dxc -nologo -T ps_6_6 -E $($entry[0]) -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/$($entry[1]).dxil.h`" -Vn $($entry[1])_dxil `"$PSScriptRoot/pipeline_cache_depth.hlsl`" >nul || exit /b 1"}
# I-0132 (the root aliases): two programmes embedding one root signature.
foreach($entry in @(@('red','pipeline_cache_red'),@('green','pipeline_cache_green'))){
    $commands+="dxc -nologo -T ps_6_6 -E $($entry[0]) -O3 -Qstrip_debug -Qstrip_reflect -Fh `"$out/$($entry[1]).dxil.h`" -Vn $($entry[1])_dxil `"$PSScriptRoot/pipeline_cache_root.hlsl`" >nul || exit /b 1"}
$commands+="$cxx /Fo`"$out/pipeline_cache_test.obj`" `"$PSScriptRoot/pipeline_cache_test.cpp`" /I`"$out`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/pipeline_cache_test.exe`" `"$out/pipeline_cache_test.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
# I-0082: the minidump writer and stack report on threads of the test process, the device-removal report on a removed WARP device
# (its PDB names the test's functions in the report).
$commands+="$cxx /Fo`"$out/raytracing_base_test.obj`" `"$PSScriptRoot/raytracing_base_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/raytracing_base_test.exe`" `"$out/raytracing_base_test.obj`" `"$out/raytracing_base.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/rt_scene_test.obj`" `"$PSScriptRoot/rt_scene_test.cpp`" /I`"$out`" || exit /b 1"   # I-0117 (R2 T0, T1)
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/rt_scene_test.exe`" `"$out/rt_scene_test.obj`" `"$out/rt_scene.obj`" `"$out/raytracing_base.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/scene_view.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/original_image_test.obj`" `"$PSScriptRoot/original_image_test.cpp`" || exit /b 1"
$commands+="link /nologo /Brepro /OUT:`"$out/original_image_test.exe`" `"$out/original_image_test.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/crash_dump_test.obj`" `"$PSScriptRoot/crash_dump_test.cpp`" || exit /b 1"
$commands+="link /nologo /DEBUG /INCREMENTAL:NO /OPT:REF /PDB:`"$out/crash_dump_test.pdb`" /Brepro /OUT:`"$out/crash_dump_test.exe`" `"$out/crash_dump_test.obj`" `"$out/crash_dump.obj`" `"$out/device_removal.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib dbghelp.lib || exit /b 1"
# m45 frame cost: the sampling profiler on a known call chain, and the symbolizer of its file (sample-profile.flag).
$commands+="$cxx /Fo`"$out/sample_symbolize.obj`" `"$PSScriptRoot/sample_symbolize.cpp`" || exit /b 1"
$commands+="link /nologo /Brepro /OUT:`"$out/sample_symbolize.exe`" `"$out/sample_symbolize.obj`" dbghelp.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/sample_profile_test.obj`" `"$PSScriptRoot/sample_profile_test.cpp`" || exit /b 1"
$commands+="link /nologo /DEBUG /INCREMENTAL:NO /OPT:REF,ICF /PDB:`"$out/sample_profile_test.pdb`" /Brepro /OUT:`"$out/sample_profile_test.exe`" `"$out/sample_profile_test.obj`" `"$out/sample_profile.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/collect_profile_test.obj`" `"$PSScriptRoot/collect_profile_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/collect_profile_test.exe`" `"$out/collect_profile_test.obj`" || exit /b 1"
# I-0091: the collect's frame store and frame array (kept blocks, places constructed once).
$commands+="$cxx /Fo`"$out/frame_store_test.obj`" `"$PSScriptRoot/frame_store_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/frame_store_test.exe`" `"$out/frame_store_test.obj`" || exit /b 1"
# Phase E E1/E2: the control of the frame graph planners and the shadow recorder (T-E1, T-E2 recorder part, T-E3, T-E6).
$commands+="$cxx /Fo`"$out/frame_graph_test.obj`" `"$PSScriptRoot/frame_graph_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/frame_graph_test.exe`" `"$out/frame_graph_test.obj`" `"$out/frame_graph.obj`" `"$out/frame_graph_shadow.obj`" `"$out/frame_plan.obj`" d3d12.lib dxgi.lib || exit /b 1"
# R13 cR3 T-R1, worker parts of T-R3/T-R4, T-R9: the render worker alone (user32 for the pumped wait's window).
$commands+="$cxx /Fo`"$out/render_worker_test.obj`" `"$PSScriptRoot/render_worker_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/render_worker_test.exe`" `"$out/render_worker_test.obj`" `"$out/render_worker.obj`" user32.lib || exit /b 1"
# W3 G0-1: the display curve and the CPU model of the gamma pass (GAMMA-COLOUR-PATH-READY T1, T2).
$commands+="$cxx /Fo`"$out/display_curve_test.obj`" `"$PSScriptRoot/display_curve_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/display_curve_test.exe`" `"$out/display_curve_test.obj`" `"$out/display_curve.obj`" || exit /b 1"
# I-0113 tranche 22: FF630's ordered-dither pair (the model programmes' t38).
$commands+="$cxx /fp:strict /Fo`"$out/fade_dither_test.obj`" `"$PSScriptRoot/fade_dither_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/fade_dither_test.exe`" `"$out/fade_dither_test.obj`" `"$out/model_common_textures.obj`" || exit /b 1"
# I-0097 tranche 4 (D-0211): the content hash of the native cache.
$commands+="$cxx /Fo`"$out/content_hash_test.obj`" `"$PSScriptRoot/content_hash_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/content_hash_test.exe`" `"$out/content_hash_test.obj`" || exit /b 1"
# I-0101, I-0113 tranche 10d: a non-finite pose row is kept in its palettes (the GPU drops the triangles it places), no refusal.
$commands+="$cxx /Fo`"$out/model_pose_test.obj`" `"$PSScriptRoot/model_pose_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/model_pose_test.exe`" `"$out/model_pose_test.obj`" `"$out/model_pose.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" d3dcompiler.lib || exit /b 1"
# I-0101: the GX MTD image of a material read back as DS3 reads it (STUDY §2).
$commands+="$cxx /Fo`"$out/gx_mtd_image_test.obj`" `"$PSScriptRoot/gx_mtd_image_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/gx_mtd_image_test.exe`" `"$out/gx_mtd_image_test.obj`" || exit /b 1"
# I-0114: the frame-rate laws of the unlocked logic (k = 1 bit for bit, half-calls, the locomotion factor).
$commands+="$cxx /Fo`"$out/logic_rate_test.obj`" `"$PSScriptRoot/logic_rate_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/logic_rate_test.exe`" `"$out/logic_rate_test.obj`" || exit /b 1"
# I-0114: the follow camera's display blend between two ticks.
$commands+="$cxx /Fo`"$out/view_blend_test.obj`" `"$PSScriptRoot/view_blend_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/view_blend_test.exe`" `"$out/view_blend_test.obj`" || exit /b 1"
# I-0113 tranche 14: the region choice of a world object's IBL.
$commands+="$cxx /Fo`"$out/env_region_test.obj`" `"$PSScriptRoot/env_region_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/env_region_test.exe`" `"$out/env_region_test.obj`" || exit /b 1"
# I-0113 tranche 15: the model rows' order key (57A20, BF330).
$commands+="$cxx /Fo`"$out/sort_key_test.obj`" `"$PSScriptRoot/sort_key_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/sort_key_test.exe`" `"$out/sort_key_test.obj`" || exit /b 1"
# I-0129 R9a: the traced probes' CPU side (rt_probes.h).
$commands+="$cxx /Fo`"$out/rt_probes_test.obj`" `"$PSScriptRoot/rt_probes_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/rt_probes_test.exe`" `"$out/rt_probes_test.obj`" || exit /b 1"
# I-0111 tranche 4 step 2: the DLSS words' parser (rekindled-settings.ini's dlss).
$commands+="$cxx /Fo`"$out/dlss_mode_test.obj`" `"$PSScriptRoot/dlss_mode_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/dlss_mode_test.exe`" `"$out/dlss_mode_test.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/frame_rate_option_test.obj`" `"$PSScriptRoot/frame_rate_option_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/frame_rate_option_test.exe`" `"$out/frame_rate_option_test.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/dark_logo_test.obj`" `"$PSScriptRoot/dark_logo_test.cpp`" || exit /b 1"   # I-0135 (final review L5)
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/dark_logo_test.exe`" `"$out/dark_logo_test.obj`" || exit /b 1"
# I-0132 R12b: rekindled-settings.ini's model (its grammar, its defaults, D-0265's rules).
$commands+="$cxx /Fo`"$out/rekindled_settings_test.obj`" `"$PSScriptRoot/rekindled_settings_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/rekindled_settings_test.exe`" `"$out/rekindled_settings_test.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/graphics_rows_test.obj`" `"$PSScriptRoot/graphics_rows_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/graphics_rows_test.exe`" `"$out/graphics_rows_test.obj`" || exit /b 1"
# I-0138 tranche 5b: the options film's added slots (options_film.h).
$commands+="$cxx /Fo`"$out/options_film_test.obj`" `"$PSScriptRoot/options_film_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/options_film_test.exe`" `"$out/options_film_test.obj`" || exit /b 1"
# I-0132 R12a: DS3's menu fonts (DefineFont3 read back, exact coverage, the shadow's blur, the shelf) and the overlay's text (UTF-8, the glyph cache, a line's layout).
$commands+="$cxx /Fo`"$out/font_raster_test.obj`" `"$PSScriptRoot/font_raster_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/font_raster_test.exe`" `"$out/font_raster_test.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/overlay_text_test.obj`" `"$PSScriptRoot/overlay_text_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/overlay_text_test.exe`" `"$out/overlay_text_test.obj`" || exit /b 1"
# R14 cR4: the present pacer on a fake and on the system clock (T-R6).
$commands+="$cxx /Fo`"$out/frame_pacer_test.obj`" `"$PSScriptRoot/frame_pacer_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/frame_pacer_test.exe`" `"$out/frame_pacer_test.obj`" `"$out/frame_pacer.obj`" user32.lib || exit /b 1"
# W5a SW-2 T-W1/T-W2: the shadow state and scene words against the golden shadow-world-blocks.tsv.
$commands+="$cxx /Fo`"$out/shadow_state_test.obj`" `"$PSScriptRoot/shadow_state_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/shadow_state_test.exe`" `"$out/shadow_state_test.obj`" `"$out/shadow_state.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/glow_math_test.obj`" `"$PSScriptRoot/glow_math_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/glow_math_test.exe`" `"$out/glow_math_test.obj`" `"$out/glow_math.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/upscaler_test.obj`" `"$PSScriptRoot/upscaler_test.cpp`" || exit /b 1"   # I-0111 (R1): jitter and unjittered vectors
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/upscaler_test.exe`" `"$out/upscaler_test.obj`" `"$out/producers.obj`" `"$out/scene_view.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/tess_constants_test.obj`" `"$PSScriptRoot/tess_constants_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/tess_constants_test.exe`" `"$out/tess_constants_test.obj`" `"$out/tess_constants.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/glow_bank_test.obj`" `"$PSScriptRoot/glow_bank_test.cpp`" || exit /b 1"
$zlibLinks=((@('adler32','crc32','inflate','inffast','inftrees','uncompr','zutil') | ForEach-Object { "`"$out/zlib_$_.obj`"" }) -join ' ')
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/glow_bank_test.exe`" `"$out/glow_bank_test.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/gparam_listing_test.obj`" `"$PSScriptRoot/gparam_listing_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/gparam_listing_test.exe`" `"$out/gparam_listing_test.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
# I-0102 lot 2f: the tests of live code no build ran before (census A.1): the decal constants, the GPARAM reader's
# refusals, the host's memory-only input, the G-buffer encoder (in the profile loop below).
$commands+="$cxx /fp:strict /Fo`"$out/decal_constants_test.obj`" `"$root/native/content/decal_constants_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/decal_constants_test.exe`" `"$out/decal_constants_test.obj`" `"$out/decal_constants.obj`" || exit /b 1"
$commands+="$cxx /Fo`"$out/gparam_test.obj`" `"$root/native/content/gparam_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/gparam_test.exe`" `"$out/gparam_test.obj`" `"$out/gparam.obj`" $zlibLinks || exit /b 1"
$commands+="$cxx /Fo`"$out/host_input_test.obj`" `"$root/native/world/host_input_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/host_input_test.exe`" `"$out/host_input_test.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/gbuffer_encoder_test.obj`" `"$root/native/renderer/gbuffer_encoder_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/gbuffer_encoder_test.exe`" `"$out/gbuffer_encoder_test.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/yebis_pins_test.obj`" `"$PSScriptRoot/yebis_pins_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/yebis_pins_test.exe`" `"$out/yebis_pins_test.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/yebis_blocks_test.obj`" `"$PSScriptRoot/yebis_blocks_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/yebis_blocks_test.exe`" `"$out/yebis_blocks_test.obj`" `"$out/yebis_blocks.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
# W10 D-0175: the native tone-map constants against the original's captured words (m37).
$commands+="$cxx /Fo`"$out/yebis_tonemap_test.obj`" `"$PSScriptRoot/yebis_tonemap_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/yebis_tonemap_test.exe`" `"$out/yebis_tonemap_test.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
# I-0132 R11h: the automatic exposure's kernels as built (R12h step 2) and its math (auto_exposure_math.h, header only).
$commands+="$cxx /Fo`"$out/auto_exposure_test.obj`" `"$PSScriptRoot/auto_exposure_test.cpp`" /I`"$out`" || exit /b 1"   # R12h step 2: the built programmes
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/auto_exposure_test.exe`" `"$out/auto_exposure_test.obj`" d3dcompiler.lib dxguid.lib || exit /b 1"
# I-0132 R11h (the final review's M1): the stage's kernels on WARP against their CPU mirror.
$commands+="$cxx /Fo`"$out/auto_exposure_stage_test.obj`" `"$PSScriptRoot/auto_exposure_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/auto_exposure_stage_test.exe`" `"$out/auto_exposure_stage_test.obj`" `"$out/auto_exposure.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" $zlibLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
# I-0132 R11j: the TAA's resolve on WARP.
$commands+="$cxx /Fo`"$out/taa_test.obj`" `"$PSScriptRoot/taa_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/taa_test.exe`" `"$out/taa_test.obj`" `"$out/taa_stage.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
# I-0113 tranche 27: the menu views' GX tone map (TabledToneMapPS and its tables) against independently computed values.
$commands+="$cxx /Fo`"$out/menu_tonemap_test.obj`" `"$PSScriptRoot/menu_tonemap_test.cpp`" /I`"$out`" || exit /b 1"   # R12h step 2: the built programmes
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/menu_tonemap_test.exe`" `"$out/menu_tonemap_test.obj`" `"$out/menu_tonemap.obj`" d3dcompiler.lib dxguid.lib || exit /b 1"
# I-0113 tranche 29: the half-float scan of effect-scan.flag (renderer/effect_scan.h, header only).
$commands+="$cxx /Fo`"$out/effect_scan_test.obj`" `"$PSScriptRoot/effect_scan_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/effect_scan_test.exe`" `"$out/effect_scan_test.obj`" || exit /b 1"
# W4 FW-1 T-F4: the effect observers' signature-free thunk, histograms and rows.
$commands+="$cxx /Fo`"$out/ffx_observe_test.obj`" `"$PSScriptRoot/ffx_observe_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/ffx_observe_test.exe`" `"$out/ffx_observe_test.obj`" `"$out/ffx_observe.obj`" || exit /b 1"
# W3 G0-3: the display stage with the original shaders (T4-T7), WARP and hardware.
$commands+="$cxx /Fo`"$out/display_stage_test.obj`" `"$PSScriptRoot/display_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/display_stage_test.exe`" `"$out/display_stage_test.obj`" `"$out/display_stage.obj`" `"$out/display_curve.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
# W6 GL-5 T4/T5: the glow stage with the original's four programmes (I-0063 programme cache; skipped without it), WARP and hardware.
$commands+="$cxx /Fo`"$out/glow_stage_test.obj`" `"$PSScriptRoot/glow_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/glow_stage_test.exe`" `"$out/glow_stage_test.obj`" `"$out/glow_stage.obj`" `"$out/glow_math.obj`" `"$out/shader_interface.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/tonemap_stage_test.obj`" `"$PSScriptRoot/tonemap_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/tonemap_stage_test.exe`" `"$out/tonemap_stage_test.obj`" `"$out/tonemap_stage.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" $zlibLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
# W10 GR T17 (W10-NATIVE-PASSES 12): the colour grading stage (LUT copy and grading draw) against Reference1696's draw 50.
$commands+="$cxx /Fo`"$out/grading_stage_test.obj`" `"$PSScriptRoot/grading_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/grading_stage_test.exe`" `"$out/grading_stage_test.obj`" `"$out/grading_stage.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" $zlibLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
# I-0138 tranche 1: our tone-map pass against the original's captured draws and the two stages it replaces; its parameter block.
$commands+="$cxx /Fo`"$out/post_tone_stage_test.obj`" `"$PSScriptRoot/post_tone_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/post_tone_stage_test.exe`" `"$out/post_tone_stage_test.obj`" `"$out/post_tone_stage.obj`" `"$out/tonemap_stage.obj`" `"$out/grading_stage.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" $zlibLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/hdr_output_test.obj`" `"$PSScriptRoot/hdr_output_test.cpp`" /I`"$out`" || exit /b 1"   # I-0138 tranche 5
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/hdr_output_test.exe`" `"$out/hdr_output_test.obj`" `"$out/output_stage.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/post_tone_test.obj`" `"$PSScriptRoot/post_tone_test.cpp`" /I`"$out`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/post_tone_test.exe`" `"$out/post_tone_test.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib dxguid.lib || exit /b 1"
# W10 GL-1/GL-2/GL-3 (W10-NATIVE-PASSES 5): T13 the captured glare draws replayed through PostDraws, the native glare plan against the captured blocks, T14 the glare stage's chain.
$commands+="$cxx /Fo`"$out/post_draws_test.obj`" `"$PSScriptRoot/post_draws_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/post_draws_test.exe`" `"$out/post_draws_test.obj`" `"$out/post_draws.obj`" `"$out/post_glare_programs.obj`" `"$out/yebis_shafts.obj`" `"$out/producers.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" $zlibLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/yebis_glare_test.obj`" `"$PSScriptRoot/yebis_glare_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/yebis_glare_test.exe`" `"$out/yebis_glare_test.obj`" `"$out/yebis_glare.obj`" `"$out/yebis_shafts.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/yebis_shafts_test.obj`" `"$PSScriptRoot/yebis_shafts_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/yebis_shafts_test.exe`" `"$out/yebis_shafts_test.obj`" `"$out/yebis_shafts.obj`" `"$out/yebis_glare.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/glare_stage_test.obj`" `"$PSScriptRoot/glare_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/glare_stage_test.exe`" `"$out/glare_stage_test.obj`" `"$out/glare_stage.obj`" `"$out/post_draws.obj`" `"$out/post_glare_programs.obj`" `"$out/yebis_glare.obj`" `"$out/yebis_shafts.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" $zlibLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
# W10 DF-1/DF-2 (W10-NATIVE-PASSES 9): the DoF plan from the holder and from the native holder, and the DoF table, against the DoF state captures; DF-3 T15 the DoF drawn by the glare stage.
$commands+="$cxx /Fo`"$out/yebis_dof_test.obj`" `"$PSScriptRoot/yebis_dof_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/yebis_dof_test.exe`" `"$out/yebis_dof_test.obj`" `"$out/yebis_dof.obj`" `"$out/yebis_shafts.obj`" `"$out/yebis_glare.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/dof_stage_test.obj`" `"$PSScriptRoot/dof_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/dof_stage_test.exe`" `"$out/dof_stage_test.obj`" `"$out/glare_stage.obj`" `"$out/post_draws.obj`" `"$out/post_glare_programs.obj`" `"$out/yebis_dof.obj`" `"$out/yebis_glare.obj`" `"$out/yebis_shafts.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" $zlibLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
# W10 MB-1/MB-2 (W10-NATIVE-PASSES 10): the motion blur's plan against the captured draws, and T16 the plan drawn by the glare stage.
$commands+="$cxx /Fo`"$out/yebis_motionblur_test.obj`" `"$PSScriptRoot/yebis_motionblur_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/yebis_motionblur_test.exe`" `"$out/yebis_motionblur_test.obj`" `"$out/yebis_motionblur.obj`" `"$out/yebis_shafts.obj`" `"$out/yebis_glare.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" $zlibLinks d3dcompiler.lib || exit /b 1"
$commands+="$cxx /Fo`"$out/mb_stage_test.obj`" `"$PSScriptRoot/mb_stage_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/mb_stage_test.exe`" `"$out/mb_stage_test.obj`" `"$out/glare_stage.obj`" `"$out/post_draws.obj`" `"$out/post_glare_programs.obj`" `"$out/yebis_motionblur.obj`" `"$out/yebis_glare.obj`" `"$out/yebis_shafts.obj`" `"$out/yebis_tonemap.obj`" `"$out/light_producers.obj`" `"$out/producers.obj`" `"$out/cb_reads.obj`" `"$out/light_tiles.obj`" `"$out/gparam.obj`" `"$out/glow_math.obj`" `"$out/backend.obj`" `"$out/streamline_host.obj`" `"$out/content.obj`" `"$out/acquire.obj`" $zlibLinks d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib user32.lib bcrypt.lib || exit /b 1"
# W7 TS-3 T-Q7: every _PN hull/domain pair of the I-0063 programme cache makes a patch pipeline (skipped without the cache).
$commands+="$cxx /Fo`"$out/tess_pso_test.obj`" `"$PSScriptRoot/tess_pso_test.cpp`" || exit /b 1"
$commands+="link /nologo /OPT:REF /Brepro /OUT:`"$out/tess_pso_test.exe`" `"$out/tess_pso_test.obj`" `"$out/shader_interface.obj`" d3d12.lib dxgi.lib dxguid.lib || exit /b 1"
$commands+="dumpbin /nologo /imports `"$out/game_bootstrap.dll`" >`"$out/imports.txt`" || exit /b 1"
# ChangedUnits is an explicit incremental build; include every translation unit
# affected by an edited header. The normal path remains a complete build.
if($ChangedUnits.Count){
    if(-not $NoControls){throw 'Incremental compilation requires explicit NoControls'}
    foreach($unit in $ChangedUnits){
        if($unit -notmatch '^[a-z][a-z0-9_]*$' -or $unit -notin $objects){throw "Unknown module unit: $unit"}
        if(@($commands | Where-Object { $_.StartsWith('cl ') -and $_.Contains('/Fo"'+$out+'/'+$unit+'.obj"') }).Count -ne 1){throw "Missing compile command: $unit"}
    }
    # Final review M2: the dxc lines always run, and the unit embedding their headers is compiled with them (never a stale shader).
    foreach($embedding in @('raytracing_base','rt_scene','rt_denoise','post_tone_stage','post_glare_programs','output_stage','game_texture_ingress')+$movedProgramUnits){if($embedding -notin $ChangedUnits){$ChangedUnits=@($ChangedUnits)+@($embedding)}}   # I-0117 T2: rt_scene embeds rt_skin
    foreach($unit in $objects | Where-Object { $_ -notin $ChangedUnits }){
        if(-not (Test-Path -LiteralPath "$out/$unit.obj")){throw 'Incremental compilation requires an existing complete build (its objects kept with -KeepObjects, D-0260)'}
    }
    $commands=@($commands | Where-Object {
        $line=$_; $line -eq '@echo off' -or $line.StartsWith('call ') -or $line.StartsWith('dxc ') -or $line.StartsWith('fxc ') -or $line.StartsWith('link /nologo /DLL ') -or
        $line.StartsWith('dumpbin ') -or @($ChangedUnits | Where-Object { $line.StartsWith('cl ') -and $line.Contains('/Fo"'+$out+'/'+$_.ToString()+'.obj"') }).Count
    })
}elseif($NoControls){$commands=@($commands | Where-Object {$_ -notmatch '_test[.]cpp|_test[.]exe'})}
# cmd.exe truncates lines above8191 characters (long worktree paths): pass each
# long link command's arguments through a response file instead.
$rspIndex=0
$steps=@(foreach($line in $commands){   # each step keeps its original line: the Ninja graph reads a long link's objects from it
    if($line.StartsWith('link ') -and $line.Length -gt 4000){
        $arguments=$line.Substring(5) -replace '\s*\|\|\s*exit /b 1$',''
        $rsp="$out/link-$rspIndex.rsp";$rspIndex++
        Set-GeneratedContent $rsp $arguments   # rewritten only when its arguments change: its link depends on it (I-0142)
        [pscustomobject]@{Original=$line;Command="link @`"$rsp`" || exit /b 1";Rsp=$rsp}
    }elseif($line.StartsWith('cl ')){
        # I-0142: every compilation lists its headers (/showIncludes, English notes through VSLANG) in both paths. A /Z7 object records
        # its command line and working folder, so the serial and the parallel build run the same lines in the same folder (the build's)
        # and give the same bytes; Ninja reads the lists, the serial path drops them from its output.
        $listed='cl /showIncludes '+$line.Substring(3)
        [pscustomobject]@{Original=$listed;Command=$listed}
    }elseif($line -eq "call `"$vcvars`" >nul || exit /b 1"){
        [pscustomobject]@{Original=$line;Command=$line}; [pscustomobject]@{Original='set VSLANG=1033';Command='set VSLANG=1033'}
    }else{[pscustomobject]@{Original=$line;Command=$line}}
})
$commands=@($steps.Command)
Set-Content -LiteralPath "$out/build.cmd" -Value $commands -Encoding ascii   # always written: the commands' record, and what -Serial runs
# I-0142: a link updates an existing PDB in place, and /Brepro's identity (the PE time, the PDB's GUID, the repro hash) follows the
# PDB: every build starts from no PDB, so its module does not depend on the folder's earlier builds; in a cache too (the final review's
# M1: each in-place update shifted the module), where the links whose PDB is gone (the module and two tests) run again, seconds.
foreach($pdb in @($steps | ForEach-Object { [regex]::Match($_.Original,'/PDB:"([^"]+)"').Groups[1].Value } | Where-Object { $_ })){
    Remove-Item -LiteralPath $pdb -Force -ErrorAction SilentlyContinue
    if(Test-Path -LiteralPath $pdb){ throw "A PDB could not be removed before the build (locked?): $pdb" } }   # the final review's L5
if($Serial){
    Push-Location -LiteralPath $out
    try{ & cmd /c "`"$out/build.cmd`"" | Where-Object { $_ -notmatch '^Note: including file:' } }finally{ Pop-Location }
    if($LASTEXITCODE){ throw "Native bootstrap build failed: $LASTEXITCODE" }
}else{
    # The same commands as a graph run in parallel by Ninja (the build study of 2026-11-16: build.cmd ran its 443 commands one after
    # another on 16 threads, 10.5 min). Each compilation, shader and link names its output and inputs (parsed from its line, an
    # unplaceable line fails the build); a compilation that reads the output folder's generated headers (/I"$out", /I"$nrdOut") waits
    # for every shader; a link waits for its objects (and its response file). Without -Cache, the log is removed first, so every edge
    # runs, and every output must be newer than the build's start. With -Cache (D-0295), the log and MSVC's own list of each unit's
    # headers (/showIncludes, deps = msvc, English notes through VSLANG) rerun only the commands whose sources, headers or command
    # line changed; a shader also depends on the shared .hlsli and NRD's configuration. Either way Ninja must then find nothing left
    # to do. Ninja 1.13.2 is Visual Studio's own (no download).
    $ninja='C:/Program Files/Microsoft Visual Studio/18/Insiders/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
    if(-not (Test-Path -LiteralPath $ninja)){ throw "Ninja not found ($ninja): reinstall Visual Studio's CMake tools, or build with -Serial" }
    # A path inside the build folder is written relative to it (Ninja runs there, -C): Ninja records a compilation's headers relative to
    # its working folder, and a generated header must be the same node as the command that writes it, or its includers are not rebuilt
    # in the build that rewrites it (the cache series' shader case, I-0142 RESULT).
    $outFull=[IO.Path]::GetFullPath($out).TrimEnd([char[]]'\/')
    function ConvertTo-NinjaPath([string]$path){
        $full=[IO.Path]::GetFullPath($path)
        if($full.StartsWith($outFull+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){ $path=$full.Substring($outFull.Length+1).Replace('\','/') }
        $path.Replace('$','$$').Replace(':','$:').Replace(' ','$ ') }
    $graph=[Collections.Generic.List[string]]::new()
    $graph.Add('# Written by Build-GameBootstrap.ps1: build.cmd''s commands as a graph, run in parallel by Ninja (-Serial runs build.cmd).')
    $graph.Add('ninja_required_version = 1.10')
    $graph.Add("builddir = $($out.Replace('$','$$'))")
    $graph.Add('rule run'); $graph.Add('  command = cmd /d /s /c "$cmd"'); $graph.Add('  description = $desc')
    $graph.Add('rule cc'); $graph.Add('  command = cmd /d /s /c "$cmd"'); $graph.Add('  description = $desc'); $graph.Add('  deps = msvc'); $graph.Add('  msvc_deps_prefix = Note: including file:')
    $shaderSharedFull=@([IO.Path]::GetFullPath("$nrdOut/NRDConfig.hlsli"))   # besides each programme's own includes (NRD's configuration, which NRD.hlsli includes)
    $includeMemo=@{}   # the fix review's M1: each file's resolved includes, once per run and search list
    # Step review M1, M2: the toolchain's versions (written by build-ninja.cmd before Ninja starts; Ninja's header lists leave out
    # Visual Studio's and the Windows SDK's headers) and the third-party pins (headers outside the repository, the shaders' SHaRC, NRD,
    # MathLib and RTXDI among them) are inputs of every command: a Visual Studio, Windows SDK or pin change reruns everything.
    $toolchainStamp="$out/toolchain.stamp"; $pinsStamp="$out/third-party.stamp"
    Set-GeneratedContent $pinsStamp (@("minhook $minHook $((git -C $minHook rev-parse HEAD).Trim())","nrd $nrd $((git -C $nrd rev-parse HEAD).Trim())","streamline $streamline")+
        @(@(Get-ChildItem -LiteralPath $sharc -File)+@(Get-ChildItem -LiteralPath "$streamline/include" -Recurse -File)+@(Get-Item -LiteralPath "$nrdDownload/mathlib-v11.zip","$nrdDownload/shadermake-18f5a344.zip",$rtxdiZip) | ForEach-Object { "$($_.FullName) $((Get-FileHash -LiteralPath $_.FullName).Hash)" }))
    $stamps="$(ConvertTo-NinjaPath $toolchainStamp) $(ConvertTo-NinjaPath $pinsStamp)"
    $edgeOutputs=[Collections.Generic.List[string]]::new(); $shaderOutputs=[Collections.Generic.List[string]]::new(); $compiledOutputs=[Collections.Generic.List[string]]::new(); $quotedIncludes=@{}; $pdbOutputs=[Collections.Generic.List[string]]::new()
    foreach($step in $steps){
        $line=$step.Original
        if($line -eq '@echo off' -or $line -eq "call `"$vcvars`" >nul || exit /b 1" -or $line -eq 'set VSLANG=1033' -or $line -eq 'dxc --version || exit /b 1'){ continue }   # the wrapper below runs these
        if($line.Contains('%')){ throw "A build command with % (a batch file and cmd /c read it differently): $line" }   # step review L2
        $waitsForShaders=$false; $rule='run'; $implicit=$stamps; $sideOutput=''; $command=$step.Command -replace '\s*\|\|\s*exit /b 1$',''
        if($line.StartsWith('cl ')){
            $rule='cc'   # its line already lists its headers (/showIncludes, above)
            $output=[regex]::Match($line,'/Fo"([^"]+)"').Groups[1].Value
            $inputs=@([regex]::Matches($line,'"([^"]+\.(?:c|cpp))"') | ForEach-Object { $_.Groups[1].Value })
            $waitsForShaders=$line.Contains("/I`"$out`"") -or $line.Contains("/I`"$nrdOut`""); $compiledOutputs.Add($output)
            $quotedIncludes[$output]=[bool](Select-String -LiteralPath $inputs[0] -Pattern '^\s*#\s*include\s*"' -Quiet)   # a header of its own: its list cannot be empty
        }elseif($line.StartsWith('dxc ') -or $line.StartsWith('fxc ')){
            $output=[regex]::Match($line,'[-/]Fh "([^"]+)"').Groups[1].Value
            $inputs=@([regex]::Matches($line,'"([^"]+\.hlsl)"') | ForEach-Object { $_.Groups[1].Value })
            # The final review's M2: every file the programme includes, followed through its includes (the including file's folder,
            # then the command's -I folders, as dxc and fxc search), is an input. An include found nowhere is skipped (the fix review's
            # L2: one under a conditional the compiler does not take; one it takes fails the compilation itself).
            if($inputs.Count -ne 1){ throw "Build command without one output and its inputs: $line" }   # the fix review's L3
            $searchDirs=@([regex]::Matches($line,'(?:-I|/I)\s*"([^"]+)"') | ForEach-Object { $_.Groups[1].Value })
            $included=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase); $pending=[Collections.Generic.Stack[string]]::new(); $pending.Push([IO.Path]::GetFullPath($inputs[0]))
            while($pending.Count){ $file=$pending.Pop(); $key="$file|$($searchDirs -join '|')"
                if(-not $includeMemo.ContainsKey($key)){
                    $includeMemo[$key]=@(foreach($match in [regex]::Matches([IO.File]::ReadAllText($file),'(?m)^\s*#\s*include\s*[<"]([^>"]+)[>"]')){ $name=$match.Groups[1].Value
                        $found=@(@([IO.Path]::GetDirectoryName($file))+$searchDirs | ForEach-Object { Join-Path $_ $name } | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1)
                        if($found){ [IO.Path]::GetFullPath($found[0]) } }) }
                foreach($full in $includeMemo[$key]){ if($included.Add($full)){ $pending.Push($full) } } }
            foreach($shared in $shaderSharedFull){ [void]$included.Add($shared) }
            $shaderOutputs.Add($output); $implicit+=(@($included | Sort-Object | ForEach-Object { ' '+(ConvertTo-NinjaPath $_) }) -join '')
        }elseif($line.StartsWith('link ')){
            $output=[regex]::Match($line,'/OUT:"([^"]+)"').Groups[1].Value
            $inputs=@([regex]::Matches($line,'"([^"]+\.obj)"') | ForEach-Object { $_.Groups[1].Value })
            if($step.Rsp){ $implicit+=" $(ConvertTo-NinjaPath $step.Rsp)" }
            $pdb=[regex]::Match($line,'/PDB:"([^"]+)"').Groups[1].Value   # step review L3: the PDB a link writes is its output too
            if($pdb){ if($edgeOutputs.Contains($pdb)){ throw "Two build commands write $pdb" }; $sideOutput=" | $(ConvertTo-NinjaPath $pdb)"; $edgeOutputs.Add($pdb); $pdbOutputs.Add($pdb) }
        }elseif($line.StartsWith('dumpbin ')){
            $output=[regex]::Match($line,'>"([^"]+)"').Groups[1].Value
            $inputs=@([regex]::Matches($line,'"([^"]+\.dll)"') | ForEach-Object { $_.Groups[1].Value })
        }else{ throw "Build command Ninja cannot place: $line" }
        if(-not $output -or -not $inputs.Count -or ($inputs.Count -ne 1 -and -not $line.StartsWith('link '))){ throw "Build command without one output and its inputs: $line" }
        if($edgeOutputs.Contains($output)){ throw "Two build commands write $output" }
        $edgeOutputs.Add($output)
        $graph.Add("build $(ConvertTo-NinjaPath $output)$($sideOutput): $rule $(@($inputs | ForEach-Object { ConvertTo-NinjaPath $_ }) -join ' ') | $implicit$(if($waitsForShaders){' || shaders'})")
        $graph.Add("  cmd = $($command.Replace('$','$$'))")
        $graph.Add("  desc = $([IO.Path]::GetFileName($output))")
    }
    $graph.Add("build shaders: phony $(@($shaderOutputs | ForEach-Object { ConvertTo-NinjaPath $_ }) -join ' ')")
    $graph.Add("default $(@($edgeOutputs | ForEach-Object { ConvertTo-NinjaPath $_ }) -join ' ')")
    Set-Content -LiteralPath "$out/build.ninja" -Value $graph -Encoding ascii
    # No log without a cache: every edge runs. A cache whose log was written by another form of this graph (its nodes spelled otherwise)
    # starts again from no log: cleandead would take its outputs, under their old spellings, for dead ones (the cache series, I-0142).
    $graphForm='I-0142 graph 2: outputs relative to the build folder'
    if(-not $Cache -or -not (Test-Path -LiteralPath "$out/.ninja-graph-form") -or (Get-Content -LiteralPath "$out/.ninja-graph-form" -Raw).Trim() -ne $graphForm){
        Remove-Item -LiteralPath "$out/.ninja_log","$out/.ninja_deps" -Force -ErrorAction SilentlyContinue }
    if($Cache){ Set-Content -LiteralPath "$out/.ninja-graph-form" -Encoding ascii -Value $graphForm }
    # The toolchain stamp (the re-review's M2, L1; the third review's L4): in vcvars' environment, each tool's path, and every file of
    # the tools' folders (their DLLs: c1xx, c2, dxcompiler...) and of the Visual Studio and Windows SDK include and x64 library folders
    # by size and date (an in-place servicing update changes them);
    # a missing tool or folder fails the build. Rewritten only when it changes.
    $stampWin=$toolchainStamp.Replace('/','\')   # cmd's fc, copy and del read / as a switch
    Set-Content -LiteralPath "$out/toolchain-stamp.ps1" -Encoding ascii -Value @('param([string]$Path)','$ErrorActionPreference=''Stop''',
        '$lines=[Collections.Generic.List[string]]::new(); $lines.Add("VC $env:VCToolsVersion"); $lines.Add("SDK $env:WindowsSDKVersion $env:WindowsSDKLibVersion")',
        '$toolFolders=[Collections.Generic.List[string]]::new()',
        'foreach($tool in ''cl'',''link'',''dumpbin'',''dxc'',''fxc''){ $item=Get-Item -LiteralPath (Get-Command "$tool.exe" -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source; $lines.Add("$tool $($item.FullName)"); if(-not $toolFolders.Contains($item.DirectoryName)){ $toolFolders.Add($item.DirectoryName) } }',
        'foreach($folder in @($toolFolders)+@((Join-Path $env:WindowsSdkDir "Include\$env:WindowsSDKVersion"),(Join-Path $env:WindowsSdkDir "Lib\$($env:WindowsSDKLibVersion)um\x64"),(Join-Path $env:WindowsSdkDir "Lib\$($env:WindowsSDKLibVersion)ucrt\x64"),(Join-Path $env:VCToolsInstallDir ''include''),(Join-Path $env:VCToolsInstallDir ''lib\x64''))){',
        '    if(-not (Test-Path -LiteralPath $folder)){ throw "Toolchain folder missing: $folder" }',
        '    foreach($file in @(Get-ChildItem -LiteralPath $folder -Recurse -File | Sort-Object FullName)){ $lines.Add("$($file.FullName) $($file.Length) $($file.LastWriteTimeUtc.Ticks)") } }',
        'Set-Content -LiteralPath $Path -Encoding utf8 -Value $lines')
    Set-Content -LiteralPath "$out/build-ninja.cmd" -Encoding ascii -Value @('@echo off',"call `"$vcvars`" >nul || exit /b 1",'set VSLANG=1033','dxc --version || exit /b 1',
        "pwsh -NoProfile -File `"$($out.Replace('/','\'))\toolchain-stamp.ps1`" `"$stampWin.new`" || exit /b 1",
        "fc /b `"$stampWin.new`" `"$stampWin`" >nul 2>&1 || copy /y `"$stampWin.new`" `"$stampWin`" >nul || exit /b 1","del `"$stampWin.new`"",
        "`"$ninja`" --version || exit /b 1")
    # Step review L2: in a cache, the outputs of commands no longer in the graph (a removed shader's header, a -NoControls build's
    # tests) are deleted first, so a stale one can neither be included nor published. Ninja runs in the build's folder (the
    # re-review's M1: its header lists are relative to its working folder, which must not be another checkout).
    if($Cache){ Add-Content -LiteralPath "$out/build-ninja.cmd" -Encoding ascii -Value "`"$ninja`" -C `"$out`" -f `"$out/build.ninja`" -t cleandead || exit /b 1" }
    Add-Content -LiteralPath "$out/build-ninja.cmd" -Encoding ascii -Value "`"$ninja`" -C `"$out`" -f `"$out/build.ninja`" -j $Jobs || exit /b 1"
    if($Cache){   # the re-review's L5: a header this script no longer writes and no command outputs is removed, as a complete build lacks it
        $kept=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach($path in @($edgeOutputs)+@($generated)){ [void]$kept.Add([IO.Path]::GetFullPath($path)) }
        Get-ChildItem -LiteralPath $out,$nrdOut -File | Where-Object { $_.Extension -in '.h','.inc','.hlsli','.rsp' -and -not $kept.Contains($_.FullName) } | Remove-Item -Force }
    $buildStart=[DateTime]::UtcNow
    & cmd /c "`"$out/build-ninja.cmd`""
    if($LASTEXITCODE){ throw "Native bootstrap build failed: $LASTEXITCODE" }
    $fresh=@($edgeOutputs | Where-Object { (Test-Path -LiteralPath $_) -and (Get-Item -LiteralPath $_).LastWriteTimeUtc -ge $buildStart.AddSeconds(-2) })
    if(-not $Cache -and $fresh.Count -ne $edgeOutputs.Count){ throw "Build outputs not written by this build: $($edgeOutputs.Count-$fresh.Count) of $($edgeOutputs.Count)" }
    $ran=@($fresh | Where-Object { -not $pdbOutputs.Contains($_) }).Count; $commandCount=$edgeOutputs.Count-$pdbOutputs.Count   # the final review's L2: commands, not outputs
    $left=@(& $ninja -C $out -f "$out/build.ninja" -n 2>&1)
    if($LASTEXITCODE -or -not ($left -match '^ninja: no work to do')){ throw "Ninja finds work left after the build: $($left | Select-Object -First 3)" }
    # Every compilation's header list recorded (a compiler speaking another language would leave them empty: no rebuild on a header).
    $deps=@{}
    foreach($row in @(& $ninja -C $out -f "$out/build.ninja" -t deps 2>&1)){ if("$row" -match '^(.+): #deps (\d+), deps mtime -?\d+ \((\w+)\)$'){ $deps[[IO.Path]::GetFullPath($Matches[1],$outFull)]=@([int]$Matches[2],$Matches[3]) } }
    # A unit including only system headers (Ninja leaves those out) has an empty list: its record must still be there and valid.
    $unlisted=@($compiledOutputs | Where-Object { $d=$deps[[IO.Path]::GetFullPath($_)]; -not $d -or $d[1] -ne 'VALID' -or ($d[0] -lt 1 -and $quotedIncludes[$_]) })
    if($unlisted.Count){ throw "MSVC header lists missing for $($unlisted.Count) of $($compiledOutputs.Count) compilations, first $($unlisted[0])" }
    Write-Output ("Ninja build: {0} of {1} commands run{2}, -j {3}, {4:N0} s" -f $ran,$commandCount,$(if($Cache){" (cache $out)"}else{''}),$Jobs,([DateTime]::UtcNow-$buildStart).TotalSeconds)
}
if(Select-String -LiteralPath "$out/imports.txt" -Pattern 'd3d11.dll' -SimpleMatch){ throw 'D3D11 import in native module' }
# I-0100 step review M1: the module takes DXGI from the system library only (backend.cpp SystemFactory); an import would bind to a
# proxy dxgi.dll in the game's folder.
if(Select-String -LiteralPath "$out/imports.txt" -Pattern 'dxgi.dll' -SimpleMatch){ throw 'DXGI import in native module' }
# I-0132 R12h step 2: no run-time HLSL compilation (a function's row; the DLL's own row, D3DCOMPILER_47.dll, is upper case).
if(Select-String -LiteralPath "$out/imports.txt" -Pattern ' D3DCompile' -SimpleMatch -CaseSensitive){ throw 'Run-time HLSL compilation in the native module (I-0132 R12h step 2: its programmes are built by fxc)' }
# I-0111: Streamline's DLLs beside the module (loaded at every start unless no-dlss.flag); a copy differing from the SDK's is replaced.
foreach($dll in $streamlineDlls){$source="$streamline/bin/x64/$dll";$target="$out/$dll"
    if(-not (Test-Path -LiteralPath $source)){throw "Streamline DLL missing: $dll"}
    if(-not (Test-Path -LiteralPath $target) -or (Get-FileHash -LiteralPath $target).Hash -ne (Get-FileHash -LiteralPath $source).Hash){
        # D-0260: a hard link to the SDK's file (no copy: ~113 MB a build).
        if(Test-Path -LiteralPath $target){Remove-Item -LiteralPath $target -Force}
        # A copy when the link fails (another volume behind a junction, NTFS's 1023 links per file). The SDK's file and the build's are
        # one file: nothing may write into a build folder's Streamline DLLs (the launchers only copy them out).
        try{ New-Item -ItemType HardLink -Path $target -Target $source | Out-Null }
        catch{ Copy-Item -LiteralPath $source -Destination $target -Force }}}
# I-0132 R11f (Thomas, 2026-11-09 04:48): the Agility SDK 1.620.1's runtime in the module's D3D12 folder (backend.cpp: the device
# factory, by default since I-0132 R11i, no-agility.flag the inbox runtime): its x64 D3D12Core.dll and d3d12SDKLayers.dll (the debug layer's), pinned by hash and checked signed
# by Microsoft, hard-linked as Streamline's (D-0260). Never in the game folder.
$agility='C:/Users/User/Tools/DS3-Decompilation/third-party/agility-1.620.1/pkg/build/native/bin/x64'
New-Item -ItemType Directory -Force -Path "$out/D3D12" | Out-Null
foreach($pin in @(@('D3D12Core.dll','DE33D6921AE9284F789682475793207051E129F67E305DDA9AD06D5748FB8324'),@('d3d12SDKLayers.dll','75427E41321C80CB7EE804AC9BD8566F77F4519B38BB485EF27E17D3F98FA465'))){
    $source="$agility/$($pin[0])";$target="$out/D3D12/$($pin[0])"
    if((Get-FileHash -LiteralPath $source).Hash -ne $pin[1]){ throw "Agility SDK pin mismatch: $($pin[0])" }
    $signature=Get-AuthenticodeSignature -FilePath $source
    if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch '^CN=Microsoft Corporation,'){ throw "Agility SDK signature: $($pin[0])" }
    if(-not (Test-Path -LiteralPath $target) -or (Get-FileHash -LiteralPath $target).Hash -ne $pin[1]){
        if(Test-Path -LiteralPath $target){Remove-Item -LiteralPath $target -Force}
        try{ New-Item -ItemType HardLink -Path $target -Target $source | Out-Null }
        catch{ Copy-Item -LiteralPath $source -Destination $target -Force }}}
if($NoControls){
    Write-Output "Build only: changed units [$($ChangedUnits -join ',')]; unchanged control suites not run."
    Publish-CacheBuild
    Clear-BuildObjects
    Get-FileHash -LiteralPath "$out/game_bootstrap.dll","$out/game_bootstrap_ctl.exe" -Algorithm SHA256
    return
}
& "$out/game_primitive_programs_test.exe" $exe *> "$out/primitive-programs.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/primitive-programs.txt";throw 'Installed primitive programmes verification failed'}
Get-Content -LiteralPath "$out/primitive-programs.txt"
& "$out/game_primitive_uniforms_test.exe" $exe *> "$out/primitive-uniforms.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/primitive-uniforms.txt";throw 'Installed UI uniform source verification failed'}
Get-Content -LiteralPath "$out/primitive-uniforms.txt"
& "$out/game_primitive_geometry_test.exe" *> "$out/primitive-geometry.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/primitive-geometry.txt";throw 'CPU primitive geometry verification failed'}
Get-Content -LiteralPath "$out/primitive-geometry.txt"
& "$out/game_primitive_mesh_source_test.exe" *> "$out/primitive-mesh-source.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/primitive-mesh-source.txt";throw 'CPU staged primitive source verification failed'}
Get-Content -LiteralPath "$out/primitive-mesh-source.txt"
& "$out/game_ui_mesh_output_test.exe" *> "$out/ui-mesh-output.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/ui-mesh-output.txt";throw 'Owned CPU mesh output verification failed'}
Get-Content -LiteralPath "$out/ui-mesh-output.txt"
& "$out/game_primitive_image_source_test.exe" *> "$out/primitive-image-source.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/primitive-image-source.txt";throw 'CPU primitive image source verification failed'}
Get-Content -LiteralPath "$out/primitive-image-source.txt"
& "$out/shadow_state_test.exe" "$root/evidence/I-0068/queries/shadow-ready/shadow-world-blocks.tsv" "$root/investigations/I-0066-independent-native-startup/shadow-sl5-oracle.tsv" *> "$out/shadow-state.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/shadow-state.txt";throw 'Shadow state verification failed'}
Get-Content -LiteralPath "$out/shadow-state.txt"
# I-0111 T-U2, T-U3, T-U5, T-U7, T-U13: the upscaler's jitter, the camera words and the unjittered vectors.
& "$out/upscaler_test.exe" *> "$out/upscaler.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/upscaler.txt";throw 'Upscaler verification failed'}
Get-Content -LiteralPath "$out/upscaler.txt"
# W5d GL-1 T1: the glow constant images, the tap words against the executable.
& "$out/glow_math_test.exe" $exe *> "$out/glow-math.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/glow-math.txt";throw 'Glow constant verification failed'}
Get-Content -LiteralPath "$out/glow-math.txt"
# W5d GL-2 T2: the glow bank reader on the admitted default.gparam (I-0067 extraction in the evidence store).
$env:DS3_EXE=$exe   # I-0141 tranche 3: its light-set records (Banks::Sun) need the executable's constants through Rdata, as yebis_blocks_test below
& "$out/glow_bank_test.exe" "$root/evidence/I-0067/queries/status-light" *> "$out/glow-bank.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/glow-bank.txt";throw 'Glow bank verification failed'}
Get-Content -LiteralPath "$out/glow-bank.txt"
# W10 YB-2 T3: the world technique pins against the executable's DXBC containers.
& "$out/yebis_pins_test.exe" $exe *> "$out/yebis-pins.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/yebis-pins.txt";throw 'YEBIS pins verification failed'}
Get-Content -LiteralPath "$out/yebis-pins.txt"
# W5f YB-1 T1/T2: the YEBIS bank blocks against the golden and the typed accessor on default.gparam.
# W10 YB-5 T6: DS3_EXE (set above, before glow_bank_test) gives CustomDofTable the executable constants through Rdata.
& "$out/yebis_blocks_test.exe" "$root/evidence/I-0068/queries/yebis/zone-gparam" "$root/evidence/I-0068/queries/yebis-ready/bank-blocks.tsv" "$root/evidence/I-0067/queries/status-light" "$root/evidence/I-0068/queries/ffx-ready/bank-blocks.tsv" "$root/evidence/I-0068/queries/yebis-second/second-blocks.tsv" *> "$out/yebis-blocks.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/yebis-blocks.txt";throw 'YEBIS bank block verification failed'}
Get-Content -LiteralPath "$out/yebis-blocks.txt"
& "$out/yebis_tonemap_test.exe" $exe *> "$out/yebis-tonemap.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/yebis-tonemap.txt";throw 'YEBIS tone-map constants verification failed'}
Get-Content -LiteralPath "$out/yebis-tonemap.txt" | Select-Object -Last 1
& "$out/post_tone_test.exe" $exe *> "$out/post-tone.txt"   # I-0138 tranche 1
if($LASTEXITCODE -or (Select-String -LiteralPath "$out/post-tone.txt" -Pattern 'skipped' -SimpleMatch -Quiet)){Get-Content -LiteralPath "$out/post-tone.txt";throw 'Post tone parameter verification failed or skipped'}   # the integrity review: no pass on a skip
Get-Content -LiteralPath "$out/post-tone.txt" | Select-Object -Last 1
& "$out/menu_tonemap_test.exe" *> "$out/menu-tonemap.txt"   # I-0113 tranche 27
if($LASTEXITCODE){Get-Content -LiteralPath "$out/menu-tonemap.txt";throw 'menu tone map verification failed'}
Get-Content -LiteralPath "$out/menu-tonemap.txt" | Select-Object -Last 1
& "$out/auto_exposure_test.exe" *> "$out/auto-exposure.txt"   # I-0132 R11h
if($LASTEXITCODE){Get-Content -LiteralPath "$out/auto-exposure.txt";throw 'auto exposure verification failed'}
Get-Content -LiteralPath "$out/auto-exposure.txt" | Select-Object -Last 1
& "$out/auto_exposure_stage_test.exe" *> "$out/auto-exposure-stage.txt"   # I-0132 R11h: the kernels on WARP
if($LASTEXITCODE){Get-Content -LiteralPath "$out/auto-exposure-stage.txt";throw 'auto exposure stage verification failed'}
Get-Content -LiteralPath "$out/auto-exposure-stage.txt" | Select-Object -Last 1
& "$out/taa_test.exe" *> "$out/taa.txt"   # I-0132 R11j: the TAA on WARP
if($LASTEXITCODE){Get-Content -LiteralPath "$out/taa.txt";throw 'taa verification failed'}
Get-Content -LiteralPath "$out/taa.txt" | Select-Object -Last 1
& "$out/effect_scan_test.exe" *> "$out/effect-scan.txt"   # I-0113 tranche 29
if($LASTEXITCODE){Get-Content -LiteralPath "$out/effect-scan.txt";throw 'effect scan verification failed'}
Get-Content -LiteralPath "$out/effect-scan.txt" | Select-Object -Last 1
# W10 GL-4: the glare zone captures (draws and library state of one frame each: m37, m31, m50, m45; W10 closure: 1705 m40, 1706 m30).
$glareZones=@('i66-reference-1649-m37-glarestate','i66-reference-1650-m31-glarestate','i66-reference-1651-m50-glarestate','i66-reference-1652-m45-glarestate','i66-reference-1705-m40-yebis','i66-reference-1706-m30-yebis') | ForEach-Object {"$PSScriptRoot/../../evidence/I-0066/artifacts/$_"}
# I-0138 tranche 8f: and GlareShape 3 (« Standard »), the original's 47 draws of cutscene 32000020 at two moments (frames 240, 900).
$glareStandardZones=@('i138-t8e-oracle-240','i138-t8e-oracle-900') | ForEach-Object {"$PSScriptRoot/../../evidence/I-0066/artifacts/$_"}
# I-0138 tranche 8i: GlareShape 3 with its shafts, the original's 57 draws of cutscene 32000020 in cut 1 (1250 library frames after the handing).
$glareStandardShaftZones=@(@('i138-t8i-oracle-shafts-1250') | ForEach-Object {"$PSScriptRoot/../../evidence/I-0066/artifacts/$_"})   # @() kept: one zone
# The captures whose light shafts carry light (i138-t8i-oracle-sun-<delay>), listed by hand once read: the tests require content in their
# shaft draws by the name (post_draws_test, glare_stage_test: « -sun- »); a capture that missed the light is left out of this list.
$sunShaftZones=@()
# The draws replayed on a shafts capture: every draw before its last, from the capture's own files (i138-t8i-oracle-shafts-1250: 0 to 55).
# Tranche 8i (c7e0ac515's step review, L2): the last captured draw must be the tone map (its reference_yebis_draw row, the one before its
# reference_yebis_draw_capture row in startup.jsonl, as glare_capture::DrawRows pairs them), else the build fails.
function Get-ShaftCaptureDraws([string]$zone){$last=@(Get-ChildItem -LiteralPath $zone -File -Filter 'reference-draw-*' | ForEach-Object {if($_.Name -match '^reference-draw-([0-9]+)-'){[int]$Matches[1]}} | Sort-Object -Unique)[-1]
    if($null -eq $last -or $last -lt 1){throw "no reference draws in $zone"}
    $row='';$pass=''
    foreach($l in [System.IO.File]::ReadLines("$zone/startup.jsonl")){if($l.Contains('"event":"reference_yebis_draw"')){$row=$l}elseif($l.Contains('"event":"reference_yebis_draw_capture"') -and $l -match '"draw":([0-9]+)' -and [int]$Matches[1] -eq $last){if($row -match '"pass":"([^"]*)"'){$pass=$Matches[1]}}}
    if(-not $pass.StartsWith('tech_Tonemap')){throw "the last captured draw ($last) of $zone is '$pass', not the tone map"}
    (0..($last-1)) -join ','}
$glareStandardShaftZones=@($glareStandardShaftZones)+@($sunShaftZones | ForEach-Object {"$PSScriptRoot/../../evidence/I-0066/artifacts/$_"})
# Tranche 8i (the step review's H1): a second shape-3 shafts frame, the slot's sun on screen but its scene under the bright pass's threshold, so
# its surfaces zero from the bright pass on (i138-t8i2-oracle-sun-2120): its words (yebis_glare_test, yebis_shafts_test) and the lit replay below, not its texels (its mask's depth gate closed).
# Its name says « -sun- » (the runner's) but its shafts carry no light: never in $sunShaftZones nor a texel replay; in the lit replay.
$shaftWordZones=@(@('i138-t8i2-oracle-sun-2120') | ForEach-Object {"$PSScriptRoot/../../evidence/I-0066/artifacts/$_"})   # @() kept: one zone
& "$out/yebis_glare_test.exe" $exe @glareZones @glareStandardZones @glareStandardShaftZones @shaftWordZones *> "$out/yebis-glare.txt"
if($LASTEXITCODE -or (Select-String -LiteralPath "$out/yebis-glare.txt" -Pattern 'PASS yebis glare: skipped' -SimpleMatch -Quiet)){Get-Content -LiteralPath "$out/yebis-glare.txt";throw 'YEBIS glare plan verification failed or skipped'}   # tranche 8i's final review, L3: its own skip line (the gate line's « skipped » is a case, not a skip)
Get-Content -LiteralPath "$out/yebis-glare.txt" | Select-Object -Last 1
# W10 LS-1: Alpha_R's block against the shaft-state captures (Reference1660 m37).
$shaftZones=@(@('i66-reference-1660-m37-shaftstate','i66-reference-1661-m31-shaftstate','i66-reference-1662-m50-shaftstate','i66-reference-1663-m45-shaftstate','i66-reference-1664-m37-shaftplan','i66-reference-1665-m31-shaftplan','i66-reference-1666-m50-shaftplan','i66-reference-1667-m37-shaftdriver','i66-reference-1668-m31-shaftdriver','i66-reference-1669-m50-shaftdriver','i66-reference-1705-m40-yebis','i66-reference-1706-m30-yebis') | ForEach-Object {"$PSScriptRoot/../../evidence/I-0066/artifacts/$_"})
& "$out/yebis_shafts_test.exe" $exe @shaftZones @glareStandardShaftZones @shaftWordZones *> "$out/yebis-shafts.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/yebis-shafts.txt";throw 'YEBIS light-shaft verification failed'}
Get-Content -LiteralPath "$out/yebis-shafts.txt" | Select-Object -Last 2
# W10 DF-1/DF-2: the DoF state captures (Reference1676 m37, 1677 m31, 1678 m50; W10 closure: 1705 m40, whose nine-tap levels draw the 25-tap blur, 1706 m30).
$dofZones=@('i66-reference-1676-m37-dofstate','i66-reference-1677-m31-dofstate','i66-reference-1678-m50-dofstate','i66-reference-1705-m40-yebis','i66-reference-1706-m30-yebis') | ForEach-Object {"$PSScriptRoot/../../evidence/I-0066/artifacts/$_"}
& "$out/yebis_dof_test.exe" $exe @dofZones *> "$out/yebis-dof.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/yebis-dof.txt";throw 'YEBIS depth-of-field verification failed'}
Get-Content -LiteralPath "$out/yebis-dof.txt" | Select-Object -Last 1
# W10 MB-1: the motion blur's plan against the same captures (draws 11-13 and the library at Alpha_R).
$mbZones=@($dofZones)+@("$PSScriptRoot/../../evidence/I-0066/artifacts/i66-reference-1693-m37-motionblur-turn2400")   # MB review: a turning camera (draws capture at frame 2400)
& "$out/yebis_motionblur_test.exe" @mbZones *> "$out/yebis-motionblur.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/yebis-motionblur.txt";throw 'YEBIS motion blur verification failed'}
Get-Content -LiteralPath "$out/yebis-motionblur.txt" | Select-Object -Last 1
# W5e T-Q5/T-Q8/T-Q9: the GX04/GX90 parse, cbPNTriangles against the golden and cbTessScene.
& "$out/tess_constants_test.exe" "$root/evidence/I-0068/queries/water-ready/water-tess-blocks.tsv" *> "$out/tess-constants.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/tess-constants.txt";throw 'Tessellation constant verification failed'}
Get-Content -LiteralPath "$out/tess-constants.txt"
# I-0102 lot 2f: the decal constants (the embedded decal oracle), the GPARAM
# reader's refusals, the host's memory-only input (run from the repository: it reads native/content as a memory root).
foreach($control in @('decal_constants_test','gparam_test')){
    & "$out/$control.exe" *> "$out/$control.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/$control.txt";throw "Constant or reader verification failed: $control"}
    Get-Content -LiteralPath "$out/$control.txt" | Select-Object -Last 1
}
Push-Location -LiteralPath $root
try{& "$out/host_input_test.exe" *> "$out/host_input_test.txt";$hostInput=$LASTEXITCODE}finally{Pop-Location}
if($hostInput){Get-Content -LiteralPath "$out/host_input_test.txt";throw "Host memory input verification failed: $hostInput"}
Get-Content -LiteralPath "$out/host_input_test.txt"
# 1255: the GX quality object writer against the I-0067 oracle images (QUALITY-OBJECT-NATIVE Q1/Q3).
& "$out/game_quality_object_test.exe" "$root/investigations/I-0067-static-menus-subsystems-map/quality-table.tsv" *> "$out/quality-object.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/quality-object.txt";throw 'Quality object verification failed'}
Get-Content -LiteralPath "$out/quality-object.txt"
& "$out/sample_profile_test.exe" "$out/sample-profile-test.txt" *> "$out/sample-profile.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/sample-profile.txt";throw 'Sampling profiler verification failed'}
& "$out/sample_symbolize.exe" "$out/sample-profile-test.txt" "$out/sample-profile-test-report.txt" sample_profile_test.exe *>> "$out/sample-profile.txt"
$named=Get-Content -LiteralPath "$out/sample-profile-test-report.txt" -Raw
if($LASTEXITCODE -or $named -notmatch 'symbols: pdb, lines' -or $named -notmatch 'SampleTestLeaf' -or $named -notmatch 'SampleTestMiddle'){Get-Content -LiteralPath "$out/sample-profile.txt";throw 'Sample symbolizer verification failed'}
Get-Content -LiteralPath "$out/sample-profile.txt"
foreach($control in @('register_plan_test','collect_profile_test','frame_store_test','render_worker_test','frame_graph_test','display_curve_test','frame_pacer_test','content_hash_test','fade_dither_test','model_pose_test','gx_mtd_image_test','ffx_observe_test','gparam_listing_test','logic_rate_test','dlss_mode_test','frame_rate_option_test','dark_logo_test','rekindled_settings_test','graphics_rows_test','options_film_test','font_raster_test','overlay_text_test','view_blend_test','env_region_test','sort_key_test','rt_probes_test')){
    & "$out/$control.exe" *> "$out/$control.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/$control.txt";throw "Workstream K control failed: $control"}
    Get-Content -LiteralPath "$out/$control.txt"
}
& "$out/raytracing_base_test.exe" *> "$out/raytracing-base.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/raytracing-base.txt";throw 'Ray-tracing base verification failed'}
Get-Content -LiteralPath "$out/raytracing-base.txt" | Select-Object -Last 1
& "$out/rt_scene_test.exe" *> "$out/rt-scene.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/rt-scene.txt";throw 'Ray-tracing scene verification failed'}
Get-Content -LiteralPath "$out/rt-scene.txt" | Select-Object -Last 1
& "$out/original_image_test.exe" $exe *> "$out/original-image.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/original-image.txt";throw 'Original image identity verification failed'}
Get-Content -LiteralPath "$out/original-image.txt" | Select-Object -Last 1
# I-0137 (A1): the launchers' finder of the game (Steam's library list and manifest), on a made-up Steam folder.
& pwsh -NoProfile -File "$PSScriptRoot/../../tools/Find-DS3Game.ps1" -SelfTest *> "$out/find-ds3game.txt"
if($LASTEXITCODE -or -not (Select-String -LiteralPath "$out/find-ds3game.txt" -Pattern '^PASS Find-DS3Game' -Quiet)){Get-Content -LiteralPath "$out/find-ds3game.txt";throw 'The game finder self-test failed'}
Get-Content -LiteralPath "$out/find-ds3game.txt" | Select-Object -Last 1
& "$out/crash_dump_test.exe" *> "$out/crash-dump.txt"
if($LASTEXITCODE){Get-Content -LiteralPath "$out/crash-dump.txt";throw 'Crash dump and device removal verification failed'}
Get-Content -LiteralPath "$out/crash-dump.txt" | Select-Object -Last 1
foreach($profile in @('warp','hardware')) {
    & "$out/startup_primitive_test.exe" $profile *> "$out/startup-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/startup-$profile.txt";throw "Native startup primitive verification failed: $profile"}
    $startupPass=@(Get-Content -LiteralPath "$out/startup-$profile.txt" | Where-Object {$_ -like 'PASS *'})
    if($startupPass.Count -ne 1){throw 'Startup control success receipt missing'}
    Set-Content -LiteralPath "$out/startup-$profile.txt" -Encoding utf8NoBOM -Value $startupPass
    Get-Content -LiteralPath "$out/startup-$profile.txt"
    & "$out/frame_ring_test.exe" $profile *> "$out/frame-ring-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/frame-ring-$profile.txt";throw "Frame ring verification failed: $profile"}
    Get-Content -LiteralPath "$out/frame-ring-$profile.txt"
    & "$out/frame_presenter_test.exe" $profile *> "$out/frame-presenter-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/frame-presenter-$profile.txt";throw "Frame presenter verification failed: $profile"}
    Get-Content -LiteralPath "$out/frame-presenter-$profile.txt"
    & "$out/pipeline_cache_test.exe" $profile *> "$out/pipeline-cache-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/pipeline-cache-$profile.txt";throw "Pipeline cache verification failed: $profile"}
    Get-Content -LiteralPath "$out/pipeline-cache-$profile.txt"
    & "$out/hdr_output_test.exe" $profile *> "$out/hdr-output-$profile.txt"   # I-0138 tranche 5: the numbers, the block's layout and the stage's draw
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/hdr-output-$profile.txt";throw "HDR output verification failed: $profile"}
    Get-Content -LiteralPath "$out/hdr-output-$profile.txt"
    & "$out/display_stage_test.exe" $profile $exe *> "$out/display-stage-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/display-stage-$profile.txt";throw "Display stage verification failed: $profile"}
    Get-Content -LiteralPath "$out/display-stage-$profile.txt"
    & "$out/glow_stage_test.exe" $profile 'C:/Users/User/.codex/I0063-cache/programs' *> "$out/glow-stage-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/glow-stage-$profile.txt";throw "Glow stage verification failed: $profile"}
    Get-Content -LiteralPath "$out/glow-stage-$profile.txt"
    & "$out/tonemap_stage_test.exe" $profile $exe "$PSScriptRoot/../../evidence/I-0066/artifacts/i66-reference-1643-m37-tonemapcapture" "$out/tonemap-stage-$profile.raw" *> "$out/tonemap-stage-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/tonemap-stage-$profile.txt";throw "Tone-map stage verification failed: $profile"}
    Get-Content -LiteralPath "$out/tonemap-stage-$profile.txt"
    & "$out/tonemap_stage_test.exe" $profile $exe "$PSScriptRoot/../../evidence/I-0066/artifacts/i66-reference-1646-m31-tonemapcapture" "$out/tonemap-stage-m31-$profile.raw" *> "$out/tonemap-stage-m31-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/tonemap-stage-m31-$profile.txt";throw "Tone-map stage (m31, vignette) verification failed: $profile"}
    # W10 closure (rule 1: m30 and m40 for the grading and tone variants): T12 and T17 on Reference1705 (m40) and 1706 (m30) too.
    foreach($zone in @('i66-reference-1705-m40-yebis','i66-reference-1706-m30-yebis')){
        & "$out/tonemap_stage_test.exe" $profile $exe "$PSScriptRoot/../../evidence/I-0066/artifacts/$zone" *> "$out/tonemap-stage-$zone-$profile.txt"
        if($LASTEXITCODE){Get-Content -LiteralPath "$out/tonemap-stage-$zone-$profile.txt";throw "Tone-map stage verification failed: $profile $zone"}
        Get-Content -LiteralPath "$out/tonemap-stage-$zone-$profile.txt" | Select-Object -Last 1}
    foreach($zone in @('i66-reference-1696-m31-grading','i66-reference-1705-m40-yebis','i66-reference-1706-m30-yebis')){
        & "$out/grading_stage_test.exe" $profile $exe 'C:/Users/User/.codex/I0063-cache/programs' "$PSScriptRoot/../../evidence/I-0066/artifacts/$zone" *> "$out/grading-stage-$zone-$profile.txt"
        if($LASTEXITCODE){Get-Content -LiteralPath "$out/grading-stage-$zone-$profile.txt";throw "Grading stage (T17) verification failed: $profile $zone"}
        Get-Content -LiteralPath "$out/grading-stage-$zone-$profile.txt" | Select-Object -Last 1}
    # I-0138 tranche 1: our pass on T12's and T17's captures (the tone map alone at m37 and m31, with the LUT at m31, m40 and m30).
    foreach($zone in @('i66-reference-1643-m37-tonemapcapture','i66-reference-1646-m31-tonemapcapture','i66-reference-1696-m31-grading','i66-reference-1705-m40-yebis','i66-reference-1706-m30-yebis','i138-4b-opening2-orig-1500')){   # I-0138 part 4c: the opening scene's Reinhard record (F 0.36)
        & "$out/post_tone_stage_test.exe" $profile $exe 'C:/Users/User/.codex/I0063-cache/programs' "$PSScriptRoot/../../evidence/I-0066/artifacts/$zone" *> "$out/post-tone-stage-$zone-$profile.txt"
        if($LASTEXITCODE){Get-Content -LiteralPath "$out/post-tone-stage-$zone-$profile.txt";throw "Post tone stage verification failed: $profile $zone"}
        # The step review's L2: a skip (no executable, programme cache or capture) is a failure here: the test is this pass's proof.
        if(Select-String -LiteralPath "$out/post-tone-stage-$zone-$profile.txt" -Pattern 'skipped' -SimpleMatch -Quiet){throw "Post tone stage verification skipped: $profile $zone"}
        Get-Content -LiteralPath "$out/post-tone-stage-$zone-$profile.txt"}
    $glareCapture="$PSScriptRoot/../../evidence/I-0066/artifacts/i66-reference-1648-m37-drawscapture"
    & "$out/post_draws_test.exe" $profile $exe $glareCapture *> "$out/post-draws-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/post-draws-$profile.txt";throw "Post draws (T13) verification failed: $profile"}
    Get-Content -LiteralPath "$out/post-draws-$profile.txt" | Select-Object -Last 1
    # W10 LS-1: Alpha_R (draw 10, alpha-only write) replayed at each shaft zone.
    foreach($zone in $shaftZones){
        & "$out/post_draws_test.exe" $profile $exe $zone 10 *> "$out/post-draws-alpha-$profile.txt"
        if($LASTEXITCODE){Get-Content -LiteralPath "$out/post-draws-alpha-$profile.txt";throw "Post draws (Alpha_R) verification failed: $profile $zone"}
        Get-Content -LiteralPath "$out/post-draws-alpha-$profile.txt" | Select-Object -Last 1
    }
    # W10 DoF (W10-NATIVE-PASSES 9): the ten depth-of-field draws (depth factor, source, aperture filters, blurs, composite) replayed.
    foreach($zone in @('i66-reference-1667-m37-shaftdriver','i66-reference-1668-m31-shaftdriver','i66-reference-1669-m50-shaftdriver','i66-reference-1705-m40-yebis','i66-reference-1706-m30-yebis')){
        & "$out/post_draws_test.exe" $profile $exe "$PSScriptRoot/../../evidence/I-0066/artifacts/$zone" '0,1,2,3,4,5,6,7,8,9' *> "$out/post-draws-dof-$zone-$profile.txt"
        if($LASTEXITCODE){Get-Content -LiteralPath "$out/post-draws-dof-$zone-$profile.txt";throw "Post draws (depth of field) verification failed: $profile $zone"}
        Get-Content -LiteralPath "$out/post-draws-dof-$zone-$profile.txt" | Select-Object -Last 1
    }
    # W10 LS-3: the light shafts' draws (mask, radial blurs, MadT2) replayed at each shaft zone.
    foreach($pair in @(@('i66-reference-1667-m37-shaftdriver','37,38,39,40,41,42,43'),@('i66-reference-1668-m31-shaftdriver','41,42,43,44,45,46,47'),@('i66-reference-1669-m50-shaftdriver','35,36,37,38,39,40,41'))){
        & "$out/post_draws_test.exe" $profile $exe "$PSScriptRoot/../../evidence/I-0066/artifacts/$($pair[0])" $pair[1] *> "$out/post-draws-shafts-$profile.txt"
        if($LASTEXITCODE){Get-Content -LiteralPath "$out/post-draws-shafts-$profile.txt";throw "Post draws (light shafts) verification failed: $profile $($pair[0])"}
        Get-Content -LiteralPath "$out/post-draws-shafts-$profile.txt" | Select-Object -Last 1
    }
    # I-0138 tranche 8f: the two GlareShape 3 captures' library draws (0 to 45) replayed with the library's programmes.
    foreach($zone in $glareStandardZones){
        & "$out/post_draws_test.exe" $profile $exe $zone ((0..45) -join ',') *> "$out/post-draws-standard-$profile.txt"
        if($LASTEXITCODE){Get-Content -LiteralPath "$out/post-draws-standard-$profile.txt";throw "Post draws (GlareShape 3) verification failed: $profile $zone"}
        Get-Content -LiteralPath "$out/post-draws-standard-$profile.txt" | Select-Object -Last 1
    }
    # I-0138 tranche 8i: the shape-3 capture with its shafts, draws 0 to 55 (Alpha_R, the DoF, the glare, the shafts, MadT5, the 2x draw).
    foreach($zone in $glareStandardShaftZones){
        & "$out/post_draws_test.exe" $profile $exe $zone (Get-ShaftCaptureDraws $zone) *> "$out/post-draws-standard-shafts-$profile.txt"
        if($LASTEXITCODE -or (Select-String -LiteralPath "$out/post-draws-standard-shafts-$profile.txt" -Pattern 'skipped' -SimpleMatch -Quiet)){Get-Content -LiteralPath "$out/post-draws-standard-shafts-$profile.txt";throw "Post draws (GlareShape 3 with its shafts) verification failed or skipped: $profile $zone"}
        Get-Content -LiteralPath "$out/post-draws-standard-shafts-$profile.txt" | Select-Object -Last 1
    }
    # W10 MB review: the two blur passes of the turning-camera capture replayed on their own captured inputs (the velocity draw is MB-0's half unit).
    & "$out/post_draws_test.exe" $profile $exe "$PSScriptRoot/../../evidence/I-0066/artifacts/i66-reference-1693-m37-motionblur-turn2400" '12,13' *> "$out/post-draws-mb-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/post-draws-mb-$profile.txt";throw "Post draws (motion blur) verification failed: $profile"}
    Get-Content -LiteralPath "$out/post-draws-mb-$profile.txt" | Select-Object -Last 1
    # LS-3: the glare zones, then the shaft runs with the shafts appended (the whole merged chain from the original's scene HDR).
    $stageZones=@($glareZones)+@($glareStandardZones)+@($glareStandardShaftZones)+@(@('i66-reference-1667-m37-shaftdriver','i66-reference-1668-m31-shaftdriver','i66-reference-1669-m50-shaftdriver') | ForEach-Object {"$PSScriptRoot/../../evidence/I-0066/artifacts/$_"})
    & "$out/glare_stage_test.exe" $profile $exe @stageZones *> "$out/glare-stage-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/glare-stage-$profile.txt";throw "Glare stage (T14) verification failed: $profile"}
    Get-Content -LiteralPath "$out/glare-stage-$profile.txt" | Select-Object -Last 1
    # I-0138 tranche 2: T13 and T14 again with our own programmes (the profile's -ours form): the glare family's captured draws at m37,
    # Alpha_R and the shafts' draws at their zones, then every plan at the nine zones, there also against the library's on this device.
    $oursRuns=[System.Collections.Generic.List[object]]::new();$oursRuns.Add(@($glareCapture,''))
    foreach($zone in $shaftZones){$oursRuns.Add(@($zone,'10'))}
    foreach($pair in @(@('i66-reference-1667-m37-shaftdriver','37,38,39,40,41,42,43'),@('i66-reference-1668-m31-shaftdriver','41,42,43,44,45,46,47'),@('i66-reference-1669-m50-shaftdriver','35,36,37,38,39,40,41'))){
        $oursRuns.Add(@("$PSScriptRoot/../../evidence/I-0066/artifacts/$($pair[0])",$pair[1]))}
    # I-0138 tranche 8f: every library draw of the original's two GlareShape 3 captures (32000020: Alpha_R, the DoF's two, the 43 glare
    # draws), the ghost's strip and trim included; with the library's programmes, below, too (the capture's adapter: exactness required).
    $standardDraws=(0..45) -join ','
    foreach($zone in $glareStandardZones){$oursRuns.Add(@($zone,$standardDraws))}
    foreach($zone in $glareStandardShaftZones){$oursRuns.Add(@($zone,(Get-ShaftCaptureDraws $zone)))}   # I-0138 tranche 8i
    $oursIndex=0
    foreach($run in $oursRuns){ $oursIndex++; $oursLog="$out/post-draws-ours-$oursIndex-$profile.txt"
        if($run[1]){& "$out/post_draws_test.exe" "$profile-ours" $exe $run[0] $run[1] *> $oursLog}else{& "$out/post_draws_test.exe" "$profile-ours" $exe $run[0] *> $oursLog}
        if($LASTEXITCODE -or (Select-String -LiteralPath $oursLog -Pattern 'skipped' -SimpleMatch -Quiet)){Get-Content -LiteralPath $oursLog;throw "Post draws (T13, our glare programmes) verification failed or skipped: $profile $($run[0])"}
        Get-Content -LiteralPath $oursLog | Select-Object -Last 1}
    # I-0138 tranche 8i (the step review's H1): the shape-3 shafts frames' draws with our programmes and the pinned originals from synthetic
    # light (post_draws_test's -lit: an input zero in RGB given light, its alpha kept): the two byte for byte, the radial blurs, MadT5 and the 2x draw lit (1 % of texels normal; the mask, shape 1's programme, reported); the
    # captured outputs, drawn from zeros, not compared. With i138-t8i2-oracle-sun-2120: its mask's depth gate (shaft_mask.w, the record's
    # word 19, 0.99929) is above every alpha of the frame's bright pass (at most 0.857: no sky behind the sun), alpha the replay keeps,
    # so its mask stays zero whatever the light (OBSERVED, build-t8i-13), reported; its blurs, MadT5 and 2x draw are required lit.
    $litIndex=0
    foreach($zone in @($glareStandardShaftZones)+@($shaftWordZones)){ $litIndex++; $litLog="$out/post-draws-lit-$litIndex-$profile.txt"
        & "$out/post_draws_test.exe" "$profile-ours-lit" $exe $zone (Get-ShaftCaptureDraws $zone) *> $litLog
        if($LASTEXITCODE -or (Select-String -LiteralPath $litLog -Pattern 'skipped' -SimpleMatch -Quiet)){Get-Content -LiteralPath $litLog;throw "Post draws (T13, lit replay) verification failed or skipped: $profile $zone"}
        Get-Content -LiteralPath $litLog | Select-Object -Last 3}
    & "$out/glare_stage_test.exe" "$profile-ours" $exe @stageZones *> "$out/glare-stage-ours-$profile.txt"
    if($LASTEXITCODE -or (Select-String -LiteralPath "$out/glare-stage-ours-$profile.txt" -Pattern 'skipped' -SimpleMatch -Quiet)){Get-Content -LiteralPath "$out/glare-stage-ours-$profile.txt";throw "Glare stage (T14, our glare programmes) verification failed or skipped: $profile"}
    Get-Content -LiteralPath "$out/glare-stage-ours-$profile.txt" | Select-Object -Last 3
    # W10 DF-3 T15: the native DoF drawn by the glare stage from each DoF zone's scene and depth.
    & "$out/dof_stage_test.exe" $profile $exe @dofZones *> "$out/dof-stage-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/dof-stage-$profile.txt";throw "DoF stage (T15) verification failed: $profile"}
    Get-Content -LiteralPath "$out/dof-stage-$profile.txt" | Select-Object -Last 1
    # W10 MB-2 T16: the native motion blur drawn by the glare stage from each DoF zone's depth, object velocity and scene.
    & "$out/mb_stage_test.exe" $profile $exe @mbZones *> "$out/mb-stage-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/mb-stage-$profile.txt";throw "Motion blur stage (T16) verification failed: $profile"}
    Get-Content -LiteralPath "$out/mb-stage-$profile.txt" | Select-Object -Last 1
    # I-0138 tranche 4: T15 and T16 again with our own programmes (post_glare.hlsl): the same comparisons with the captured draws,
    # then every surface against the library's programmes on this device, at the captures and away from them (other extents,
    # synthetic inputs, the composite's forty counts, the blur's seven tap counts). A skipped run fails the build.
    foreach($stageTest in @(@('dof','DoF stage (T15',$dofZones),@('mb','Motion blur stage (T16',$mbZones))){$oursLog="$out/$($stageTest[0])-stage-ours-$profile.txt"
        & "$out/$($stageTest[0])_stage_test.exe" "$profile-ours" $exe @($stageTest[2]) *> $oursLog
        if($LASTEXITCODE -or (Select-String -LiteralPath $oursLog -Pattern 'skipped' -SimpleMatch -Quiet)){Get-Content -LiteralPath $oursLog;throw "$($stageTest[1]), our programmes) verification failed or skipped: $profile"}
        Get-Content -LiteralPath $oursLog | Select-Object -Last 4}
    Get-Content -LiteralPath "$out/tonemap-stage-m31-$profile.txt"
    & "$out/tess_pso_test.exe" $profile 'C:/Users/User/.codex/I0063-cache/programs' *> "$out/tess-pso-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/tess-pso-$profile.txt";throw "Tess pipeline verification failed: $profile"}
    Get-Content -LiteralPath "$out/tess-pso-$profile.txt"
    & "$out/model_draw_test.exe" $profile 'C:/Users/User/.codex/I0063-cache/programs' *> "$out/model-draw-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/model-draw-$profile.txt";throw "Native model draw verification failed: $profile"}
    Get-Content -LiteralPath "$out/model-draw-$profile.txt"
    & "$out/game_texture_ingress_test.exe" $profile *> "$out/texture-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/texture-$profile.txt";throw "Native texture verification failed: $profile"}
    Get-Content -LiteralPath "$out/texture-$profile.txt"
    & "$out/gbuffer_encoder_test.exe" $profile *> "$out/gbuffer-encoder-$profile.txt"
    if($LASTEXITCODE){Get-Content -LiteralPath "$out/gbuffer-encoder-$profile.txt";throw "G-buffer encoder verification failed: $profile"}
    Get-Content -LiteralPath "$out/gbuffer-encoder-$profile.txt" | Select-Object -Last 1
}
# W10 YB-5 T6: the YEBIS GX pins (yebis_stage.h) against the I-0063 programme cache (skipped without it).
$programmeCache='C:/Users/User/.codex/I0063-cache/programs'
if(Test-Path -LiteralPath "$programmeCache/gxposteffect__PointSampleColorClamp1.ppo"){$gxChecked=0
    foreach($pin in [regex]::Matches((Get-Content -LiteralPath "$root/native/renderer/yebis_stage.h" -Raw),'\{L"(\w+)",(\d)u,(\d+),"([0-9A-F]{64})"\}')){
        $file="$programmeCache/gxposteffect__$($pin.Groups[1].Value)."+$(if($pin.Groups[2].Value -eq '5'){'cpo'}else{'ppo'})
        if(-not (Test-Path -LiteralPath $file) -or (Get-Item -LiteralPath $file).Length -ne [int]$pin.Groups[3].Value -or (Get-FileHash -LiteralPath $file).Hash -ne $pin.Groups[4].Value){throw "YEBIS GX pin mismatch: $($pin.Groups[1].Value)"}
        $gxChecked++}
    if($gxChecked -ne 3){throw "YEBIS GX pins: $gxChecked of 3"}
    'PASS YEBIS GX pins: 3 programmes equal the programme cache'}else{'PASS YEBIS GX pins: skipped, no programme cache'}
Publish-CacheBuild
Clear-BuildObjects
Get-FileHash -LiteralPath "$out/game_bootstrap.dll","$out/game_bootstrap_ctl.exe" -Algorithm SHA256
