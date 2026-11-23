param([switch]$Update)
# R13 cF.5 (phase F sheet, item cF.5; checks S-1): the functions that may run on the render worker (F1) or that prepare
# frames (F2) are listed explicitly (a regular expression cannot compute reachability). In each body this reports
#   - thread_local declarations and mutable function-local statics (not static const, constexpr or function declarations),
#     compared with the reviewed allow-list scripts/thread-affinity-allow.json, keyed "file|function|name";
#   - S-1: in the prepare functions, a read of native_offscreen_preparing, an std::ofstream or control->journal.
# -Update writes the current findings into the allow-list (to review in the diff, never blindly).
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$allowPath=Join-Path $PSScriptRoot 'thread-affinity-allow.json'
$scope=@(
    @{file='native/frame/game_bootstrap/*.inc';functions=@('PrepareNativeModelFrame','PopinNativeFrame','PopinLoadFrame','CollectNativeMovieFrame','CollectNativeOffscreenViews','NoteNativeOffscreenViewState','ProduceNativeSssInputs','ProduceNativeMenuEnvRegions','NativeGparamRecords>Build','NativeEngineFogLookup','NativeEngineFogAnswer','LogEffectFogKey','NativeUiTextureImport','AcquireNativeLightingPrograms','TextureWitness','EffectModelTextureWitness','RunNativeFrameJob','JournalSibling','ProduceNativeSnowRate','CollectNativeRtInstances','CollectNativeRtScene','WalkNativeLayers','NativeToneLookRequested','EntityOffered','NativeEngineLightAnswer','LogEffectLightKey','ObserveFogBlockBuild','NativeFadeMoving','ObserveDofBlend','NativeExternalFocus','ObserveShaftKeeper','AppendNativeShafts','CollectNativeYebisInputs','CollectNativeOffscreenScenes','CollectNativeMenuSelfShadow','CollectNativeMenuRtScene');prepare=@('PrepareNativeModelFrame','CollectNativeMovieFrame','CollectNativeOffscreenViews','JournalSibling','ProduceNativeSssInputs','ProduceNativeMenuEnvRegions','NativeGparamRecords>Build','AcquireNativeLightingPrograms')},
    @{file='native/renderer/scene_pass*.cpp';functions=@('SceneRenderer::ExecuteFrame','SceneRenderer::RenderModels','SceneRenderer::RenderStartupFrame','SceneRenderer::RenderNativeOffscreen','SceneRenderer::RenderNativeMovie','SceneRenderer::NativeLightingResources','SceneRenderer::EncodeNativeLighting','SceneRenderer::EncodeNativeSss','SceneRenderer::EncodeNativeEnvRegions','SceneRenderer::EncodeNativePointLights','SceneRenderer::EncodeNativeVelocity','SceneRenderer::NativeVelocityResources','SceneRenderer::CompleteFrame','SceneRenderer::RecordPixels','SceneRenderer::FrameListSlot','SceneRenderer::AdmitPrimitiveWork','SceneRenderer::WaitFrameSlot','SceneRenderer::TakeReleased','SceneRenderer::Readbacks','SceneRenderer::DepthTarget','SceneRenderer::PrimitiveDepthTarget','SceneRenderer::NativeModelTargets','SceneRenderer::EncodeRtScene','SceneRenderer::RtViewFrame','SceneRenderer::EncodeRtView','SceneRenderer::FailRtView');prepare=@()},
    @{file='native/renderer/scene_pass_internal.h';functions=@('ScenePassFile');prepare=@()},
    @{file='native/world/post_tone.h';functions=@('ApplyToneLook');prepare=@()},   # I-0138 tranche 6: called from PrepareNativeModelFrame
    @{file='native/renderer/primitive_pass.cpp';functions=@('PrimitivePass::Encode');prepare=@()},
    @{file='native/renderer/backend.cpp';functions=@('UploadArena::Allocate','FrameRing::Begin','FrameRing::Retire','FrameRing::End','DeviceQueues::Execute');prepare=@()},
    @{file='native/renderer/render_worker.cpp';functions=@('RenderWorker::Loop','RenderWorker::WaitIdle','RenderWorker::Post');prepare=@()},
    @{file='native/renderer/model_draw.cpp';functions=@('OwnedNativeModelDraw::Prepare','OwnedNativeModelDraw::Encode');prepare=@()}
)
function Get-Body([string[]]$lines,[string]$name,[string]$definition=''){
    # 'Class>Member': the member defined inside the class body (an inline member such as NativeGparamRecords>Build).
    if($name -match '^(\w+)>(\w+)$'){
        $member=$Matches[2];$class=$Matches[1]
        for($i=0;$i -lt $lines.Count;$i++){if($lines[$i] -match "^\s*(class|struct)\s+$class\b[^;]*$"){
            $depth=0;$started=$false
            for($j=$i;$j -lt $lines.Count;$j++){foreach($c in $lines[$j].ToCharArray()){if($c -eq '{'){$depth++;$started=$true}elseif($c -eq '}'){$depth--}}
                if($started -and $depth -le 0){
                    # A definition: a return type before the name (not a call such as 'return Build(').
                    $inner=Get-Body ([string[]]$lines[($i+1)..$j]) $member ('^\s*(static\s+)?(?!return\b)[\w:<>\*&]+\s+'+$member+'\s*\(')
                    if($inner){foreach($entry in $inner){$entry.line+=$i+1}}
                    return $inner}}}}
        return $null
    }
    # The definition: the name followed by '(' on a line that does not end the statement with ';' before a '{'.
    $pattern=if($definition){$definition}else{'(^|[\s\*&:])'+[regex]::Escape($name)+'\s*\('}
    for($i=0;$i -lt $lines.Count;$i++){
        if($lines[$i] -notmatch $pattern){continue}
        $depth=0;$started=$false;$body=New-Object System.Collections.Generic.List[object]
        for($j=$i;$j -lt $lines.Count;$j++){
            $text=($lines[$j] -replace '//.*$','') -replace '"(\\.|[^"\\])*"','""'
            $text=$text -replace "'(\\.|[^'\\])'","''"   # character literals such as '{'
            if(-not $started -and $text -match ';\s*$' -and $text -notmatch '\{'){break}   # a declaration or a call
            foreach($c in $text.ToCharArray()){if($c -eq '{'){$depth++;$started=$true}elseif($c -eq '}'){$depth--}}
            $body.Add([pscustomobject]@{line=$j+1;text=$lines[$j]})
            if($started -and $depth -le 0){return $body}
        }
    }
    return $null
}
$findings=New-Object System.Collections.Generic.List[string];$missing=New-Object System.Collections.Generic.List[string];$s1=New-Object System.Collections.Generic.List[string]
foreach($unit in $scope){
    # A wildcard names the parts of one translation unit (game_bootstrap/*.inc, code cleanup D-0191), read in order as one
    # text; a finding is keyed by the part that holds the function.
    $parts=@(Get-ChildItem -Path (Join-Path $root $unit.file) | Sort-Object Name)
    $lines=New-Object System.Collections.Generic.List[string];$starts=New-Object System.Collections.Generic.List[object]
    foreach($part in $parts){$starts.Add([pscustomobject]@{first=$lines.Count+1;file=[IO.Path]::GetRelativePath($root,$part.FullName).Replace('\','/')});$lines.AddRange([string[]](Get-Content -LiteralPath $part.FullName))}
    $lines=[string[]]$lines
    foreach($name in $unit.functions){
        $body=Get-Body $lines $name
        if(-not $body){$missing.Add("$($unit.file)|$name");continue}
        $part=@($starts | Where-Object first -le $body[0].line)[-1];$file=$part.file
        foreach($entry in $body){
            $code=$entry.text -replace '//.*$','';$at=$entry.line-$part.first+1
            foreach($m in [regex]::Matches($code,'\b(thread_local|static)\s+(?!const\b(?![\w:<>,\s&]*\*(?!\s*const\b))|constexpr\b|inline\b|void\b|bool\b\s*\w+\s*\()([\w:<>,\s\*&]+?)\s+(\w+)\s*(\{|=|;|\[)')){
                $declaration=$m.Groups[0].Value
                if($m.Groups[1].Value -eq 'static' -and $code -match 'static\s+(const\b(?![\w:<>,\s&]*\*(?!\s*const\b))|constexpr\b)'){continue}   # a pointer to const is mutable, a const pointer is not (lot 2g step review L2)
                $findings.Add("$file|$name|$($m.Groups[3].Value)")}
            if($unit.prepare -contains $name){
                if($code -match '\bnative_offscreen_preparing\b'){$s1.Add("${file}:$at $name reads native_offscreen_preparing")}
                if($code -match 'std::ofstream'){$s1.Add("${file}:$at $name writes a file")}
                if($code -match 'control->journal'){$s1.Add("${file}:$at $name reads control->journal")}}
        }
    }
}
$found=@($findings | Sort-Object -Unique)
if($Update){
    $existing=@{};if(Test-Path -LiteralPath $allowPath){(Get-Content -LiteralPath $allowPath -Raw | ConvertFrom-Json).PSObject.Properties | ForEach-Object {$existing[$_.Name]=$_.Value}}
    $out=[ordered]@{};foreach($key in $found){$out[$key]=if($existing.ContainsKey($key)){$existing[$key]}else{'UNREVIEWED'}}
    $out | ConvertTo-Json | Set-Content -LiteralPath $allowPath -Encoding utf8;"allow-list written: $($found.Count) entries"
}
$allow=@{};if(Test-Path -LiteralPath $allowPath){(Get-Content -LiteralPath $allowPath -Raw | ConvertFrom-Json).PSObject.Properties | ForEach-Object {$allow[$_.Name]=$_.Value}}
$new=@($found | Where-Object {-not $allow.ContainsKey($_) -or $allow[$_] -eq 'UNREVIEWED'})
$stale=@($allow.Keys | Where-Object {$found -notcontains $_})
"functions not found: $($missing.Count)";$missing | ForEach-Object {"  MISSING $_"}
"thread-local or mutable statics: $($found.Count), not reviewed: $($new.Count), allow-list entries no longer present: $($stale.Count)"
$new | ForEach-Object {"  UNREVIEWED $_"};$stale | ForEach-Object {"  STALE $_"}
"S-1 (prepare reads that F2 must remove): $($s1.Count)";$s1 | ForEach-Object {"  S-1 $_"}
if($missing.Count -or $new.Count){exit 1}
"PASS thread affinity: every listed function found, every thread-local or mutable static reviewed (S-1 reported, gated at F2)"
