param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Tag,[Parameter(Mandatory)][string]$Build,[string]$NotBefore='',
    [Parameter(Mandatory)][string[]]$Plan)
# I-0097 tranche 3: the presentation on a named screen under PresentMon (an announced live run, D-0116). Each item
# « <label>:<kind>:<screen>[:<switch>+<switch>] »: kind native (measure mode, frame-dump) or original (Reference with the probe),
# the screen a base key (« 1920x1080@60 »), the switches Invoke-ReplayRun's experiment switches (NoWaitablePresent, ...); the
# run is named <Tag>-<label>. Read with Get-ReplayMeasure (pm_late, pm_over150, pm_modes).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Plan=@($Plan -split ',' | Where-Object { $_ })
# Step review: every item checked before the first run (a misspelt switch would stop the series half-way).
$known='InlineRender','InlinePrepare','NoPersistentDraws','NoModelRecords','GpuValidation','NoWaitablePresent','OneFrameSlot','LaunchOnScreen'
foreach($item in $Plan){ $p=$item -split ':',4;if($p.Count -lt 3 -or $p[1] -notin 'native','original'){ throw "plan item $item" };$null=& "$repo/scripts/Get-Screens.ps1" -Key $p[2]
    foreach($s in @($p[3] -split '\+' | Where-Object { $_ })){ if($s -notin $known -and $s -notlike 'flag-*' -and $s -notlike 'gl-*=*'){ throw "plan item ${item}: unknown switch $s" } } }
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){ Start-Sleep 5 } }
foreach($item in $Plan){ $label,$kind,$screen,$extra=$item -split ':',4
    $switches=@($extra -split '\+' | Where-Object { $_ });$name="$Tag-$label"
    $launch=$switches -contains 'LaunchOnScreen';$flags=@('frame-dump')+@($switches | Where-Object { $_ -like 'flag-*' } | ForEach-Object { $_.Substring(5) })   # Invoke-ReplayRun -LaunchOnScreen; « flag-<name> »: an effect flag
    $levels=(@($switches | Where-Object { $_ -like 'gl-*' } | ForEach-Object { $_.Substring(3) }) -join ';')   # « gl-<Key>=<TOKEN> »: GraphicsConfig for the run
    $switches=@($switches | Where-Object { $_ -ne 'LaunchOnScreen' -and $_ -notlike 'flag-*' -and $_ -notlike 'gl-*' })
    "== $name $(Get-Date -Format HH:mm:ss) $kind on $screen $($switches -join ' ')"
    & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $name -Build $Build -Kind $(if($kind -eq 'native'){'Measure'}else{'Reference'}) -Screen $screen -EffectFlags ($flags -join ';') -PresentMon -LaunchOnScreen:$launch -GraphicsLevels $levels -ExtraSwitch $switches 2>&1 | Select-Object -Last 2
    $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 }
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ "the game is still running after ${name}: series stopped";break };Start-Sleep 5 }
"== done $(Get-Date -Format HH:mm:ss)"
