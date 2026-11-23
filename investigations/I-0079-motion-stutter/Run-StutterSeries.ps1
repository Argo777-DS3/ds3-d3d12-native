param([Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][string[]]$Plan,[string]$NotBefore='',[string]$Tag='')
# I-0079: a series of Run-Stutter runs in one process (each waits for the previous game to close). -Plan items are
# "name:mode:motion[:sample start[:flags]]", e.g. "i79-01-walk-native-prof:Native:walk:2300"; modes Native, NativeDebug (the
# debug layer on), NativeNoK (I-0081: measure mode with the collect timers off), Reference. -NotBefore HH:mm (the PC's
# local time) holds the first launch until the announced start.
$ErrorActionPreference='Continue'
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null); while((Get-Date) -lt $at){ Start-Sleep 5 } }
function Idle{ $t=(Get-Date).AddMinutes(3); while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 } }
foreach($item in @($Plan -split ';' | Where-Object {$_})){
    $f=$item -split ':';$name=$f[0];$mode=$f[1];$debug=$mode -eq 'NativeDebug';$noK=$mode -eq 'NativeNoK';if($debug -or $noK){$mode='Native'};$motion=$f[2];$sample=if($f.Count -gt 3 -and $f[3]){[int]($f[3] -split '-')[0]}else{0};$sampleEnd=if($f.Count -gt 3 -and $f[3] -like '*-*'){[int]($f[3] -split '-')[1]}else{0};$flags=if($f.Count -gt 4){$f[4]}else{''}
    "== $name $(Get-Date -Format HH:mm:ss)"
    & "$PSScriptRoot/Run-Stutter.ps1" -RunName $name -Build $Build -Mode $mode -Motion $motion -SampleStart $sample -SampleEnd $sampleEnd -EffectFlags $flags -DebugLayer:$debug -NoKProfile:$noK 2>&1 | Select-Object -Last 3
    Idle; Start-Sleep 5
}
"== series end $(Get-Date -Format HH:mm:ss)"
