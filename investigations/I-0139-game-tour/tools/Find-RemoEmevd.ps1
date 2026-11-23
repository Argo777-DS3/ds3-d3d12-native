# I-0139 cutscenes-events sheet: which remo bundles and event files exist in Data1, DLC1 and DLC2 (names built from the dictionary's hash, prime 37),
# and extract the event files (licensed bytes outside Git). Read-only on the game; derived from I-0068 Recover-NamedGameData.ps1.
[CmdletBinding()] param([string]$OutDir='C:\Users\User\Tools\DS3-Decompilation\gamedata-i0139\cutscenes-events',[string]$Report='')
$ErrorActionPreference='Stop'
$game='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game'; $tools='C:\Users\User\.codex\I0033-tools\BinderTool-v0.5.2'
Add-Type -Path (Join-Path $tools 'BouncyCastle.Crypto.dll'); Add-Type -Path (Join-Path $tools 'BinderTool.Core.dll')
$binder=[Reflection.Assembly]::Load([IO.File]::ReadAllBytes((Join-Path $tools 'BinderTool.exe')))
$keyType=$binder.GetType('BinderTool.DecryptionKeys',$true)
$hashFn=$binder.GetType('BinderTool.FileNameDictionary').GetMethod('GetHashCode',[Reflection.BindingFlags]'Static,Public,NonPublic')
$wanted=@{}
function Add-Name([string]$n){ $h=[UInt64]($hashFn.Invoke($null,@(('/'+$n),[uint32]37))); $wanted[$h]=$n }
foreach($aa in 20..59){ foreach($bb in 0..3){ foreach($vv in 0..9999){ Add-Name ('remo/s{0:D2}_{1:D2}_{2:D4}.remobnd.dcx' -f $aa,$bb,$vv) } } }
foreach($aa in 10..59){ foreach($bb in 0..3){ foreach($cc in 0..1){ Add-Name ('event/m{0:D2}_{1:D2}_{2:D2}_00.emevd.dcx' -f $aa,$bb,$cc) } } }
Add-Name 'event/common.emevd.dcx'; Add-Name 'event/common_func.emevd.dcx'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$lines=@()
foreach($archive in 'Data1','DLC1','DLC2'){
  $key=[string]$keyType.GetField(($archive -replace '^DLC','Dlc')+'Key',[Reflection.BindingFlags]'NonPublic,Static').GetRawConstantValue()
  $stream=[BinderTool.Core.CryptographyUtility]::DecryptRsa((Join-Path $game "$archive.bhd"),$key)
  try{ $index=[BinderTool.Core.Bhd5.Bhd5File]::Read($stream,[BinderTool.Core.GameVersion]::DarkSouls3) } finally { $stream.Dispose() }
  $bdt=$null
  try{ foreach($bucket in $index.GetBuckets()){ foreach($entry in $bucket.GetEntries()){
    $h=[UInt64]$entry.FileNameHash; if(-not $wanted.ContainsKey($h)){continue}
    $n=$wanted[$h]; $lines+=("{0}`t{1}`t{2}" -f $archive,$n,$entry.PaddedFileSize)
    if($n -like 'event/*'){
      if(-not $bdt){ $bdt=[BinderTool.Core.Bdt5.Bdt5FileStream]::OpenFile((Join-Path $game "$archive.bdt"),[IO.FileMode]::Open,[IO.FileAccess]::Read) }
      $range=$bdt.Read([Int64]$entry.FileOffset,[Int64]$entry.PaddedFileSize); try{ $bytes=[byte[]]$range.ToArray() } finally { $range.Dispose() }
      if($entry.AesKey){ $ms=[IO.MemoryStream]::new($bytes,$true); try{ [BinderTool.Core.CryptographyUtility]::DecryptAesEcb($ms,[byte[]]$entry.AesKey.Key,$entry.AesKey.Ranges); $bytes=[byte[]]$ms.ToArray() } finally { $ms.Dispose() } }
      if($entry.FileSize -gt 0 -and $entry.FileSize -lt $bytes.Length){ $bytes=$bytes[0..([int]$entry.FileSize-1)] }
      [IO.File]::WriteAllBytes((Join-Path $OutDir ($archive.ToLower()+'_'+($n -replace '/','_'))),$bytes) } } } } finally { if($bdt){$bdt.Dispose()} }
}
if($Report){ $lines | Set-Content -LiteralPath $Report -Encoding utf8 }
$lines
