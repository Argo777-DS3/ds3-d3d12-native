[CmdletBinding()]
param([string]$Build='',[switch]$ReloadShaders,[switch]$Borderless,
    # I-0097 tranche 4 (Thomas, 2026-10-24): the game's V-sync off (vsync-off.flag: paced to 60 frames/s, then Present(0) on a
    # chain that may tear; the screen's VRR and the driver's V-sync decide), and the screen the game opens on (a key « 2560x1440@280 »
    # or a device name \\.\DISPLAY2, scripts/Get-Screens.ps1; display-screen.txt: the window placed there before its swap chain).
    [switch]$NoVsync,[string]$Screen='',
    # I-0100 (Thomas, 2026-10-26 03:53): popin-observe.flag, the per-object rows of what the native draws (popin_native, with each
    # object's world position and its distance to the camera) in the play journal, across loads and zones; read by
    # investigations/I-0100-popin/Get-PopinJournal.ps1 -Runs <play-*.jsonl>. A diagnostic: it costs a little CPU in play.
    [switch]$PopinObserve,
    # I-0100 (Thomas, 2026-10-26 04:07: « je préfère un gros test »): other diagnostic flags for a play session (e.g. crossing-observe,
    # the slow admissions), written beside the module; names only (letters, digits, dashes), never a test or experiment switch (a test-* name is refused: test-death.flag would kill the player in his save, I-0100 tranche 4 step review L1).
    # I-0114 (final review of tranche 3): the unlocked rate's and Reflex's test flags too (unlock-*, original-unlock, the corrections'
    # A/B switches, the counters, reflex-on, reflex-boost): the rate is -FrameRate's, Reflex -Reflex's.
    # Security audit L5 (2026-11-23): only *-observe names; the development switches (pix-capture, pin-census, no-black-window and
    # the like) stay the runners' tools. A naming rule, not a list: locomotion-observe and jump-observe also pin the frame rate.
    [ValidatePattern('^(?!test-|unlock-)[a-z0-9-]+-observe$')][string[]]$DiagnosticFlags=@(),
    # I-0100 tranche 2 (for Thomas, 2026-10-26: the original object by object): -Original plays the original D3D11 renderer instead
    # (the controller's reference mode, no time bound) with original-probe.flag, so -PopinObserve writes the original's rows
    # (popin_reference, popin_reference_shadow) at the same places; -Borderless, -NoVsync and -Screen belong to the native
    # presentation and are ignored (the original keeps its own screen options).
    [switch]$Original,
    # I-0114 tranche 3 (the rate and Reflex in the options, until R12's page): -FrameRate, a whole number of frames a second from 20
    # to 1000 or Unlimited (the logic's corrections and the unlocked rate; 60 is DS3's own rate), and -Reflex Off, On or Boost (Off by
    # default, Thomas 2026-10-31 22:23), written to the settings file (I-0132 R12b: rekindled-settings.ini beside the module, keys
    # frame_rate and reflex). Each is kept in the play folder until changed; the original renderer (-Original) ignores both.
    [ValidatePattern('^([0-9]{2,4}|[Uu]nlimited)$')][string]$FrameRate='',[ValidateSet('Off','On','Boost')][string]$Reflex='')
# D-0182 (Thomas, 2026-10-16): a double-click launcher that starts Dark Souls III on the native D3D12 renderer for play, without
# the witness instrumentation of the live tests: the latest built module is copied to its own folder (so a test run's flag
# files never reach it), with measure.flag (no periodic readback) and no debug layer (I-0082: opt-in in the module, so no
# flag); the game starts through Steam
# as usual and the controller injects the module, then waits for the game to exit (no time bound). The saves are copied
# first to %USERPROFILE%\.ds3-play-backups\<date> (the last ten kept); nothing is restored afterwards: this is play.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
if(-not $Build){$Build=Join-Path $repo 'evidence/I-0066/artifacts/game-bootstrap-build'}
function Pause-Exit([string]$text){Write-Host $text;Read-Host 'Entrée pour fermer' | Out-Null;exit 1}
# I-0137 (I-0120 A1): the game where Steam installed it, whatever the library (Find-DS3Game.ps1), not a fixed path.
$game=& (Join-Path $PSScriptRoot 'Find-DS3Game.ps1')
if(-not $game){Pause-Exit 'Dark Souls III introuvable dans les bibliothèques Steam de ce PC.'}
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){Pause-Exit 'Dark Souls III tourne déjà : ferme-le avant de lancer la version DX12.'}
foreach($f in 'game_bootstrap.dll','game_bootstrap_ctl.exe'){if(-not (Test-Path -LiteralPath (Join-Path $Build $f))){Pause-Exit "Module introuvable : $Build\$f"}}
if((Get-FileHash -LiteralPath $game).Hash -ne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'){Pause-Exit 'Version du jeu inattendue (le module vise la 1.15.2).'}
$play=Join-Path $env:LOCALAPPDATA 'DS3-DX12'
New-Item -ItemType Directory -Force -Path $play | Out-Null
Copy-Item -LiteralPath (Join-Path $Build 'game_bootstrap.dll') -Destination $play -Force
Copy-Item -LiteralPath (Join-Path $Build 'game_bootstrap_ctl.exe') -Destination $play -Force
# I-0082: the module's PDB beside it, so a crash report names the functions of this module even if the build is redone.
if(Test-Path -LiteralPath (Join-Path $Build 'game_bootstrap.pdb')){Copy-Item -LiteralPath (Join-Path $Build 'game_bootstrap.pdb') -Destination $play -Force}
# I-0111 (R1): Streamline's signed DLLs and NVIDIA's models beside the module, every one the build put in its folder (sl.*,
# nvngx_*, NvLowLatencyVk: the build's list grows with each Streamline feature; final review M1). Without them DLSS is refused at
# start and DS3's own AA filter runs when its Antialiasing option is on (Thomas's play, 2026-10-31 07:14: « sl.interposer.dll absent »).
Get-ChildItem -LiteralPath $Build -File | Where-Object {$_.Name -like 'sl.*.dll' -or $_.Name -like 'nvngx_*.dll' -or $_.Name -eq 'NvLowLatencyVk.dll'} | Copy-Item -Destination $play -Force
# I-0132 R11f: the Agility SDK's runtime (the build's D3D12 folder: D3D12Core.dll, d3d12SDKLayers.dll) beside the module, used by
# default since I-0132 R11i (no-agility.flag: Windows' own runtime; WARP and PIX keep it too).
if(Test-Path -LiteralPath (Join-Path $Build 'D3D12')){New-Item -ItemType Directory -Force -Path (Join-Path $play 'D3D12') | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $Build 'D3D12') -File -Filter '*.dll' | Copy-Item -Destination (Join-Path $play 'D3D12') -Force}
# The dev hooks that change the game (test-* flags and files: no AI, death, invincibility, boss revival; witness-* files: event
# flags, teleport, phantom) belong to the test launcher's folder and the study runners' builds, never to play: any of them here,
# hidden or not, refuses the start before anything is removed (a play is not restored). Since the security audit (M1) the module
# reads witness-* files only in a runner's bounded run, never in a launcher's session.
if(Get-ChildItem -LiteralPath $play -Force | Where-Object {$_.Name -like 'test-*' -or $_.Name -like 'witness-*'}){Pause-Exit "Un outil de test (test-* ou witness-*) est présent dans $play : le lanceur de jeu refuse de démarrer."}
Get-ChildItem -LiteralPath $play -Filter '*.flag' -Force | Remove-Item -Force
# I-0072 (D-0172): every start compiles the listed pipelines while the game boots (no driver library since D-0241), so
# -ReloadShaders only removes the files of the builds before it (the driver libraries and lists v1 and v2, unread) and a
# half-written list; the list native-pipelines.v3.psc stays, so the next start still finds its pipelines ready.
if($ReloadShaders){Get-ChildItem -LiteralPath $play -Force | Where-Object {$_.Name -like 'native-pipelines.v1.*' -or $_.Name -like 'native-pipelines.v2.*' -or $_.Name -like 'native-pipelines*.writing'} | Remove-Item -Force
    Write-Host 'Shaders : les pipelines sont recompilés à chaque démarrage ; les anciens fichiers de cache sont effacés.'}
# The play flags, and only these: two frames in flight with the waitable three-buffer chain are the module's default since
# 2026-10-18 (Thomas: two frames, as in modern games), so the launcher must never place one-frame-slot.flag or
# no-waitable-present.flag.
if($Original){Set-Content -LiteralPath (Join-Path $play 'original-probe.flag') -Value 'play';Write-Host 'Rendu : D3D11 original (référence).'
    if($Borderless -or $NoVsync -or $Screen){Write-Host "-Borderless, -NoVsync et -Screen sont ignorés : le rendu original garde ses propres options d'écran."}
    $Borderless=$false;$NoVsync=$false;$Screen=''}
else{foreach($flag in 'measure.flag','no-kprofile.flag'){Set-Content -LiteralPath (Join-Path $play $flag) -Value 'play'}}
if($PopinObserve){Set-Content -LiteralPath (Join-Path $play 'popin-observe.flag') -Value 'play';Write-Host 'Pop-in : chaque objet dessiné est noté dans le journal (popin-observe).'}
foreach($name in $DiagnosticFlags){Set-Content -LiteralPath (Join-Path $play "$name.flag") -Value 'play';Write-Host "Diagnostic : $name."}Remove-Item -LiteralPath (Join-Path $play 'display-screen.txt') -ErrorAction SilentlyContinue
if($Screen){try{$target=& (Join-Path $repo 'scripts/Get-Screens.ps1') -Key $Screen}catch{Pause-Exit "Écran introuvable : $Screen ($($_.Exception.Message))"}
    Set-Content -LiteralPath (Join-Path $play 'display-screen.txt') -Value $target.device -Encoding ascii;Write-Host "Écran : $($target.key) ($($target.device))."}
# I-0132 R12b: the player's settings, one file beside the module (native/renderer/rekindled_settings.h: « key=value » lines, any
# case, « ; » comments), read once at start; R12c's page will write it too. The module no longer reads the one-word files of
# before (dlss-mode.txt, rr-mode.txt, rt-level.txt, rt-bounces.txt, reflex-mode.txt, fg-mode.txt, frame-rate.txt, sharpen.txt):
# when the file does not exist yet, their words are written into it once, read as the module read them (what it did not
# recognise was the default, so it is not written; a UTF-16 file, which the old parsers of rr, rt-level, reflex, fg and sharpen
# misread, is read as its text: the old launcher wrote ASCII); the old files are then renamed *.migrated, never deleted.
$settings=Join-Path $play 'rekindled-settings.ini'
# The last value of a key that $valid accepts, as the module keeps the last accepted one (the final review's L2).
function Get-Setting([string]$key,[scriptblock]$valid={$true}){if(-not (Test-Path -LiteralPath $settings)){return $null}
    $value=$null;foreach($line in [IO.File]::ReadAllLines($settings)){if($line -match "^\s*$([regex]::Escape($key))\s*=(.*)$"){
        $candidate=$Matches[1].Trim().ToLowerInvariant();if(& $valid $candidate){$value=$candidate}}};$value}
function Set-Setting([string]$key,[string]$value){$lines=[Collections.Generic.List[string]]::new();$done=$false
    if(Test-Path -LiteralPath $settings){foreach($line in [IO.File]::ReadAllLines($settings)){
        if($line -match "^\s*$([regex]::Escape($key))\s*="){if(-not $done){$lines.Add("$key=$value");$done=$true}}else{$lines.Add($line)}}}
    else{$lines.Add('; Dark Souls III: Rekindled, its settings (I-0132 R12b). Written by the launcher and the Rekindled page.')}
    if(-not $done){$lines.Add("$key=$value")}
    [IO.File]::WriteAllText($settings,(($lines -join "`n")+"`n"),[Text.UTF8Encoding]::new($false))}
if(-not (Test-Path -LiteralPath $settings)){$moved=@()
    function Read-OldWord([string]$name){$path=Join-Path $play $name;if(-not (Test-Path -LiteralPath $path)){return $null}
        $text=[IO.File]::ReadAllText($path);if($text){$text.Replace([string][char]0xFEFF,'').Replace([string][char]0,'').Trim().ToLowerInvariant()}else{''}}
    $word=Read-OldWord 'dlss-mode.txt';if($word -in 'off','dlaa','quality','balanced','performance','ultra-performance','ultra_performance','ultraperformance'){
        $moved+="dlss=$(if($word -like 'ultra*'){'ultra-performance'}else{$word})"}
    $word=Read-OldWord 'rr-mode.txt';if($word -match '^[^a-z]*off([^a-z]|$)'){$moved+='rr=off'}
    $word=Read-OldWord 'rt-level.txt';if($word -match '^[^a-z]*pt([^a-z]|$)'){$moved+='level=pt'}
    $word=Read-OldWord 'rt-bounces.txt';if($word -match '[1-9]'){$moved+="bounces=$($Matches[0])"}   # the module clamps to its most
    $word=Read-OldWord 'reflex-mode.txt';if($word -match '^[^a-z]*(on|boost)([^a-z]|$)'){$moved+="reflex=$($Matches[1])"}
    $word=Read-OldWord 'fg-mode.txt';if($word -match '^[^a-z0-9]*(on|x2|x3|x4)([^a-z0-9]|$)'){$moved+="frame_generation=$(if($Matches[1] -eq 'on'){'x2'}else{$Matches[1]})"}
    $word=Read-OldWord 'frame-rate.txt';if($word -eq 'unlimited' -or ($word -match '^[0-9]{1,4}$' -and [int]$word -ge 20 -and [int]$word -le 1000 -and [int]$word -ne 60)){$moved+="frame_rate=$(if($word -eq 'unlimited'){$word}else{([int]$word).ToString()})"}
    $word=Read-OldWord 'sharpen.txt';$number=0.0
    if($word -and $word.Replace(',','.') -match '^([+]?([0-9]*\.?[0-9]+)([eE][+-]?[0-9]+)?)' -and [double]::TryParse($Matches[1],[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$number) -and $number -ge 0.001){
        $moved+="sharpen=$([Math]::Min($number,1.0).ToString([Globalization.CultureInfo]::InvariantCulture))"}
    foreach($entry in $moved){$key,$value=$entry -split '=',2;Set-Setting $key $value}
    if(-not $moved.Count){Set-Setting 'level' 'rt'}
    # The final review's L3: the old files renamed (.migrated, never deleted), so a later removal of the settings file brings the
    # defaults back, not the old words.
    $renamed=@();foreach($name in 'dlss-mode.txt','rr-mode.txt','rt-level.txt','rt-bounces.txt','reflex-mode.txt','fg-mode.txt','frame-rate.txt','sharpen.txt'){
        $path=Join-Path $play $name;if((Test-Path -LiteralPath $path) -and -not (Test-Path -LiteralPath "$path.migrated")){Rename-Item -LiteralPath $path -NewName "$name.migrated";$renamed+=$name}}
    Write-Host "Réglages : $settings créé$(if($moved.Count){', repris des anciens fichiers .txt : '+($moved -join ', ')})$(if($renamed.Count){' ; renommés en .migrated : '+($renamed -join ', ')})."}
# I-0076 (D-0170): -Borderless (Launch-DS3-DX12-Borderless.cmd): full screen becomes the borderless window over the monitor; -NoVsync
# the game's V-sync off. I-0132 R12f (R12c's final review L11): written as the settings' borderless and vsync keys, and only when the
# file has no such key yet, so the page's Screen Mode and V-sync rows (the settings live) keep the player's later choice; the module no
# longer gets borderless.flag or vsync-off.flag from here (both stay the test runs' tools).
$kept=Get-Setting 'borderless' {param($v)$v -in 'on','off','true','false','yes','no','1','0'}
if($Borderless -and $null -ne $kept){Write-Host "Plein écran : -Borderless ignoré, le fichier de réglages garde borderless=$kept (la page Graphismes le change)."}
if($Borderless -and $null -eq $kept){Set-Setting 'borderless' 'on';Write-Host 'Plein écran : fenêtre sans bordure (réglage initial ; la page Graphismes le change ensuite).'}
$kept=Get-Setting 'vsync' {param($v)$v -in 'on','off','true','false','yes','no','1','0'}
if($NoVsync -and $null -ne $kept){Write-Host "V-Sync : -NoVsync ignoré, le fichier de réglages garde vsync=$kept (la page Graphismes la change)."}
if($NoVsync -and $null -eq $kept){Set-Setting 'vsync' 'off';Write-Host 'V-Sync du jeu : désactivée (réglage initial ; la page Graphismes la change ensuite).'}
if($Original -and ($FrameRate -or $Reflex)){Write-Host '-FrameRate et -Reflex sont ignorés avec -Original (le rendu original garde son rythme) : rien n''est écrit.'}
elseif($FrameRate){$unlimited=$FrameRate -match '^[Uu]nlimited$'
    if(-not $unlimited -and ([int]$FrameRate -lt 20 -or [int]$FrameRate -gt 1000)){Pause-Exit "Fréquence refusée : $FrameRate (de 20 à 1000, ou Unlimited)."}
    Set-Setting 'frame_rate' $(if($unlimited){'unlimited'}else{([int]$FrameRate).ToString()})}
if($Reflex -and -not $Original){Set-Setting 'reflex' $Reflex.ToLowerInvariant()}
# As the module reads them (frame_rate_option.h; rekindled_settings.h): what it does not recognise is DS3's 60, Off and no DLSS mode.
$rate=Get-Setting 'frame_rate' {param($v)$v -eq 'unlimited' -or ($v -match '^0*[0-9]{1,4}$' -and [int]$v -ge 20 -and [int]$v -le 1000)}
if($rate -match '^0*[0-9]{1,4}$'){$rate=([int]$rate).ToString()}   # leading zeros as the module reads them
elseif($rate -ne 'unlimited'){$rate='60'}   # I-0132 R12f (its step review): this branch was swallowed by the comment above since R12b
$latency=Get-Setting 'reflex' {param($v)$v -in 'off','on','boost'};if($latency -notin 'on','boost'){$latency='off'}
$upscaler=Get-Setting 'dlss' {param($v)$v -in 'off','dlaa','quality','balanced','performance','ultra-performance','ultra_performance','ultraperformance'}
if($upscaler -in 'ultra_performance','ultraperformance'){$upscaler='ultra-performance'}
if($upscaler -notin 'dlaa','quality','balanced','performance','ultra-performance'){$upscaler='off (TAA)'}
if(-not $Original){Write-Host "Fréquence de la logique : $rate ; Reflex : $latency ; DLSS : $upscaler."
    # I-0114 (final review of tranche 3, CONTRACT « a rate above 60 offered once its mechanics are corrected »): what is not yet.
    if($rate -ne '60'){Write-Host 'Au-dessus ou en dessous de 60 (I-0114) : les effets à fondu, les os, les minuteries d''interface et les portées du lock-on sont corrigés mais pas vus en jeu, le saut au-dessus de 60 seulement ; pas encore corrigés : la physique, le minuteur de cible du lock-on, le fondu des bornes de lumière ; après un demi-tour à 240, la caméra et la ligne suivie peuvent différer de 60 (à l''étude) ; les strafes verrouillés vont 2-3 % trop loin à 240.'}}
# The saves, before the game can write them.
$saves=Join-Path $env:APPDATA 'DarkSoulsIII'
$backups=Join-Path $env:USERPROFILE '.ds3-play-backups'
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
if(Test-Path -LiteralPath $saves){Copy-Item -LiteralPath $saves -Destination (Join-Path $backups $stamp) -Recurse -Force
    Get-ChildItem -LiteralPath $backups -Directory | Sort-Object Name -Descending | Select-Object -Skip 10 | Remove-Item -Recurse -Force}
$journal=Join-Path $play "play-$stamp.jsonl"
Write-Host $(if($Original){'Lancement de Dark Souls III (rendu D3D11 original)...'}else{'Lancement de Dark Souls III (rendu D3D12 natif)...'})
# Security audit H1 (2026-11-23): the module changes the game's code; playing online with it may put the Steam account at risk.
Write-Host 'Hors ligne : ne te connecte pas aux serveurs du jeu avec cette version (dans les options réseau du jeu, règle le lancement sur hors ligne).'
$env:SteamAppId='374320';$env:SteamGameId='374320'
Start-Process 'steam://rungameid/374320'
$began=Get-Date;$target=$null
while(-not $target -and ((Get-Date)-$began).TotalSeconds -lt 90){$target=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1;if(-not $target){Start-Sleep -Milliseconds 10}}
if(-not $target){Pause-Exit 'Steam n''a pas lancé le jeu.'}
$result=& (Join-Path $play 'game_bootstrap_ctl.exe') $target.Id (Join-Path $play 'game_bootstrap.dll') $journal $(if($Original){'reference'}else{'native'}) 0
Write-Host ($result -join "`n")
# Old journals: the last five kept. I-0082: the crash dumps and reports (crashes\play-<date>-crash<N>.dmp/.txt), the last five
# dumps kept; on a crash the controller has already shown the player a message.
Get-ChildItem -LiteralPath $play -Filter 'play-*.jsonl' | Sort-Object Name -Descending | Select-Object -Skip 5 | Remove-Item -Force
$crashes=Join-Path $play 'crashes'
if(Test-Path -LiteralPath $crashes){Get-ChildItem -LiteralPath $crashes -Filter '*.dmp' | Sort-Object LastWriteTime -Descending | Select-Object -Skip 5 |
    ForEach-Object {Remove-Item -LiteralPath $_.FullName -Force;Remove-Item -LiteralPath ([IO.Path]::ChangeExtension($_.FullName,'.txt')) -Force -ErrorAction SilentlyContinue}}
if(($result -join ' ') -match '"crash_dumps":[1-9]'){Write-Host "Un minidump a été écrit dans $crashes."}
if(($result -join ' ') -notmatch '"ready":1'){Pause-Exit 'Le module natif n''a pas démarré (journal : '+$journal+').'}
