# I-0117 T0 (R2-PLAN §4): Eta at Firelink Shrine (hash-checked copy of its m40 zone save, Continue), natively, once with
# rt-scene.flag (every admitted model's BLAS built and compacted: the native_rt_blas rows) and once without, both with
# world-pictures.flag (the lossless tone map, grading volume, display intermediate and glow every 300th world frame), so the
# grading and the glow can be compared between the two (I-0113 gap 4's readers gap4_oracle.py and gap4_blocks.py).
# One process; each Run-ZoneWalk.ps1 restores by hash at its end. -NotBefore HH:mm waits for the announced start (same day).
# T3: -Zone m30 runs Theta at the High Wall of Lothric instead (its hash-checked m30 zone save), for the alpha-tested foliage.
# T4: -View adds rt-view.flag to the 'on' mode (the control view: native_rt_view rows, the rt-view pictures every 300th frame).
# T5: -Zone m45 runs Epsilon at m45 (its hash-checked zone save, as Run-AlphaCast's), the control view's third zone.
# T5b: -Stay keeps the game open after the walk (a PIX capture at 3060 written before the close: i117-t5h-pix's was cut). -Extra adds effect flags to the 'on' mode (pix-capture.flag: PIX captures at presented frames 3060 to 4860, the GPU's times).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string]$NotBefore='',[int]$Seconds=180,[ValidateSet('on','off')][string[]]$Modes=@('on','off'),   # T1: -Modes on
    [ValidateSet('m40','m30','m45')][string]$Zone='m40',[switch]$View,[ValidatePattern('^[a-z0-9,-]*$')][string]$Extra='',[ValidateRange(0,600)][int]$Stay=0)
$ErrorActionPreference='Stop'
$walk=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-ZoneWalk.ps1'
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2'
$hash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
if($Zone -eq 'm30'){$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-theta-m30-20261014-095541\DS30000.sl2'
    $hash='397FDB20DC014B708586ECD7C427BB90C15E9F37CA8B09964A4DF0471F667578'}
if($Zone -eq 'm45'){$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-epsilon-m45-20261013-045053\DS30000.sl2'
    $hash='8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C'}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($mode in $Modes){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$mode skipped: DarkSoulsIII already runs";break}
    $run="$Prefix-$mode";"$(Get-Date -Format HH:mm:ss) $run"
    $flags=if($mode -eq 'on'){'test-no-ai,world-pictures,rt-scene'+$(if($View){',rt-view'}else{''})+$(if($Extra){",$Extra"}else{''})}else{'test-no-ai,world-pictures,no-rt-scene'}   # D-0253: the rays the default, cut for the off leg
    & $walk -RunName $run -BootstrapMode Native -TestSave $save -TestSaveSha256 $hash -Seconds $Seconds -SettleSeconds 20 `
        -Look '-300,0;-300,0;-300,0' -Shots 3 -BuildFolder $BuildFolder -EffectFlags $flags -StaySeconds $Stay
    "$(Get-Date -Format HH:mm:ss) $run done"}
