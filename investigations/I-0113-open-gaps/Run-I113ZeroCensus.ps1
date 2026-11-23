# I-0113 tranche 7 (ZERO-CONSTANTS-CENSUS §5): the constant words natively and in the original at the witness zones, one announced
# slot (Run-ZoneWalk.ps1 each, offline, restored by hash): each zone's Shebang save loaded, 180 s in the zone, no input; natively
# zero-census.flag (native_zero_census), the original with zero-census, decal-observe (the mesh loop 63CB0 and the draw wrappers:
# reference_zero_census) and light-observe (5B3D0: reference_zero_instance). Compare-I113ZeroCensus.py reads each pair.
# -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string[]]$Zones=@('m40','m31','m37','m45','m50','m30'),[ValidateSet('native','original','both')][string]$Modes='both',[string]$NotBefore='')
$ErrorActionPreference='Stop'
$Zones=@($Zones | ForEach-Object { $_ -split ',' } | Where-Object { $_ })   # « m40,m31 » from pwsh -File arrives as one string
$walk=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-ZoneWalk.ps1'
$root='C:\Users\User\.ds3-i0039-backups'
$saves=@{m40=@('shebang-zones-eta-m40-20261014-095203','1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4')
    m31=@('shebang-zones-beta-m31-20261013-044305','93D3A76E8C8CC4EA5D6CB4FE76EBFBF23AA30F2A114422EF355968F651CEB0C0')
    m37=@('shebang-zones-gamma-m37-anorlondo-20261026-123818','E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411')
    m45=@('shebang-zones-epsilon-m45-20261013-045053','8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C')
    m50=@('shebang-zones-delta-m50-20261013-044839','CA2DC958DA179DAA5B9A7BFF45C63736B7D1E0CBB6D95035C900B03EA5FFC7DF')
    m30=@('shebang-zones-theta-m30-20261014-095541','397FDB20DC014B708586ECD7C427BB90C15E9F37CA8B09964A4DF0471F667578')}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($zone in $Zones){if(-not $saves.ContainsKey($zone)){"$zone skipped: no save";continue}
    foreach($mode in 'native','original'){if($Modes -ne 'both' -and $Modes -ne $mode){continue}
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$zone $mode skipped: DarkSoulsIII already runs";break}
        $run="$Prefix-$zone-$mode";"$(Get-Date -Format HH:mm:ss) $run"
        $native=$mode -eq 'native'
        & $walk -RunName $run -BootstrapMode $(if($native){'Native'}else{'Reference'}) -TestSave (Join-Path $root "$($saves[$zone][0])\DS30000.sl2") `
            -TestSaveSha256 $saves[$zone][1] -Seconds 180 -SettleSeconds 25 -BuildFolder $BuildFolder -Shots 1 `
            -EffectFlags $(if($native){'test-no-ai,zero-census'}else{'test-no-ai,zero-census,decal-observe,light-observe'})
        "$(Get-Date -Format HH:mm:ss) $run done"}}
