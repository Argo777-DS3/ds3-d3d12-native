param([Parameter(Mandatory)][string]$Series,[string]$Build='',[string]$NotBefore='',[string[]]$Only=@(),[ValidateSet('base','fix')][string]$Plan='base')
# I-0084: the audit's live runs on the replayed Firelink path (announced runs under D-0116, Invoke-ReplayRun.ps1, never a gate
# record). Plan "base" on a build of main: gbv (the debug layer with GPU-based validation, lead B1), cold (measure mode without
# the pipeline library or its list, the first start on a new PC, lead A1), sample (measure mode with the sampler, lead C3).
# Plan "fix" on the workstream's build: m1 and m2 (measure mode; a new build folder has no library, so m1 is the first start and
# m2 the second), vram4g and vram1500 (a ballast leaving 4096 or 1500 MB of headroom above the usage at the device's opening, lead D1, through
# Run-StartupTrace -VideoMemoryBudgetMb), warp (the device on WARP, lead G3: a start witness, its timings meaningless).
$ErrorActionPreference='Stop'
$Only=@($Only | ForEach-Object { $_ -split '[,;\s]+' } | Where-Object { $_ })   # pwsh -File passes a list as one string
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(-not $Build){ $Build="$repo/evidence/I-0083/gates/build-$((git -C $repo rev-parse HEAD:native).Trim().Substring(0,12))" }
if(-not (Test-Path -LiteralPath "$Build/game_bootstrap.dll")){ throw "no module in $Build" }
$runs=if($Plan -eq 'base'){ [ordered]@{
    gbv=@{Kind='Validation';ExtraSwitch=@('GpuValidation')}
    cold=@{Kind='Measure';EffectFlags='no-pipeline-library'}
    sample=@{Kind='Measure';EffectFlags='sample-profile'}} }else{ [ordered]@{
    m1=@{Kind='Measure'};m2=@{Kind='Measure'}
    vram4g=@{Kind='Measure';Budget=4096};vram1500=@{Kind='Measure';Budget=1500}
    warp=@{Kind='Measure';EffectFlags='warp'}} }
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){ Start-Sleep 5 } }
foreach($k in $runs.Keys){ if($Only.Count -and $k -notin $Only){ continue }
    $name="a84-$Series-$k";$a=@{}+$runs[$k];$folder=$Build
    if($a.Budget){ $a.VideoMemoryBudgetMb=$a.Budget;$a.Remove('Budget') }
    "== $name $(Get-Date -Format HH:mm:ss)"
    & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $name -Build $folder @a 2>&1 | Select-Object -Last 3
    $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5 }
"== done $(Get-Date -Format HH:mm:ss)"
