# I-0139 tranche 4 (sheets/npcs-merchants.md « Firelink standing points »): a place's NPCs in one process. For each row
# « label=tabX,rowY|x y z|x y z|... » (Eta's Travel tab and row of the place's bonfire, then up to nine NPC points): Eta's travel there,
# then F4 (invincible, test-tools.flag next to the module) and for each point Run-AlphaCast -KeySteps T<k> (the player placed -Offset
# metres off the point toward -Toward "x y z" on the ground plane, or along x without it; held 3 s), then -PerNpc (default: E to
# talk, a shot 5 s later; a shop's page ignores Esc, so no menu row is chosen). A shot after each step (step-<i>.png). Natively then in
# the original; one process per mode; restored by hash. A file `stop-tour.txt` in evidence/I-0139 stops the series between two rows.
# 2026-11-14 23:24 (the first runs): travel to another bonfire of the same map (Eta stands at Firelink: its travel to Firelink Shrine
# leaves the list open), e.g. Cemetery of Ash `263,313`; toward the bonfire, a point 1.2 m along x was over a drop. 2026-11-15 01:31:
# the camera looked straight down at several points 1.2 m toward the bonfire (both renderers): -Lift raises the point (metres).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [Parameter(Mandatory)][string[]]$Rows,[string]$Log='',[string]$Screen='1920x1080@60',[string]$GraphicsLevels='',
    [switch]$NoReference,[switch]$NoNative,[double]$Offset=1.2,[string]$Toward='',[double]$Lift=0,[string]$EffectFlags='test-no-ai',
    [string]$PerNpc='w2;69,0;w5')
$ErrorActionPreference='Stop'
if($NoNative -and $NoReference){throw 'Run-I139Npc: -NoNative and -NoReference leave no run'}
if($Toward -and $Toward -notmatch '^-?[0-9.]+ -?[0-9.]+ -?[0-9.]+$'){throw "Run-I139Npc: -Toward '$Toward' is not « x y z »"}
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
$inv=[Globalization.CultureInfo]::InvariantCulture
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    foreach($row in @($Rows | Where-Object { $_ })){
        if($row -notmatch '^([a-z0-9-]+)=([0-9]{3,4}),([0-9]{3})((\|-?[0-9.]+ -?[0-9.]+ -?[0-9.]+){1,9})$'){& $say "$row refused: label=tabX,rowY|x y z[|x y z...]";continue}
        $label=$Matches[1];$tp="$($Matches[2]),191;400,$($Matches[3])"
        $to=$(if($Toward){@($Toward -split ' ' | ForEach-Object { [double]::Parse($_,$inv) })}else{$null})
        $points=@($Matches[4].TrimStart('|') -split '\|' | ForEach-Object { $v=@($_ -split ' ' | ForEach-Object { [double]::Parse($_,$inv) })
            $dx=$Offset;$dz=0.0
            if($to){$ax=$to[0]-$v[0];$az=$to[2]-$v[2];$len=[Math]::Sqrt($ax*$ax+$az*$az);if($len -gt 0.01){$dx=$Offset*$ax/$len;$dz=$Offset*$az/$len}}
            '{0} {1} {2} 3' -f ($v[0]+$dx).ToString('0.0',$inv),($v[1]+$Lift).ToString('0.0',$inv),($v[2]+$dz).ToString('0.0',$inv) })
        $steps='115,0;'+((1..$points.Count | ForEach-Object { "T$_;$PerNpc" }) -join ';')
        if(Test-Path -LiteralPath "$repo/evidence/I-0139/stop-tour.txt"){& $say "stopped before ${label}: stop-tour.txt";break}
        $modes=@();if(-not $NoNative){$modes+='Native'};if(-not $NoReference){$modes+='Reference'}
        foreach($mode in $modes){
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$label $mode skipped: DarkSoulsIII already runs";continue}
            $run="$Prefix-$label-$(if($mode -eq 'Native'){'nat'}else{'orig'})"
            if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$run/restoration.json"){& $say "$run done before: skipped";continue}
            & $say "$(Get-Date -Format HH:mm:ss) $run ($($points.Count) NPCs)"
            $castArgs=@{RunName=$run;BootstrapMode=$mode;SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;BonfirePages=$true;TravelPress=$tp
                ArriveSeconds=45;StaySeconds=5;Casts=0;Seconds=900;BuildFolder=$BuildFolder;EffectFlags=$EffectFlags;Teleport2=($points -join '|');KeySteps=$steps}
            if($GraphicsLevels){$castArgs.GraphicsLevels=$GraphicsLevels}
            # The test tools' flag made here and removed only if made here (one left by an earlier pilot is refused, not inherited).
            $flagPath=Join-Path $BuildFolder 'test-tools.flag'
            if(Test-Path -LiteralPath $flagPath){& $say "$run refused: $flagPath already there";continue}
            New-Item -ItemType File -Path $flagPath -Value 'test' | Out-Null
            try{ & $cast @castArgs *>&1 | ForEach-Object { & $say "$_" } }
            finally{ Remove-Item -LiteralPath $flagPath -Force -ErrorAction SilentlyContinue } } }
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
