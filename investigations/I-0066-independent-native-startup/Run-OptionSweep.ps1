param([Parameter(Mandatory)][int]$First,[string[]]$Levels=@('DISABLE','LOW','MEDIUM','HIGH','MAX'),[string[]]$Modes=@('Reference','Native'),
    [ValidateSet('All','PerKey')][string]$Shape='All',[string[]]$Keys=@(),[int]$Seconds=420,[int]$StaySeconds=60,
    [ValidatePattern('^[a-z0-9-]*$')][string]$TagSuffix="",[switch]$NoDebugLayer)
# D-0166 (Thomas, 2026-10-12): every level of every graphics setting is a live row (options sheet section 6, OP-0 to OP-3,
# widened). One announced series of pairs on Thomas's save (Run-PilotPair.ps1, High Wall, in-game taps, Equipment page),
# numbered from -First. Run-StartupTrace.ps1 edits GraphicsConfig.xml through -GraphicsLevels after its hash-checked
# backup and restores every protected file by hash at the end. This series also checks, before each pair and at the end,
# that the configuration is still Thomas's (its SHA-256 at the start) and that no game is running; otherwise it stops,
# so a failed run can never be followed by a run that backs up an edited configuration as his.
#   -Shape All     one pair per level: every quality key at that level at once (QualitySetting CUSTOM; Antialiasing OFF
#                  below HIGH, ON from HIGH: the block level of ON is 3, QL1). Five pairs cover every level of every key.
#   -Shape PerKey  one pair per key and level, the other keys left as in Thomas's file (attribution of an image change to
#                  one option; -Keys narrows the list).
# The native_quality_object row of each run gives the levels the module read; the frame audit rows give the deltas.
$ErrorActionPreference='Stop'
$quality=@('TextureQuality','SSAO','DepthOfField','MotionBlur','ShadowQuality','LightingQuality','EffectsQuality','ReflectionQuality','WaterSurfaceQuality','ShadeQuality')
$Levels=@($Levels -split '[,;\s]+' | Where-Object { $_ } | ForEach-Object { $_.ToUpperInvariant() })
# Keys are matched case-sensitively by the configuration edit: map each to its canonical spelling.
$Keys=@($Keys -split '[,;\s]+' | Where-Object { $_ } | ForEach-Object { $k=$_;$canonical=@($quality+@('Antialiasing') | Where-Object { $_ -ieq $k })
    if($canonical.Count -ne 1){ throw "Unknown key $k" };$canonical[0] })
if(-not $Keys.Count){ $Keys=$quality+@('Antialiasing') }elseif($Shape -eq 'All'){ throw '-Keys applies to -Shape PerKey only' }
foreach($level in $Levels){ if($level -cnotin @('DISABLE','LOW','MEDIUM','HIGH','MAX')){ throw "Unknown level $level" } }
$aa={param($level) if($level -in @('HIGH','MAX')){'ON'}else{'OFF'}}
$series=@(
    if($Shape -eq 'All'){
        foreach($level in $Levels){
            $pairs=@('QualitySetting=CUSTOM')+@($quality | ForEach-Object { "$_=$level" })+@("Antialiasing=$(& $aa $level)")
            @{tag="opt-all-$($level.ToLowerInvariant())";levels=($pairs -join ';')} }
    }else{
        foreach($key in $Keys){
            $values=if($key -eq 'Antialiasing'){@('OFF','ON')}else{$Levels}
            foreach($value in $values){ @{tag="opt-$($key.ToLowerInvariant())-$($value.ToLowerInvariant())";levels="QualitySetting=CUSTOM;$key=$value"} } }
    })
$config="$env:APPDATA/DarkSoulsIII/GraphicsConfig.xml"
$thomas=(Get-FileHash -LiteralPath $config).Hash
$keep=Join-Path $env:USERPROFILE ".ds3-i0039-backups/option-sweep-$First-$(Get-Date -Format yyyyMMdd-HHmmss)-GraphicsConfig.xml"
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $keep) | Out-Null
Copy-Item -LiteralPath $config -Destination $keep
if((Get-FileHash -LiteralPath $keep).Hash -ne $thomas){ throw 'Sweep copy of GraphicsConfig.xml mismatch' }
function Assert-Clean([string]$when){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ throw "Option sweep stopped $($when): a game process is still running; Thomas's configuration copy: $keep" }
    if((Get-FileHash -LiteralPath $config).Hash -ne $thomas){ throw "Option sweep stopped $($when): GraphicsConfig.xml differs from Thomas's (copy kept at $keep, SHA-256 $thomas)" } }
"GraphicsConfig.xml SHA-256 $thomas, copy $keep"
$number=$First
foreach($run in $series){
    Assert-Clean "before $number"
    $tag="$number-$($run.tag)$TagSuffix"
    "[$((Get-Date).ToString('HH:mm:ss'))] series $tag ($($run.levels))"
    $extra=@(); if($NoDebugLayer){ $extra+='-NoDebugLayer' }
    pwsh -NoProfile -File "$PSScriptRoot/Run-PilotPair.ps1" -Tag $tag -Modes ($Modes -join ',') -Seconds $Seconds -StaySeconds $StaySeconds `
        -InGameTaps -EquipPage -GraphicsLevels $run.levels @extra 2>&1
    $root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    foreach($mode in @($Modes -split '[,;\s]+' | Where-Object { $_ })){
        $restoration="$root/evidence/I-0066/artifacts/i66-$($mode.ToLower())-$tag/restoration.json"
        if(-not (Test-Path -LiteralPath $restoration) -or -not (Get-Content -LiteralPath $restoration -Raw | ConvertFrom-Json).ok){
            throw "Option sweep stopped after $($tag): no verified restoration for $mode ($restoration)" } }
    ++$number
}
Assert-Clean 'at the end'
"Option sweep done; GraphicsConfig.xml is Thomas's ($thomas)"
