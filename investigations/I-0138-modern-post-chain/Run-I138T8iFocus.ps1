# I-0138 tranche 8i: the game's frame limiter out of focus. One live run per mode (Native, then Reference; -NoNative / -NoReference
# skip one), in play (the world, standing still, no AI: flags test-no-ai,shaft-observe), through Run-AlphaCast (-Continue -Casts 0
# -HoldSeconds). A hidden side job watches the run's journal for the world, waits -InFocusSeconds, gives the foreground to the
# taskbar (Shell_TrayWnd: always present, owned by explorer, harmless, no document or app state touched), holds it
# -OutOfFocusSeconds (logging the foreground's title and whether DS3 is in front every 5 s), then gives it back to DS3 for
# -BackSeconds. The side job sends no key and no click; Run-AlphaCast's own hold sends none either (it only sleeps and, with
# -HoldShotEvery, takes shots, not used here).
# Save: the Firelink path's (scripts/replay/firelink.path.json, the one scripts/Invoke-StartCheck.ps1 uses for standing in the world):
# shebang-zones-eta-m40-20261014-095203/DS30000.sl2, SHA-256 1E905EF8...EB4. Screen by its key as Run-I138Cutscenes.ps1.
# World row: the first startup.jsonl row native_model_draw_submitted (Native; Invoke-StartCheck's own world row) or
# native_player_model_update (both modes: the Reference journal has no native_model_draw_submitted). Offline; never run unannounced.
# Run-AlphaCast's -HoldSeconds starts some seconds after that row (load end, arrival): InFocusSeconds should cover them, check
# the log (the hold has no journal row) and the shots of the run.
param([Parameter(Mandatory)][string]$Prefix,[Parameter(Mandatory)][string]$NativeBuild,[Parameter(Mandatory)][string]$ReferenceBuild,[Parameter(Mandatory)][string]$Log,
    [int]$InFocusSeconds=40,[int]$OutOfFocusSeconds=60,[int]$BackSeconds=20,[string]$Screen='1920x1080@60',[switch]$NoNative,[switch]$NoReference,
    [ValidatePattern('^[a-z0-9-]+(,[a-z0-9-]+)*$')][string]$EffectFlags='test-no-ai,shaft-observe')   # tranche 8i's final review M2: + unlock-120 or unlock-unlimited
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2'
$hash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) $l="$(Get-Date -Format HH:mm:ss.fff) $t";for($i=0;$i -lt 5;$i++){try{$l | Add-Content -LiteralPath $Log -ErrorAction Stop;break}catch{Start-Sleep -Milliseconds 100}}}
$side={param($journal,$log,$inS,$outS,$backS)
    Add-Type -Namespace F -Name U -MemberDefinition @'
[DllImport("user32.dll")] public static extern System.IntPtr GetForegroundWindow();
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(System.IntPtr h);
[DllImport("user32.dll")] public static extern System.IntPtr FindWindow(string c,string t);
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(System.IntPtr h,out uint pid);
[DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a,uint b,bool f);
[DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern int GetWindowText(System.IntPtr h,System.Text.StringBuilder s,int n);
[DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
'@
    function Say($t){ $l="$(Get-Date -Format HH:mm:ss.fff) [focus] $t";for($i=0;$i -lt 5;$i++){try{$l | Add-Content -LiteralPath $log -ErrorAction Stop;break}catch{Start-Sleep -Milliseconds 100}} }
    function Title([IntPtr]$h){$b=New-Object System.Text.StringBuilder 256;[void][F.U]::GetWindowText($h,$b,256);$b.ToString()}
    function Give([IntPtr]$h){   # SetForegroundWindow with the thread-attach workaround (no ALT key: nothing is typed into any window)
        $fg=[F.U]::GetForegroundWindow();$p=[uint32]0;$ft=[F.U]::GetWindowThreadProcessId($fg,[ref]$p);$me=[F.U]::GetCurrentThreadId()
        if($ft -ne $me){[void][F.U]::AttachThreadInput($me,$ft,$true)};$ok=[F.U]::SetForegroundWindow($h);if($ft -ne $me){[void][F.U]::AttachThreadInput($me,$ft,$false)};$ok }
    function Report($tag){$game=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1;$fg=[F.U]::GetForegroundWindow()
        Say "$tag foreground '$(Title $fg)', DS3 in front: $(if($game){$fg -eq $game.MainWindowHandle}else{'game gone'})"}
    try{
        $dead=(Get-Date).AddMinutes(8);while((Get-Date) -lt $dead -and -not ((Test-Path -LiteralPath $journal) -and (Select-String -LiteralPath $journal -Pattern '"event":"(native_model_draw_submitted|native_player_model_update)"' -Quiet))){Start-Sleep -Milliseconds 500}
        if((Get-Date) -ge $dead){Say 'no world row in 8 min: no focus change';return}
        Say "world row seen; $inS s in focus";Start-Sleep -Seconds $inS
        $game=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1;if(-not $game){Say 'game gone before the focus change';return}
        $gh=$game.MainWindowHandle;$tray=[F.U]::FindWindow('Shell_TrayWnd',$null);if($tray -eq [IntPtr]::Zero){Say 'no Shell_TrayWnd: no focus change';return}
        Report 'before';Say "giving the foreground to the taskbar: $(Give $tray)"
        $end=(Get-Date).AddSeconds($outS);while((Get-Date) -lt $end -and (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){Report 'out';Start-Sleep -Seconds 5}
    }finally{
        $game=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1
        if($game){ $ok=Give $game.MainWindowHandle;Start-Sleep -Milliseconds 500;Say "foreground given back to DS3: $ok";Report 'back'
            $end=(Get-Date).AddSeconds($backS);while((Get-Date) -lt $end -and (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){Start-Sleep -Seconds 5;Report 'back' } }
        Say 'side job ends' } }
$hold=[math]::Min(720,$InFocusSeconds+$OutOfFocusSeconds+$BackSeconds+30)
# Tranche 8i (c7e0ac515's step review, M3): Run-StartupTrace's -Seconds bounds the whole run (the load, about 150 s to the world,
# then the hold), so it scales with the hold (300 s at least, Run-AlphaCast's 900 at most: the hold capped at 720).
$seconds=[math]::Max(300,$hold+180)
try{
    if($InFocusSeconds+$OutOfFocusSeconds+$BackSeconds+30 -gt 720){throw "a hold of $($InFocusSeconds+$OutOfFocusSeconds+$BackSeconds+30) s is over 720 (Run-AlphaCast's 900 with the load)"}
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "screen $device, hold $hold s"
    $runs=@();if(-not $NoNative){$runs+=,@('Native',"$Prefix-nat",$NativeBuild)};if(-not $NoReference){$runs+=,@('Reference',"$Prefix-orig",$ReferenceBuild)}
    foreach($r in $runs){
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$($r[1]) skipped: DarkSoulsIII already runs";continue}
        $journal="$repo/evidence/I-0066/artifacts/$($r[1])/startup.jsonl";$job=$null
        & $say "$($r[1]) start ($($r[0]))"
        try{
            $job=Start-Job -ScriptBlock $side -ArgumentList $journal,$Log,$InFocusSeconds,$OutOfFocusSeconds,$BackSeconds
            & $cast -RunName $r[1] -BootstrapMode $r[0] -BuildFolder $r[2] -SaveCopy $save -SaveCopySha256 $hash -Continue -Casts 0 -Seconds $seconds -HoldSeconds $hold -EffectFlags $EffectFlags *>&1 | ForEach-Object { & $say "$_" }
        }catch{& $say "$($r[1]) ERROR $_"}
        finally{ if($job){ Wait-Job $job -Timeout 150 | Out-Null;Stop-Job $job -ErrorAction SilentlyContinue;Receive-Job $job -ErrorAction SilentlyContinue | ForEach-Object { & $say "job: $_" };Remove-Job $job -Force -ErrorAction SilentlyContinue }
            & $say "$($r[1]) end" }
    }
}catch{& $say "ERROR $_"}
finally{& $say 'END'}
