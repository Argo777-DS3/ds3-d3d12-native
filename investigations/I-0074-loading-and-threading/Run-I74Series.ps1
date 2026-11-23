param([Parameter(Mandatory)][string]$Tag,[Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][int]$First,
    [string[]]$Only=@('walk','m45','m32'),[string]$Flags='',[int]$SampleStart=3000)
# I-0074 measurement series (one announced series), the I-0070/I-0071 scenes and key paths: the Firelink walk (m40, Eta),
# the m45 steady scene (Epsilon), the Archdragon Peak walk (m32, travelled from Eta's bonfire). Each run is one process
# (its trace job restores every protected file by hash); runs are named i74-native-NN-<scene>-<tag> from -First.
# 'walkprof' is the Firelink walk with the sampler from the first world frame (sample-profile-load: 75,000 samples a thread,
# the engine thread, the render worker and the loader threads), read with Get-SampleProfile.ps1 -SlowFrames. 'm32prof' is
# the Archdragon Peak walk with the sampler from world frame -SampleStart (sample-profile-start.txt in the build folder for
# that run only: the fast travel, the arrival and the walk).
$ErrorActionPreference='Continue'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$inv="$repo/investigations/I-0066-independent-native-startup"
$eta='C:/Users/User/.ds3-i0039-backups/shebang-zones-eta-m40-20261014-095203/DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$eps='C:/Users/User/.ds3-i0039-backups/shebang-zones-epsilon-m45-20261013-045053/DS30000.sl2';$epsHash='8FA5E590599BC92D4CB9BD99E6949439531D1FE82396B53818252F98999A1D9C'
$walk='Z5000,D1500,Z5000,Q1500,Z5000,S6000,Z5000'
$n=$First
function Next([string]$what){ $name='i74-native-{0:D2}-{1}-{2}' -f $script:n,$what,$Tag; $script:n++; Write-Host "== $name $(Get-Date -Format HH:mm:ss)"; return $name }
function Idle{ $t=(Get-Date).AddMinutes(3); while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 } }
function With([string]$extra){ (@($Flags,$extra) | Where-Object {$_}) -join ',' }
foreach($scene in @($Only -split "[,; ]+" | Where-Object {$_})){
    switch($scene){
        'walkprof' { $r=Next 'm40-walkprof'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 300 -TestSave $eta -TestSaveSha256 $etaHash -Measure -NoDebugLayer -EffectFlags (With 'sample-profile-load') -SettleSeconds 40 -KeyPath $walk -Shots 1 -StaySeconds 20 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'walk' { $r=Next 'm40-walk'; $f=With ''; $e=@{}; if($f){$e.EffectFlags=$f}; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 300 -TestSave $eta -TestSaveSha256 $etaHash -Measure -NoDebugLayer @e -SettleSeconds 40 -KeyPath $walk -Shots 1 -StaySeconds 20 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'check' { $r=Next 'm40-walk-check'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 300 -TestSave $eta -TestSaveSha256 $etaHash -Measure -NoDebugLayer -EffectFlags (With 'record-revalidation') -SettleSeconds 40 -KeyPath $walk -Shots 1 -StaySeconds 20 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'm45' { $r=Next 'm45'; $f=With ''; $e=@{}; if($f){$e.EffectFlags=$f}; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 300 -TestSave $eps -TestSaveSha256 $epsHash -Measure -NoDebugLayer -NoKProfile @e -SettleSeconds 40 -Shots 1 -StaySeconds 120 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'm45split' { $r=Next 'm45-split'; $f=With ''; $e=@{}; if($f){$e.EffectFlags=$f}; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 300 -TestSave $eps -TestSaveSha256 $epsHash -Measure -NoDebugLayer @e -SettleSeconds 40 -Shots 1 -StaySeconds 120 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }   # I-0077: m45 with the collect split (kprofile on)
        'm45debug' { $r=Next 'm45-debug'; $f=With ''; $e=@{}; if($f){$e.EffectFlags=$f}; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -TestSave $eps -TestSaveSha256 $epsHash @e -SettleSeconds 40 -Shots 2 -StaySeconds 30 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }   # the D3D12 debug layer on: a validation message fails the frame
        'm45prof' { $r=Next 'm45-prof'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 300 -TestSave $eps -TestSaveSha256 $epsHash -Measure -NoDebugLayer -NoKProfile -EffectFlags (With 'sample-profile') -SettleSeconds 40 -Shots 1 -StaySeconds 120 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }   # the sampler after 1,200 world frames, 30,000 samples
        'm45full' { $r=Next 'm45-fullscreen'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -TestSave $eps -TestSaveSha256 $epsHash -GraphicsLevels 'ScreenMode=FULLSCREEN' -SettleSeconds 40 -Shots 2 -StaySeconds 30 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }   # I-0076: started in full screen, debug layer on
        'm45full1440' { $r=Next 'm45-fullscreen1440'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -TestSave $eps -TestSaveSha256 $epsHash -GraphicsLevels 'ScreenMode=FULLSCREEN;Resolution-FullScreenWidth=2560;Resolution-FullScreenHeight=1440' -SettleSeconds 40 -Shots 2 -StaySeconds 30 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }   # I-0076: started in full screen at the screen's own 2560x1440, debug layer on
        'm45window' { $r=Next 'm45-window1600'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -TestSave $eps -TestSaveSha256 $epsHash -GraphicsLevels 'Resolution-WindowScreenWidth=1600;Resolution-WindowScreenHeight=900' -SettleSeconds 40 -Shots 2 -StaySeconds 30 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }   # I-0076: another window size, debug layer on
        'm45cycle' { $r=Next 'm45-display-cycle'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -TestSave $eps -TestSaveSha256 $epsHash -EffectFlags 'display-cycle' -SettleSeconds 40 -Shots 2 -StaySeconds 30 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }   # I-0076: the options' Apply in play (window 1600x900, full screen 2560x1440 and 1920x1080, window 1280x720, full screen 2560x1440), debug layer on
        'm45borderless' { $r=Next 'm45-borderless-cycle'; & "$inv/Run-ZoneWalk.ps1" -RunName $r -Seconds 240 -TestSave $eps -TestSaveSha256 $epsHash -EffectFlags 'display-cycle,borderless' -GraphicsLevels 'ScreenMode=FULLSCREEN' -SettleSeconds 40 -Shots 2 -StaySeconds 30 -BuildFolder $Build 2>&1 | Select-Object -Last 3 }   # I-0076: borderless.flag, started in full screen (borderless), then the same Apply cycle, debug layer on
        'm32' {$r=Next 'm32-walk'; $f=With ''; $e=@{}; if($f){$e.EffectFlags=$f}; & "$inv/Run-AlphaCast.ps1" -RunName $r -Seconds 420 -SaveCopy $eta -SaveCopySha256 $etaHash -Continue -BonfirePages -TravelPress '803,191;388,280' -ArriveSeconds 45 -StaySeconds 20 -KeyPath 'Z4000,Q700,Z4000,D700,Z4000,S5000' -Casts 0 -Measure -NoDebugLayer @e -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        'm32prof' { $r=Next 'm32-walkprof'; $start=Join-Path $Build 'sample-profile-start.txt'; Set-Content -LiteralPath $start -Value $SampleStart -NoNewline
            try{ & "$inv/Run-AlphaCast.ps1" -RunName $r -Seconds 420 -SaveCopy $eta -SaveCopySha256 $etaHash -Continue -BonfirePages -TravelPress '803,191;388,280' -ArriveSeconds 45 -StaySeconds 20 -KeyPath 'Z4000,Q700,Z4000,D700,Z4000,S5000' -Casts 0 -Measure -NoDebugLayer -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
            finally{ Remove-Item -LiteralPath $start -ErrorAction SilentlyContinue } }
        'sweepcheck' { $r=Next 'm32-sweep-check'; & "$inv/Run-AlphaCast.ps1" -RunName $r -Seconds 420 -SaveCopy $eta -SaveCopySha256 $etaHash -Continue -BonfirePages -TravelPress '803,191;388,280' -ArriveSeconds 45 -StaySeconds 20 -LookSweep '16,250,2' -Casts 0 -Measure -NoDebugLayer -EffectFlags (With 'record-revalidation') -BuildFolder $Build 2>&1 | Select-Object -Last 3 }
        default { throw "unknown scene $scene" }
    }
    Idle
}
"== series end $(Get-Date -Format HH:mm:ss)"
