param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Tag,[Parameter(Mandatory)][string]$Build,[string]$NotBefore='',
    [string[]]$Plan=@('native:2560x1440@280','native:1920x1080@60','original:2560x1440@280','original:1920x1080@60'),[switch]$PresentMon)
# I-0098: control runs of the replayed path on named screens (an announced live run, D-0116): each item « native:<screen> »
# (measure mode, frame-dump) or « original:<screen> » (the original with the module's probe, Reference, frame-dump), run in order
# by Invoke-ReplayRun -Screen (-PresentMon: what the screen showed), named <Tag>-n<hz> or <Tag>-o<hz>.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){ Start-Sleep 5 } }
foreach($item in $Plan){ $kind,$screen=$item -split ':',2;$hz=($screen -replace '^.*@','')
    $name="$Tag-$(if($kind -eq 'native'){'n'}else{'o'})$hz"
    "== $name $(Get-Date -Format HH:mm:ss) $kind on $screen"
    & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $name -Build $Build -Kind $(if($kind -eq 'native'){'Measure'}else{'Reference'}) -Screen $screen -EffectFlags 'frame-dump' -PresentMon:$PresentMon 2>&1 | Select-Object -Last 2
    $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5 }
"== done $(Get-Date -Format HH:mm:ss)"
