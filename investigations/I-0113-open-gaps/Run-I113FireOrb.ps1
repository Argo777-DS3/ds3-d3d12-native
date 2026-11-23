# I-0113 tranche 18, gap 16 (Fire Orb's pink, purple and cyan ring at Archdragon Peak, Thomas's capture of 2026-10-31 08:02:19):
# the three earlier tries (tranches 10, 11, 15) cast against dark stone; Thomas's burst was over the bright fog and sky behind the
# bonfire. One announced slot, Run-AlphaCast.ps1 each (one process, restored by hash): Alpha's all-pyromancies copy loads at Firelink,
# rests, travels to Archdragon Peak, turns the camera half a turn (to the cliff side), attunes Fire Orb (Up twice from Seething
# Chaos) and casts it -Casts times.
# Runs (-Runs, in order):
#   native    ffx-record.flag (ffx_distortion_witness: the Normal 608's g_color, g_vColorScale, g_ps_ColorBlendType), filmed
#   original  the Reference with ffx-draw-observe.flag (the same witness), filmed
#   nocopy    native with no-ffx-effect-copy.flag as well (the 608s read the forward pass's copy, without the 0x29 effects), filmed
#   near-native, near-original  the same with a step forward first (Z 200 ms: the character faces the camera's way); meant to make
#             the orb hit the ground ahead, it did not (i113-t18-orb-near-*: every orb burst in the sky over the cliff); with
#             -LookSweep '1,2300,2' (tranche 25) every orb burst at the dead tree a few metres ahead, the sky behind
#   near-native-debug  near-native with the D3D12 debug layer (tranche 29: with -ExtraFlags effect-scan, the scan's barriers checked)
#   pix       native with pix-capture.flag and pix-f8.txt (« delay spacing »): F8 just before cast 2's click, four PIX captures
# -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateSet('native','original','nocopy','pix','near-native','near-original','near-native-debug')][string[]]$Runs=@('native','original'),[ValidateRange(1,8)][int]$Casts=4,
    [string]$PixDelay='70 12',[string]$LookSweep='1,2000,2',[string]$NotBefore='',
    # tranche 26 (gap 16 at 2560x1440 in the OLED's window, WORKFLOW): Run-AlphaCast -GraphicsLevels, restored by hash
    [string]$GraphicsLevels='',
    # tranche 27 (gap 16's native A/B: no-rr, no-rt-scene): flags added to the native runs' ffx-record
    [ValidatePattern('^([a-z0-9-]+(,[a-z0-9-]+)*)?$')][string]$ExtraFlags='',
    # tranche 30 (gap 16, the original's scene target scanned where the effects copy it: effect-scan): flags added to the original runs' ffx-draw-observe
    [ValidatePattern('^([a-z0-9-]+(,[a-z0-9-]+)*)?$')][string]$OriginalFlags='',
    # I-0140 tranche 26 (step 4, the menus' portrait cases): Run-AlphaCast -QuitPortrait, the Quit Game portrait shot after the load
    # (Firelink) and after the arrival (Archdragon Peak), in every run
    [switch]$QuitPortrait)
$ErrorActionPreference='Stop'
$cast=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-AlphaCast.ps1'
$save='C:\Users\User\.ds3-i0039-backups\alpha-allspells-pyro-20261101-164410\DS30000.sl2';$hash='EF4634FDA017AC756EFF86A0F4002A902AFF1164ED5CBE12018ACDD2A8C1E786'
if(-not $env:DS3_PILOT_SCREEN){throw 'DS3_PILOT_SCREEN unset: take it from scripts/Get-Screens.ps1 -Key 1920x1080@60 (the ASUS) or 2560x1440@280 (the OLED, with -GraphicsLevels) first'}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
$f8=Join-Path $BuildFolder 'pix-f8.txt'
$nf=$(if($ExtraFlags){"ffx-record,$ExtraFlags"}else{'ffx-record'})
$of=$(if($OriginalFlags){"ffx-draw-observe,$OriginalFlags"}else{'ffx-draw-observe'})
foreach($r in $Runs){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$r skipped: DarkSoulsIII already runs";break}
    $name="$Prefix-$r";"$(Get-Date -Format HH:mm:ss) start $name"
    $common=@{RunName=$name;BuildFolder=$BuildFolder;SaveCopy=$save;SaveCopySha256=$hash;Continue=$true;BonfirePages=$true;
        TravelPress='803,191;400,280';ArriveSeconds=45;LookSweep=$LookSweep;SpellUpsFirst=2;Casts=$Casts;CastEvery=8;Seconds=420}
    if($GraphicsLevels){$common.GraphicsLevels=$GraphicsLevels}
    if($QuitPortrait){$common.QuitPortrait=$true}
    switch($r){
        'native'  {& $cast @common -BootstrapMode Native -EffectFlags $nf -Video -NoDebugLayer}
        'original'{& $cast @common -BootstrapMode Reference -EffectFlags $of -Video}
        'near-native'  {& $cast @common -BootstrapMode Native -EffectFlags $nf -Video -NoDebugLayer -KeyPath 'Z200'}
        'near-native-debug'{& $cast @common -BootstrapMode Native -EffectFlags $nf -Video -KeyPath 'Z200'}   # tranche 29: with the debug layer
        'near-original'{& $cast @common -BootstrapMode Reference -EffectFlags $of -Video -KeyPath 'Z200'}
        'nocopy'  {& $cast @common -BootstrapMode Native -EffectFlags 'ffx-record,no-ffx-effect-copy' -Video -NoDebugLayer}
        'pix'     {Set-Content -LiteralPath $f8 -Value $PixDelay -NoNewline -Encoding ascii
                   try{& $cast @common -BootstrapMode Native -EffectFlags 'pix-capture' -CaptureAt '2,-1' -NoDebugLayer}finally{Remove-Item -LiteralPath $f8 -Force -ErrorAction SilentlyContinue}}}
    "$(Get-Date -Format HH:mm:ss) end $name"}
"SERIES-DONE $(Get-Date -Format HH:mm:ss)"
