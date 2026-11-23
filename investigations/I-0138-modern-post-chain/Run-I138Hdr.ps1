# I-0138 tranche 5: the HDR10 output's witness runs on a screen without Windows' HDR (the ASUS): one process, Firelink Shrine's
# Shebang save through I-0126's Run-R7aGi.ps1 (offline, restored by hash, world-pictures: the presented picture every 300th world
# frame and, beside it, the float picture the output pass read; the runs made before the UI was drawn in place wrote the 8-bit
# picture or the float scene and a UI layer).
#   forced, paper white 80 nits (the compositor's own white for an SDR screen): 1280x720, 1920x1080 and a 1500x840 window;
#   forced, paper white 200 nits: 1280x720;
#   auto: 1280x720 (the screen's HDR is off: the chain must stay the 8-bit SDR one).
# The pictures are read by Decode-Hdr10.py. -Step names the runs (i138-t5<step>-...).
param([Parameter(Mandatory)][string]$BuildFolder,[Parameter(Mandatory)][ValidatePattern('^[a-z0-9]+$')][string]$Step,[Parameter(Mandatory)][string]$Log,
    [string]$Zone='m40',[int]$Seconds=100,[string]$NotBefore='')
$ErrorActionPreference='Stop'
$runner=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0126-rt-gi/Run-R7aGi.ps1'
$base='test-no-ai,world-pictures,rt-scene,rt-fail-magenta'
try{
    & $runner -Prefix "i138-t5$Step-f80" -BuildFolder $BuildFolder -Zones $Zone -Modes taa -Sizes '1280x720,1920x1080,1500x840' -Seconds $Seconds -Flags "$base,hdr-output-force,hdr-paper-80" -Log $Log -NotBefore $NotBefore
    & $runner -Prefix "i138-t5$Step-f200" -BuildFolder $BuildFolder -Zones $Zone -Modes taa -Sizes '1280x720' -Seconds $Seconds -Flags "$base,hdr-output-force" -Log $Log
    & $runner -Prefix "i138-t5$Step-auto" -BuildFolder $BuildFolder -Zones $Zone -Modes taa -Sizes '1280x720' -Seconds $Seconds -Flags "$base,hdr-output" -Log $Log
}catch{"ERROR $_" | Add-Content -LiteralPath $Log}
finally{"SERIES-END $(Get-Date -Format HH:mm:ss)" | Add-Content -LiteralPath $Log}
