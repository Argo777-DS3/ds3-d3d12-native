param([string]$Journal='',[ValidateRange(1,115)][int]$Minutes=95,[ValidateRange(20,300)][int]$StaySeconds=45,
    [string]$ShotDir='',[ValidateRange(1,8)][double]$TimeScale=1,[switch]$Once,[ValidateRange(1,10)][int]$MaxMisses=3,[switch]$Blind,
    [string[]]$OnlyZones=@(),[ValidateRange(0,200)][int]$Visits=0,[ValidateRange(0,300)][int]$TitleSeconds=0)
# -OnlyZones (names as below, e.g. 'Beta m31'; pwsh -File passes a list as one string, split on commas or semicolons) tours only those;
# -Visits stops after that many loads (the replayed path gate's memory leg: one zone loaded four times, D-0202 as extended by I-0085).
# -Blind (the original game, which writes no journal): the world and the title are waited for by fixed times (36 s after the
# load's OK, 27 s after the quit's), the screenshots then show what happened; the notices keep their pixel test.
if(-not $Blind -and -not $Journal){ throw 'a journal is needed unless -Blind' }
# I-0085: the night endurance tour, during an announced run on the Shebang zone save (Beta m31, Gamma m37, Delta m50, Epsilon
# m45, Zeta m51; Alpha untouched), started at the title's main menu (Pilot-TitleContinue -MenuY 0). It loads the five zones in a
# loop until -Minutes have passed (or once each with -Once), each stay with camera turns, then Quit Game to the title.
# Unlike I-0081's Pilot-ZoneTour (it missed every other zone, i81-19: the network notice comes after a connection timeout of
# varying length), every state is checked: the title's two notices by Pilot-TitleContinue's pixel test, the world and the title
# by the journal's native_present_pacing rows (content draws above 200 in a zone, 0 at the title). A missed load is retried
# from the title after Escape; -MaxMisses consecutive misses, a fault row or the game's exit end the tour. -TimeScale stretches
# the waits (GPU-based validation runs at a few frames a second). Each line carries the UTC time and GetTickCount64, which
# native_frame_times rows carry too.
$ErrorActionPreference='Stop'
$inv=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup'
$pilot=Join-Path $inv 'Pilot-Game.ps1'
function P([string[]]$a){ pwsh -NoProfile -File $pilot @a | Out-Null }
function Stamp{ "$([DateTime]::UtcNow.ToString('o')) tick=$([Environment]::TickCount64)" }
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
$zones=@(@{name='Beta m31';y=285;scroll=0},@{name='Gamma m37';y=389;scroll=0},@{name='Delta m50';y=493;scroll=0},@{name='Epsilon m45';y=597;scroll=0},@{name='Zeta m51';y=493;scroll=1})
$OnlyZones=@($OnlyZones | ForEach-Object { $_ -split '[,;]' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
if($OnlyZones.Count){ $unknown=@($OnlyZones | Where-Object { $_ -notin $zones.name });if($unknown.Count){ throw "unknown zones: $($unknown -join ', ')" }
    $zones=@($zones | Where-Object { $_.name -in $OnlyZones }) }
# The journal is open for writing by the game (FILE_SHARE_READ): read what was appended since the last call.
$script:offset=0L;$script:frame=0L;$script:draws=-1;$script:fault=''
function Read-Journal{
    if(-not $Journal -or -not (Test-Path -LiteralPath $Journal)){ return }
    $s=[IO.FileStream]::new($Journal,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
    try { if($s.Length -le $script:offset){ return }; $null=$s.Seek($script:offset,[IO.SeekOrigin]::Begin)
        $bytes=New-Object byte[] ($s.Length-$script:offset);$n=$s.Read($bytes,0,$bytes.Length)
        $text=[Text.Encoding]::UTF8.GetString($bytes,0,$n);$cut=$text.LastIndexOf("`n")
        if($cut -lt 0){ return };$script:offset+=[Text.Encoding]::UTF8.GetByteCount($text.Substring(0,$cut+1))
        foreach($line in $text.Substring(0,$cut).Split("`n")){
            if($line -match '^\{"event":"native_present_pacing","frame":(\d+),.*?"content_draws":(\d+)'){ $script:frame=[long]$Matches[1];$script:draws=[int]$Matches[2] }
            # The fault exits (I-0082); a native_crash_dump alone may be a handled exception: the game's exit tells.
            elseif($line -match '^\{"event":"(native_failure|refusal|native_device_removed)"'){ $script:fault=$line } } }
    finally { $s.Dispose() } }
# Waits until a pacing row newer than $after satisfies $test; $false on the timeout, a fault or the game's exit.
function Wait-Pacing([long]$after,[scriptblock]$test,[double]$seconds){
    if($Blind){ Start-Sleep -Seconds ([int](0.3*$seconds*$TimeScale)); return (Alive) }
    $until=[DateTime]::UtcNow.AddSeconds($seconds*$TimeScale)
    while([DateTime]::UtcNow -lt $until){ Read-Journal
        if($script:fault -or -not (Alive)){ return $false }
        if($script:frame -gt $after -and (& $test $script:draws)){ return $true }
        Start-Sleep -Seconds 2 }
    $false }
function Shot([string]$name){ if($ShotDir){ P '-Action','shot','-Path',(Join-Path $ShotDir "$name.jpg") } }
function Title{ pwsh -NoProfile -File (Join-Path $inv 'Pilot-TitleContinue.ps1') -Trace 'none' -MenuY 0 -TimeoutSeconds ([int][Math]::Min(600,150*$TimeScale)) -ContinueDelayMs ([int](3000*$TimeScale)) 2>&1 | Select-Object -Last 1 }
$deadline=[DateTime]::UtcNow.AddMinutes($Minutes)
# A zone's whole cycle: about 55 s besides the stay in i85-long-2 (title, load, quit); 120 s keeps a margin for a slow one.
$cycle=[int](120*[Math]::Min($TimeScale,2))+$StaySeconds
$atTitle=$true;$visit=0;$misses=0;$loads=0
"tour start $(Stamp) minutes=$Minutes stay=$StaySeconds scale=$TimeScale once=$([bool]$Once)"
:tour for($loop=1;;$loop++){
    for($zi=0;$zi -lt $zones.Count;$zi++){ $z=$zones[$zi]
        if($Once -and $loop -gt 1){ break tour }
        # The last title stay is measured too (the gate's memory leg): 40 s at the title before the end.
        if($Visits -and $loads -ge $Visits){ Start-Sleep -Seconds ([int](40*$TimeScale)); "tour visits done $(Stamp)"; break tour }
        if(-not $Once -and [DateTime]::UtcNow.AddSeconds($cycle) -gt $deadline){ "tour deadline $(Stamp)"; break tour }
        if($script:fault){ "tour fault $(Stamp) $script:fault"; break tour }
        if(-not (Alive)){ "tour game gone $(Stamp)"; break tour }
        $visit++
        if(-not $atTitle){ "title $(Title) $(Stamp)"
            # I-0096: -TitleSeconds holds the title before the next load (a memory run measures the title after each visit).
            if($TitleSeconds){ Start-Sleep -Seconds $TitleSeconds; "title held $TitleSeconds s $(Stamp)" } }
        $atTitle=$false
        Shot "v$visit-menu"; P '-Action','click','-X','648','-Y','555'; Start-Sleep -Seconds 3              # Load Game
        if($z.scroll){ P '-Action','wheel','-X','640','-Y','400','-Notches','-1'; Start-Sleep -Seconds 1 }
        Shot "v$visit-list"; P '-Action','click','-X','640','-Y',"$($z.y)"; Start-Sleep -Seconds 2     # the character
        Shot "v$visit-ok"; Read-Journal; $before=$script:frame
        P '-Action','click','-X','501','-Y','428'                                                        # OK
        "zone $($z.name) loading visit=$visit loop=$loop before=$before $(Stamp)"
        if(-not (Wait-Pacing $before { param($d) $d -ge 200 } 120)){
            Shot "v$visit-missed";$misses++
            "zone $($z.name) missed visit=$visit misses=$misses fault=$([bool]$script:fault) $(Stamp)"
            if($script:fault -or -not (Alive) -or $misses -ge $MaxMisses){ "tour stopped $(Stamp)"; break tour }
            # Back to the title screen from wherever the menus stopped (a notice, the main menu, the save list): Escape twice.
            P '-Action','key','-Vk','27'; Start-Sleep -Seconds 3; P '-Action','key','-Vk','27'; Start-Sleep -Seconds 3
            $zi--;continue }
        $misses=0;$loads++
        "zone $($z.name) in world visit=$visit frame=$script:frame $(Stamp)"
        # The stay: a camera turn every 8 s (the view sweeps the zone; no walking, so no fall, fight or death in an unattended run).
        $until=[DateTime]::UtcNow.AddSeconds($StaySeconds*$TimeScale);$turn=0
        while([DateTime]::UtcNow -lt $until -and (Alive) -and -not $script:fault){
            Start-Sleep -Seconds 8;$turn++;P '-Action','look','-X',$(if($turn%4 -lt 2){'600'}else{'-600'}),'-Y','0'; Read-Journal }
        Shot "v$visit-zone"
        "zone $($z.name) leaving visit=$visit $(Stamp)"
        Read-Journal;$before=$script:frame
        P '-Action','key','-Vk','27'; Start-Sleep -Milliseconds 1500                   # Quit Game (Pilot-QuitAndKeepSave's clicks)
        P '-Action','click','-X','525','-Y','390'; Start-Sleep -Milliseconds 1500
        P '-Action','click','-X','742','-Y','191'; Start-Sleep -Milliseconds 1500
        P '-Action','click','-X','648','-Y','556'; Start-Sleep -Milliseconds 1500
        Shot "v$visit-quit"; P '-Action','click','-X','501','-Y','428'
        if(-not (Wait-Pacing $before { param($d) $d -eq 0 } 90)){
            Shot "v$visit-noquit";"zone $($z.name) quit not seen visit=$visit $(Stamp)"
            if($script:fault -or -not (Alive)){ "tour stopped $(Stamp)"; break tour }
            # Still in the world (the last row drew the zone, not a late quit's loading screen): the quit menu missed; once more.
            Read-Journal
            if($script:draws -ge 200){
                P '-Action','key','-Vk','27'; Start-Sleep -Seconds 2
                P '-Action','key','-Vk','27'; Start-Sleep -Milliseconds 1500
                P '-Action','click','-X','525','-Y','390'; Start-Sleep -Milliseconds 1500
                P '-Action','click','-X','742','-Y','191'; Start-Sleep -Milliseconds 1500
                P '-Action','click','-X','648','-Y','556'; Start-Sleep -Milliseconds 1500
                P '-Action','click','-X','501','-Y','428' }
            if(-not (Wait-Pacing $before { param($d) $d -eq 0 } 90)){ Shot "v$visit-stuck";"tour stuck in the world $(Stamp)"; break tour } }
        "zone $($z.name) left visit=$visit $(Stamp)"
        Start-Sleep -Seconds ([int](6*$TimeScale)) } }
"tour end $(Stamp) visits=$visit loads=$loads"
