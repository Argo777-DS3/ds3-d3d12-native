# I-0138 tranche 9 (the TAA's static detail, I-0140 tranche 25's handover): the Firelink west entrance's view of I-0113 tranche 39
# and I-0140 tranche 25 (Eta's m40 save at the bonfire, put outside the west entrance at (228.0 -50.5 595.0), released after 3 s,
# the camera on the entrance by 3 turns of 250 units, a still every 4 s for 24 s and the hold filmed), natively under the main
# view's TAA with each coefficient of -Kernels in turn (taa-kernel.txt written into the build folder; 0: no file, the stage's
# 2.29), optionally -MipBias (taa-mip-bias.txt), then -Extra runs: notaa (no-taa.flag), noreactive (2.29 without the reactive
# weighting), dlaa (dlss-dlaa.flag), dlaa-norr (DLAA with no-rr.flag: NRD's lighting), orig (the original). The view is static (the camera fixed after the sweep): every still of a
# run is the same view, so the controls' crops compare by the view, not by a clock (D-0299 pairs moments; the camera still, the mist,
# the hair and the foliage move). -MipBias is written for every native run but applied only under the main view's TAA (the kernel
# runs, noreactive; notaa none, the DLAA runs DLSS's -1): name it in -Prefix; -AddFlags only to the kernel runs. The ASUS window 1920x1080, offline, restored by hash, no measure (stills and films only).
param(
    [Parameter(Mandatory)][string]$BuildFolder,
    [string]$Prefix='i138-t9',
    [double[]]$Kernels=@(0,4,8,30),
    [double]$MipBias=[double]::NaN,
    [string[]]$Extra=@('notaa','noreactive','dlaa','orig'),
    [string[]]$Only=@(),
    [string]$AddFlags=''   # more flags for the native kernel runs, e.g. sharpen-50 (a tag suffix -<flag>)
)
$ErrorActionPreference='Continue'
# The module's ranges (01_state.inc's NativeTaaTest): a value outside them would be refused there while the run kept its name.
foreach($k in $Kernels){if(-not ($k -ge 0 -and $k -le 30)){throw "Run-I138T9Detail: kernel $k outside [0, 30] (0: the stage's 2.29)"}}
if(-not [double]::IsNaN($MipBias) -and -not ($MipBias -ge -2 -and $MipBias -le 0)){throw "Run-I138T9Detail: mip bias $MipBias outside [-2, 0]"}
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$inv="$repo/investigations/I-0066-independent-native-startup"
Remove-Item -LiteralPath "$BuildFolder/rekindled-settings.ini" -ErrorAction SilentlyContinue
$kfile="$BuildFolder/taa-kernel.txt";$bfile="$BuildFolder/taa-mip-bias.txt"
Remove-Item -LiteralPath $kfile,$bfile -ErrorAction SilentlyContinue
$env:DS3_PILOT_SCREEN=(& "$repo/scripts/Get-Screens.ps1" -Key 1920x1080@60).device
if(-not $env:DS3_PILOT_SCREEN){throw 'no 1920x1080@60 screen'}
$win='ScreenMode=WINDOW;Resolution-WindowScreenWidth=1920;Resolution-WindowScreenHeight=1080'
$s='C:/Users/User/.ds3-i0039-backups/shebang-zones-eta-m40-20261014-095203/DS30000.sl2';$h='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$common=@{BuildFolder=$BuildFolder;SaveCopy=$s;SaveCopySha256=$h;Continue=$true;Casts=0;Seconds=300;GraphicsLevels=$win;Teleport='228.0 -50.5 595.0 3';TeleportWait=6;LookSweepAfter='3,250,1';HoldSeconds=24;HoldShotEvery=4;VideoHold=$true}
$runs=[System.Collections.Generic.List[object]]::new()
foreach($k in $Kernels){$tag=if($k -le 0){'k229'}else{'k'+("$k" -replace '\.','p')};$f='chr-positions';if($AddFlags){$f+=",$AddFlags";$tag+='-'+($AddFlags -replace '[,;]','-')}
    $runs.Add(@{tag=$tag;kernel=$k;mode='Native';flags=$f})}
foreach($e in $Extra){switch($e){
    'notaa'{$runs.Add(@{tag='notaa';kernel=0;mode='Native';flags='chr-positions,no-taa'})}
    'noreactive'{$runs.Add(@{tag='k229-noreactive';kernel=0;mode='Native';flags='chr-positions,no-taa-reactive'})}
    'dlaa'{$runs.Add(@{tag='dlaa';kernel=0;mode='Native';flags='chr-positions,dlss-dlaa'})}
    'dlaa-norr'{$runs.Add(@{tag='dlaa-norr';kernel=0;mode='Native';flags='chr-positions,dlss-dlaa,no-rr'})}   # DLAA over NRD (Ray Reconstruction off)
    'orig'{$runs.Add(@{tag='orig';kernel=0;mode='Reference';flags='chr-positions'})}
    default{throw "unknown extra run $e"}}}
"RUNNER-START $(Get-Date -Format HH:mm:ss) screen=$env:DS3_PILOT_SCREEN build=$BuildFolder"
try{
 foreach($r in $runs){
  if($Only.Count -and $Only -notcontains $r.tag){continue}
  Remove-Item -LiteralPath $kfile,$bfile -ErrorAction SilentlyContinue
  if($r.mode -eq 'Native'){
   if($r.kernel -gt 0){Set-Content -LiteralPath $kfile -Value ([string]::Format([cultureinfo]::InvariantCulture,'{0}',$r.kernel)) -Encoding ascii}
   if(-not [double]::IsNaN($MipBias)){Set-Content -LiteralPath $bfile -Value ([string]::Format([cultureinfo]::InvariantCulture,'{0}',$MipBias)) -Encoding ascii}}
  $name="$Prefix-$($r.tag)"
  "$(Get-Date -Format HH:mm:ss) start $name kernel=$($r.kernel) bias=$MipBias flags=$($r.flags)"
  if($r.mode -eq 'Native'){& "$inv/Run-AlphaCast.ps1" -RunName $name @common -BootstrapMode Native -NoDebugLayer -EffectFlags $r.flags}
  else{& "$inv/Run-AlphaCast.ps1" -RunName $name @common -BootstrapMode Reference -EffectFlags $r.flags}
  "$(Get-Date -Format HH:mm:ss) end $name exit=$LASTEXITCODE"
 }
}catch{"THROW $_"}
finally{Remove-Item -LiteralPath $kfile,$bfile -ErrorAction SilentlyContinue;"RUNNER-END $(Get-Date -Format HH:mm:ss)"}
