param([Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][int]$First,[Parameter(Mandatory)][string[]]$Runs)
# I-0090 measurement series (I-0088's runner, one announced series): each -Runs item is "<scene>:<variant>[:<flags>]", run in order as one process
# each (its trace job restores every protected file by hash), named i90-NN-<scene>-<variant> from -First. Scenes: m45 (Epsilon at
# the m45 bonfire, standing 60 s), walk (Eta at Firelink, I-0074's walk path), prof (m45 standing with the sampler after 1,200
# world frames). Measure mode without the debug layer and with the collect split (the stamps), except 'debug' (the D3D12 debug
# layer on, a short m45 stay: a validation message fails the frame). Flags are module flags joined by ';' (original-unlock: the
# game's rate 360 and Present(0); no-ui-worker: P1 off; no-state-reuse: I-0090 step 1 off; inline-retire: step 2 off).
$ErrorActionPreference='Continue'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$inv="$repo/investigations/I-0066-independent-native-startup"
$eta='C:/Users/User/.ds3-i0039-backups/shebang-zones-eta-m40-20261014-095203/DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$eps='C:/Users/User/.ds3-i0039-backups/shebang-zones-epsilon-m45-20261013-045053/DS30000.sl2';$epsHash='8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C'
$walk='Z5000,D1500,Z5000,Q1500,Z5000,S6000,Z5000'
function Idle{ $t=(Get-Date).AddMinutes(3); while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 } }
$n=$First
foreach($item in @($Runs -split ',' | Where-Object {$_})){
    $scene,$variant,$flags=$item -split ':';$flags=($flags -replace ';',',');$name='i90-{0:D2}-{1}-{2}' -f $n,$scene,$variant;$n++
    Write-Host "== $name $(Get-Date -Format HH:mm:ss) flags=$flags"
    $e=@{};if($flags){$e.EffectFlags=$flags}
    switch($scene){
        'm45' { & "$inv/Run-ZoneWalk.ps1" -RunName $name -Seconds 240 -TestSave $eps -TestSaveSha256 $epsHash -Measure -NoDebugLayer @e -SettleSeconds 40 -Shots 1 -StaySeconds 60 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'walk' { & "$inv/Run-ZoneWalk.ps1" -RunName $name -Seconds 240 -TestSave $eta -TestSaveSha256 $etaHash -Measure -NoDebugLayer @e -SettleSeconds 40 -KeyPath $walk -Shots 1 -StaySeconds 10 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'prof' { $e.EffectFlags=(@($flags,'sample-profile') | Where-Object {$_}) -join ','
            & "$inv/Run-ZoneWalk.ps1" -RunName $name -Seconds 300 -TestSave $eps -TestSaveSha256 $epsHash -Measure -NoDebugLayer -NoKProfile @e -SettleSeconds 40 -Shots 1 -StaySeconds 120 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        # the Firelink walk with the sampler from world frame 7,000 (after the load and the settle at about 150 frames/s unlocked;
        # sample-profile-start.txt in the build folder for this run only), 75,000 samples a thread
        'walkprof' { $start=Join-Path $Build 'sample-profile-start.txt';Set-Content -LiteralPath $start -Value 7000 -NoNewline
            try{ & "$inv/Run-ZoneWalk.ps1" -RunName $name -Seconds 300 -TestSave $eta -TestSaveSha256 $etaHash -Measure -NoDebugLayer -NoKProfile @e -SettleSeconds 40 -KeyPath "$walk,$walk,$walk" -Shots 1 -StaySeconds 10 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
            finally{ Remove-Item -LiteralPath $start -ErrorAction SilentlyContinue } }
        # the guards (I-0074 'check': the walk's record revalidation and serial walk check, the prepare shadow) on the Firelink walk
        'check' { $e.EffectFlags=(@($flags,'record-revalidation') | Where-Object {$_}) -join ','
            & "$inv/Run-ZoneWalk.ps1" -RunName $name -Seconds 240 -TestSave $eta -TestSaveSha256 $etaHash -Measure -NoDebugLayer @e -SettleSeconds 40 -KeyPath $walk -Shots 1 -StaySeconds 10 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        # I-0078's menu check: the in-game menu and the Status screen under the debug layer (flags not taken)
        'menu' { & "$repo/investigations/I-0078-engine-collect-120-b/Run-MenuCheck.ps1" -RunName $name -Build $Build 2>&1 | Select-Object -Last 3 }
        'debug' {& "$inv/Run-ZoneWalk.ps1" -RunName $name -Seconds 200 -TestSave $eps -TestSaveSha256 $epsHash @e -SettleSeconds 40 -Shots 2 -StaySeconds 30 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        default { throw "unknown scene $scene" }
    }
    Idle
}
"== series end $(Get-Date -Format HH:mm:ss)"
