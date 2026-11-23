# I-0123 RR-a: Ray Reconstruction against DLSS Super Resolution at the witness zones (a copy of I-0122's Run-R4ZoneTour.ps1, D-0217:
# its measure differs: the screen's pictures, not the light pairs). Each zone's Shebang save, natively, Run-ZoneWalk.ps1 (offline,
# restored by hash), 60 s, three looks and their pictures (walk-1..3.png), rt-scene; -Runs: zone@WIDTHxHEIGHT[@flag;flag] entries,
# the third part the run's own extra flags (no-rr: DLSS Super Resolution; dlss-quality: the scaled mode), named into the run's name.
# One process; -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string[]]$Runs=@('m40@1280x720','m40@1280x720@no-rr','m40@1920x1080','m40@1920x1080@no-rr','m40@1531x861','m40@1531x861@no-rr',
        'm40@1920x1080@dlss-quality','m40@1920x1080@no-rr;dlss-quality','m30@1280x720','m30@1280x720@no-rr','m37@1280x720','m37@1280x720@no-rr',
        'm45@1280x720','m45@1280x720@no-rr'),[string]$NotBefore='',[int]$Seconds=60,[string]$Log='')
$ErrorActionPreference='Stop'
$Runs=@($Runs | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$walk="$repo/investigations/I-0066-independent-native-startup/Run-ZoneWalk.ps1"
$root='C:\Users\User\.ds3-i0039-backups'
$saves=@{m40=@('shebang-zones-eta-m40-20261014-095203','1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4')
    m31=@('shebang-zones-beta-m31-20261013-044305','93D3A76E8C8CC4EA5D6CB4FE76EBFBF23AA30F2A114422EF355968F651CEB0C0')
    m37=@('shebang-zones-gamma-m37-anorlondo-20261026-123818','E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411')
    m45=@('shebang-zones-epsilon-m45-20261013-045053','8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C')
    m50=@('shebang-zones-delta-m50-20261013-044839','CA2DC958DA179DAA5B9A7BFF45C63736B7D1E0CBB6D95035C900B03EA5FFC7DF')
    m30=@('shebang-zones-theta-m30-20261014-095541','397FDB20DC014B708586ECD7C427BB90C15E9F37CA8B09964A4DF0471F667578')}
$say={param($t) if($Log){$t | Add-Content $Log}else{$t}}
try{
    $screen=@(& "$repo/scripts/Get-Screens.ps1" -Key '1920x1080@60')[-1].device
    if(-not $screen){throw 'no 1920x1080@60 screen'}
    $env:DS3_PILOT_SCREEN=$screen;& $say "$(Get-Date -Format HH:mm:ss) screen $screen"
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    foreach($entry in $Runs){$zone,$size,$extra=$entry -split '@';$w,$h=($size -split 'x' | ForEach-Object {[int]$_})
        if(-not $saves.ContainsKey($zone)){& $say "$zone skipped: no save";continue}
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$zone skipped: DarkSoulsIII already runs";break}
        $flags=@('test-no-ai','rt-scene')+@(if($extra){$extra -split ';'})
        $run="$Prefix-$zone-$($w)x$h"+$(if($extra){'-'+($extra -replace ';','-')}else{''});& $say "$(Get-Date -Format HH:mm:ss) $run"
        & $walk -RunName $run -BootstrapMode Native -TestSave (Join-Path $root "$($saves[$zone][0])\DS30000.sl2") -TestSaveSha256 $saves[$zone][1] `
            -Seconds $Seconds -SettleSeconds 20 -Look '-300,0;-300,0;-300,0' -Shots 3 -BuildFolder $BuildFolder -WindowWidth $w -WindowHeight $h `
            -EffectFlags ($flags -join ',') *>&1 | ForEach-Object { & $say "$_" }
        & $say "$(Get-Date -Format HH:mm:ss) $run done"}
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
