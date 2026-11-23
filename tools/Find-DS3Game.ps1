# I-0137 (I-0120 A1): where Dark Souls III is installed, for the launchers: the Steam library that holds app 374320, read from
# Steam's own records (its folder in the registry, steamapps/libraryfolders.vdf, the app's manifest), never a fixed path. Prints
# the full path of DarkSoulsIII.exe, or nothing when no library holds it; it never throws for a record it cannot read (a library
# on an unplugged disk, a manifest Steam holds): that folder is passed over. -SteamPath stands for the registry (the self-test,
# which the module's build runs).
param([string]$SteamPath='',[switch]$SelfTest)
$ErrorActionPreference='Stop'
function Get-SteamLibraries([string]$Vdf){   # every "path" "<folder>" of libraryfolders.vdf, its escaped backslashes undone
    foreach($m in [regex]::Matches($Vdf,'"path"\s+"((?:[^"\\]|\\.)*)"')){ $m.Groups[1].Value -replace '\\(.)','$1' } }
function Find-DS3GameIn([string]$Steam){   # one Steam folder: its own library and those it lists
    $libraries=@($Steam)
    foreach($list in 'steamapps\libraryfolders.vdf','config\libraryfolders.vdf'){
        try{ $file=[IO.Path]::Combine($Steam,$list)
            if(Test-Path -LiteralPath $file){ $libraries+=@(Get-SteamLibraries (Get-Content -LiteralPath $file -Raw)) } }catch{} }
    foreach($library in ($libraries | Where-Object { $_ } | Select-Object -Unique)){
        try{   # [IO.Path]::Combine, not Join-Path: Join-Path throws for a drive that is not there
            $manifest=[IO.Path]::Combine($library,'steamapps','appmanifest_374320.acf')
            if(-not (Test-Path -LiteralPath $manifest)){ continue }
            $folder=[regex]::Match((Get-Content -LiteralPath $manifest -Raw),'"installdir"\s+"((?:[^"\\]|\\.)*)"')
            if(-not $folder.Success){ continue }
            $game=[IO.Path]::Combine($library,'steamapps','common',($folder.Groups[1].Value -replace '\\(.)','$1'),'Game','DarkSoulsIII.exe')
            if(Test-Path -LiteralPath $game){ return [IO.Path]::GetFullPath($game) }
        }catch{} }
}
function Find-DS3Game([string]$Steam){
    if($Steam){ return Find-DS3GameIn $Steam }
    # Every Steam folder the registry names, in turn: a stale one (an old install's key) does not hide the right one.
    $seen=@()
    foreach($key in 'HKCU:\Software\Valve\Steam','HKLM:\SOFTWARE\WOW6432Node\Valve\Steam','HKLM:\SOFTWARE\Valve\Steam'){
        $entry=Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
        foreach($candidate in @($entry.SteamPath,$entry.InstallPath)){
            if(-not $candidate -or $seen -contains $candidate){ continue }
            $seen+=$candidate;$game=Find-DS3GameIn $candidate
            if($game){ return $game } } }
}
if($SelfTest){
    # A made-up Steam folder whose third library, on a path with a space and an accent, holds the game; its second library is
    # on a drive letter this PC does not have (an unplugged disk).
    $root=Join-Path ([IO.Path]::GetTempPath()) "find-ds3-selftest-$PID";$third=Join-Path $root 'Jeux Vidéo/Steam Library'
    $absent=[char[]](68..90) | Where-Object { -not (Test-Path -LiteralPath "${_}:\") } | Select-Object -First 1
    try{
        New-Item -ItemType Directory -Force -Path (Join-Path $root 'Steam/steamapps'),(Join-Path $third 'steamapps/common/DS3 Folder/Game') | Out-Null
        $escaped={param($p)$p -replace '\\','\\'}
        $unplugged=if($absent){"`t`"1`"`n`t{`n`t`t`"path`"`t`t`"${absent}:\\SteamLibrary`"`n`t}`n"}else{''}
        "`"libraryfolders`"`n{`n`t`"0`"`n`t{`n`t`t`"path`"`t`t`"$(& $escaped (Join-Path $root 'Steam'))`"`n`t}`n$unplugged`t`"2`"`n`t{`n`t`t`"path`"`t`t`"$(& $escaped $third)`"`n`t`t`"apps`"`n`t`t{`n`t`t`t`"374320`"`t`t`"1`"`n`t`t}`n`t}`n}" |
            Set-Content -LiteralPath (Join-Path $root 'Steam/steamapps/libraryfolders.vdf') -Encoding utf8
        if(Find-DS3Game (Join-Path $root 'Steam')){ throw 'self-test: a game found before its manifest exists' }
        "`"AppState`"`n{`n`t`"appid`"`t`t`"374320`"`n`t`"installdir`"`t`t`"DS3 Folder`"`n}" | Set-Content -LiteralPath (Join-Path $third 'steamapps/appmanifest_374320.acf') -Encoding utf8
        if(Find-DS3Game (Join-Path $root 'Steam')){ throw 'self-test: a game found before its executable exists' }
        $exe=Join-Path $third 'steamapps/common/DS3 Folder/Game/DarkSoulsIII.exe';Set-Content -LiteralPath $exe -Value 'x'
        $found=Find-DS3Game (Join-Path $root 'Steam')
        if($found -ne (Resolve-Path -LiteralPath $exe).Path){ throw "self-test: the third library's game not found (got '$found')" }
        if(Find-DS3Game (Join-Path $root 'No Steam Here')){ throw 'self-test: a game found without a Steam folder' }
        if($absent -and (Find-DS3Game "${absent}:\Steam")){ throw 'self-test: a game found on a drive that is not there' }
        "PASS Find-DS3Game: nothing without a manifest or an executable; the game found in a third library on a path with a space and an accent, past a library on a drive that is not there ($(if($absent){"${absent}:"}else{'no free letter: not tried'})); nothing without a Steam folder"
    }finally{ Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue }
    return
}
Find-DS3Game $SteamPath
