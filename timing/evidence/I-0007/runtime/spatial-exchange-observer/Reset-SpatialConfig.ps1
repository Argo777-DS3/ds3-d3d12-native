$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Config = Join-Path $Here 'pass_c_config.txt'
@"
condition=i-0007-spatial-exchange-disabled
target_fps=60
enable_timing_override=0
force_present_zero=0
callback_detail=0
marker_hotkeys=1
locomotion_trials=0
locomotion_phase_rca=0
locomotion_write_watch=0
locomotion_writer_mirror=0
semantic_sample_stride=1
locomotion_warmup_delta=1
locomotion_measure_delta=1.5
auto_stop_seconds=900
causal_distance_patch=0
elapsed_recurrence_patch=0
"@ | Set-Content -LiteralPath $Config -Encoding ASCII
Get-Content -LiteralPath $Config
