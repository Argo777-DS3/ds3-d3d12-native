[CmdletBinding()]
param([string[]]$Bosses=@(),[string]$Build='',[switch]$Borderless,[switch]$Keep,[switch]$ListBosses,[switch]$Restore,
    # -Shebang: the Shebang save (every boss defeated, every zone unlocked; WORKFLOW « Live game rule ») is installed as the save
    # for this test, from its preserved copy, hash-checked and checked as bound to this Steam account. -SaveCopy/-SaveCopySha256
    # install another copy (for example a zone copy under ~\.ds3-i0039-backups\shebang-zones-*).
    [switch]$Shebang,[string]$SaveCopy='',[string]$SaveCopySha256='',
    # Observation sessions (Thomas, 2026-10-27 03:54: observe without being killed): -NoAi places test-no-ai.flag (DS3's own
    # debug byte: no enemy takes the player as its target), a dev hook of this test launcher only: the play launcher refuses every
    # test-* name. -PopinObserve and -DiagnosticFlags write the play launcher's diagnostics (popin-observe.flag, e.g.
    # crossing-observe) into this test folder, so its journal test-*.jsonl reads with Get-PopinJournal.ps1 -Runs <journal>.
    [switch]$NoAi,[switch]$PopinObserve,[ValidatePattern('^(?!test-)[a-z0-9-]+$')][string[]]$DiagnosticFlags=@())
# I-0089 (Thomas, 2026-10-22): the TEST launcher, separate from the play launcher (Launch-DS3-DX12.ps1, which stays as it is):
# the same native D3D12 start, in a folder of its own (%LOCALAPPDATA%\DS3-DX12-Test) with test-tools.flag, which switches on
# two test tools of the module: F4 = invincibility on/off (a high beep when on, a low beep when off, the window title says
# [TEST]) and the revival of the defeated bosses (test-revive.txt: the boss-defeated event flags cleared once the world has
# run, a middle beep; F5 clears them again). A zone reloads its boss when you rest at a bonfire or travel: the boss is back.
# Like every test run the game starts without the online services (the two offline notices at the title): do not go online with it.
# THE SAVES ARE A SANDBOX: they are copied first to %USERPROFILE%\.ds3-test-backups\<date>, a marker (PENDING.json) says a
# restoration is owed, and when the game has exited they are put back, file by file by SHA-256 (what the game wrote during the
# test is thrown away, new files are moved to the backup folder), then the marker goes. If the console was closed before that,
# the next start refuses to go on until `-Restore` has put the saves back. -Keep leaves the saves as the test changed them.
# -Bosses: nothing (invincibility only), all, a flag id, a key (see -ListBosses) or a map (m30_00); several allowed.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$table=(Get-Content -LiteralPath (Join-Path $repo 'investigations/I-0089-test-tools/boss-flags.json') -Raw | ConvertFrom-Json).bosses
if($ListBosses){$table | ForEach-Object {'{0}  {1,-12} {2,-7} {3} - {4}  ({5})' -f $_.flag,$_.key,$_.map,$_.zone,$_.name,$_.confidence};return}
function Pause-Exit([string]$text){Write-Host $text;Read-Host 'Entrée pour fermer' | Out-Null;exit 1}
$saves=Join-Path $env:APPDATA 'DarkSoulsIII'
$backups=Join-Path $env:USERPROFILE '.ds3-test-backups'
$marker=Join-Path $backups 'PENDING.json'
# Puts the saves back from a backup folder against the hashes taken before the test; every file is retried, nothing stops the rest.
function Restore-Saves([string]$backup,$before){
    $failed=@();$restored=0
    foreach($relative in $before.Keys){
        $target=Join-Path $saves $relative;$done=$false
        for($try=1;$try -le 6 -and -not $done;$try++){
            try{
                if((Test-Path -LiteralPath $target) -and (Get-FileHash -LiteralPath $target).Hash -eq $before[$relative]){$done=$true;break}
                New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
                # What is about to be overwritten is kept first (a restoration run late must not lose what was played since).
                if((Test-Path -LiteralPath $target) -and $try -eq 1){$kept=Join-Path $backup ('_avant-restauration\' + ($relative -replace '[\/]','_'));New-Item -ItemType Directory -Force -Path (Split-Path -Parent $kept) | Out-Null;Copy-Item -LiteralPath $target -Destination $kept -Force}
                Copy-Item -LiteralPath (Join-Path $backup $relative) -Destination $target -Force;$restored++
                if((Get-FileHash -LiteralPath $target).Hash -eq $before[$relative]){$done=$true}
            }catch{Start-Sleep -Seconds 2}
        }
        if(-not $done){$failed+=$relative}
    }
    $extra=Join-Path $backup '_nouveaux-fichiers'
    try{foreach($file in Get-ChildItem -LiteralPath $saves -Recurse -File -Force){
        $relative=$file.FullName.Substring($saves.Length).TrimStart('\')
        if(-not $before.ContainsKey($relative)){New-Item -ItemType Directory -Force -Path $extra | Out-Null;Move-Item -LiteralPath $file.FullName -Destination (Join-Path $extra ($relative -replace '[\\/]','_')) -Force}}
    }catch{$failed+='(fichiers nouveaux)'}
    [pscustomobject]@{Failed=$failed;Restored=$restored}
}
function Read-Marker{$m=Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json;$h=@{};foreach($p in $m.files.PSObject.Properties){$h[$p.Name]=$p.Value};[pscustomobject]@{Backup=$m.backup;Before=$h}}
if($Restore){
    if(-not (Test-Path -LiteralPath $marker)){Write-Host 'Aucune restauration en attente.';return}
    $m=Read-Marker
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){Pause-Exit 'Ferme Dark Souls III avant de restaurer.'}
    $r=Restore-Saves $m.Backup $m.Before
    if($r.Failed.Count){Pause-Exit "ATTENTION : non restauré ($($r.Failed -join ', ')). La copie intacte est dans $($m.Backup)."}
    Set-Content -LiteralPath (Join-Path $m.Backup 'restored-ok.txt') -Value (Get-Date -Format o);Remove-Item -LiteralPath $marker -Force
    Write-Host "Sauvegardes restaurées ($($r.Restored) fichier(s) remis en place).";Start-Sleep -Seconds 4;return
}
if(Test-Path -LiteralPath $marker){Pause-Exit "Une restauration des sauvegardes est restée en attente d'un test précédent (copie : $((Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json).backup)). Lance d'abord : pwsh -File Launch-DS3-DX12-Test.ps1 -Restore"}
if(-not $Build){$Build=Join-Path $repo 'evidence/I-0066/artifacts/game-bootstrap-build'}
$game=& (Join-Path $PSScriptRoot 'Find-DS3Game.ps1')   # I-0137 (I-0120 A1): where Steam installed it, not a fixed path
if(-not $game){Pause-Exit 'Dark Souls III introuvable dans les bibliothèques Steam de ce PC.'}
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){Pause-Exit 'Dark Souls III tourne déjà : ferme-le avant de lancer la version de test.'}
foreach($f in 'game_bootstrap.dll','game_bootstrap_ctl.exe'){if(-not (Test-Path -LiteralPath (Join-Path $Build $f))){Pause-Exit "Module introuvable : $Build\$f"}}
if((Get-FileHash -LiteralPath $game).Hash -ne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'){Pause-Exit 'Version du jeu inattendue (le module vise la 1.15.2).'}
# The boss selection, resolved to flag ids before anything is touched.
$ids=@()
foreach($want in @($Bosses | ForEach-Object {$_ -split '[,\s]+'} | Where-Object {$_})){
    $hit=if($want -ieq 'all'){$table}else{@($table | Where-Object {"$($_.flag)" -eq $want -or $_.key -ieq $want -or $_.map -ieq $want})}
    if(-not $hit){Pause-Exit "Boss inconnu : $want (liste : -ListBosses)."}
    $ids+=@($hit | ForEach-Object {$_.flag})
}
$ids=@($ids | Select-Object -Unique)
if($Shebang -and -not $SaveCopy){
    $SaveCopy=Join-Path $env:USERPROFILE '.ds3-i0039-backups/shebang-alpha-prepared-20261007-134754/DS30000.sl2'
    $SaveCopySha256='4D6263B30550C43CC062927E82B31C6668C2129F279FE744F0C85F9254FC4346'
}
$install=$null
if($SaveCopy){
    if(-not $SaveCopySha256){Pause-Exit 'Donne aussi -SaveCopySha256 avec -SaveCopy.'}
    if(-not (Test-Path -LiteralPath $SaveCopy) -or (Get-FileHash -LiteralPath $SaveCopy).Hash -ne $SaveCopySha256){Pause-Exit "Copie de sauvegarde absente ou différente de son empreinte : $SaveCopy"}
    $check=pwsh -NoProfile -File (Join-Path $repo 'investigations/I-0066-independent-native-startup/Test-SaveSteamId.ps1') -Save $SaveCopy | ConvertFrom-Json
    if(-not $check.md5_all_ok -or $check.entries_with_id -lt 1){Pause-Exit 'Cette sauvegarde n''est pas liée à ton compte Steam.'}
    $install=$SaveCopy
}
$play=Join-Path $env:LOCALAPPDATA 'DS3-DX12-Test'
New-Item -ItemType Directory -Force -Path $play | Out-Null
foreach($f in 'game_bootstrap.dll','game_bootstrap_ctl.exe','game_bootstrap.pdb'){if(Test-Path -LiteralPath (Join-Path $Build $f)){Copy-Item -LiteralPath (Join-Path $Build $f) -Destination $play -Force}}
# I-0111 (R1): Streamline's DLLs and NVIDIA's models, as the play launcher copies them.
Get-ChildItem -LiteralPath $Build -File | Where-Object {$_.Name -like 'sl.*.dll' -or $_.Name -like 'nvngx_*.dll' -or $_.Name -eq 'NvLowLatencyVk.dll'} | Copy-Item -Destination $play -Force
# I-0132 R11f: the Agility SDK's runtime (the build's D3D12 folder: D3D12Core.dll, d3d12SDKLayers.dll) beside the module, used by
# default since I-0132 R11i (no-agility.flag: Windows' own runtime; WARP and PIX keep it too).
if(Test-Path -LiteralPath (Join-Path $Build 'D3D12')){New-Item -ItemType Directory -Force -Path (Join-Path $play 'D3D12') | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $Build 'D3D12') -File -Filter '*.dll' | Copy-Item -Destination (Join-Path $play 'D3D12') -Force}
Get-ChildItem -LiteralPath $play -Include '*.flag','test-revive.txt' -Recurse | Remove-Item -Force
foreach($flag in 'measure.flag','test-tools.flag'){Set-Content -LiteralPath (Join-Path $play $flag) -Value 'test'}
if($Borderless){Set-Content -LiteralPath (Join-Path $play 'borderless.flag') -Value 'test'}
if($NoAi){Set-Content -LiteralPath (Join-Path $play 'test-no-ai.flag') -Value 'test'}
if($PopinObserve){Set-Content -LiteralPath (Join-Path $play 'popin-observe.flag') -Value 'test'}
foreach($name in $DiagnosticFlags){Set-Content -LiteralPath (Join-Path $play "$name.flag") -Value 'test'}
if($ids.Count){Set-Content -LiteralPath (Join-Path $play 'test-revive.txt') -Value ($ids -join ' ') -Encoding ascii}
# The saves, before the game can write them, with their hashes for the restoration; the marker is written before anything changes.
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$backup=Join-Path $backups $stamp
$before=@{}
New-Item -ItemType Directory -Force -Path $backups | Out-Null
if(Test-Path -LiteralPath $saves){
    Copy-Item -LiteralPath $saves -Destination $backup -Recurse -Force
    foreach($file in Get-ChildItem -LiteralPath $saves -Recurse -File -Force){
        $relative=$file.FullName.Substring($saves.Length).TrimStart('\');$copy=Join-Path $backup $relative
        $hash=(Get-FileHash -LiteralPath $file.FullName).Hash
        if(-not (Test-Path -LiteralPath $copy) -or (Get-FileHash -LiteralPath $copy).Hash -ne $hash){Pause-Exit "Copie de sauvegarde incorrecte : $relative"}
        $before[$relative]=$hash}
    if(-not $Keep){[ordered]@{backup=$backup;created=(Get-Date -Format o);files=$before} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $marker -Encoding utf8}
    # Only copies whose restoration was verified are ever pruned: the newest ten of those stay.
    Get-ChildItem -LiteralPath $backups -Directory | Where-Object {Test-Path -LiteralPath (Join-Path $_.FullName 'restored-ok.txt')} | Sort-Object Name -Descending | Select-Object -Skip 10 | Remove-Item -Recurse -Force
}
$journal=Join-Path $play "test-$stamp.jsonl"
Write-Host 'VERSION DE TEST : F4 = invincibilité (bip aigu = activée, bip grave = coupée), F5 = ranime les boss choisis (bip moyen).'
if($NoAi){Write-Host 'Ennemis sans IA : ils ne te prennent pas pour cible (test-no-ai).'}
if($PopinObserve){Write-Host "Pop-in : chaque objet dessiné est noté dans le journal ($journal)."}
foreach($name in $DiagnosticFlags){Write-Host "Diagnostic : $name."}
if($ids.Count){Write-Host "Boss à ranimer : $($ids -join ', '). Une fois la partie chargée, repose-toi à un feu de camp ou voyage : la zone se recharge."}
if(-not $Keep){Write-Host "Les sauvegardes seront remises telles qu'elles étaient à la fermeture du jeu (copie : $backup). Ne ferme pas cette fenêtre avant."}
# Security audit L6 (2026-11-23): with -Keep a test save installed stays the active save; the player's own is only in the backup.
elseif($install){Write-Host "ATTENTION : -Keep avec une sauvegarde de test : elle reste ta sauvegarde active après le test. La tienne est copiée dans $backup ; -Restore ne la remet pas (aucun marqueur avec -Keep) : recopie-la à la main."}
Write-Host 'Lancement de Dark Souls III (rendu D3D12 natif, test)...'
$result=@();$failure=$null
try{
    # Inside the try: whatever fails after this point, the finally puts Thomas's saves back.
    if($install){
        $active=@(Get-ChildItem -LiteralPath $saves -Recurse -File -Filter 'DS30000.sl2')
        if($active.Count -ne 1){throw 'Une seule sauvegarde active DS30000.sl2 était attendue.'}
        Copy-Item -LiteralPath $install -Destination $active[0].FullName -Force
        if((Get-FileHash -LiteralPath $active[0].FullName).Hash -ne $SaveCopySha256){throw 'Installation de la sauvegarde de test incorrecte.'}
        Write-Host "Sauvegarde de test installée : $install"
    }
    $env:SteamAppId='374320';$env:SteamGameId='374320'
    Start-Process 'steam://rungameid/374320'
    $began=Get-Date;$target=$null
    while(-not $target -and ((Get-Date)-$began).TotalSeconds -lt 90){$target=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1;if(-not $target){Start-Sleep -Milliseconds 10}}
    if(-not $target){throw 'Steam n''a pas lancé le jeu.'}
    $result=& (Join-Path $play 'game_bootstrap_ctl.exe') $target.Id (Join-Path $play 'game_bootstrap.dll') $journal native 0
    Write-Host ($result -join "`n")
}catch{$failure=$_.Exception.Message;Write-Host "ERREUR : $failure"}finally{
    # The game must be gone before the saves are put back.
    while(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){Start-Sleep -Seconds 1}
    if(-not $Keep -and $before.Count){
        Start-Sleep -Seconds 3
        $r=Restore-Saves $backup $before
        if($r.Failed.Count){Write-Host "ATTENTION : sauvegardes NON restaurées ($($r.Failed -join ', ')). La copie intacte est dans $backup ; relance avec -Restore.";Read-Host 'Entrée pour fermer' | Out-Null}
        else{Set-Content -LiteralPath (Join-Path $backup 'restored-ok.txt') -Value (Get-Date -Format o);Remove-Item -LiteralPath $marker -Force -ErrorAction SilentlyContinue
            Write-Host "Sauvegardes restaurées ($($r.Restored) fichier(s) remis en place)."}
    }
}
if($failure){Pause-Exit "Le test n'a pas abouti : $failure"}
Get-ChildItem -LiteralPath $play -Filter 'test-*.jsonl' | Sort-Object Name -Descending | Select-Object -Skip 5 | Remove-Item -Force
$crashes=Join-Path $play 'crashes'
if(Test-Path -LiteralPath $crashes){Get-ChildItem -LiteralPath $crashes -Filter '*.dmp' | Sort-Object LastWriteTime -Descending | Select-Object -Skip 5 |
    ForEach-Object {Remove-Item -LiteralPath $_.FullName -Force;Remove-Item -LiteralPath ([IO.Path]::ChangeExtension($_.FullName,'.txt')) -Force -ErrorAction SilentlyContinue}}
if(($result -join ' ') -match '"crash_dumps":[1-9]'){Write-Host "Un minidump a été écrit dans $crashes."}
if(($result -join ' ') -notmatch '"ready":1'){Pause-Exit 'Le module natif n''a pas démarré (journal : '+$journal+').'}
Start-Sleep -Seconds 4
