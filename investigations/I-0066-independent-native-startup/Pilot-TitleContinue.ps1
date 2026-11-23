[CmdletBinding()]
param([Parameter(Mandatory)][string]$Trace,[ValidateRange(30,600)][int]$TimeoutSeconds=240,[ValidateRange(0,720)][int]$MenuY=525,[ValidateRange(500,20000)][int]$ContinueDelayMs=1500)
# I-0066 (1123): scripted title sequence of an announced live run (Shebang/Thomas saves, offline): skip the intro,
# dismiss the two offline notices, then Continue. Each step waits for its screen by sampling the pixels of the
# notice's OK button border (orange) on the primary screen; inputs go through Pilot-Game.ps1 (foreground guard).
$ErrorActionPreference='Stop'
$pilot=Join-Path $PSScriptRoot 'Pilot-Game.ps1'
Add-Type -AssemblyName System.Drawing,System.Windows.Forms
# Fable review (2026-10-18): the measured coordinates below are screen coordinates of a 1280x720 client at (8,31). For another
# window size (Run-StartupTrace -WindowWidth/-WindowHeight) they are scaled onto the game's actual client rectangle.
Add-Type @'
using System;using System.Runtime.InteropServices;
public static class TC{
 [DllImport("user32.dll")]public static extern IntPtr FindWindow(string c,string n);
 [StructLayout(LayoutKind.Sequential)]public struct RECT{public int L,T,R,B;}
 [StructLayout(LayoutKind.Sequential)]public struct PT{public int X,Y;}
 [DllImport("user32.dll")]public static extern bool GetClientRect(IntPtr h,out RECT r);
 [DllImport("user32.dll")]public static extern bool ClientToScreen(IntPtr h,ref PT p);
}
'@
$script:ox=8;$script:oy=31;$script:sx=1.0;$script:sy=1.0
function Measure-Client{ $h=[TC]::FindWindow($null,'DARK SOULS III'); if($h -eq [IntPtr]::Zero){ $gp=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1; if($gp){ $h=$gp.MainWindowHandle } }
    if($h -eq [IntPtr]::Zero){ return }; $r=New-Object TC+RECT; $p=New-Object TC+PT; if(-not [TC]::GetClientRect($h,[ref]$r) -or $r.R -lt 320 -or $r.B -lt 200){ return }; [void][TC]::ClientToScreen($h,[ref]$p)
    $script:ox=$p.X;$script:oy=$p.Y;$script:sx=$r.R/1280.0;$script:sy=$r.B/720.0 }
function SX([int]$x){ [int]($script:ox+($x-8)*$script:sx) }
function SY([int]$y){ [int]($script:oy+($y-31)*$script:sy) }
function Pixel([int]$x,[int]$y){ $b=New-Object Drawing.Bitmap 1,1;$g=[Drawing.Graphics]::FromImage($b)
    $g.CopyFromScreen((SX $x),(SY $y),0,0,(New-Object Drawing.Size 1,1));$c=$b.GetPixel(0,0);$g.Dispose();$b.Dispose();$c }
# I-0098: on the screen DS3_PILOT_SCREEN names (Pilot-Game.ps1), the window is first placed there, the pixels are read where it
# is, and the clicks are given relative to that screen's top-left as Pilot-Game takes them.
$script:origin=[Windows.Forms.Screen]::PrimaryScreen.Bounds.Location
if($env:DS3_PILOT_SCREEN){ $script:origin=@([Windows.Forms.Screen]::AllScreens | Where-Object { $_.DeviceName -eq $env:DS3_PILOT_SCREEN })[0].Bounds.Location
    pwsh -NoProfile -File $pilot -Action place 2>&1 | Select-Object -Last 1;if($LASTEXITCODE){ throw 'the game window could not be placed on its screen' } }
function Click([int]$x,[int]$y){ pwsh -NoProfile -File $pilot -Action click -X ((SX $x)-$script:origin.X) -Y ((SY $y)-$script:origin.Y) -Exact | Out-Null }   # scaled here (Pilot-Game -Exact)
# The OK border is a dim orange (measured 1123: about 70-105, 40-60, 12-30) at x=546, rows 414-440 (first notice) and 437-463 (second).
function Orange([int]$y){ Measure-Client; $hits=0; foreach($dy in 0,4,8){ $c=Pixel 546 ($y+$dy); if($c.R -ge 60 -and $c.R -le 140 -and $c.R -gt $c.G+15 -and $c.G -gt $c.B+10){ $hits++ } }; $hits -ge 2 }
$deadline=[DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
function Wait([scriptblock]$ready,[string]$what){ while(-not (& $ready)){ if([DateTime]::UtcNow -gt $deadline){ throw "timeout waiting for $what" }; Start-Sleep -Milliseconds 500 } }
# The UI is running once the trace has presented 250 frames.
if($Trace -ne 'none'){ Wait { (Test-Path -LiteralPath $Trace) -and (Select-String -LiteralPath $Trace -Pattern '"serial":250,' -SimpleMatch:$false -Quiet) } 'native UI frames' } # Reference traces carry no native frames: pass -Trace none at the title.
# I-0096: nine launches on 2026-10-23 stayed on « Press any button »: the network notice's test answered true before the game's
# window was painted, on Steam's library behind it (the DARK SOULS III cover art lies under these pixels: i96-pix-2), so the
# pilot stopped pressing and waited for an offline notice that never came (no 01_010_MessageBox movie in those runs). The notice now counts when seen twice 500 ms apart, and when the offline notice
# does not follow within 8 s the pilot goes back to pressing; each such retry is counted in the line this pilot ends with.
$step=0;$retries=0
for(;;){
    $seen=0
    while($seen -lt 2){
        if([DateTime]::UtcNow -gt $deadline){ throw 'timeout waiting for the network notice' }
        if(Orange 418){ $seen++ }else{ $seen=0
            if($step++ % 6 -eq 0){ Click 648 300; pwsh -NoProfile -File $pilot -Action key -Vk 0x45 | Out-Null } }
        Start-Sleep -Milliseconds 500 }
    Click 648 428
    $until=[DateTime]::UtcNow.AddSeconds(8)
    while(-not ((Orange 446) -and -not (Orange 418)) -and [DateTime]::UtcNow -lt $until){ Start-Sleep -Milliseconds 500 }
    if((Orange 446) -and -not (Orange 418)){ break }
    if([DateTime]::UtcNow -gt $deadline){ throw 'timeout waiting for the offline notice' }
    $retries++;Click 648 300;pwsh -NoProfile -File $pilot -Action key -Vk 0x45 | Out-Null }   # a press even if the false notice persists
Click 648 451
Start-Sleep -Milliseconds $ContinueDelayMs   # the original's main menu appears later than the native one (Reference: about 6000)
if($MenuY -eq 0){ "title reached $([DateTime]::UtcNow.ToString('o')) notice_retries=$retries"; return }
Click 648 $MenuY
"continue clicked $([DateTime]::UtcNow.ToString('o')) notice_retries=$retries"
