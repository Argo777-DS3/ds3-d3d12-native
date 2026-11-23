$ErrorActionPreference = 'Stop'

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Source = Join-Path $Here 'configs/transient-disabled.txt'
$Destination = Join-Path $Here 'transient-capture/pass_c_config.txt'
Copy-Item -LiteralPath $Source -Destination $Destination -Force

$Config = Get-Content -LiteralPath $Destination -Raw
foreach ($Line in @('enable_timing_override=0', 'locomotion_trials=0',
                     'locomotion_phase_rca=0', 'locomotion_write_watch=0',
                     'locomotion_writer_mirror=0', 'causal_distance_patch=0')) {
    if ($Config -notmatch "(?m)^$([regex]::Escape($Line))\r?$") {
        throw "Disabled configuration is missing: $Line"
    }
}
Write-Output 'Restored the checkpoint-disabled I-0007 causal-patch configuration.'
