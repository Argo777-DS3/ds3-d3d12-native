# I-0113 gap 1 (Anor Londo's cathedral walls, Thomas 2026-10-30 04:44): Gamma at the Anor Londo bonfire (hash-checked copy of
# its zone save, Continue) walks up the great stairs facing it (or is teleported, -Teleport "x y z hold" in the frame of
# witness_player_position), a shot after each look and step (Pilot-Game holds a key 5000 ms at most), natively then on the
# original, from this one process (each Run-ZoneWalk.ps1 restores by hash at its end). chr-positions.flag logs the player's placement (the
# teleport frame for later runs); test-no-ai keeps the enemies off. -NotBefore HH:mm waits for the announced start (same day).
# Tranche 6: -HideTextures "frag1,frag2" (Run-ZoneWalk's, hide-texture.txt in turn: "#name", "!name", "@x y z r" or a texture path
# fragment) and -ExtraFlags (more effect flags, e.g. no-dlss so both renderers draw without DLSS).
# Tranche 25 (the LOD crossfades and distance fades with tranche 22's dither, D-0181): -Video films the walk (Run-ZoneWalk's walk.mp4,
# 60 frames/s), read with scripts/video/videosheet.py native against original; -NoDebugLayer for the native walk (the films' cadence;
# tranche 25's native walk kept the debug layer). -ExtraFlags popin-observe places the crossfades (popin_lod, popin_*_fade).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string]$Look='',[string]$Teleport='',[string]$KeyPath='Z5000,Z5000,Z5000,Z5000,Z5000,Z5000',[ValidateSet('native','original','both')][string]$Modes='both',
    [string]$NotBefore='',[int]$Seconds=300,[string]$HideTextures='',[string]$ExtraFlags='',[switch]$Video,[switch]$NoDebugLayer)
$ErrorActionPreference='Stop'
$walk=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-ZoneWalk.ps1'
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-gamma-m37-anorlondo-20261026-123818\DS30000.sl2'
$hash='E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411'
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($mode in 'native','original'){if($Modes -ne 'both' -and $Modes -ne $mode){continue}
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$mode skipped: DarkSoulsIII already runs";break}
    $run="$Prefix-$mode";"$(Get-Date -Format HH:mm:ss) $run"
    & $walk -RunName $run -BootstrapMode $(if($mode -eq 'native'){'Native'}else{'Reference'}) -TestSave $save -TestSaveSha256 $hash `
        -Seconds $Seconds -SettleSeconds 40 -Teleport $Teleport -Look $Look -KeyPath $KeyPath -Shots 3 -EffectFlags ('test-no-ai,chr-positions'+$(if($ExtraFlags){",$ExtraFlags"})) -HideTextures $HideTextures -BuildFolder $BuildFolder -Video:$Video -NoDebugLayer:($NoDebugLayer -and $mode -eq 'native')
    "$(Get-Date -Format HH:mm:ss) $run done"}
