param([string]$Out)
# Static replica of PrepareModelMaterialPolicy + ClassifyModelMaterial (native/content/model_parameters.cpp:399-450)
# over every MTD used by a character FLVER (chrmat.tsv). Declarations of the shader source are not replicated (INFERRED: none of the
# classification parameters is declared by the families listed).
$bits=12,17,6,18,4,1,5,9,2,3,7,8,13,14,15,10,19,20,21,22
$pol=Get-Content 'C:\Users\User\Desktop\DS3 Decompilation\evidence\I-0066\artifacts\direct-material-policy-759\policy-data-759.json' -Raw | ConvertFrom-Json
$kind=@{}; foreach($p in $pol.parameters){ $kind[$p.name]=[int]$p.kind }
$mt=@{}; foreach($l in Get-Content 'C:\Users\User\AppData\Local\Temp\claude\C--Users-User-Desktop-DS3-Decompilation--claude-worktrees-wizardly-hawking-24fa47\13d0580e-3f3f-44a0-85f1-24d06b5a7305\scratchpad\static\all-mtd.tsv' | Select-Object -Skip 1){
  $c=$l -split "`t"; $k=$c[0].ToLower(); if(-not $mt.ContainsKey($k)){ $mt[$k]=@{spx=$c[1];p=@{}} }; if($c[2] -notlike 'tex:*'){ $mt[$k].p[$c[2]]=@($c[3],$c[4]) } }
$used=@{}
foreach($l in Get-Content 'C:\Users\User\Desktop\DS3 Decompilation\evidence\I-0067\queries\chr\chrmat.tsv' | Select-Object -Skip 1){ $c=$l -split "`t"; $k=$c[1].ToLower() -replace '^c\d{4}_',''; if(-not $used.ContainsKey($c[1].ToLower())){$used[$c[1].ToLower()]=@()}; $used[$c[1].ToLower()]+=$c[0] }
$rows=@("mtd`tspx`tclass`tflags_hex`ttransparent`talpha_ref`tmat_type`tdepth_write_off`tforward`ttexture_flags`tcharacters")
foreach($name in ($used.Keys | Sort-Object)){
  # chrmat names carry a "cNNNN_" prefix for some per-character MTDs; the MTD dump keeps the file name as is
  $m=$mt[$name]; if(-not $m){ $m=$mt[($name -replace '^c\d{4}_','')]; }
  if(-not $m){ $rows+=("{0}`tNOMTD`t`t`t`t`t`t`t`t`t{1}" -f $name,(($used[$name]|Select-Object -Unique) -join ',')); continue }
  $flags=0; $tr=$false; $ar=0; $mtype=''; $dw=$false
  foreach($pn in $m.p.Keys){
    if(-not $kind.ContainsKey($pn)){continue}; $k=$kind[$pn]; $t=$m.p[$pn][0]; $v=$m.p[$pn][1]
    $first=($v -split ',')[0]
    $raw=0; if($t -eq 'Bool'){ $raw=if($first -eq 'True'){1}else{0} } elseif($t -like 'Int*'){ $raw=[int]$first }
    if($pn -eq 'g_BloodAmount'){ $flags=$flags -bor 4 }
    if($k -eq 6){ if($raw -ne 0){$tr=$true} }
    elseif($k -eq 7){ if($raw -gt $ar){$ar=$raw} }
    elseif($k -eq 16){ $mtype=$raw; if($raw -eq 3){$flags=$flags -bor 0x800} elseif($raw -eq 2){$flags=$flags -bor 0x2000} }
    elseif($k -eq 20){ $dw=($raw -band 255) -ne 0 }
    elseif($k -ge 21 -and $k -le 40){ $mask=1 -shl $bits[$k-21]; if($raw -band 1){$flags=$flags -bor $mask}else{$flags=$flags -band (-bnot $mask)} }
  }
  $fam=$m.spx
  $rnm=$fam.Contains('RNM'); $dol=(-not $rnm) -and $fam.Contains('DOL')
  if($dol){$flags=$flags -bor 0x400}
  $fwd=(($flags -band 0x1000) -ne 0)
  if($rnm -or $dol){$flags=$flags -bor 1}
  elseif($fam.Length -lt 12 -and $fam.Contains('GXFlver_Col')){$fwd=$true;$flags=$flags -bor 0x1001}
  else{ $gi=$fam.IndexOf('GI'); if($gi -ge 0 -and $gi+2 -eq $fam.Length){$flags=$flags -bor 1} }
  if($fam.Contains('Emi')){$flags=$flags -bor 0x8000}
  if($fam.Contains('Glow')){$flags=$flags -bor 0x10000}
  $cls = if($tr -or ($flags -band 0x200000)){2} elseif($fwd){0x80} elseif($flags -band 0x8000){0x100} elseif($ar -ne 0){4} else {1}
  $rows+=("{0}`t{1}`t0x{2:X}`t0x{3:X}`t{4}`t{5}`t{6}`t{7}`t{8}`t{9}`t{10}" -f $name,$fam,$cls,$flags,$tr,$ar,$mtype,$dw,$fwd,'',(($used[$name]|Select-Object -Unique|Sort-Object) -join ','))
}
$rows | Set-Content $Out
