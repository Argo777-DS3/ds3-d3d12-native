param([Parameter(Mandatory)][string]$Path,[Parameter(Mandatory)][string]$Marks,[Parameter(Mandatory)][string]$Log)
# I-0083 (D-0200): plays a recorded path (scripts/replay/*.path.json) into the DARK SOULS III window from this one process, each
# event sent at its time from the motion start on a high-resolution clock (the I-0079 pilots started one PowerShell per key hold,
# so each hold drifted by a process start and a 300 ms foreground wait). Announced live runs only (D-0116), the same input calls
# as Pilot-Game.ps1 (keybd_event with the key's scan code, mouse_event). Before every event the foreground window must belong
# to DarkSoulsIII, otherwise every held key is released and the replay stops (exit 2): a lost foreground is the environment,
# not the build. -Marks gets motion_start/motion_end and each segment's start and end (GetTickCount64, the clock of the
# journal's tick_ms rows); -Log gets one line per event with its lateness, then the summary (max and mean lateness).
$ErrorActionPreference='Stop'
Add-Type @'
using System;using System.Diagnostics;using System.Runtime.InteropServices;using System.Threading;
public static class ReplayInput{
 [DllImport("user32.dll")]static extern void keybd_event(byte vk,byte scan,uint f,UIntPtr e);
 [DllImport("user32.dll")]static extern void mouse_event(uint f,uint x,uint y,uint d,UIntPtr e);
 [DllImport("user32.dll")]static extern uint MapVirtualKey(uint c,uint t);
 [DllImport("user32.dll")]public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")]public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")]static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("kernel32.dll")]public static extern ulong GetTickCount64();
 [DllImport("winmm.dll")]public static extern uint timeBeginPeriod(uint p);
 [DllImport("winmm.dll")]public static extern uint timeEndPeriod(uint p);
 public static bool GameInFront(uint gamePid){uint pid;GetWindowThreadProcessId(GetForegroundWindow(),out pid);return pid==gamePid;}
 public static void Key(byte vk,bool down){keybd_event(vk,(byte)MapVirtualKey(vk,0),down?0u:2u,UIntPtr.Zero);}
 // Buttons: 1 left, 2 right, 3 middle; a relative move in steps of 20 pixels (Pilot-Game's look).
 public static void Button(int b,bool down){uint f=b==1?(down?2u:4u):b==2?(down?8u:0x10u):(down?0x20u:0x40u);mouse_event(f,0,0,0,UIntPtr.Zero);}
 public static void Move(int dx,int dy){mouse_event(1,(uint)dx,(uint)dy,0,UIntPtr.Zero);}
 // Waits until the stopwatch reaches ms (sleeps to 2 ms before it, then spins), the game's window in front at least every
 // 20 ms and at the end; false as soon as it is not.
 public static bool WaitUntil(Stopwatch w,double ms,uint gamePid){double checkedAt=-100;while(true){double now=w.Elapsed.TotalMilliseconds,left=ms-now;
   if(left<=0)return GameInFront(gamePid);if(now-checkedAt>=20){if(!GameInFront(gamePid))return false;checkedAt=now;}
   if(left>2.5)Thread.Sleep(1);else Thread.SpinWait(50);}}
}
'@
# (Step review: not $path, which is the [string] parameter $Path under another case.)
$recording=Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
if($recording.schema -ne 1){ throw "unknown path schema $($recording.schema)" }
$inv=[Globalization.CultureInfo]::InvariantCulture
function Vk([string]$k){ switch -CaseSensitive($k){ 'Space'{0x20} 'Shift'{0x10} 'Ctrl'{0x11} 'Alt'{0x12} 'Tab'{0x09} 'Escape'{0x1B} 'Enter'{0x0D} 'Up'{0x26} 'Down'{0x28} 'Left'{0x25} 'Right'{0x27}
    default{ if($k -cmatch '^[A-Z0-9]$'){ [int][char]$k }else{ throw "unknown key '$k' (letters in capitals)" } } } }
# The timeline: every segment's cycles laid end to end, each event with its absolute time; every key resolved and every event
# checked inside its cycle and in time order before any input is sent.
# I-0097 tranche 5: a reconnaissance path may take pictures (« shot »: the game's screen as <shot>-<cycle>.jpg next to -Log);
# a picture costs tens of milliseconds, so a measured path has none.
if(@($recording.segments | ForEach-Object { $_.events } | Where-Object { $_.shot }).Count){ Add-Type -AssemblyName System.Drawing,System.Windows.Forms }
function Shot([string]$name,[IntPtr]$window){ $b=[Windows.Forms.Screen]::FromHandle($window).Bounds
    $bmp=New-Object Drawing.Bitmap $b.Width,$b.Height;$g=[Drawing.Graphics]::FromImage($bmp)
    try{ $g.CopyFromScreen($b.Location,[Drawing.Point]::Empty,$b.Size);$bmp.Save((Join-Path (Split-Path -Parent $Log) "$name.jpg"),[Drawing.Imaging.ImageFormat]::Jpeg) }finally{ $g.Dispose();$bmp.Dispose() } }
$timeline=[Collections.Generic.List[object]]::new();$at=0.0
foreach($segment in $recording.segments){
    $timeline.Add([pscustomobject]@{t=$at;mark="segment_$($segment.name)_start"});$previous=-1.0
    foreach($e in $segment.events){ if([double]$e.t -lt $previous -or [double]$e.t -ge [double]$segment.cycle_ms){ throw "segment $($segment.name): event at $($e.t) ms out of order or outside its $($segment.cycle_ms) ms cycle" }
        $previous=[double]$e.t;if($e.key){ $null=Vk $e.key }
        if($e.shot -and [string]$e.shot -notmatch '^[a-z0-9-]+$'){ throw "segment $($segment.name): shot name '$($e.shot)' (a-z, 0-9, -)" } }
    for($c=0;$c -lt [int]$segment.cycles;$c++){
        foreach($e in $segment.events){ $timeline.Add([pscustomobject]@{t=$at+$c*[double]$segment.cycle_ms+[double]$e.t;event=$e;cycle=$c+1;vk=$(if($e.key){Vk $e.key}else{0})}) } }
    $at+=[int]$segment.cycles*[double]$segment.cycle_ms
    $timeline.Add([pscustomobject]@{t=$at;mark="segment_$($segment.name)_end"}) }
$game=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1
if(-not $game){ throw 'DarkSoulsIII is not running' }
if(-not [ReplayInput]::GameInFront([uint32]$game.Id)){ [void][ReplayInput]::SetForegroundWindow($game.MainWindowHandle);Start-Sleep -Milliseconds 300 }
if(-not [ReplayInput]::GameInFront([uint32]$game.Id)){ throw 'DarkSoulsIII is not in the foreground; replay refused' }
$held=[Collections.Generic.HashSet[string]]::new();$markRows=[Collections.Generic.List[string]]::new();$late=[Collections.Generic.List[double]]::new();$lines=[Collections.Generic.List[string]]::new()
function Release{ foreach($h in @($held)){ if($h -like 'key:*'){ [ReplayInput]::Key([byte]($h.Substring(4)),$false) }else{ [ReplayInput]::Button([int]($h.Substring(7)),$false) } };$held.Clear() }
[void][ReplayInput]::timeBeginPeriod(1);$status='running'
# Step review N6: the first calls warmed before the clock starts (the first event was 15 ms late on every run).
$warm=[Diagnostics.Stopwatch]::StartNew();[void][ReplayInput]::WaitUntil($warm,1,[uint32]$game.Id);[void][ReplayInput]::GameInFront([uint32]$game.Id)
$watch=[Diagnostics.Stopwatch]::StartNew()
$markRows.Add("motion_start $([ReplayInput]::GetTickCount64()) $(Get-Date -Format o)")
try{
    foreach($item in $timeline){
        # The foreground is checked during every wait too (a focus lost and back inside a hold drops the held key).
        if(-not [ReplayInput]::WaitUntil($watch,$item.t,[uint32]$game.Id)){ $status='foreground_lost';break }
        if($item.mark){ $markRows.Add("$($item.mark) $([ReplayInput]::GetTickCount64())");continue }   # written after the path (a file write here made the next event 15-25 ms late)
        $e=$item.event
        if($e.key){ [ReplayInput]::Key([byte]$item.vk,[bool]$e.down);if($e.down){[void]$held.Add("key:$($item.vk)")}else{[void]$held.Remove("key:$($item.vk)")} }
        elseif($e.button){ [ReplayInput]::Button([int]$e.button,[bool]$e.down);if($e.down){[void]$held.Add("button:$($e.button)")}else{[void]$held.Remove("button:$($e.button)")} }
        elseif($e.move){ [ReplayInput]::Move([int]$e.move[0],[int]$e.move[1]) }
        elseif($e.shot){ Shot "$($e.shot)-$($item.cycle)" $game.MainWindowHandle }
        $l=$watch.Elapsed.TotalMilliseconds-$item.t;$late.Add($l);$lines.Add([string]::Format($inv,'{0,9:F1} {1,7:F2} {2}',$item.t,$l,($e | ConvertTo-Json -Compress)))
    }
    if($status -eq 'running'){ $status='complete' }   # a stop (Ctrl+C) that skips the catch leaves 'running', not 'complete'
}catch{ $status='error';$lines.Add("error: $_") }
finally{ Release;[void][ReplayInput]::timeEndPeriod(1)
    $markRows.Add("motion_end $([ReplayInput]::GetTickCount64()) $(Get-Date -Format o)");Add-Content -LiteralPath $Marks -Value $markRows
    $max=if($late.Count){($late | Measure-Object -Maximum).Maximum}else{0};$mean=if($late.Count){($late | Measure-Object -Average).Average}else{0}
    $lines.Add([string]::Format($inv,'replay {0} events={1} late_max_ms={2:F2} late_mean_ms={3:F2} path={4}',$status,$late.Count,$max,$mean,$recording.name))
    Set-Content -LiteralPath $Log -Value $lines }
"replay $status"
if($status -ne 'complete'){ exit 2 }
