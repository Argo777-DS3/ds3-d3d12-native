# I-0140 (the spells' audit): one sheet per spell from a native and an original run of Run-I140Spells (the same family): the native
# row above the original's, the cast's shots at -ShotsMs (cast-<k>-<ms>.png, the game's client 1280x720 at (8,31) of the screen
# picture, scaled to 480x270), named by the k-th id of the family's list: sheets/<family>/<k>-<id>.png. ffmpeg on the PATH.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][ValidateSet('pyromancies','sorceries','miracles','sorceries2','miracles2','thomas','thomas-weapon','thomas-tools','thomas-buffs','thomas-arts','thomas-status')][string]$Family,
    [int[]]$ShotsMs=@(400,900,1600,3000),[string]$Out='')
$ErrorActionPreference='Stop'
$here=$PSScriptRoot;$repo=Split-Path -Parent (Split-Path -Parent $here)
$art="$repo/evidence/I-0066/artifacts";$nat="$art/$Prefix-$Family-nat";$orig="$art/$Prefix-$Family-orig"
if(-not $Out){$Out="$repo/evidence/I-0140/$Prefix-sheets/$Family"};New-Item -ItemType Directory -Force -Path $Out | Out-Null
$ids=@(Get-Content -LiteralPath "$here/attune-$Family.txt" | ForEach-Object { $_.Trim() } | Where-Object { $_ -and $_ -notmatch '=' })
$blank="color=c=black:s=480x270:d=1"
for($k=1;$k -le $ids.Count;$k++){
    $inputs=@();$filters=@();$n=0
    foreach($dir in $nat,$orig){ foreach($ms in $ShotsMs){ $f="$dir/cast-$k-$ms.png"
        if(Test-Path -LiteralPath $f){$inputs+=@('-i',$f);$filters+="[$n]crop=1280:720:8:31,scale=480:270[v$n]"}else{$inputs+=@('-f','lavfi','-i',$blank);$filters+="[$n]null[v$n]"};$n++ } }
    $cols=$ShotsMs.Count;$row1=(0..($cols-1) | ForEach-Object { "[v$_]" }) -join '';$row2=($cols..(2*$cols-1) | ForEach-Object { "[v$_]" }) -join ''
    $graph=($filters -join ';')+";${row1}hstack=$cols[a];${row2}hstack=$cols[b];[a][b]vstack"
    & ffmpeg -v error -y @inputs -filter_complex $graph -frames:v 1 "$Out/$('{0:D2}' -f $k)-$($ids[$k-1].TrimStart('g')).png"
}
"$($ids.Count) sheets in $Out"
