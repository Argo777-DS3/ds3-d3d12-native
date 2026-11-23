# MTD-FLAG-BITS-INVENTORY-SHEET census: evaluates the original policy word gxMtd+0x88 (= material+0x24 without bit 28)
# for every MTD of AllMaterialBnd from the authored parameters, exactly as the original handler does:
#   reader+0x3C from FUN_1400A7E40 (65-row builtin table, bit = value & 1; MaterialType 3 -> 0x800, 2 -> 0x2000;
#   g_BloodAmount -> |= 4), then the shader-name bits of FUN_14002DD60.
# Usage: pwsh -NoProfile -File Census-MtdBits.ps1 [-Out <dir>]
# Inputs (Evidence Store): I-0068 all-mtd.tsv, builtin-material-parameters.tsv; I-0063 flver.tsv; name tables read from .data
# (evidence/I-0066/queries/mtd-bits/name-tables.tsv). Static: the declaration check ("a matching SPX declaration wins")
# cannot be evaluated here; the result is the upper bound over authored builtin names.
param([string]$Out)
$ErrorActionPreference='Stop'
$root=Resolve-Path (Join-Path $PSScriptRoot '..\..')
if(-not $Out){$Out=Join-Path $root 'evidence\I-0066\queries\mtd-bits'}
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$ev=Join-Path $root 'evidence'
$builtin=@{}
foreach($l in Get-Content (Join-Path $ev 'I-0068\queries\glow\builtin-material-parameters.tsv')){
  if($l.StartsWith('#')){continue}; $p=$l -split "`t"; $builtin[$p[1]]=[int]$p[2] }
$tables=Get-Content (Join-Path $Out 'name-tables.tsv') | ForEach-Object { $p=$_ -split "`t"; [pscustomobject]@{Addr=$p[0];Text=$p[1]} }
$t1=@($tables | Where-Object { [Convert]::ToUInt64($_.Addr,16) -ge 0x1447371e0 -and [Convert]::ToUInt64($_.Addr,16) -lt 0x1447371e0+43*0x200 } | ForEach-Object Text)
$t2=@($tables | Where-Object { [Convert]::ToUInt64($_.Addr,16) -ge 0x144731be0 -and [Convert]::ToUInt64($_.Addr,16) -lt 0x144731be0+43*0x200 } | ForEach-Object Text)
$cloth27='GXFlver_SSSEmiIblGlow_Bloody_Cloth'
$bitOf=@{21=12;22=17;23=6;24=18;25=4;26=1;27=5;28=9;29=2;30=3;31=7;32=8;33=13;34=14;35=15;36=10;37=19;38=20;39=21;40=22}
$bitName=@{0='GI/RNM/DOL/GXFlver_Col name';1='WindEffect';2='PaintDecal_Mat / g_BloodAmount';3='PaintDecal_Disp';4='PlanarReflection';5='Refraction';6='WaterDepth';7='SnowSurface';8='DeepSnow';9='FrozenEffect';10='DominantLightMap / DOL name';11='SSS (MaterialType 3)';12='ForceForward / GXFlver_Col name';13='Cloth (MaterialType 2 / GXFT_Cloth)';14='ClothMatching';15='Emissive / Emi name';16='Glow name';17='WaterInteraction';18='WaterApplyDynamicLight';19='Eclipse';20='InvalidLighting';21='Recursive';22='SATBlendShader';23='MultiBlend4 name';24='MultiBlend3 name';25='MultiBlend2 name';26='alt-set shader table 1447371E0';27='shader table 144731BE0 / SSSEmiIblGlow_Bloody_Cloth';28='program-set predicate (virtual)'}
function Truth([string]$type,[string]$v){ if($type -eq 'Bool'){ if($v -eq 'True'){return 1}else{return 0} }
  $d=0.0; if([double]::TryParse($v,[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$d)){ return [int64]$d }; return 0 }
$rows=Import-Csv -Delimiter "`t" (Join-Path $ev 'I-0068\queries\npc-families\all-mtd.tsv')
$by=[ordered]@{}
foreach($r in $rows){ $k=$r.mtd.ToLowerInvariant(); if(-not $by.Contains($k)){$by[$k]=New-Object System.Collections.ArrayList}; [void]$by[$k].Add($r) }
# MTDs without any authored parameter are absent from all-mtd.tsv: take them (and their shader) from the I-0063 scan.
$spxOf=@{}
foreach($r in Import-Csv -Delimiter "`t" (Join-Path $ev 'I-0063\artifacts\scan\mtd.tsv')){ $spxOf[$r.mtd.ToLowerInvariant()]=($r.shader -replace '\.spx$','') }
$noParams=@()
foreach($k in $spxOf.Keys){ if(-not $by.Contains($k)){ $by[$k]=New-Object System.Collections.ArrayList; [void]$by[$k].Add([pscustomobject]@{mtd=$k;shader=$spxOf[$k];param='';type='';value=''}); $noParams+=$k } }
$res=@{}
foreach($k in $by.Keys){
  $list=$by[$k]; $spx=($list[0].shader -replace '\.spx$','')
  [uint32]$w=0
  foreach($r in $list){
    if($r.param -eq 'g_BloodAmount'){ $w=$w -bor 4 }
    if(-not $builtin.ContainsKey($r.param)){continue}
    $kind=$builtin[$r.param]; $val=Truth $r.type $r.value
    if($kind -eq 16){ if($val -eq 3){$w=$w -bor 0x800}elseif($val -eq 2){$w=$w -bor 0x2000} }
    elseif($kind -ge 21 -and $kind -le 40){ $m=[uint32](1 -shl $bitOf[$kind]); if($val -band 1){$w=$w -bor $m}else{$w=$w -band (-bnot $m)} }
  }
  $rnm=$spx.Contains('RNM'); $dol=$spx.Contains('DOL')
  if($t1 -contains $spx){$w=$w -bor 0x4000000}
  if($spx -eq $cloth27 -or ($t2 -contains $spx)){$w=$w -bor 0x8000000}
  if(-not $rnm -and $dol){$w=$w -bor 0x400}
  $bit12=($w -band 0x1000) -ne 0
  if($rnm -or $dol){$w=$w -bor 1}
  elseif($spx.Length -lt 12 -and $spx.Contains('GXFlver_Col')){$w=$w -bor 0x1001}
  else{ $i=$spx.IndexOf('GI'); if($i -ge 0 -and $i+2 -eq $spx.Length){$w=$w -bor 1} }
  if($spx.Contains('Emi')){$w=$w -bor 0x8000}
  if($spx.Contains('Glow')){$w=$w -bor 0x10000}
  if($spx.Contains('MultiBlend4')){$w=$w -bor 0x800000}
  if($spx.Contains('MultiBlend3')){$w=$w -bor 0x1000000}
  if($spx.Contains('MultiBlend2')){$w=$w -bor 0x2000000}
  $res[$k]=[pscustomobject]@{mtd=$k;spx=$spx;word=$w}
}
$lines=@("mtd`tspx`tword`tbits")
foreach($k in ($res.Keys | Sort-Object)){ $r=$res[$k]; $bs=@(0..28 | Where-Object { $r.word -band [uint32](1 -shl $_) }); $lines+="$($r.mtd)`t$($r.spx)`t0x{0:X8}`t$($bs -join ',')" -f $r.word }
[IO.File]::WriteAllLines((Join-Path $Out 'mtd-policy-bits.tsv'),$lines)
# per-bit summary
$sum=@("bit`tmask`tmtds`tspx_families`tmeaning")
foreach($b in 0..28){ $m=[uint32](1 -shl $b); $hit=@($res.Values | Where-Object { $_.word -band $m }); $spxs=@($hit | ForEach-Object spx | Sort-Object -Unique)
  $sum+="$b`t0x{0:X}`t$($hit.Count)`t$($spxs.Count)`t$($bitName[$b])" -f $m }
[IO.File]::WriteAllLines((Join-Path $Out 'bit-summary.tsv'),$sum)
# usage by domain from the FLVER scan (materials, meshes) per bit
$flv=Import-Csv -Delimiter "`t" (Join-Path $ev 'I-0063\artifacts\scan\flver.tsv')
$use=@{}
foreach($r in $flv){
  $base=($r.mtd -split '\\')[-1].ToLowerInvariant(); if(-not $res.Contains($base)){continue}
  $c=$r.container -replace '\\','/'
  $dom=if($c -match '/map/(m\d\d)_'){'map '+$Matches[1]}elseif($c -match '/chr/'){'chr'}elseif($c -match '/obj/'){'obj'}elseif($c -match '/parts/'){'parts'}elseif($c -match '/sfx/'){'sfx'}else{'other'}
  $w=$res[$base].word
  foreach($b in 0..28){ if($w -band [uint32](1 -shl $b)){ $key="$b`t$dom"; if(-not $use.ContainsKey($key)){$use[$key]=@{mat=0;mesh=0;mtd=@{}}}; $use[$key].mat++; $use[$key].mesh+=[int]$r.meshes; $use[$key].mtd[$base]=1 } }
}
$ul=@("bit`tdomain`tmaterials`tmeshes`tmtds")
foreach($k in ($use.Keys | Sort-Object { [int](($_ -split "`t")[0]) },{$_})){ $u=$use[$k]; $ul+="$k`t$($u.mat)`t$($u.mesh)`t$($u.mtd.Count)" }
[IO.File]::WriteAllLines((Join-Path $Out 'bit-usage.tsv'),$ul)
"mtds: $($res.Count)"
