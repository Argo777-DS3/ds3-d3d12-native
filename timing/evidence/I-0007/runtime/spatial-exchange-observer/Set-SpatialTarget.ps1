[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet(60,165)]
    [int]$TargetFps,
    [ValidateRange(0.5,6.0)]
    [double]$MeasureSeconds = 1.5
)

$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Config = Join-Path $Here 'pass_c_config.txt'
@"
condition=i-0007-spatial-exchange-$TargetFps
target_fps=$TargetFps
enable_timing_override=1
force_present_zero=0
callback_detail=0
marker_hotkeys=1
locomotion_trials=1
locomotion_phase_rca=1
locomotion_write_watch=0
locomotion_writer_mirror=1
semantic_sample_stride=1
locomotion_warmup_delta=1
locomotion_measure_delta=$($MeasureSeconds.ToString([Globalization.CultureInfo]::InvariantCulture))
auto_stop_seconds=900
causal_distance_patch=0
elapsed_recurrence_patch=0
"@ | Set-Content -LiteralPath $Config -Encoding ASCII
Get-Content -LiteralPath $Config
