param([ValidateSet('shot','click','rclick','mclick','press','key','hold','wheel','look','place','sweep','stance')][string]$Action,[int]$X,[int]$Y,[string]$Path,[int]$Vk=0x45,[ValidateRange(50,5000)][int]$Ms=1000,[ValidateRange(-20,20)][int]$Notches=-1,[ValidateRange(0,10000)][int]$SettleMs=600,[int]$Vk2=0,[switch]$Exact,[ValidateRange(1,60)][int]$SweepSeconds=10,[ValidateRange(0,[long]::MaxValue)][long]$AtTick=0)
# Thomas (2026-10-09) chose a permanent permission rule for this scripted pilot of
# announced live runs only: screenshot of the primary screen, click and key on
# the DARK SOULS III window. Never used outside an announced D-0116 protocol.
# Live-run pilot without UI automation grants: full-screen capture and real
# SendInput mouse/keyboard events on the primary screen.
# I-0098 (Thomas, 2026-10-24: the gate on each of his two screens): DS3_PILOT_SCREEN (a display device name, \\.\DISPLAY1) names
# the screen the game is placed on (-Action place: the window moved to that screen's top-left, as the game opens at the primary
# screen's); X and Y of the positioned actions (click, rclick, press, wheel) are then relative to that screen's top-left as they
# are to the primary's without it, and a shot captures that screen (the game's, in place of the primary).
Add-Type -AssemblyName System.Drawing,System.Windows.Forms
Add-Type @'
using System;using System.Runtime.InteropServices;
public static class P{
 [DllImport("user32.dll")]public static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")]public static extern void mouse_event(uint f,uint x,uint y,uint d,UIntPtr e);
 [DllImport("winmm.dll")]public static extern uint timeBeginPeriod(uint p);
 [DllImport("winmm.dll")]public static extern uint timeEndPeriod(uint p);
 [DllImport("user32.dll")]public static extern void keybd_event(byte vk,byte scan,uint f,UIntPtr e);
 [DllImport("user32.dll")]public static extern uint MapVirtualKey(uint c,uint t);
 [DllImport("user32.dll")]public static extern IntPtr FindWindow(string c,string n);
 [DllImport("user32.dll")]public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")]public static extern IntPtr GetForegroundWindow();
 [StructLayout(LayoutKind.Sequential)]public struct PT{public int X,Y;}
 [DllImport("user32.dll")]public static extern IntPtr WindowFromPoint(PT p);
 [DllImport("user32.dll")]public static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("user32.dll")]public static extern bool GetWindowRect(IntPtr h,out RECT r);
 [DllImport("user32.dll")]public static extern bool SetWindowPos(IntPtr h,IntPtr after,int x,int y,int cx,int cy,uint flags);
 [DllImport("user32.dll")]public static extern bool IsWindowVisible(IntPtr h);
 [StructLayout(LayoutKind.Sequential)]public struct RECT{public int L,T,R,B;}
 [DllImport("user32.dll")]public static extern bool GetClientRect(IntPtr h,out RECT r);
 [DllImport("user32.dll")]public static extern bool ClientToScreen(IntPtr h,ref PT p);
 [DllImport("user32.dll")]public static extern bool AttachThreadInput(uint a,uint b,bool f);
 [DllImport("kernel32.dll")]public static extern uint GetCurrentThreadId();
 [DllImport("user32.dll")]public static extern bool BringWindowToTop(IntPtr h);
 [DllImport("user32.dll")]public static extern bool ShowWindow(IntPtr h,int c);
 [DllImport("user32.dll")]public static extern bool IsIconic(IntPtr h);
}
'@
# 1173: FindWindow by title can miss the window (observed 0 while it was on screen): fall back to the process's main window.
# I-0132 R11c (the final review's M3): the window found by its title must be DarkSoulsIII's (an Explorer window on the game's folder has
# the same title), else the process's main window.
function Get-GameWindow{ $w=[P]::FindWindow($null,'DARK SOULS III')
 if($w -ne [IntPtr]::Zero){ $wp=0;[void][P]::GetWindowThreadProcessId($w,[ref]$wp);if((Get-Process -Id $wp -ErrorAction SilentlyContinue).ProcessName -ne 'DarkSoulsIII'){ $w=[IntPtr]::Zero } }
 if($w -eq [IntPtr]::Zero){ $gp=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1; if($gp){ $w=$gp.MainWindowHandle } }
 return $w }
$h=Get-GameWindow
# 1116 guard: input actions only when the foreground window belongs to DarkSoulsIII (a closed or
# faulted game must never send clicks or keys to another application).
# I-0132 R11c (2026-11-10: the Claude app, then explorer, kept the foreground through a series and every look was refused): the game's
# window brought to the foreground through the foreground thread's input (AttachThreadInput), as a plain SetForegroundWindow from
# this background process is refused by Windows; the input is still sent only when the window under it is the game's (Assert-GameWindow).
function Focus-Game([IntPtr]$w){ if($w -eq [IntPtr]::Zero -or [P]::GetForegroundWindow() -eq $w){return}; $fp=0;$ft=[P]::GetWindowThreadProcessId([P]::GetForegroundWindow(),[ref]$fp);$me=[P]::GetCurrentThreadId()
  $att=$ft -ne 0 -and $ft -ne $me -and [P]::AttachThreadInput($me,$ft,$true); if([P]::IsIconic($w)){[void][P]::ShowWindow($w,9)}; [void][P]::BringWindowToTop($w);[void][P]::SetForegroundWindow($w); if($att){[void][P]::AttachThreadInput($me,$ft,$false)}
  # Windows refused it (the foreground lock: explorer kept it, 2026-11-10 10:50): a click on the game window's own title bar (its
  # non-client area, no game input), which the system takes as the user's activation; only for a window with a caption.
  Start-Sleep -Milliseconds 100; if([P]::GetForegroundWindow() -eq $w){return}
  $r=New-Object P+RECT;$c=New-Object P+PT; if([P]::GetWindowRect($w,[ref]$r) -and [P]::ClientToScreen($w,[ref]$c) -and $c.Y-$r.T -ge 20){
   $at=New-Object P+PT;$at.X=[int](($r.L+$r.R)/2);$at.Y=$r.T+12;$under=[P]::WindowFromPoint($at);$up=0;[void][P]::GetWindowThreadProcessId($under,[ref]$up);$wp=0;[void][P]::GetWindowThreadProcessId($w,[ref]$wp)
   if($up -ne $wp -or (Get-Process -Id $wp -ErrorAction SilentlyContinue).ProcessName -ne 'DarkSoulsIII'){return}   # never a click on another application's window
   [void][P]::SetCursorPos($at.X,$at.Y);Start-Sleep -Milliseconds 50
   $under=[P]::WindowFromPoint($at);$up=0;[void][P]::GetWindowThreadProcessId($under,[ref]$up);if($up -ne $wp){return}   # checked again at the click (a toast in between)
   [P]::mouse_event(2,0,0,0,[UIntPtr]::Zero);[P]::mouse_event(4,0,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds 200} }
function Assert-GameWindow([IntPtr]$w){ $pid0=0;[void][P]::GetWindowThreadProcessId($w,[ref]$pid0)
  $p=Get-Process -Id $pid0 -ErrorAction SilentlyContinue;if(-not $p -or $p.ProcessName -ne 'DarkSoulsIII'){throw "foreground is not DarkSoulsIII ($($p.ProcessName)); input refused"} }
$screen=[System.Windows.Forms.Screen]::PrimaryScreen
if($env:DS3_PILOT_SCREEN){ $screen=@([System.Windows.Forms.Screen]::AllScreens | Where-Object { $_.DeviceName -eq $env:DS3_PILOT_SCREEN })[0]
  if(-not $screen){ throw "DS3_PILOT_SCREEN $($env:DS3_PILOT_SCREEN) is not an attached screen" } }
if($Action -in 'click','rclick','press','wheel','stance'){ $X+=$screen.Bounds.X;$Y+=$screen.Bounds.Y
  # I-0113 tranche 19 (Thomas, 2026-11-06 19:18: the tests on the OLED at 2560x1440 while it is on): the positioned actions are
  # authored for the 1280x720 window at the screen's top-left (its client at 8, 31: start-client.txt); a game client of another
  # size takes them at the same place of its picture (the game's interface scales with the client), from the client's own origin.
  # A 1280x720 client keeps them as they are; -Exact (Pilot-TitleContinue, which scales its own) takes them as given.
  # Only a point inside the reference client is scaled (the final review's H1): a title-bar point (y < 31) stays as given, but the
  # reference close box (x 1250..1288, as the runners' 1265, 15) goes to the window's own close box, the same distance from its right.
  if(-not $Exact -and $h -ne [IntPtr]::Zero){ $c=New-Object P+RECT;$o=New-Object P+PT;$w=New-Object P+RECT
   if([P]::GetClientRect($h,[ref]$c) -and [P]::ClientToScreen($h,[ref]$o) -and [P]::GetWindowRect($h,[ref]$w) -and $c.R -gt 0 -and $c.B -gt 0 -and ($c.R -ne 1280 -or $c.B -ne 720)){
    $rx=$X-$screen.Bounds.X;$ry=$Y-$screen.Bounds.Y
    if($ry -ge 31 -and $rx -ge 8 -and $rx -lt 1288 -and $ry -lt 751){ $X=$o.X+[int][Math]::Round(($rx-8)*$c.R/1280.0);$Y=$o.Y+[int][Math]::Round(($ry-31)*$c.B/720.0) }
    elseif($ry -lt 31 -and $rx -ge 1250 -and $rx -lt 1288){ $X=$w.R-(1296-$rx);$Y=$w.T+$ry } } } }
# I-0105: PageUp..Down, End, Home, the arrows, Insert and Delete are extended keys: without KEYEVENTF_EXTENDEDKEY the game reads their scan
# code as the numeric keypad's (the spell run i105-spells-1: Up and Right arrows changed nothing).
function Get-ExtendedFlag([int]$code){ if(($code -ge 0x21 -and $code -le 0x28) -or $code -eq 0x2D -or $code -eq 0x2E){1}else{0} }
switch($Action){
 'place'{
  # I-0098: the game's window (waited for, up to 120 s, until visible) moved to the named screen's top-left, its size kept.
  $until=(Get-Date).AddSeconds(120)
  while(($h -eq [IntPtr]::Zero -or -not [P]::IsWindowVisible($h)) -and (Get-Date) -lt $until){ Start-Sleep -Milliseconds 500
   $h=Get-GameWindow }   # the step review's M: the same check as the first lookup
  if($h -eq [IntPtr]::Zero){ throw 'no DARK SOULS III window to place' }
  $r=New-Object P+RECT;[void][P]::GetWindowRect($h,[ref]$r)
  if($r.L -ne $screen.Bounds.X -or $r.T -ne $screen.Bounds.Y){ [void][P]::SetWindowPos($h,[IntPtr]::Zero,$screen.Bounds.X,$screen.Bounds.Y,0,0,0x15);[void][P]::GetWindowRect($h,[ref]$r) }   # NOSIZE, NOZORDER, NOACTIVATE
  # Step review: a window that did not move fails here, not later as a picture of an empty screen.
  if($r.L -ne $screen.Bounds.X -or $r.T -ne $screen.Bounds.Y){ throw "the window stayed at $($r.L),$($r.T), not at $($screen.DeviceName)'s top-left" }
  "placed window=$h screen=$($screen.DeviceName) rect=$($r.L),$($r.T),$($r.R),$($r.B)"}
 'shot'{
  $b=$screen.Bounds
  $bmp=New-Object System.Drawing.Bitmap $b.Width,$b.Height;$g=[System.Drawing.Graphics]::FromImage($bmp)
  # I-0140 tranche 38 (tranche 37: shots 0.6 to 1.2 s after their target, this process's own start): -AtTick n holds the capture
  # until GetTickCount64 reaches n (the caller starts this process ahead of it): Thread.Sleep(1) under a 1 ms timer period (its step
  # review's M1: PowerShell's Start-Sleep -Milliseconds 1 slept about 15 ms there), then a spin over the last 2 ms. ready= is the
  # tick this process reached the wait (after a warm-up copy of the whole screen into the same bitmap, so the first copy's set-up is not inside the timed one);
  # a process ready only after n captures at once (late = ready - n, else 0). t0= is the tick right before the screen copy. A target
  # more than 30 s ahead is refused, and a game gone during the wait (checked every 250 ms, no check begun in the last 300 ms: its
  # final review's L1) or before the copy prints « gone » and takes no picture.
  $late=$null;$ready=$null
  if($AtTick -gt 0){ $now=[Environment]::TickCount64; if($AtTick -gt $now+30000){ throw "-AtTick $AtTick is $($AtTick-$now) ms ahead (30 s at most)" }
   $g.CopyFromScreen($b.Location,[System.Drawing.Point]::Empty,$b.Size)   # the warm-up: one full copy into the same bitmap (its second step review's L4)
   $ready=[Environment]::TickCount64;$late=[Math]::Max([long]0,$ready-$AtTick);$gone=$false;$seen=$ready
   [void][P]::timeBeginPeriod(1); try{ while([Environment]::TickCount64 -lt $AtTick-2){ [Threading.Thread]::Sleep(1)
     if([Environment]::TickCount64 -ge $seen+250 -and [Environment]::TickCount64 -lt $AtTick-300){ $seen=[Environment]::TickCount64; if(-not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){ $gone=$true; break } } }
    while(-not $gone -and [Environment]::TickCount64 -lt $AtTick){} }finally{ [void][P]::timeEndPeriod(1) }
   if($gone){ "gone at=$AtTick ready=$ready tick=$([Environment]::TickCount64)"; return } }
  $t0=[Environment]::TickCount64
  # I-0140 tranche 30: the capture's moment on GetTickCount64 (the module's row clock), read right after the screen copy.
  $g.CopyFromScreen($b.Location,[System.Drawing.Point]::Empty,$b.Size);$tick=[Environment]::TickCount64
  if($AtTick -gt 0 -and -not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){ "gone at=$AtTick ready=$ready tick=$tick"; return }   # checked after the copy, not between the target and it
  $bmp.Save($Path,[System.Drawing.Imaging.ImageFormat]::Jpeg)
  $r=New-Object P+RECT;if($h -ne [IntPtr]::Zero){[void][P]::GetWindowRect($h,[ref]$r)}
  "screen=$($b.Width)x$($b.Height) window=$h rect=$($r.L),$($r.T),$($r.R),$($r.B) tick=$tick t0=$t0$(if($null -ne $late){" at=$AtTick ready=$ready late=$late"})"}
 'click'{
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  $pt=New-Object P+PT;$pt.X=$X;$pt.Y=$Y;Assert-GameWindow ([P]::WindowFromPoint($pt))
  [void][P]::SetCursorPos($X,$Y);Start-Sleep -Milliseconds 200
  # I-0140 tranche 16 (the weapon arts: Shift with a click): -Vk2 a key held around the click, as -Action hold does.
  if($Vk2){$ext2=Get-ExtendedFlag $Vk2;$s2=[byte][P]::MapVirtualKey($Vk2,0);[P]::keybd_event([byte]$Vk2,$s2,$ext2,[UIntPtr]::Zero);Start-Sleep -Milliseconds 80}
  [P]::mouse_event(2,0,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds 120;[P]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
  if($Vk2){Start-Sleep -Milliseconds 50;[P]::keybd_event([byte]$Vk2,$s2,(2 -bor $ext2),[UIntPtr]::Zero)}
  "clicked $X,$Y window=$h"}
 'rclick'{
  # Menu audit: a right click (the menus' back on mouse and keyboard), foreground DarkSoulsIII only.
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  $pt=New-Object P+PT;$pt.X=$X;$pt.Y=$Y;Assert-GameWindow ([P]::WindowFromPoint($pt))
  [void][P]::SetCursorPos($X,$Y);Start-Sleep -Milliseconds $SettleMs
  Assert-GameWindow ([P]::WindowFromPoint($pt))   # review: checked again after the settle delay
  if($Vk2){$ext2=Get-ExtendedFlag $Vk2;$s2=[byte][P]::MapVirtualKey($Vk2,0);[P]::keybd_event([byte]$Vk2,$s2,$ext2,[UIntPtr]::Zero);Start-Sleep -Milliseconds 80}
  [P]::mouse_event(8,0,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds $Ms;[P]::mouse_event(0x10,0,0,0,[UIntPtr]::Zero)
  if($Vk2){Start-Sleep -Milliseconds 50;[P]::keybd_event([byte]$Vk2,$s2,(2 -bor $ext2),[UIntPtr]::Zero)}
  "right-clicked $X,$Y $Ms window=$h"}
 'stance'{
  # I-0140 tranche 17 (a weapon art held, then an attack: the Stance arts, a bow's art): -Vk2 held (Shift), the right button held,
  # the left button clicked -Ms after it, the right button released 800 ms later, then -Vk2; foreground DarkSoulsIII only. Its
  # step review's M1: every release in the finally (the left button, the right, then -Vk2), whatever stops it. The left click's
  # time is printed (clicked=<UTC>), from which the runner times the cast (its L2).
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  $pt=New-Object P+PT;$pt.X=$X;$pt.Y=$Y;Assert-GameWindow ([P]::WindowFromPoint($pt))
  [void][P]::SetCursorPos($X,$Y);Start-Sleep -Milliseconds 200;Assert-GameWindow ([P]::WindowFromPoint($pt))
  $ext2=0;$s2=[byte]0;$leftDown=$false;if($Vk2){$ext2=Get-ExtendedFlag $Vk2;$s2=[byte][P]::MapVirtualKey($Vk2,0)};$clicked=$null
  try{ if($Vk2){[P]::keybd_event([byte]$Vk2,$s2,$ext2,[UIntPtr]::Zero);Start-Sleep -Milliseconds 80}
   [P]::mouse_event(8,0,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds $Ms
   $clicked=[DateTime]::UtcNow;[P]::mouse_event(2,0,0,0,[UIntPtr]::Zero);$leftDown=$true;Start-Sleep -Milliseconds 120;[P]::mouse_event(4,0,0,0,[UIntPtr]::Zero);$leftDown=$false;Start-Sleep -Milliseconds 800 }
  finally{ if($leftDown){[P]::mouse_event(4,0,0,0,[UIntPtr]::Zero)};[P]::mouse_event(0x10,0,0,0,[UIntPtr]::Zero);if($Vk2){Start-Sleep -Milliseconds 50;[P]::keybd_event([byte]$Vk2,$s2,(2 -bor $ext2),[UIntPtr]::Zero)} }
  "stance $X,$Y $Ms window=$h clicked=$(if($clicked){$clicked.ToString('o')})"}
 'mclick'{
  # DECALS-WAVE1: a middle click (the lock-on button: with no target it turns the camera behind the character, so a following
  # walk starts from the same camera whatever the frame rate), foreground DarkSoulsIII only.
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  Assert-GameWindow ([P]::GetForegroundWindow())
  [P]::mouse_event(0x20,0,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds 150;[P]::mouse_event(0x40,0,0,0,[UIntPtr]::Zero)
  "middle-clicked window=$h"}
 'press'{
  # 1180: a click held -Ms with the cursor placed 600 ms before, for native frames of 130-170 ms (a 120 ms click can fall
  # between two frames and the pointer needs a frame on the target first).
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  $pt=New-Object P+PT;$pt.X=$X;$pt.Y=$Y;Assert-GameWindow ([P]::WindowFromPoint($pt))
  [void][P]::SetCursorPos($X,$Y);Start-Sleep -Milliseconds $SettleMs
  if($Vk2){$ext2=Get-ExtendedFlag $Vk2;$s2=[byte][P]::MapVirtualKey($Vk2,0);[P]::keybd_event([byte]$Vk2,$s2,$ext2,[UIntPtr]::Zero);Start-Sleep -Milliseconds 80}
  [P]::mouse_event(2,0,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds $Ms;[P]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
  if($Vk2){Start-Sleep -Milliseconds 50;[P]::keybd_event([byte]$Vk2,$s2,(2 -bor $ext2),[UIntPtr]::Zero)}
  "pressed $X,$Y $Ms window=$h"}
 'key'{
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  Assert-GameWindow ([P]::GetForegroundWindow())
  $ext=Get-ExtendedFlag $Vk
  $s=[byte][P]::MapVirtualKey($Vk,0);[P]::keybd_event([byte]$Vk,$s,$ext,[UIntPtr]::Zero);Start-Sleep -Milliseconds 120;[P]::keybd_event([byte]$Vk,$s,(2 -bor $ext),[UIntPtr]::Zero)
  "key $Vk window=$h"}
 'wheel'{
  # 1118: mouse wheel over a point of the game window (inventory grids scroll only with the wheel or clicks).
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  $pt=New-Object P+PT;$pt.X=$X;$pt.Y=$Y;Assert-GameWindow ([P]::WindowFromPoint($pt))
  [void][P]::SetCursorPos($X,$Y);Start-Sleep -Milliseconds 150
  for($i=0;$i -lt [Math]::Abs($Notches);$i++){[P]::mouse_event(0x0800,0,0,[BitConverter]::ToUInt32([BitConverter]::GetBytes([int](120*[Math]::Sign($Notches))),0),[UIntPtr]::Zero);Start-Sleep -Milliseconds 120}
  "wheel $Notches at $X,$Y window=$h"}
 'look'{
  # 1118: relative mouse motion (camera) of X pixels in steps, foreground DarkSoulsIII only.
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  Assert-GameWindow ([P]::GetForegroundWindow())
  $n=[Math]::Max(1,[int]([Math]::Abs($X)/20));$dx=[BitConverter]::ToUInt32([BitConverter]::GetBytes([int]([Math]::Sign($X)*20)),0)
  if($X){for($i=0;$i -lt $n;$i++){[P]::mouse_event(1,$dx,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds 15}}
  # Wave 1 last effects: -Y, a vertical motion the same way (negative looks up), for the sky witnesses.
  if($Y){$m=[Math]::Max(1,[int]([Math]::Abs($Y)/20));$dy=[BitConverter]::ToUInt32([BitConverter]::GetBytes([int]([Math]::Sign($Y)*20)),0)
   for($i=0;$i -lt $m;$i++){[P]::mouse_event(1,0,$dy,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds 15}}
  "look $X,$Y window=$h"}
 'sweep'{
  # I-0132 (ROADMAP workstream 2, the GI quality in motion): a camera turn of X relative mouse units at a constant rate over -SweepSeconds,
  # paced by a stopwatch (every 2 ms, the system timer at 1 ms while it runs, the units owed so far: each 30 frames/s frame then gets
  # its share within a unit or two, the step review's L1), foreground DarkSoulsIII only.
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  Assert-GameWindow ([P]::GetForegroundWindow())
  [void][P]::timeBeginPeriod(1)
  try{
  $clock=[Diagnostics.Stopwatch]::StartNew();$total=[double]$SweepSeconds*1000;$sent=0;$front=[P]::GetForegroundWindow()
  while($sent -ne $X){if([P]::GetForegroundWindow() -ne $front){"sweep stopped: the game left the front after $sent units";break}   # the final review's L4
   $owed=[int][Math]::Round($X*[Math]::Min(1.0,$clock.Elapsed.TotalMilliseconds/$total))-$sent
   if($owed){[P]::mouse_event(1,[BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$owed),0),0,0,[UIntPtr]::Zero);$sent+=$owed}
   if($clock.Elapsed.TotalMilliseconds -ge $total -and $sent -eq $X){break};[Threading.Thread]::Sleep(2)}
  }finally{[void][P]::timeEndPeriod(1)}
  "sweep $X over $($clock.Elapsed.TotalSeconds.ToString('0.00')) s window=$h"}
 'hold'{
  if($h -ne [IntPtr]::Zero){Focus-Game $h;Start-Sleep -Milliseconds 300}
  Assert-GameWindow ([P]::GetForegroundWindow())
  # I-0078 (Thomas: sprint = Space held while moving): -Vk2, a second key held over the same time (pressed first, released last).
  if($Vk2){$ext2=Get-ExtendedFlag $Vk2;$s2=[byte][P]::MapVirtualKey($Vk2,0);[P]::keybd_event([byte]$Vk2,$s2,$ext2,[UIntPtr]::Zero);Start-Sleep -Milliseconds 50}
  $ext=Get-ExtendedFlag $Vk
  $s=[byte][P]::MapVirtualKey($Vk,0);[P]::keybd_event([byte]$Vk,$s,$ext,[UIntPtr]::Zero);Start-Sleep -Milliseconds $Ms;[P]::keybd_event([byte]$Vk,$s,(2 -bor $ext),[UIntPtr]::Zero)
  if($Vk2){[P]::keybd_event([byte]$Vk2,$s2,(2 -bor $ext2),[UIntPtr]::Zero)}
  "hold $Vk $Ms window=$h"}
}
